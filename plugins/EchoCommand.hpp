#ifndef WORKFLOW_ENGINE_ECHO_COMMAND_HPP
#define WORKFLOW_ENGINE_ECHO_COMMAND_HPP

#include "ICommand.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief EchoCommand — the simplest possible pipeline filter.
 *
 * Logs its input DataPacket contents and the configured message,
 * then passes the input through unchanged. Useful for debugging
 * pipeline data flow.
 *
 * JSON config params:
 * @code
 * {
 *   "message": "Optional message to log",
 *   "log_input": true   // Whether to dump input keys
 * }
 * @endcode
 */
class EchoCommand : public ICommand {
public:
    /**
     * @param instance_name  Unique identifier for this command instance.
     * @param params         Command-specific JSON parameters.
     */
    EchoCommand(const std::string& instance_name, const nlohmann::json& params);

    /** Default constructor for template-based registration. */
    EchoCommand();

    std::string name() const override;

    Result<DataPacket> execute(
        const DataPacket& input,
        DataBus& bus,
        ILogger& logger
    ) override;

private:
    std::string instance_name_;
    std::string message_;
    bool log_input_ = true;
};

}

#endif