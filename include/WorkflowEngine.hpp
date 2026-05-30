#ifndef WORKFLOW_ENGINE_WORKFLOW_ENGINE_HPP
#define WORKFLOW_ENGINE_WORKFLOW_ENGINE_HPP

#include "DataPacket.hpp"
#include "DataBus.hpp"
#include "ICommand.hpp"
#include "ILogger.hpp"
#include "IPlugin.hpp"
#include "Result.hpp"
#include "WorkflowConfig.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief Central orchestrator: loads JSON configs and executes command pipelines.
 *
 * The WorkflowEngine follows the PIPE & FILTER architecture:
 *   - Reads a JSON workflow definition (WorkflowDefinition).
 *   - For each CommandConfig in pipeline order:
 *       1. Looks up the command type in the internal factory registry.
 *       2. Instantiates the ICommand via the factory.
 *       3. Calls command->execute(input, bus, logger) inside a try-catch block.
 *       4. If an exception is caught or Result::is_error(), the engine
 *          handles it according to the workflow's `on_error` setting
 *          (HALT or CONTINUE).
 *       5. On success, the output becomes the input for the next command.
 *       6. If `audit: true`, a snapshot (JSON) of DataPacket + DataBus
 *          is written to logs/audit/ after every step.
 *
 * ## Observability & Resilience (Industrial-Grade)
 *   - **Try-catch isolation**: Each command is wrapped in try-catch.
 *     Exceptions from plugins never crash the engine.
 *   - **Snapshotting**: Pre- and post-execution state saved to JSON files
 *     when `audit: true`.
 *   - **Post-mortem**: On failure, the pre-failure DataPacket is snapshotted
 *     for forensic analysis.
 *   - **Audit trail**: Complete timeline of pipeline execution in
 *     logs/audit/ directory.
 *
 * ## Dependency Injection
 * WorkflowEngine is constructed with a logger and a DataBus. Every command
 * receives references to these during execution.
 *
 * ## Memory Management
 * All ownership is expressed via std::unique_ptr.
 */
class WorkflowEngine {
public:
    /**
     * @brief Signature for a command factory function.
     */
    using CommandFactory = std::function<std::unique_ptr<ICommand>(
        const std::string& instance_name,
        const nlohmann::json& params)>;



    WorkflowEngine(
        std::unique_ptr<ILogger> logger,
        std::unique_ptr<DataBus> bus
    );

    ~WorkflowEngine();


    WorkflowEngine(const WorkflowEngine&) = delete;
    WorkflowEngine& operator=(const WorkflowEngine&) = delete;
    WorkflowEngine(WorkflowEngine&&) noexcept;
    WorkflowEngine& operator=(WorkflowEngine&&) noexcept;



    template <typename Cmd>
    void register_command(const std::string& type_name) {
        registry_[type_name] = [](const std::string&, const nlohmann::json&) {
            return std::make_unique<Cmd>();
        };
    }

    void register_command_factory(const std::string& type_name,
                                  CommandFactory factory);

    void sync_from_registry();

    size_t load_plugins(const std::string& plugin_dir);
    size_t load_plugins_and_sync(const std::string& plugin_dir);



    /**
     * @brief Override the default audit output directory.
     * Default: "logs/audit"
     */
    void set_audit_dir(const std::string& dir) { audit_dir_ = dir; }

    /**
     * @brief Returns the current audit directory path.
     */
    const std::string& audit_dir() const { return audit_dir_; }



    Result<DataPacket> execute_from_file(const std::string& config_path);
    Result<DataPacket> execute_from_file(const std::string& config_path,
                                         DataPacket initial);
    Result<DataPacket> execute(const WorkflowDefinition& config);
    Result<DataPacket> execute(const WorkflowDefinition& config,
                               DataPacket initial);

    /**
     * @brief Returns the number of registered command types.
     */
    size_t command_count() const noexcept { return registry_.size(); }

private:
    /**
     * @brief Core pipeline executor with full observability hooks.
     *
     * For each command in the pipeline:
     *  1. Instantiate via factory.
     *  2. If `audit_enabled`, capture pre-execution snapshot.
     *  3. Execute inside try-catch.
     *  4. On exception: write post-mortem snapshot, halt or continue.
     *  5. On Result::error: write error snapshot, halt or continue.
     *  6. On success: write post-execution snapshot, advance DataPacket.
     */
    Result<DataPacket> execute_pipeline(
        const WorkflowDefinition& config,
        DataPacket initial_input);

    /**
     * @brief Write an audit snapshot JSON file to the audit directory.
     *
     * Files are named: {workflow}_step{N}_{command}_{status}.json
     * Written atomically (temp file + rename).
     */
    void write_audit_snapshot(
        const std::string& workflow_name,
        size_t step_index,
        const std::string& command_name,
        const std::string& status,
        const nlohmann::json& snapshot);

    /**
     * @brief Build a comprehensive snapshot of current DataPacket + DataBus state.
     */
    nlohmann::json build_snapshot(
        const DataPacket& data_packet,
        const DataBus& bus);

    std::unordered_map<std::string, CommandFactory> registry_;
    std::unique_ptr<ILogger> logger_;
    std::unique_ptr<DataBus> bus_;
    PluginLoader plugin_loader_;
    std::string audit_dir_ = "logs/audit";
};

}

#endif