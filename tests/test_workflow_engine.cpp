#include "WorkflowEngine.hpp"
#include "DataPacket.hpp"
#include "Result.hpp"
#include "mocks/MockLogger.hpp"
#include "mocks/MockCommand.hpp"

#include <iostream>
#include <memory>
#include <cassert>

using namespace workflow;
using namespace workflow::testing;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    do { \
        std::cout << "  TEST: " << (name) << " ... "; \
    } while(0)

#define PASS() \
    do { \
        std::cout << "PASSED" << std::endl; \
        tests_passed++; \
    } while(0)

#define FAIL(msg) \
    do { \
        std::cout << "FAILED: " << (msg) << std::endl; \
        tests_failed++; \
    } while(0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { FAIL(#expr); return; } \
    } while(0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { FAIL(#expr " (expected false)"); return; } \
    } while(0)



void test_engine_empty_pipeline_runs_trivially() {
    TEST("Engine with empty pipeline returns ok");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));

    WorkflowDefinition config;
    config.name = "empty-pipeline";
    config.description = "No commands";

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_ok());

    ASSERT_TRUE(result.value().size() == 0);
    PASS();
}

void test_engine_single_success_command() {
    TEST("Engine with a single success command returns its output");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));


    engine.register_command<MockCommand>("MockSuccess");

    WorkflowDefinition config;
    config.name = "single-success";
    CommandConfig cmd_cfg;
    cmd_cfg.type = "MockSuccess";
    cmd_cfg.instance_name = "test-command";
    config.pipeline.push_back(cmd_cfg);


    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_ok());
    PASS();
}

void test_engine_error_command_halt_pipeline() {
    TEST("Engine halts pipeline when a command returns error");
    auto logger = std::make_unique<MockLogger>();
    MockLogger* logger_ptr = logger.get();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));


    engine.register_command_factory("FailingCommand",
        [](const std::string&, const nlohmann::json& /*params*/) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<MockCommand>("FailingCommand");
            cmd->set_error_result("intentional failure for testing", 42);
            return cmd;
        });

    WorkflowDefinition config;
    CommandConfig cmd_cfg;
    cmd_cfg.type = "FailingCommand";
    cmd_cfg.instance_name = "should-fail";
    config.pipeline.push_back(cmd_cfg);

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_error());
    ASSERT_TRUE(result.error_code() == 42);


    const auto& calls = logger_ptr->calls();
    bool found_error = false;
    for (const auto& entry : calls) {
        LogLevel level = std::get<0>(entry);
        const std::string& msg = std::get<1>(entry);
        if (level == LogLevel::Error && msg.find("intentional failure") != std::string::npos) {
            found_error = true;
            break;
        }
    }
    ASSERT_TRUE(found_error);
    PASS();
}

void test_engine_pipeline_data_flow() {
    TEST("Engine passes DataPacket through pipeline commands");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));


    class AppendCommand : public ICommand {
    public:
        std::string name() const override { return "AppendCommand"; }
        Result<DataPacket> execute(const DataPacket& input, DataBus&, ILogger&) override {
            DataPacket output = input;
            output.set(key_, value_);
            return Result<DataPacket>::ok(std::move(output));
        }
        void set_key_value(const std::string& k, int v) {
            key_ = k;
            value_ = v;
        }
    private:
        std::string key_ = "default";
        int value_ = 0;
    };

    engine.register_command_factory("Append",
        [](const std::string&, const nlohmann::json& params) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<AppendCommand>();
            if (params.contains("key") && params.contains("value")) {
                cmd->set_key_value(
                    params["key"].get<std::string>(),
                    params["value"].get<int>());
            }
            return cmd;
        });

    WorkflowDefinition config;
    config.name = "data-flow-test";

    CommandConfig cmd1;
    cmd1.type = "Append";
    cmd1.instance_name = "step1";
    cmd1.params = nlohmann::json{{"key", "a"}, {"value", 1}};
    config.pipeline.push_back(cmd1);

    CommandConfig cmd2;
    cmd2.type = "Append";
    cmd2.instance_name = "step2";
    cmd2.params = nlohmann::json{{"key", "b"}, {"value", 2}};
    config.pipeline.push_back(cmd2);

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_ok());

    DataPacket output = result.value();
    ASSERT_TRUE(output.size() == 2);
    ASSERT_TRUE(output.has("a"));
    ASSERT_TRUE(output.has("b"));
    ASSERT_TRUE(output.get<int>("a").value() == 1);
    ASSERT_TRUE(output.get<int>("b").value() == 2);
    PASS();
}

void test_engine_unknown_command_type_returns_error() {
    TEST("Engine returns error for unknown command type");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));

    WorkflowDefinition config;
    config.name = "unknown-type-test";

    CommandConfig cmd;
    cmd.type = "NonExistentCommand";
    cmd.instance_name = "ghost";
    config.pipeline.push_back(cmd);

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_error());
    PASS();
}

void test_engine_pipeline_stops_on_first_error() {
    TEST("Engine does not execute later commands after error");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));


    engine.register_command_factory("FailFirst",
        [](const std::string&, const nlohmann::json&) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<MockCommand>("FailFirst");
            cmd->set_error_result("first fails", 1);
            return cmd;
        });


    int second_call_count = 0;
    engine.register_command_factory("Second",
        [&second_call_count](const std::string&, const nlohmann::json&) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<MockCommand>("Second");
            cmd->set_success_result(DataPacket{});
            cmd->set_execute_callback([&second_call_count](const DataPacket&, DataBus&, ILogger&) {
                second_call_count++;
            });
            return cmd;
        });

    WorkflowDefinition config;
    CommandConfig cmd1;
    cmd1.type = "FailFirst";
    cmd1.instance_name = "step1";
    config.pipeline.push_back(cmd1);

    CommandConfig cmd2;
    cmd2.type = "Second";
    cmd2.instance_name = "step2";
    config.pipeline.push_back(cmd2);

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_error());
    ASSERT_TRUE(result.error_code() == 1);
    ASSERT_TRUE(second_call_count == 0);
    PASS();
}

void test_engine_bus_shared_state() {
    TEST("DataBus allows inter-command shared state");
    auto logger = std::make_unique<MockLogger>();
    auto bus = std::make_unique<DataBus>();

    WorkflowEngine engine(std::move(logger), std::move(bus));


    engine.register_command_factory("Publisher",
        [](const std::string&, const nlohmann::json&) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<MockCommand>("Publisher");
            cmd->set_execute_callback([](const DataPacket&, DataBus& bus, ILogger&) {
                bus.publish("shared_key", std::string("shared_value"));
            });
            cmd->set_success_result(DataPacket{});
            return cmd;
        });


    int consumer_found = 0;
    engine.register_command_factory("Consumer",
        [&consumer_found](const std::string&, const nlohmann::json&) -> std::unique_ptr<ICommand> {
            auto cmd = std::make_unique<MockCommand>("Consumer");
            cmd->set_execute_callback([&](const DataPacket&, DataBus& bus, ILogger&) {
                auto val = bus.consume<std::string>("shared_key");
                if (val.is_ok() && val.value() == "shared_value") {
                    consumer_found = 1;
                }
            });
            cmd->set_success_result(DataPacket{});
            return cmd;
        });

    WorkflowDefinition config;
    CommandConfig cmd1;
    cmd1.type = "Publisher";
    cmd1.instance_name = "step1";
    config.pipeline.push_back(cmd1);

    CommandConfig cmd2;
    cmd2.type = "Consumer";
    cmd2.instance_name = "step2";
    config.pipeline.push_back(cmd2);

    auto result = engine.execute(config);
    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(consumer_found == 1);
    PASS();
}



int main() {
    std::cout << "=== WorkflowEngine Unit Tests ===" << std::endl;

    test_engine_empty_pipeline_runs_trivially();
    test_engine_single_success_command();
    test_engine_error_command_halt_pipeline();
    test_engine_pipeline_data_flow();
    test_engine_unknown_command_type_returns_error();
    test_engine_pipeline_stops_on_first_error();
    test_engine_bus_shared_state();

    std::cout << "\n=== Summary === " << std::endl;
    std::cout << "Passed: " << tests_passed << std::endl;
    std::cout << "Failed: " << tests_failed << std::endl;

    return tests_failed > 0 ? 1 : 0;
}