#include "TransformCommand.hpp"
#include "CommandRegistry.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>

namespace workflow {

TransformCommand::TransformCommand(const std::string& instance_name,
                                    const nlohmann::json& params)
    : instance_name_(instance_name)
    , input_key_("input")
    , output_key_("output")
{
    if (params.contains("input_key") && params["input_key"].is_string()) {
        input_key_ = params["input_key"].get<std::string>();
    }
    if (params.contains("output_key") && params["output_key"].is_string()) {
        output_key_ = params["output_key"].get<std::string>();
    }
    if (params.contains("prefix") && params["prefix"].is_string()) {
        prefix_ = params["prefix"].get<std::string>();
    }
}

TransformCommand::TransformCommand()
    : instance_name_("TransformCommand")
    , input_key_("input")
    , output_key_("output")
{
}

std::string TransformCommand::name() const {
    return instance_name_;
}

Result<DataPacket> TransformCommand::execute(
    const DataPacket& input,
    DataBus& /*bus*/,
    ILogger& logger)
{
    logger.info("[" + instance_name_ + "] TransformCommand: reading '" +
                input_key_ + "', writing to '" + output_key_ + "'" +
                (prefix_.empty() ? "" : " with prefix '" + prefix_ + "'"));


    auto value_result = input.get<std::string>(input_key_);
    if (value_result.is_error()) {
        std::string msg = "[" + instance_name_ + "] Key '" + input_key_ +
                          "' not found or wrong type in DataPacket";
        logger.error(msg);
        return Result<DataPacket>::error(msg);
    }

    std::string value = value_result.value();
    logger.debug("[" + instance_name_ + "] Raw value: '" + value + "'");


    std::string transformed = value;
    std::transform(transformed.begin(), transformed.end(),
                   transformed.begin(),
                   [](unsigned char c) { return std::toupper(c); });


    if (!prefix_.empty()) {
        transformed = prefix_ + transformed;
    }

    logger.debug("[" + instance_name_ + "] Transformed value: '" + transformed + "'");


    DataPacket output = input;
    output.set<std::string>(output_key_, transformed);




    return Result<DataPacket>::ok(std::move(output));
}

}


REGISTER_COMMAND(TransformCommand, "TransformCommand",
    [](const std::string& instance_name, const nlohmann::json& params)
        -> std::unique_ptr<workflow::ICommand> {
        return std::make_unique<workflow::TransformCommand>(instance_name, params);
    });
