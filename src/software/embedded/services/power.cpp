#include "software/embedded/services/power.h"

#include <boost/filesystem.hpp>
#include <cstdint>

#include "proto/power_frame_msg.nanopb.h"

PowerService::PowerService()
{
    nanopb_command = createNanoPbPowerPulseControl(TbotsProto::PowerControl(), 0.0, 0, 0);

    if (!boost::filesystem::exists(DEVICE_SERIAL_PORT))
    {
        throw std::runtime_error("USB not plugged into the Jetson Nano");
    }
    this->uart = std::make_unique<BoostUartCommunication>(BAUD_RATE, DEVICE_SERIAL_PORT);
    this->read_thread = std::thread(std::bind(&PowerService::continuousRead, this));
}

PowerService::~PowerService()
{
    is_running = false;
    read_thread.join();
}

void PowerService::continuousRead()
{
    while (is_running)
    {
        tick();
    }
}

void PowerService::tick()
{
    std::vector<uint8_t> power_status;
    try
    {
        uart->flushSerialPort(uart->flush_receive);
        power_status = uart->serialRead(READ_BUFFER_SIZE);
    }
    catch (std::exception& e)
    {
        LOG(FATAL) << "Read thread has crashed" << e.what();
    }

    TbotsProto_PowerFrame status_frame = TbotsProto_PowerFrame_init_default;
    if (!unmarshalUartPacket(power_status, status_frame))
    {
        LOG(WARNING) << "Unmarshal failed";
    }
    else
    {
        status = status_frame.power_msg.power_status;
    }

    auto command =
        nanopb_command.load(std::memory_order_relaxed);  // get value atomically
    auto frame                = createUartFrame(command);
    auto power_command_buffer = marshallUartPacket(frame);

    try
    {
        // Write power command
        uart->flushSerialPort(uart->flush_send);
        if (!uart->serialWrite(power_command_buffer))
        {
            LOG(WARNING) << "Writing power command failed.";
        }
    }
    catch (std::exception& e)
    {
        LOG(FATAL) << "ESP32 has disconnected. Power service has crashed" << e.what();
    }
}

TbotsProto::PowerStatus PowerService::poll(const TbotsProto::PowerControl& command,
                                           double kick_coeff, int kick_constant,
                                           int chip_constant)
{
    // Store msg for later transmission
    nanopb_command =
        createNanoPbPowerPulseControl(command, kick_coeff, kick_constant, chip_constant);
    return *createTbotsPowerStatus(status);
}
