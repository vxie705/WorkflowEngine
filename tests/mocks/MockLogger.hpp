#ifndef WORKFLOW_ENGINE_MOCK_LOGGER_HPP
#define WORKFLOW_ENGINE_MOCK_LOGGER_HPP

#include "ILogger.hpp"
#include <string>
#include <vector>
#include <tuple>

namespace workflow {
namespace testing {

/**
 * @brief MockLogger — captures log calls for test assertions.
 *
 * Instead of writing to a console or file, MockLogger records
 * every log() call into an in-memory vector. Tests can inspect
 * the recorded log entries to verify that commands log the
 * expected messages at the expected severity levels.
 *
 * Usage:
 * @code
 *   MockLogger logger;
 *   engine.execute(config);
 *   EXPECT_TRUE(logger.has_message("Data loaded successfully"));
 *   EXPECT_TRUE(logger.has_level(LogLevel::Error));
 * @endcode
 */
class MockLogger : public ILogger {
public:
    MockLogger() = default;

    void log(LogLevel level, const std::string& message) override {
        calls_.emplace_back(level, message);
    }



    /** Total number of log calls recorded. */
    size_t call_count() const noexcept { return calls_.size(); }

    /** Reset all recorded calls. */
    void clear() noexcept { calls_.clear(); }

    /** Check if any log call matches the given message (substring match). */
    bool has_message(const std::string& substring) const noexcept {
        for (const auto& [level, msg] : calls_) {
            if (msg.find(substring) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    /** Check if any log call has the given level. */
    bool has_level(LogLevel level) const noexcept {
        for (const auto& [lvl, msg] : calls_) {
            if (lvl == level) {
                return true;
            }
        }
        return false;
    }

    /** Count calls matching the given level. */
    size_t count_by_level(LogLevel level) const noexcept {
        size_t count = 0;
        for (const auto& [lvl, msg] : calls_) {
            if (lvl == level) { ++count; }
        }
        return count;
    }

    /** Get the last log message, or empty string if no calls. */
    std::string last_message() const noexcept {
        if (calls_.empty()) return {};
        return std::get<1>(calls_.back());
    }

    /** Get all recorded (level, message) pairs. */
    const std::vector<std::tuple<LogLevel, std::string>>& calls() const noexcept {
        return calls_;
    }

private:
    std::vector<std::tuple<LogLevel, std::string>> calls_;
};

}
}

#endif