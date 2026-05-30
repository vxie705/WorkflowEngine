#ifndef WORKFLOW_ENGINE_CONSOLE_LOGGER_HPP
#define WORKFLOW_ENGINE_CONSOLE_LOGGER_HPP

#include "ILogger.hpp"

namespace workflow {

/**
 * @brief Trivial ILogger implementation that writes to stdout/stderr.
 *
 * Intended for demo and development use. For production, inject a
 * FileLogger, StructuredLogger, or cloud-based logger instead.
 */
class ConsoleLogger : public ILogger {
public:
  void log(LogLevel level, const std::string &message) override;
};

}

#endif