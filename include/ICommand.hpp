#ifndef WORKFLOW_ENGINE_ICOMMAND_HPP
#define WORKFLOW_ENGINE_ICOMMAND_HPP

#include "DataPacket.hpp"
#include "DataBus.hpp"
#include "ILogger.hpp"
#include <string>
#include <memory>

namespace workflow {

/**
 * @brief Base interface for all pipeline commands (Command Pattern).
 *
 * Every command encapsulates its own logic behind a uniform execute()
 * signature. The WorkflowEngine:
 *   1. Loads a JSON config listing command types in order.
 *   2. Looks up each type in its factory registry.
 *   3. Instantiates the command (DI: logger is injected by the engine).
 *   4. Calls execute(input, bus, logger) in sequence.
 *
 * Commands MUST return Result<DataPacket>. If is_error(),
 * the engine halts the pipeline.
 *
 * @note Commands should NOT perform I/O that ties them to a specific
 *       execution context. All external dependencies (logging, shared
 *       state) arrive via the injected parameters.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /**
     * @brief Human-readable name for this command instance.
     * Used in logs and error messages. Example: "ValidateUserInput".
     */
    virtual std::string name() const = 0;

    /**
     * @brief Execute the command's logic.
     *
     * @param input   The DataPacket from the previous filter
     *                (or the initial packet for the first command).
     * @param bus     Shared communication bus for cross-command data exchange.
     *                Publish values here for downstream commands to consume.
     * @param logger  Injected logger. Commands log through this interface
     *                rather than std::cout for testability.
     *
     * @return Result<DataPacket>::ok(output)  on success.
     * @return Result<DataPacket>::error(msg)  on failure. The engine
     *         will halt and propagate this error.
     */
    virtual Result<DataPacket> execute(
        const DataPacket& input,
        DataBus& bus,
        ILogger& logger
    ) = 0;
};

}

#endif