import argparse
import contextlib
import logging
import os
import sys
import threading

import google.protobuf
from google.protobuf.internal import api_implementation

protobuf_impl_type = api_implementation.Type()
assert protobuf_impl_type == "upb", (
    f"Trying to use the {protobuf_impl_type} protobuf implementation. "
    "Please use the upb implementation, available in python protobuf version 4.21.0 and above."
    f"The current version of protobuf is {google.protobuf.__version__}"
)

from software.thunderscope.thunderscope import Thunderscope
from software.thunderscope.binary_context_managers import *
from proto.import_all_protos import *
from software.py_constants import *
from software.thunderscope.robot_communication import RobotCommunication
from software.thunderscope.wifi_communication_manager import WifiCommunicationManager
from software.thunderscope.constants import EstopMode, ProtoUnixIOTypes
from software.thunderscope.estop_helpers import get_estop_config
from software.thunderscope.proto_unix_io import ProtoUnixIO
import software.thunderscope.thunderscope_config as config
from software.thunderscope.constants import CI_DURATION_S
from software.thunderscope.util import *

from software.thunderscope.binary_context_managers.full_system import FullSystem
from software.thunderscope.binary_context_managers.simulator import Simulator
from software.thunderscope.binary_context_managers.game_controller import Gamecontroller

from software.thunderscope.binary_context_managers.tigers_autoref import TigersAutoref


###########################################################################
#                         Thunderscope Main                               #
###########################################################################

if __name__ == "__main__":
    logging.getLogger().setLevel(logging.INFO)

    # Setup parser
    parser = argparse.ArgumentParser(
        description="Thunderscope: Run with no arguments to run AI vs AI"
    )

    parser.add_argument(
        "--layout",
        action="store",
        help="Which layout to run, if not specified the last layout will run",
    )

    # Runtime directories
    parser.add_argument(
        "--simulator_runtime_dir",
        type=str,
        help="simulator runtime directory",
        default="/tmp/tbots/sim",
    )
    parser.add_argument(
        "--blue_full_system_runtime_dir",
        type=str,
        help="blue full_system runtime directory",
        default="/tmp/tbots/blue",
    )
    parser.add_argument(
        "--yellow_full_system_runtime_dir",
        type=str,
        help="yellow full_system runtime directory",
        default="/tmp/tbots/yellow",
    )

    # Debugging
    parser.add_argument(
        "--debug_blue_full_system",
        action="store_true",
        default=False,
        help="Debug blue full_system",
    )
    parser.add_argument(
        "--debug_yellow_full_system",
        action="store_true",
        default=False,
        help="Debug yellow full_system",
    )
    parser.add_argument(
        "--debug_simulator",
        action="store_true",
        default=False,
        help="Debug the simulator",
    )
    parser.add_argument(
        "--visualize_cpp_test",
        action="store_true",
        default=False,
        help="Visualize C++ Tests",
    )
    parser.add_argument(
        "--blue_log",
        action="store",
        help="Replay folder for the blue full_system",
        default=None,
        type=os.path.abspath,
    )
    parser.add_argument(
        "--yellow_log",
        action="store",
        help="Replay folder for the yellow full_system",
        default=None,
    )

    parser.add_argument(
        "--verbose",
        action="store_true",
        default=False,
        help="Include logs from the Gamecontroller and Autoref",
    )

    # Run blue or yellow full system over WiFi
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--run_blue",
        action="store_true",
        help="Run full system as the blue team, over WiFi; estop required",
    )
    group.add_argument(
        "--run_yellow",
        action="store_true",
        help="Run full system as the yellow team, over WiFi; estop required",
    )

    parser.add_argument(
        "--run_diagnostics",
        action="store_true",
        help="Run robots diagnostics for Manual or Xbox control; estop required",
    )
    parser.add_argument(
        "--interface",
        action="store",
        type=str,
        default=None,
        help="Which interface to communicate over",
    )
    parser.add_argument(
        "--channel",
        action="store",
        type=int,
        default=0,
        help="Which channel to communicate over",
    )
    parser.add_argument(
        "--enable_radio",
        action="store_true",
        default=False,
        help="Whether to use radio (True) or Wi-Fi (False) for sending primitives to robots",
    )
    parser.add_argument(
        "--visualization_buffer_size",
        action="store",
        type=int,
        default=5,
        help="How many packets to buffer while rendering",
    )
    parser.add_argument(
        "--enable_realism",
        action="store_true",
        default=False,
        help="set realism flag to use realistic config",
    )
    parser.add_argument(
        "--estop_baudrate",
        action="store",
        type=int,
        default=115200,
        help="Estop Baudrate",
    )
    parser.add_argument(
        "--ci_mode",
        action="store_true",
        default=False,
        help="Runs the simulation with sped-up time",
    )
    parser.add_argument(
        "--enable_autoref", action="store_true", default=False, help="Enable autoref"
    )
    parser.add_argument(
        "--show_autoref_gui",
        action="store_true",
        default=False,
        help="Show TigersAutoref GUI",
    )
    parser.add_argument(
        "--sudo",
        action="store_true",
        default=False,
        help="Run unix_full_system under sudo",
    )

    estop_group = parser.add_mutually_exclusive_group()
    estop_group.add_argument(
        "--keyboard_estop",
        action="store_true",
        default=False,
        help="Allows the use of the spacebar as an estop instead of a physical one",
    )
    estop_group.add_argument(
        "--disable_communication",
        action="store_true",
        default=False,
        help="Disables checking for estop plugged in (ONLY USE FOR LOCAL TESTING)",
    )

    parser.add_argument(
        "--empty",
        action="store_true",
        default=False,
        help="Whether to populate with default robot positions (False) or start with an empty field (True) for AI vs AI",
    )

    parser.add_argument(
        "--launch_gc",
        action="store_true",
        default=False,
        help="whether or not to launch the gamecontroller when --run_blue or --run_yellow is ran",
    )

    args = parser.parse_args()

    # we only have --launch_gc parameter but not args.run_yellow and args.run_blue
    if not args.run_blue and not args.run_yellow and args.launch_gc:
        parser.error(
            "--launch_gc has to be ran with --run_blue or --run_yellow argument"
        )

    ###########################################################################
    #                      Visualize CPP Tests                                #
    ###########################################################################
    # TODO (#2581) remove this
    if args.visualize_cpp_test:
        runtime_dir = "/tmp/tbots/gtest_logs"

        try:
            os.makedirs(runtime_dir)
        except OSError:
            pass

        tscope = Thunderscope(
            config=config.configure_two_ai_gamecontroller_view(
                args.visualization_buffer_size
            ),
            layout_path=args.layout,
        )
        proto_unix_io = tscope.proto_unix_io_map[ProtoUnixIOTypes.BLUE]

        # Setup LOG(VISUALIZE) handling from full system. We set from_log_visualize
        # to true to decode from base64.
        for arg in [
            {"proto_class": ObstacleList},
            {"proto_class": PathVisualization},
            {"proto_class": PassVisualization},
            {"proto_class": AttackerVisualization},
            {"proto_class": CostVisualization},
            {"proto_class": DebugShapes},
            {"proto_class": NamedValue},
            {"proto_class": PrimitiveSet},
            {"proto_class": World},
            {"proto_class": PlayInfo},
            {"proto_class": BallPlacementVisualization},
        ]:
            proto_unix_io.attach_unix_receiver(
                runtime_dir, from_log_visualize=True, **arg
            )

        proto_unix_io.attach_unix_receiver(runtime_dir + "/log", proto_class=RobotLog)

        tscope.show()

    ###########################################################################
    #              AI + Robot Communication + Robot Diagnostics               #
    ###########################################################################
    #
    # When we are running with real robots. Possible Running Options:
    #
    #       Run Blue
    #       Run Yellow
    #       Run Blue + Diagnostics
    #       Run Yellow + Diagnostics
    #       Run Diagnostics
    #
    # We want to run either 1 instance of AI or 1 instance of RobotCommunication or both which will
    # send/recv packets over the provided multicast channel.

    elif args.run_blue or args.run_yellow or args.run_diagnostics:
        tscope_config, robot_view_widget = config.configure_ai_or_diagnostics(
            args.run_blue,
            args.run_yellow,
            args.run_diagnostics,
            args.visualization_buffer_size,
        )
        tscope = Thunderscope(
            config=tscope_config,
            layout_path=args.layout,
        )

        if args.run_blue:
            runtime_dir = args.blue_full_system_runtime_dir
            friendly_colour_yellow = False
            debug = args.debug_blue_full_system
        elif args.run_yellow:
            runtime_dir = args.yellow_full_system_runtime_dir
            friendly_colour_yellow = True
            debug = args.debug_yellow_full_system

        # this will be the current fullsystem proto (blue or yellow)
        # if fullsystem is loaded
        # else, it will be the diagnostics proto
        current_proto_unix_io = tscope.proto_unix_io_map[ProtoUnixIOTypes.CURRENT]

        estop_mode, estop_path = get_estop_config(
            args.keyboard_estop, args.disable_communication
        )

        with (
            Gamecontroller(
                suppress_logs=(not args.verbose), use_conventional_port=False
            )
            if args.launch_gc
            else contextlib.nullcontext()
        ) as gamecontroller, WifiCommunicationManager(
            current_proto_unix_io=current_proto_unix_io,
            multicast_channel=getRobotMulticastChannel(args.channel),
            should_setup_full_system=(args.run_blue or args.run_yellow),
            interface=args.interface,
            referee_port=gamecontroller.get_referee_port()
            if gamecontroller
            else SSL_REFEREE_PORT,
        ) as wifi_communication_manager, RobotCommunication(
            current_proto_unix_io=current_proto_unix_io,
            communication_manager=wifi_communication_manager,
            estop_mode=estop_mode,
            estop_path=estop_path,
        ) as robot_communication:
            if estop_mode == EstopMode.KEYBOARD_ESTOP:
                tscope.keyboard_estop_shortcut.activated.connect(
                    robot_communication.toggle_keyboard_estop
                )

            config.connect_robot_view_to_robot_communication(
                robot_view_widget, robot_communication
            )

            if args.run_blue or args.run_yellow:
                full_system_runtime_dir = (
                    args.blue_full_system_runtime_dir
                    if args.run_blue
                    else args.yellow_full_system_runtime_dir
                )
                with FullSystem(
                    full_system_runtime_dir=runtime_dir,
                    debug_full_system=debug,
                    friendly_colour_yellow=friendly_colour_yellow,
                    should_restart_on_crash=True,
                    run_sudo=args.sudo,
                ) as full_system:
                    full_system.setup_proto_unix_io(current_proto_unix_io)

                    tscope.show()
            else:
                tscope.show()

    ###########################################################################
    #                              Replay                                     #
    ###########################################################################
    #
    # Don't start any binaries and just replay a log.
    elif args.blue_log or args.yellow_log:
        tscope = Thunderscope(
            config=config.configure_replay_view(
                args.blue_log,
                args.yellow_log,
                args.visualization_buffer_size,
            ),
            layout_path=args.layout,
        )
        tscope.show()

    ###########################################################################
    #           Blue AI vs Yellow AI + Simulator + Gamecontroller             #
    ###########################################################################
    #
    # Run two AIs against each other with the er-force simulator. We also run
    # the gamecontroller which can be accessed from http://localhost:8081
    #
    # The async sim ticket ticks the simulator at a fixed rate.
    else:
        tscope = Thunderscope(
            config=config.configure_two_ai_gamecontroller_view(
                args.visualization_buffer_size
            ),
            layout_path=args.layout,
        )

        def __ticker(tick_rate_ms: int) -> None:
            """Setup the world and tick simulation forever

            :param tick_rate_ms: The tick rate of the simulation

            """
            sync_simulation(
                tscope,
                0 if args.empty else DIV_B_NUM_ROBOTS,
            )

            if args.ci_mode:
                async_sim_ticker(
                    tick_rate_ms,
                    tscope.proto_unix_io_map[ProtoUnixIOTypes.BLUE],
                    tscope.proto_unix_io_map[ProtoUnixIOTypes.YELLOW],
                    tscope.proto_unix_io_map[ProtoUnixIOTypes.SIM],
                    tscope,
                )
            else:
                realtime_sim_ticker(
                    tick_rate_ms, tscope.proto_unix_io_map[ProtoUnixIOTypes.SIM], tscope
                )

        # Launch all binaries
        with Simulator(
            args.simulator_runtime_dir, args.debug_simulator, args.enable_realism
        ) as simulator, FullSystem(
            full_system_runtime_dir=args.blue_full_system_runtime_dir,
            debug_full_system=args.debug_blue_full_system,
            friendly_colour_yellow=False,
            should_restart_on_crash=False,
            run_sudo=args.sudo,
            running_in_realtime=(not args.ci_mode),
        ) as blue_fs, FullSystem(
            full_system_runtime_dir=args.yellow_full_system_runtime_dir,
            debug_full_system=args.debug_yellow_full_system,
            friendly_colour_yellow=True,
            should_restart_on_crash=False,
            run_sudo=args.sudo,
            running_in_realtime=(not args.ci_mode),
        ) as yellow_fs, Gamecontroller(
            suppress_logs=(not args.verbose),
        ) as gamecontroller, (
            # Here we only initialize autoref if the --enable_autoref flag is requested.
            # To avoid nested Python withs, the autoref is initialized as None when this flag doesn't exist.
            # All calls to autoref should be guarded with args.enable_autoref
            TigersAutoref(
                ci_mode=True,
                gc=gamecontroller,
                suppress_logs=(not args.verbose),
                tick_rate_ms=DEFAULT_SIMULATOR_TICK_RATE_MILLISECONDS_PER_TICK,
                show_gui=args.show_autoref_gui,
            )
            if args.enable_autoref
            else contextlib.nullcontext()
        ) as autoref:
            tscope.register_refresh_function(gamecontroller.refresh)

            autoref_proto_unix_io = ProtoUnixIO()

            blue_fs.setup_proto_unix_io(tscope.proto_unix_io_map[ProtoUnixIOTypes.BLUE])
            yellow_fs.setup_proto_unix_io(
                tscope.proto_unix_io_map[ProtoUnixIOTypes.YELLOW]
            )
            simulator.setup_proto_unix_io(
                tscope.proto_unix_io_map[ProtoUnixIOTypes.SIM],
                tscope.proto_unix_io_map[ProtoUnixIOTypes.BLUE],
                tscope.proto_unix_io_map[ProtoUnixIOTypes.YELLOW],
                autoref_proto_unix_io,
            )
            gamecontroller.setup_proto_unix_io(
                blue_full_system_proto_unix_io=tscope.proto_unix_io_map[
                    ProtoUnixIOTypes.BLUE
                ],
                yellow_full_system_proto_unix_io=tscope.proto_unix_io_map[
                    ProtoUnixIOTypes.YELLOW
                ],
                autoref_proto_unix_io=autoref_proto_unix_io,
                simulator_proto_unix_io=tscope.proto_unix_io_map[ProtoUnixIOTypes.SIM],
            )
            if args.enable_autoref:
                autoref.setup_ssl_wrapper_packets(
                    autoref_proto_unix_io,
                )

            # Start the simulator
            sim_ticker_thread = threading.Thread(
                target=__ticker,
                args=(DEFAULT_SIMULATOR_TICK_RATE_MILLISECONDS_PER_TICK,),
                daemon=True,
            )

            if args.enable_autoref and args.ci_mode:
                # In CI mode, we want AI vs AI to end automatically after a given time (CI_DURATION_S). The exiter
                # thread is passed an exit handler that will close the Thunderscope window
                # This exit handler is necessary because Qt runs on the main thread, so tscope.show() is a blocking
                # call so we need to somehow close it before doing our resource cleanup
                exiter_thread = threading.Thread(
                    target=exit_poller,
                    args=(autoref, CI_DURATION_S, lambda: tscope.close()),
                    daemon=True,
                )

                exiter_thread.start()  # start the exit countdown
                sim_ticker_thread.start()  # start the simulation ticking

                tscope.show()  # blocking!

                # these resource cleanups occur after tscope.close() is called by the exiter_thread
                exiter_thread.join()
                sim_ticker_thread.join()

                sys.exit(0)
            else:
                sim_ticker_thread.start()  # start the simulation ticking
                tscope.show()  # blocking!

                # resource cleanup occurs after Thunderscope is closed by the user
                sim_ticker_thread.join()
