#include "EchoCommand.hpp"
#include "CommandRegistry.hpp"
#include <nlohmann/json.hpp>

namespace workflow {

EchoCommand::EchoCommand(const std::string& instance_name, const nlohmann::json& params)
    : instance_name_(instance_name)
    , log_input_(true)
{
    if (params.contains("message") && params["message"].is_string()) {
        message_ = params["message"].get<std::string>();
    }
    if (params.contains("log_input") && params["log_input"].is_boolean()) {
        log_input_ = params["log_input"].get<bool>();
    }
}

EchoCommand::EchoCommand()
    : instance_name_("EchoCommand")
    , log_input_(true)
{
}

std::string EchoCommand::name() const {
    return instance_name_;
}

Result<DataPacket> EchoCommand::execute(
    const DataPacket& input,
    DataBus& /*bus*/,
    ILogger& logger)
{
    logger.info("[" + instance_name_ + "] " +
                (message_.empty() ? "Echoing data packet" : message_));

    if (log_input_) {
        auto keys = input.keys();
        logger.debug("[" + instance_name_ + "] Input DataPacket has " +
                     std::to_string(keys.size()) + " entries:");
        for (const auto& key : keys) {
            logger.debug("  - " + key);
        }
    }


    const std::string output_key = "output_key";
    auto output_result = input.get<std::string>(output_key);
    if (output_result.is_ok()) {
        logger.info("[" + instance_name_ + "] Valor de '" + output_key +
                    "' encontrado: '" + output_result.value() + "'");
    } else {
        logger.warn("[" + instance_name_ + "] Clave '" + output_key +
                    "' no encontrada en el DataPacket");
    }


    return Result<DataPacket>::ok(input);
}

}


REGISTER_COMMAND(EchoCommand, "EchoCommand",
    [](const std::string& instance_name, const nlohmann::json& params)
        -> std::unique_ptr<workflow::ICommand> {
        return std::make_unique<workflow::EchoCommand>(instance_name, params);
    });
