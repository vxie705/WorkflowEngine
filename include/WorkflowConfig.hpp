#ifndef WORKFLOW_ENGINE_WORKFLOW_CONFIG_HPP
#define WORKFLOW_ENGINE_WORKFLOW_CONFIG_HPP

#include "Result.hpp"
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

#include <cstdint>

namespace workflow {

/**
 * @brief Describes a single command step in the pipeline.
 */
struct CommandConfig {
    /** The registered type name used to look up the factory (e.g., "EchoCommand"). */
    std::string type;
    /** A unique instance name for logging (e.g., "validate-1"). */
    std::string instance_name;
    /** Command-specific parameters as a JSON object. */
    nlohmann::json params;
    /** Optional: keys this command depends on (present on DataBus). */
    std::vector<std::string> depends_on;
};

/**
 * @brief Error handling strategy when a command fails unexpectedly.
 */
enum class OnError {
    HALT,
    CONTINUE
};

/**
 * @brief A complete workflow definition parsed from JSON.
 */
struct WorkflowDefinition {
    /** Human-readable workflow name. */
    std::string name;
    /** Optional description. */
    std::string description;
    /** Ordered list of commands forming the filter chain. */
    std::vector<CommandConfig> pipeline;
    /** Enable audit logging: write snapshots to logs/audit/ after each step. */
    bool audit = false;
    /** Behaviour when a command throws an exception. */
    OnError on_error = OnError::HALT;
};

/**
 * @brief Parses and validates workflow JSON configuration files.
 *
 * Loads a JSON file with the following schema:
 * @code
 * {
 *   "name": "user-onboarding",
 *   "description": "Process new user registration",
 *   "pipeline": [
 *     {
 *       "type": "ValidateInput",
 *       "instance_name": "validate",
 *       "params": { "schema": "user" },
 *       "depends_on": []
 *     }
 *   ]
 * }
 * @endcode
 */
class WorkflowConfig {
public:
    /**
     * @brief Load and parse a workflow definition from a JSON file.
     * @param path  Filesystem path to the .json file.
     * @return Result<WorkflowDefinition> on success, error on failure.
     */
    static Result<WorkflowDefinition> load_from_file(const std::string& path);

    /**
     * @brief Load and parse a workflow definition from an in-memory JSON object.
     * @param j  A parsed nlohmann::json object.
     * @return Result<WorkflowDefinition> on success, error on failure.
     */
    static Result<WorkflowDefinition> load_from_json(const nlohmann::json& j);
};

}

#endif