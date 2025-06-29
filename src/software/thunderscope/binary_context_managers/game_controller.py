from __future__ import annotations

import queue
import random
import logging
import os
import socket
import time
from subprocess import Popen
from typing import Any

from proto.import_all_protos import *
from proto.ssl_gc_common_pb2 import Team as SslTeam
from software.networking.ssl_proto_communication import *
import software.python_bindings as tbots_cpp
from software.thunderscope.proto_unix_io import ProtoUnixIO
from software.python_bindings import *
from software.py_constants import *
from software.thunderscope.binary_context_managers.util import *
from software.thunderscope.thread_safe_buffer import ThreadSafeBuffer
from software.thunderscope.common.thread_safe_circular_buffer import (
    ThreadSafeCircularBuffer,
)

logger = logging.getLogger(__name__)
import itertools


class Gamecontroller:
    """Gamecontroller Context Manager"""

    CI_MODE_LAUNCH_DELAY_S = 0.3
    REFEREE_IP = "224.5.23.1"
    CI_MODE_OUTPUT_RECEIVE_BUFFER_SIZE = 9000

    def __init__(
        self,
        suppress_logs: bool = False,
        use_conventional_port: bool = False,
    ) -> None:
        """Run Gamecontroller

        :param suppress_logs: Whether to suppress the logs
        :param use_conventional_port: whether or not to use the conventional port!
        """
        self.suppress_logs = suppress_logs

        # We default to using a non-conventional port to avoid emitting
        # on the same port as what other teams may be listening on.
        if use_conventional_port:
            if not self.is_valid_port(SSL_REFEREE_PORT):
                raise OSError(f"Cannot use port {SSL_REFEREE_PORT} for Gamecontroller")

            self.referee_port = SSL_REFEREE_PORT
        else:
            self.referee_port = self.next_free_port(random.randint(1024, 65535))

        self.ci_port = self.next_free_port()
        # this allows gamecontroller to listen to override commands
        self.command_override_buffer = ThreadSafeBuffer(
            buffer_size=2, protobuf_type=ManualGCCommand
        )

        self.simulator_proto_unix_io = None
        self.blue_team_world_buffer = ThreadSafeCircularBuffer(
            buffer_size=1, protobuf_type=World
        )
        self.latest_world = None
        self.blue_removed_robot_ids = queue.Queue()
        self.yellow_removed_robot_ids = queue.Queue()

    def get_referee_port(self) -> int:
        """Sometimes, the port that we are using changes depending on context.
        We want a getter function that returns the port we are using.

        :return: the port that the game controller is currently using!
        """
        return self.referee_port

    def __enter__(self) -> Gamecontroller:
        """Enter the gamecontroller context manager.

        :return: gamecontroller context managed instance
        """
        command = ["/opt/tbotspython/gamecontroller", "--timeAcquisitionMode", "ci"]

        command += ["-publishAddress", f"{self.REFEREE_IP}:{self.referee_port}"]
        command += ["-ciAddress", f"localhost:{self.ci_port}"]

        if self.suppress_logs:
            with open(os.devnull, "w") as fp:
                self.gamecontroller_proc = Popen(command, stdout=fp, stderr=fp)

        else:
            self.gamecontroller_proc = Popen(command)

        # We can't connect to the ci port right away, it takes
        # CI_MODE_LAUNCH_DELAY_S to start up the gamecontroller
        time.sleep(Gamecontroller.CI_MODE_LAUNCH_DELAY_S)
        self.ci_socket = SslSocket(self.ci_port)

        return self

    def __exit__(self, type, value, traceback) -> None:
        """Exit the gamecontroller context manager.

        :param type: The type of exception that was raised
        :param value: The exception that was raised
        :param traceback: The traceback of the exception
        """
        self.gamecontroller_proc.terminate()
        self.gamecontroller_proc.wait()
        self.ci_socket.close()

    def refresh(self):
        """Gets any manual gamecontroller commands from the buffer and executes them"""
        manual_command = self.command_override_buffer.get(return_cached=False)

        while manual_command is not None:
            self.send_gc_command(
                gc_command=manual_command.manual_command.type,
                team=manual_command.manual_command.for_team,
                final_ball_placement_point=(
                    tbots_cpp.Point(
                        manual_command.final_ball_placement_point.x,
                        manual_command.final_ball_placement_point.y,
                    )
                    # HasField checks if the field was manually set by us
                    # as opposed to if a value exists (since a default value always exists)
                    if manual_command.HasField("final_ball_placement_point")
                    else None
                ),
            )
            manual_command = self.command_override_buffer.get(return_cached=False)

    @staticmethod
    def __update_robot_count(
        robot_states,
        team: Team,
        max_robots: int,
        removed_robot_ids: queue.Queue[int],
        field_edge_y_meters: float,
    ) -> None:
        """Static method for updating the number of robots in the world state (i.e. robot_states) for a given Team.
        This method is team & side agnostic.

        :param robot_states: WorldState <Robot ID, RobotState> map protobuf to be updated.
        :param team: the Team of robots currently in play
        :param max_robots: The number of robots we should have on the field. Must be >= 0
        :param removed_robot_ids: The robots (IDs) which have been removed already and can safely be re-added
        :param field_edge_y_meters: Places new robots at this y position along the centerline
        :return:
        """
        # build the robot state for placing robots at the edge of field
        place_state = RobotState(
            global_position=Point(x_meters=0, y_meters=field_edge_y_meters)
        )
        # Remove robots, as we have too many. Set robot velocities to zero to avoid any drift
        for count, robot in enumerate(team.team_robots, start=1):
            if count <= max_robots:
                robot_states[robot.id].CopyFrom(robot.current_state)
                velocity = robot_states[robot.id].global_velocity
                velocity.x_component_meters = 0
                velocity.y_component_meters = 0
            else:
                removed_robot_ids.put(robot.id)
        # Add robots, since we are missing some
        robots_diff: int = max_robots - len(team.team_robots)
        if robots_diff <= 0:
            return
        for _ in range(robots_diff):
            try:
                robot_states[removed_robot_ids.get_nowait()].CopyFrom(place_state)
            except queue.Empty:
                return

    def handle_referee(self, referee: Referee) -> None:
        """Updates the world state based on the referee message
        :param referee: the referee protobuf message
        """
        # Check that we are running with the simulator and have access to its
        # proto unix io
        if self.simulator_proto_unix_io is None:
            return

        # Convert the latest blue world into a WorldState we can send to the simulator and update the robots
        self.latest_world = self.blue_team_world_buffer.get(
            block=False, return_cached=True
        )

        max_allowed_bots_yellow: int = referee.yellow.max_allowed_bots
        max_allowed_bots_blue: int = referee.blue.max_allowed_bots
        # Ignore if nothing needs to be updated
        if (
            len(self.latest_world.friendly_team.team_robots) == max_allowed_bots_blue
            and len(self.latest_world.enemy_team.team_robots) == max_allowed_bots_yellow
        ):
            return

        # Populate a blank WorldState with new updated robot information
        world_state = WorldState()
        field_edge_y_meters: Final[int] = (
            self.latest_world.field.field_y_length
            - self.latest_world.field.boundary_buffer_size
        )

        self.__update_robot_count(
            world_state.blue_robots,
            self.latest_world.friendly_team,
            max_allowed_bots_blue,
            self.blue_removed_robot_ids,
            field_edge_y_meters,
        )
        self.__update_robot_count(
            world_state.yellow_robots,
            self.latest_world.enemy_team,
            max_allowed_bots_yellow,
            self.yellow_removed_robot_ids,
            -field_edge_y_meters,
        )

        # Check if we need to invert the world state
        if referee.blue_team_on_positive_half:
            for robot in itertools.chain(
                world_state.blue_robots, world_state.yellow_robots
            ):
                robot.current_state.global_position.x_meters *= -1
                robot.current_state.global_position.y_meters *= -1
                robot.current_state.global_orientation.radians += math.pi

        # Send out updated world state
        self.simulator_proto_unix_io.send_proto(WorldState, world_state)

    def is_valid_port(self, port):
        """Determine whether or not a given port is valid

        :param port: the port we are checking
        :return: True if a port is valid False otherwise
        """
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        try:
            sock.bind(("", port))
            sock.close()
            return True
        except OSError:
            return False

    def next_free_port(self, start_port: int = 40000, max_port: int = 65535) -> int:
        """Find the next free port. We need to find 2 free ports to use for the gamecontroller
        so that we can run multiple gamecontroller instances in parallel.

        :param start_port: The port to start looking from
        :param max_port: The maximum port to look up to
        :return: The next free port
        """
        while start_port <= max_port:
            if self.is_valid_port(start_port):
                return start_port
            start_port += 1

        raise IOError("no free ports")

    def setup_proto_unix_io(
        self,
        blue_full_system_proto_unix_io: ProtoUnixIO,
        yellow_full_system_proto_unix_io: ProtoUnixIO,
        autoref_proto_unix_io: ProtoUnixIO = None,
        simulator_proto_unix_io: ProtoUnixIO = None,
    ) -> None:
        """Setup gamecontroller io

        :param blue_full_system_proto_unix_io: The proto unix io of the blue full system.
        :param yellow_full_system_proto_unix_io: The proto unix io of the yellow full system.
        :param autoref_proto_unix_io: The proto unix io for the autoref
        :param simulator_proto_unix_io: The socket for the Gamecontroller to interact with the simulator
        """

        def __send_referee_command(data: Referee) -> None:
            """Send a referee command from the gamecontroller to both full
            systems.

            :param data: The referee command to send
            """
            self.handle_referee(data)
            blue_full_system_proto_unix_io.send_proto(Referee, data)
            yellow_full_system_proto_unix_io.send_proto(Referee, data)
            if autoref_proto_unix_io is not None:
                autoref_proto_unix_io.send_proto(Referee, data)

        self.receive_referee_command = tbots_cpp.SSLRefereeProtoListener(
            Gamecontroller.REFEREE_IP,
            self.referee_port,
            "lo",
            __send_referee_command,
            True,
        )

        blue_full_system_proto_unix_io.register_observer(
            ManualGCCommand, self.command_override_buffer
        )
        yellow_full_system_proto_unix_io.register_observer(
            ManualGCCommand, self.command_override_buffer
        )
        blue_full_system_proto_unix_io.register_observer(
            World, self.blue_team_world_buffer
        )
        self.simulator_proto_unix_io = simulator_proto_unix_io

    def send_gc_command(
        self,
        gc_command: proto.ssl_gc_state_pb2.Command,
        team: proto.ssl_gc_common_pb2.Team,
        final_ball_placement_point: tbots_cpp.Point = None,
    ) -> Any:
        """Send a ci input to the gamecontroller.

        CiInput -> Input -> Change ->  NewCommand -> Command -> (Type, Team)

        More info here:
        https://github.com/RoboCup-SSL/ssl-game-controller/blob/master/cmd/ssl-ci-test-client/README.md

        :param gc_command: The gc command to send
        :param team: The team to send the command to
        :param final_ball_placement_point: ball placement point for BallPlacement messages
        :return: The response CiOutput containing 1 or more refree msgs
        """
        ci_input = CiInput(timestamp=int(time.time_ns()))
        api_input = Input()
        change = Change()
        new_command = Change.NewCommand()
        command = Command(type=gc_command, for_team=team)

        new_command.command.CopyFrom(command)
        change.new_command_change.CopyFrom(new_command)
        api_input.change.CopyFrom(change)
        ci_input.api_inputs.append(api_input)

        # Do this only if ball placement pos is specified
        if final_ball_placement_point:
            # Set position
            ball_placement_pos = Change.SetBallPlacementPos()
            ball_placement_pos.pos.CopyFrom(
                Vector2(
                    x=float(final_ball_placement_point.x()),
                    y=float(final_ball_placement_point.y()),
                )
            )
            change = Change()
            api_input = Input()
            change.set_ball_placement_pos_change.CopyFrom(ball_placement_pos)
            api_input.change.CopyFrom(change)
            ci_input.api_inputs.append(api_input)

            # Start Placement
            api_input = Input()
            start_placement = ContinueAction()
            start_placement.type = ContinueAction.Type.BALL_PLACEMENT_START
            start_placement.for_team = team
            api_input.continue_action.CopyFrom(start_placement)
            ci_input.api_inputs.append(api_input)

        ci_output_list = self.send_ci_input(ci_input)

        return ci_output_list

    def send_ci_input(self, ci_input: proto.ssl_gc_ci_pb2.CiInput) -> list[CiOutput]:
        """Send CiInput proto to the Gamecontroller. Retries if the Gamecontroller output isn't parseable as a CiOutput proto

        :param CiInput proto to send to the Gamecontroller

        :return: a list of CiOutput protos received from the Gamecontroller
        """
        ci_output_list = list()
        while True:
            try:
                self.ci_socket.send(ci_input)
                ci_output_list = self.ci_socket.receive(CiOutput)
                break
            except SslSocketProtoParseException as parse_err:
                logging.info(
                    "error receiving CiOutput proto from the gamecontroller: "
                    + parse_err.args
                )

        return ci_output_list

    def reset_team(self, name: str, team: str) -> Change.UpdateTeamState:
        """Returns an UpdateTeamState proto for the gamecontroller to reset team info.

        :param name name of the new team
        :param team yellow or blue team to update

        :return: corresponding UpdateTeamState proto
        """
        update_team_state = Change.UpdateTeamState()
        update_team_state.for_team = team
        update_team_state.team_name.value = name
        update_team_state.goals.value = 0
        update_team_state.timeouts_left.value = 4
        update_team_state.timeout_time_left.value = "05:00"
        update_team_state.can_place_ball.value = True

        return update_team_state

    def reset_game(
        self, division: proto.ssl_gc_common_pb2.Division
    ) -> Change.UpdateConfig:
        """Returns an UpdateConfig proto for the Gamecontroller to reset game info.

        :param division the Division proto corresponding to the game division to set up the Gamecontroller for

        :return: corresponding UpdateConfig proto
        """
        game_update = Change.UpdateConfig()
        game_update.division = division
        game_update.first_kickoff_team = SslTeam.BLUE
        game_update.match_type = MatchType.FRIENDLY

        return game_update

    def reset_team_info(
        self, division: proto.ssl_gc_common_pb2.Division
    ) -> list[CiOutput]:
        """Sends a message to the Gamecontroller to reset Team information.

        :param division: the Division proto corresponding to the game division to set up the Gamecontroller for

        :return: a list of CiOutput protos from the Gamecontroller
        """
        ci_input = CiInput(timestamp=int(time.time_ns()))
        input_blue_update = Input()
        input_blue_update.reset_match = True
        input_blue_update.change.update_team_state_change.CopyFrom(
            self.reset_team("BLUE", SslTeam.BLUE)
        )

        input_yellow_update = Input()
        input_yellow_update.reset_match = True
        input_yellow_update.change.update_team_state_change.CopyFrom(
            self.reset_team("YELLOW", SslTeam.YELLOW)
        )

        input_game_update = Input()
        input_game_update.reset_match = True
        input_game_update.change.update_config_change.CopyFrom(
            self.reset_game(division)
        )

        ci_input.api_inputs.append(input_blue_update)
        ci_input.api_inputs.append(input_yellow_update)
        ci_input.api_inputs.append(input_game_update)

        return self.send_ci_input(ci_input)

    def update_game_engine_config(
        self, config: proto.ssl_gc_engine_config_pb2
    ) -> list[CiOutput]:
        """Sends a game engine config update.

        :param config: the new SSL game engine config

        :return: a list of CiOutput protos from the Gamecontroller
        """
        ci_input = CiInput(timestamp=int(time.time_ns()))

        game_config_input = Input()
        game_config_input.config_delta.CopyFrom(config)

        ci_input.api_inputs.append(game_config_input)

        return self.send_ci_input(ci_input)
