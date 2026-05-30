#include "WorkflowSchema.hpp"
#include <nlohmann/json.hpp>

namespace workflow {

Result<void> WorkflowSchema::validate(const nlohmann::json& j) {

    if (!j.contains("name")) {
        return Result<void>::error("Schema error: top-level 'name' field is missing");
    }
    if (!j["name"].is_string()) {
        return Result<void>::error("Schema error: 'name' must be a string, got "
                                   + std::string(j["name"].type_name()));
    }
    if (j["name"].get<std::string>().empty()) {
        return Result<void>::error("Schema error: 'name' must not be an empty string");
    }


    if (j.contains("description")) {
        if (!j["description"].is_string()) {
            return Result<void>::error("Schema error: 'description' must be a string, got "
                                       + std::string(j["description"].type_name()));
        }
    }


    if (j.contains("audit")) {
        if (!j["audit"].is_boolean()) {
            return Result<void>::error("Schema error: 'audit' must be a boolean, got "
                                       + std::string(j["audit"].type_name()));
        }
    }


    if (j.contains("on_error")) {
        if (!j["on_error"].is_string()) {
            return Result<void>::error("Schema error: 'on_error' must be a string ('halt' or 'continue'), got "
                                       + std::string(j["on_error"].type_name()));
        }
        const auto val = j["on_error"].get<std::string>();
        if (val != "halt" && val != "continue") {
            return Result<void>::error(
                "Schema error: 'on_error' must be 'halt' or 'continue', got '" + val + "'");
        }
    }


    if (!j.contains("pipeline")) {
        return Result<void>::error("Schema error: 'pipeline' field is missing");
    }
    if (!j["pipeline"].is_array()) {
        return Result<void>::error("Schema error: 'pipeline' must be an array, got "
                                   + std::string(j["pipeline"].type_name()));
    }

    const auto& pipeline = j["pipeline"];
    if (pipeline.empty()) {
        return Result<void>::error("Schema error: 'pipeline' array must not be empty");
    }


    for (size_t i = 0; i < pipeline.size(); ++i) {
        auto entry_result = validate_pipeline_entry(pipeline[i], i);
        if (entry_result.is_error()) {
            return entry_result;
        }
    }

    return Result<void>::ok();
}

Result<void> WorkflowSchema::validate_pipeline_entry(
    const nlohmann::json& entry,
    size_t index)
{
    const std::string prefix = "Schema error at pipeline[" + std::to_string(index) + "]: ";


    if (!entry.is_object()) {
        return Result<void>::error(prefix + "entry must be a JSON object, got "
                                   + std::string(entry.type_name()));
    }


    if (!entry.contains("type")) {
        return Result<void>::error(prefix + "'type' field is missing");
    }
    if (!entry["type"].is_string()) {
        return Result<void>::error(prefix + "'type' must be a string, got "
                                   + std::string(entry["type"].type_name()));
    }
    if (entry["type"].get<std::string>().empty()) {
        return Result<void>::error(prefix + "'type' must not be an empty string");
    }


    if (entry.contains("instance_name")) {
        if (!entry["instance_name"].is_string()) {
            return Result<void>::error(prefix + "'instance_name' must be a string, got "
                                       + std::string(entry["instance_name"].type_name()));
        }
    }


    if (entry.contains("params")) {
        if (!entry["params"].is_object()) {
            return Result<void>::error(prefix + "'params' must be a JSON object, got "
                                       + std::string(entry["params"].type_name()));
        }
    }


    if (entry.contains("depends_on")) {
        if (!entry["depends_on"].is_array()) {
            return Result<void>::error(prefix + "'depends_on' must be a JSON array, got "
                                       + std::string(entry["depends_on"].type_name()));
        }
        for (size_t d = 0; d < entry["depends_on"].size(); ++d) {
            if (!entry["depends_on"][d].is_string()) {
                return Result<void>::error(
                    prefix + "'depends_on' entries must be strings; "
                    "index " + std::to_string(d) + " is of type "
                    + std::string(entry["depends_on"][d].type_name()));
            }
        }
    }

    return Result<void>::ok();
}

}