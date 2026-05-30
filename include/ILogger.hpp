#ifndef WORKFLOW_ENGINE_ILOGGER_HPP
#define WORKFLOW_ENGINE_ILOGGER_HPP

#include <chrono>
#include <memory>
#include <string>

namespace workflow {

/**
 * @brief Severity level for log messages.
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error
};

/**
 * @brief Convert LogLevel to a human-readable string.
 */
inline const char* to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

/**
 * @brief Abstract logging interface.
 *
 * Every ICommand receives an ILogger& for instrumentation.
 * Production code should never use std::cout directly — inject
 * this interface instead. MockLogger in tests captures calls
 * for assertions.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Core logging method. All convenience methods delegate here.
     * @param level  Severity of the message.
     * @param message  The log payload.
     */
    virtual void log(LogLevel level, const std::string& message) = 0;



    void trace(const std::string& message) {
        log(LogLevel::Trace, message);
    }

    void debug(const std::string& message) {
        log(LogLevel::Debug, message);
    }

    void info(const std::string& message) {
        log(LogLevel::Info, message);
    }

    void warn(const std::string& message) {
        log(LogLevel::Warn, message);
    }

    void error(const std::string& message) {
        log(LogLevel::Error, message);
    }
};

}

#endif
