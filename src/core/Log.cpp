#include "Log.h"

#include <Windows.h>

#include <string>

namespace
{
    [[nodiscard]] const char* LevelName(gf::core::LogLevel level)
    {
        switch (level)
        {
        case gf::core::LogLevel::Info:
            return "info";

        case gf::core::LogLevel::Warning:
            return "warning";

        case gf::core::LogLevel::Error:
            return "error";
        }

        return "unknown";
    }
}

namespace gf::core
{
    void Log(LogLevel level, std::string_view message)
    {
        std::string output;
        output.reserve(message.size() + 16);

        output += '[';
        output += LevelName(level);
        output += "] ";
        output += message;
        output += '\n';

        OutputDebugStringA(output.c_str());
    }

    void LogInfo(std::string_view message)
    {
        Log(LogLevel::Info, message);
    }

    void LogWarning(std::string_view message)
    {
        Log(LogLevel::Warning, message);
    }

    void LogError(std::string_view message)
    {
        Log(LogLevel::Error, message);
    }
}