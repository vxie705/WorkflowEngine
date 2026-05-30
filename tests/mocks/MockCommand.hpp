#ifndef WORKFLOW_ENGINE_MOCK_COMMAND_HPP
#define WORKFLOW_ENGINE_MOCK_COMMAND_HPP

#include "ICommand.hpp"
#include "Result.hpp"
#include <string>
#include <nlohmann/json.hpp>

namespace workflow {
namespace testing {

/**
 * @brief MockCommand — a configurable mock for unit testing.
 *
 * Allows tests to control exactly what a command returns and
 * to assert what inputs it received. Extensible with a callback
 * for side-effect verification.
 *
 * Usage:
 * @code
 *   MockCommand cmd;
 *   cmd.set_result(Result<DataPacket>::ok(some_packet));
 *   auto result = cmd.execute(input, bus, logger);
 *   EXPECT_EQ(cmd.input_received().size(), 3);  // DataPacket had 3 keys
 *   EXPECT_EQ(cmd.call_count(), 1);
 * @endcode
 */
class MockCommand : public ICommand {
public:
    MockCommand()
        : name_("MockCommand")
        , result_(Result<DataPacket>::ok(DataPacket{}))
    {
    }

    explicit MockCommand(std::string name)
        : name_(std::move(name))
        , result_(Result<DataPacket>::ok(DataPacket{}))
    {
    }

    std::string name() const override { return name_; }

    Result<DataPacket> execute(
        const DataPacket& input,
        DataBus& bus,
        ILogger& logger) override
    {
        call_count_++;
        last_input_ = input;
        last_bus_ = &bus;
        last_logger_ = &logger;

        if (execute_callback_) {
            execute_callback_(input, bus, logger);
        }

        return result_;
    }



    void set_result(Result<DataPacket> result) {
        result_ = std::move(result);
    }

    void set_success_result(DataPacket packet) {
        result_ = Result<DataPacket>::ok(std::move(packet));
    }

    void set_error_result(std::string message, int code = -1) {
        result_ = Result<DataPacket>::error(std::move(message), code);
    }

    using ExecuteCallback = std::function<void(
        const DataPacket&, DataBus&, ILogger&)>;

    void set_execute_callback(ExecuteCallback cb) {
        execute_callback_ = std::move(cb);
    }



    int call_count() const noexcept { return call_count_; }

    const DataPacket& last_input() const noexcept { return last_input_; }

    DataBus* last_bus() const noexcept { return last_bus_; }

    ILogger* last_logger() const noexcept { return last_logger_; }

private:
    std::string name_;
    Result<DataPacket> result_;
    int call_count_ = 0;
    DataPacket last_input_;
    DataBus* last_bus_ = nullptr;
    ILogger* last_logger_ = nullptr;
    ExecuteCallback execute_callback_;
};

}
}

#endif