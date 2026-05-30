#include "ConsoleLogger.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace workflow {

void ConsoleLogger::log(LogLevel level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::tm local_tm{};
#ifdef _WIN32
    localtime_s(&local_tm, &time);
#else
    localtime_r(&time, &local_tm);
#endif



    auto& out = (level == LogLevel::Error || level == LogLevel::Warn)
                    ? std::cerr
                    : std::cout;

    out << "["
        << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
        << "."
        << std::setfill('0') << std::setw(3) << ms.count()
        << "] ["
        << to_string(level)
        << "] "
        << message
        << std::endl;
}

}