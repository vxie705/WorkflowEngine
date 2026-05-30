#ifndef WORKFLOW_ENGINE_DELAY_COMMAND_HPP
#define WORKFLOW_ENGINE_DELAY_COMMAND_HPP

#include "ICommand.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief DelayCommand — introduces a configurable delay in the pipeline.
 *
 * Simulates long-running operations (network calls, computation).
 * Passes the input DataPacket through unchanged after the delay.
 *
 * JSON config params:
 * @code
 * {
 *   "duration_ms": 1000   // Delay in milliseconds (default: 0)
 * }
 * @endcode
 *
 * @note Real implementation would sleep. Stub for now.
 */
class DelayCommand : public ICommand {
public:
    DelayCommand(const std::string& instance_name, const nlohmann::json& params);
    DelayCommand();

    std::string name() const override;

    Result<DataPacket> execute(
        const DataPacket& input,
        DataBus& bus,
        ILogger& logger
    ) override;

private:
    std::string instance_name_;
    int duration_ms_ = 0;
};

}

#endif