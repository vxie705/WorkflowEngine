#include "DelayCommand.hpp"
#include "CommandRegistry.hpp"
#include <nlohmann/json.hpp>
#include <thread>

namespace workflow {

DelayCommand::DelayCommand(const std::string& instance_name, const nlohmann::json& params)
    : instance_name_(instance_name)
{
    if (params.contains("duration_ms") && params["duration_ms"].is_number_integer()) {
        duration_ms_ = params["duration_ms"].get<int>();
    }
}

DelayCommand::DelayCommand()
    : instance_name_("DelayCommand")
{
}

std::string DelayCommand::name() const {
    return instance_name_;
}

Result<DataPacket> DelayCommand::execute(
    const DataPacket& input,
    DataBus& /*bus*/,
    ILogger& logger)
{
    if (duration_ms_ > 0) {
        logger.info("[" + instance_name_ + "] Delaying for " +
                    std::to_string(duration_ms_) + " ms");
        std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms_));
    } else {
        logger.debug("[" + instance_name_ + "] No delay configured, passing through");
    }


    return Result<DataPacket>::ok(input);
}

}


REGISTER_COMMAND(DelayCommand, "DelayCommand",
    [](const std::string& instance_name, const nlohmann::json& params)
        -> std::unique_ptr<workflow::ICommand> {
        return std::make_unique<workflow::DelayCommand>(instance_name, params);
    });
