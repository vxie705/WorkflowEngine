#ifndef WORKFLOW_ENGINE_TRANSFORM_COMMAND_HPP
#define WORKFLOW_ENGINE_TRANSFORM_COMMAND_HPP

#include "ICommand.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief TransformCommand — applies a transformation to a value in the DataPacket.
 *
 * Reads from @p input_key, transforms the value (uppercase or prefix), and writes
 * the result to @p output_key.
 *
 * JSON config params:
 * @code
 * {
 *   "input_key":  "raw_user",     // key to read from DataPacket
 *   "output_key": "processed",    // key to write result to
 *   "prefix":     "ID_"           // (optional) prefix to prepend
 * }
 * @endcode
 *
 * Supported input types: std::string.
 * If input_key is missing, returns Result::error.
 */
class TransformCommand : public ICommand {
public:
    TransformCommand(const std::string& instance_name, const nlohmann::json& params);
    TransformCommand();

    std::string name() const override;

    Result<DataPacket> execute(
        const DataPacket& input,
        DataBus& bus,
        ILogger& logger
    ) override;

private:
    std::string instance_name_;
    std::string input_key_;
    std::string output_key_;
    std::string prefix_;
};

}

#endif