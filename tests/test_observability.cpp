#include "WorkflowEngine.hpp"
#include "WorkflowConfig.hpp"
#include "DataPacket.hpp"
#include "DataBus.hpp"
#include "ConsoleLogger.hpp"
#include "ILogger.hpp"
#include "Result.hpp"
#include "CommandRegistry.hpp"
#include "WorkflowSchema.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace workflow;
namespace fs = std::filesystem;




class AlwaysErrorCommand : public ICommand {
public:
    std::string name() const override { return "AlwaysError"; }
    Result<DataPacket> execute(const DataPacket&, DataBus&, ILogger&) override {
        return Result<DataPacket>::error("Simulated command failure", 42);
    }
};




class AlwaysThrowCommand : public ICommand {
public:
    std::string name() const override { return "AlwaysThrow"; }
    Result<DataPacket> execute(const DataPacket&, DataBus&, ILogger&) override {
        throw std::runtime_error("Simulated exception!");
    }
};




class SimpleEchoCommand : public ICommand {
public:
    std::string name() const override { return "SimpleEcho"; }
    Result<DataPacket> execute(const DataPacket& input, DataBus& bus, ILogger&) override {
        DataPacket out = input;
        bus.publish("echoed", std::string("ok"));
        return Result<DataPacket>::ok(std::move(out));
    }
};





/** Remove directory tree recursively. */
static void remove_directory(const std::string& dir) {
    std::error_code ec;
    fs::remove_all(dir, ec);
}

/** Read all lines from a text file (for log inspection). */
static std::vector<std::string> read_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(line);
    }
    return lines;
}

/** Count JSON snapshot files matching a pattern in the given directory. */
static size_t count_snapshot_files(const std::string& dir,
                                   const std::string& workflow_name) {
    size_t count = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (entry.is_regular_file() &&
            entry.path().filename().string().rfind(workflow_name, 0) == 0) {
            ++count;
        }
    }
    return count;
}





static int passed = 0;
static int failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "FAIL: %s\n", msg); \
        ++failed; \
    } else { \
        ++passed; \
    } \
} while(0)

inline WorkflowEngine make_engine(std::unique_ptr<ILogger> logger = nullptr) {
    if (!logger) {
        logger = std::make_unique<ConsoleLogger>();
    }
    return WorkflowEngine(std::move(logger), std::make_unique<DataBus>());
}




static void test_databus_to_json() {
    DataBus bus;
    bus.publish("int_val", 42);
    bus.publish("str_val", std::string("hello"));
    bus.publish("bool_val", true);
    bus.publish("double_val", 3.14);

    auto j = bus.to_json();
    CHECK(j.is_object(), "to_json() returns object");
    CHECK(j["int_val"] == 42, "int serialized");
    CHECK(j["str_val"] == "hello", "string serialized");
    CHECK(j["bool_val"] == true, "bool serialized");


    auto ci = bus.consume<int>("int_val");
    CHECK(ci.is_ok(), "consume int");
    CHECK(ci.value() == 42, "consume int value matches");
}




static void test_workflow_config_audit_and_on_error() {
    nlohmann::json j;
    j["name"] = "test-audit";
    j["pipeline"] = nlohmann::json::array({
        {{"type", "EchoCommand"}}
    });
    j["audit"] = true;
    j["on_error"] = "continue";

    auto result = WorkflowConfig::load_from_json(j);
    CHECK(result.is_ok(), "parse with audit + on_error");
    auto wf = result.value();
    CHECK(wf.audit == true, "audit flag parsed");
    CHECK(wf.on_error == OnError::CONTINUE, "on_error continue parsed");


    nlohmann::json j2;
    j2["name"] = "test-defaults";
    j2["pipeline"] = nlohmann::json::array({
        {{"type", "EchoCommand"}}
    });
    auto result2 = WorkflowConfig::load_from_json(j2);
    CHECK(result2.is_ok(), "parse without audit/on_error");
    auto wf2 = result2.value();
    CHECK(wf2.audit == false, "audit defaults to false");
    CHECK(wf2.on_error == OnError::HALT, "on_error defaults to halt");
}




static void test_exception_halt() {
    auto engine = make_engine();
    engine.sync_from_registry();


    engine.register_command_factory("ThrowingCommand",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<AlwaysThrowCommand>();
        });

    WorkflowDefinition wf;
    wf.name = "test-exception-halt";
    wf.on_error = OnError::HALT;
    wf.audit = false;
    wf.pipeline = {
        {"ThrowingCommand", "step1", {}, {}}
    };

    DataPacket input;
    input.set("data", std::string("payload"));

    auto result = engine.execute(wf, input);
    CHECK(result.is_error(), "engine returns error on exception with HALT");
    CHECK(result.error_message().find("Simulated exception") != std::string::npos,
          "error message contains exception text");
}




static void test_exception_continue() {
    auto engine = make_engine();
    engine.sync_from_registry();

    engine.register_command_factory("ThrowingCommand",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<AlwaysThrowCommand>();
        });
    engine.register_command_factory("SimpleEcho",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<SimpleEchoCommand>();
        });

    WorkflowDefinition wf;
    wf.name = "test-exception-continue";
    wf.on_error = OnError::CONTINUE;
    wf.audit = false;
    wf.pipeline = {
        {"ThrowingCommand", "step1", {}, {}},
        {"SimpleEcho", "step2", {}, {}}
    };

    DataPacket input;
    input.set("data", std::string("payload"));

    auto result = engine.execute(wf, input);
    CHECK(result.is_ok(), "engine succeeds despite exception with CONTINUE");
    CHECK(result.value().has("data"), "output retains data from input");
}




static void test_result_error_halt() {
    auto engine = make_engine();
    engine.sync_from_registry();

    engine.register_command_factory("AlwaysError",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<AlwaysErrorCommand>();
        });

    WorkflowDefinition wf;
    wf.name = "test-error-halt";
    wf.on_error = OnError::HALT;
    wf.audit = false;
    wf.pipeline = {
        {"AlwaysError", "step1", {}, {}}
    };

    DataPacket input;
    auto result = engine.execute(wf, input);
    CHECK(result.is_error(), "engine returns error on Result::error with HALT");
    CHECK(result.error_message().find("Simulated command failure") != std::string::npos,
          "error message contains command error text");
    CHECK(result.error_code() == 42, "error code propagated");
}




static void test_result_error_continue() {
    auto engine = make_engine();
    engine.sync_from_registry();

    engine.register_command_factory("AlwaysError",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<AlwaysErrorCommand>();
        });
    engine.register_command_factory("SimpleEcho",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<SimpleEchoCommand>();
        });

    WorkflowDefinition wf;
    wf.name = "test-error-continue";
    wf.on_error = OnError::CONTINUE;
    wf.audit = false;
    wf.pipeline = {
        {"AlwaysError", "step1", {}, {}},
        {"SimpleEcho", "step2", {}, {}}
    };

    DataPacket input;
    input.set("key", std::string("value"));

    auto result = engine.execute(wf, input);
    CHECK(result.is_ok(), "engine succeeds despite Result::error with CONTINUE");
    CHECK(result.value().has("key"), "output retains data through failed step");
}




static void test_audit_snapshots_created() {
    const std::string audit_test_dir = "logs/test_audit";
    remove_directory(audit_test_dir);

    auto engine = make_engine();
    engine.sync_from_registry();
    engine.set_audit_dir(audit_test_dir);

    WorkflowDefinition wf;
    wf.name = "test-audit-snapshots";
    wf.on_error = OnError::HALT;
    wf.audit = true;
    wf.pipeline = {
        {"EchoCommand", "step1",
         nlohmann::json::object({{"message", "hello"}}), {}},
    };

    DataPacket input;
    input.set("test", std::string("audit-data"));

    auto result = engine.execute(wf, input);
    CHECK(result.is_ok(), "pipeline completes with audit enabled");


    size_t count = count_snapshot_files(audit_test_dir, "test-audit-snapshots");
    CHECK(count > 0, "audit snapshot files are created");


    CHECK(count >= 2, "at least 2 snapshots (pre + post) for 1 step");


    bool found_success = false;
    bool found_pre = false;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(audit_test_dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        std::ifstream f(entry.path());
        auto json_data = nlohmann::json::parse(f);
        if (json_data.contains("status")) {
            if (json_data["status"] == "success") found_success = true;
        }
        if (json_data.contains("snapshot") &&
            json_data["snapshot"].contains("phase") &&
            json_data["snapshot"]["phase"] == "pre-execution") {
            found_pre = true;
        }
    }
    CHECK(found_success, "at least one success snapshot exists");
    CHECK(found_pre, "at least one pre-execution snapshot exists");


    remove_directory(audit_test_dir);
}




static void test_audit_post_mortem_on_exception() {
    const std::string audit_test_dir = "logs/test_audit_pm";
    remove_directory(audit_test_dir);

    auto engine = make_engine();
    engine.sync_from_registry();
    engine.set_audit_dir(audit_test_dir);

    engine.register_command_factory("ThrowingCommand",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<AlwaysThrowCommand>();
        });
    engine.register_command_factory("SimpleEcho",
        [](const std::string&, const nlohmann::json&) {
            return std::make_unique<SimpleEchoCommand>();
        });

    WorkflowDefinition wf;
    wf.name = "test-postmortem";
    wf.on_error = OnError::HALT;
    wf.audit = true;
    wf.pipeline = {
        {"ThrowingCommand", "step1", {}, {}},
        {"SimpleEcho", "step2", {}, {}}
    };

    DataPacket input;
    input.set("forensic", std::string("critical-data"));

    auto result = engine.execute(wf, input);
    CHECK(result.is_error(), "pipeline fails as expected");


    bool found_exception_snapshot = false;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(audit_test_dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;

        std::ifstream f(entry.path());
        auto json_data = nlohmann::json::parse(f);
        if (json_data.contains("status") &&
            json_data["status"] == "exception") {
            found_exception_snapshot = true;

            CHECK(json_data["snapshot"]["data_packet"].contains("forensic"),
                  "post-mortem snapshot contains forensic data");
        }
    }
    CHECK(found_exception_snapshot, "post-mortem exception snapshot exists");

    remove_directory(audit_test_dir);
}




static void test_audit_disabled_no_files() {
    const std::string audit_test_dir = "logs/test_audit_off";
    remove_directory(audit_test_dir);

    remove_directory("logs/audit");

    auto engine = make_engine();
    engine.sync_from_registry();
    engine.set_audit_dir(audit_test_dir);

    WorkflowDefinition wf;
    wf.name = "test-audit-disabled";
    wf.on_error = OnError::HALT;
    wf.audit = false;
    wf.pipeline = {
        {"EchoCommand", "step1",
         nlohmann::json::object({{"message", "quiet"}}), {}},
    };

    DataPacket input;
    auto result = engine.execute(wf, input);
    CHECK(result.is_ok(), "pipeline completes");

    size_t count = count_snapshot_files(audit_test_dir, "test-audit-disabled");
    CHECK(count == 0, "no snapshot files when audit is disabled");

    remove_directory(audit_test_dir);
}




static void test_databus_to_json_unsupported() {
    DataBus bus;

    struct CustomData { int x; };
    bus.publish("custom", CustomData{99});

    auto j = bus.to_json();
    CHECK(j.is_object(), "to_json still returns object");
    CHECK(j.contains("custom"), "unsupported key is present");
    CHECK(j["custom"] == "<unsupported-type>", "unsupported type renders as marker");
}




static void test_engine_null_bus_auto_create() {
    auto engine = WorkflowEngine(
        std::make_unique<ConsoleLogger>(),
        nullptr
    );
    engine.sync_from_registry();

    WorkflowDefinition wf;
    wf.name = "test-null-bus";
    wf.pipeline = {
        {"EchoCommand", "step1",
         nlohmann::json::object({{"message", "test"}}), {}},
    };

    DataPacket input;
    auto result = engine.execute(wf, input);
    CHECK(result.is_ok(), "engine works with auto-created bus");
}




static void test_schema_on_error_validation() {

    nlohmann::json j1;
    j1["name"] = "test";
    j1["pipeline"] = nlohmann::json::array({
        {{"type", "EchoCommand"}}
    });
    j1["on_error"] = "halt";
    CHECK(WorkflowSchema::validate(j1).is_ok(), "'halt' is valid");


    nlohmann::json j2 = j1;
    j2["on_error"] = "continue";
    CHECK(WorkflowSchema::validate(j2).is_ok(), "'continue' is valid");


    nlohmann::json j3 = j1;
    j3["on_error"] = 42;
    CHECK(WorkflowSchema::validate(j3).is_error(), "numeric on_error is rejected");


    nlohmann::json j4 = j1;
    j4["on_error"] = "retry";
    CHECK(WorkflowSchema::validate(j4).is_error(), "'retry' is rejected");
}




static void test_schema_audit_validation() {
    nlohmann::json j;
    j["name"] = "test";
    j["pipeline"] = nlohmann::json::array({
        {{"type", "EchoCommand"}}
    });
    j["audit"] = true;
    CHECK(WorkflowSchema::validate(j).is_ok(), "audit boolean is valid");

    nlohmann::json j2 = j;
    j2["audit"] = "yes";
    CHECK(WorkflowSchema::validate(j2).is_error(), "string audit is rejected");
}




int main() {
    fprintf(stderr, "=== Observability & Resilience Tests ===\n\n");

    test_databus_to_json();
    test_workflow_config_audit_and_on_error();
    test_exception_halt();
    test_exception_continue();
    test_result_error_halt();
    test_result_error_continue();
    test_audit_snapshots_created();
    test_audit_post_mortem_on_exception();
    test_audit_disabled_no_files();
    test_databus_to_json_unsupported();
    test_engine_null_bus_auto_create();
    test_schema_on_error_validation();
    test_schema_audit_validation();

    fprintf(stderr, "\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}