#pragma once

#include <string_view>

namespace gf::core
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };

    void Log(LogLevel level, std::string_view message);

    void LogInfo(std::string_view message);
    void LogWarning(std::string_view message);
    void LogError(std::string_view message);
}