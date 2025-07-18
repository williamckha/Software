#pragma once

#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>

#include "proto/tbots_software_msgs.pb.h"
#include "shared/2021_robot_constants.h"
#include "shared/constants.h"
#include "software/embedded/primitive_executor.h"
#include "software/embedded/redis/redis_client.h"
#include "software/embedded/services/motor.h"
#include "software/embedded/services/network/network.h"
#include "software/embedded/services/power.h"
#include "software/logger/logger.h"

class Thunderloop
{
   public:
    /**
     * Thunderloop is a giant loop that runs at THUNDERLOOP_HZ.
     * It receives Primitives from AI, executes the Primitives with
     * the most recent vison data, and polls the services to interact
     * with the hardware peripherals.
     *
     * High Level Diagram: Service order in loop not shown
     *
     *                   ┌─────────────────┐
     *                   │                 │
     *                   │   Thunderloop   │
     *                   │                 │
     *  Primitives───────►                 │ Target Vel ┌────────────┐
     *                   │                 ├────────────►            │
     *                   |                 │            │ MotorBoard │
     *                   │    Services     ◄────────────┤            │
     *                   │                 │ Actual Vel └────────────┘
     *                   │  Primitive Exec │
     *                   │                 │
     *                   │                 │ Kick/Chip  ┌────────────┐
     *                   │                 ├────────────►            │
     * Robot Status◄─────┤                 │            │ PowerBoard │
     *                   │                 ◄────────────┤            │
     *                   └─────────────────┘  Voltages  └────────────┘
     *
     *
     * @param robot_constants The robot constants
     * @param enable_log_merging Whether to merge repeated log message or not
     * @param loop_hz The rate to run the loop
     */
    Thunderloop(const RobotConstants_t &robot_constants, bool enable_log_merging,
                const int loop_hz);

    ~Thunderloop();

    [[noreturn]] void runLoop();

    // Services
    std::unique_ptr<MotorService> motor_service_;
    std::unique_ptr<NetworkService> network_service_;
    std::unique_ptr<PowerService> power_service_;

    // Clients
    std::unique_ptr<RedisClient> redis_client_;

   private:
    /*
     * The struct timespec consists of nanoseconds and seconds. If the nanoseconds
     * are getting bigger than 1000000000 (= 1 second) the variable containing
     * seconds has to be incremented and the nanoseconds decremented by 1000000000.
     *
     * @param ts timespec to modify
     */
    void timespecNorm(struct timespec &ts);

    /**
     * Get the CPU temp thunderloop is running on
     *
     * @return The CPU temp.
     */
    double getCpuTemperature();

    /**
     * Converts the given timespec value to milliseconds
     * @return The time in milliseconds
     */
    double getMilliseconds(timespec time);

    /**
     * Converts the given timespec value to nanoseconds
     * @return The time in nanoseconds
     */
    double getNanoseconds(timespec time);

    /**
     * Updates ErrorCodes for BAT, CAP, CPU TEMP if over thresholds
     */
    void updateErrorCodes();

    /**
     * Poll the motor service.
     *
     * @param poll_time Populates the time taken to poll the service
     * @param motor_control Control message for the motors
     * @param time_since_prev_iteration Stores the time difference since the last call
     */
    TbotsProto::MotorStatus pollMotorService(
        struct timespec &poll_time, const TbotsProto::MotorControl &motor_control,
        const struct timespec &time_since_prev_iteration);

    /**
     * Poll the power service
     *
     * @param poll_time Populates the time taken to poll the service
     *
     * @return The polled power status message
     */
    TbotsProto::PowerStatus pollPowerService(struct timespec &poll_time);

    /**
     * Wait for networking communication to be established. This function is blocking.
     */
    void waitForNetworkUp();


    // Input Msg Buffers
    TbotsProto::World world_;
    TbotsProto::Primitive primitive_;
    TbotsProto::DirectControlPrimitive direct_control_;

    // Output Msg Buffers
    TbotsProto::RobotStatus robot_status_;
    TbotsProto::JetsonStatus jetson_status_;
    TbotsProto::NetworkStatus network_status_;
    TbotsProto::PowerStatus power_status_;
    std::optional<TbotsProto::MotorStatus> motor_status_;
    TbotsProto::ThunderloopStatus thunderloop_status_;
    TbotsProto::ChipperKickerStatus chipper_kicker_status_;
    TbotsProto::PrimitiveExecutorStatus primitive_executor_status_;
    TbotsProto::Timestamp time_sent_;

    // Current State
    RobotConstants_t robot_constants_;
    Angle current_orientation_;
    int robot_id_;
    int channel_id_;
    std::string network_interface_;
    int loop_hz_;

    // Calibrated power service constants
    double kick_coeff_;
    int kick_constant_;
    int chip_pulse_width_;

    // Primitive Executor
    PrimitiveExecutor primitive_executor_;

    // 500 millisecond timeout on receiving primitives before we stop the robots
    const double PACKET_TIMEOUT_NS = 500.0 * NANOSECONDS_PER_MILLISECOND;

    // Timeout after a failed ping request
    const int PING_RETRY_DELAY_S = 1;

    const std::string PATH_TO_RINGBUFFER_LOG = "/var/log/dmesg";

    std::ifstream log_file = std::ifstream(PATH_TO_RINGBUFFER_LOG);
};

/*
 * reads from the kernel ring buffer, likely /var/log/dmesg file, to see if the power
 * is stable
 *
 * This is not defined in Thunderloop to allow it to be unit tested easily
 *
 * @return True if the power is stable, false otherwise
 */
bool isPowerStable(std::ifstream &log_file);
