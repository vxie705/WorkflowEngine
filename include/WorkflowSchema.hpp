#ifndef WORKFLOW_ENGINE_WORKFLOW_SCHEMA_HPP
#define WORKFLOW_ENGINE_WORKFLOW_SCHEMA_HPP

#include "Result.hpp"

#include <string>
#include <nlohmann/json.hpp>

namespace workflow {

/**
 * @brief Structural validator for workflow.json configuration files.
 *
 * Uses nlohmann/json type-checking to validate that a workflow JSON document
 * conforms to the expected schema BEFORE the engine attempts execution.
 *
 * ## Validated Rules
 *   - Top-level "name" must be a non-empty string (required)
 *   - Top-level "description" is optional but must be a string if present
 *   - Top-level "pipeline" must be a non-empty array (required)
 *   - Each pipeline entry must be an object
 *   - Each pipeline entry must have "type" (non-empty string)
 *   - "instance_name" is optional but must be a string if present
 *   - "params" is optional but must be an object if present
 *   - "depends_on" is optional but must be a string array if present
 *
 * ## Validation Failure
 * Returns Result<void> with a detailed error message so the engine aborts
 * before touching any ICommand or DataBus.
 */
class WorkflowSchema {
public:
    /**
     * @brief Validate a JSON object against the workflow schema.
     * @param j  The parsed nlohmann::json object.
     * @return Result<void>::ok(), or Result<void>::error(msg) with explanation.
     */
    static Result<void> validate(const nlohmann::json& j);

private:
    static Result<void> validate_pipeline_entry(const nlohmann::json& entry, size_t index);
};

}

#endif