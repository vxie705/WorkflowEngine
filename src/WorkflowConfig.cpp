#include "WorkflowConfig.hpp"
#include "WorkflowSchema.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using nlohmann::json;

namespace workflow {

Result<WorkflowDefinition> WorkflowConfig::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<WorkflowDefinition>::error(
            "WorkflowConfig: cannot open file '" + path + "'");
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        return Result<WorkflowDefinition>::error(
            "WorkflowConfig: JSON parse error in '" + path + "': " + e.what(),
            e.id);
    }

    return load_from_json(j);
}

Result<WorkflowDefinition> WorkflowConfig::load_from_json(const json& j) {

    auto schema_result = WorkflowSchema::validate(j);
    if (schema_result.is_error()) {
        return Result<WorkflowDefinition>::error(
            schema_result.error_message(),
            schema_result.error_code());
    }


    WorkflowDefinition wf;

    wf.name = j["name"].get<std::string>();

    if (j.contains("description")) {
        wf.description = j["description"].get<std::string>();
    }


    if (j.contains("audit") && j["audit"].is_boolean()) {
        wf.audit = j["audit"].get<bool>();
    }


    if (j.contains("on_error") && j["on_error"].is_string()) {
        const auto strategy = j["on_error"].get<std::string>();
        if (strategy == "continue") {
            wf.on_error = OnError::CONTINUE;
        } else {
            wf.on_error = OnError::HALT;
        }
    }


    int index = 0;
    for (const auto& cmd_json : j["pipeline"]) {
        CommandConfig cfg;
        cfg.type = cmd_json["type"].get<std::string>();

        if (cmd_json.contains("instance_name") && cmd_json["instance_name"].is_string()) {
            cfg.instance_name = cmd_json["instance_name"].get<std::string>();
        } else {
            cfg.instance_name = cfg.type + "_" + std::to_string(index);
        }

        if (cmd_json.contains("params")) {
            cfg.params = cmd_json["params"];
        } else {
            cfg.params = json::object();
        }

        if (cmd_json.contains("depends_on") && cmd_json["depends_on"].is_array()) {
            for (const auto& dep : cmd_json["depends_on"]) {
                cfg.depends_on.push_back(dep.get<std::string>());
            }
        }

        wf.pipeline.push_back(std::move(cfg));
        ++index;
    }

    return Result<WorkflowDefinition>::ok(wf);
}

}