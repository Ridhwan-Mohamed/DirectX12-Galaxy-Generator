#pragma once

#include <Windows.h>

#include <string>
#include <string_view>
#include <system_error>

namespace gf::core
{
    [[noreturn]] inline void ThrowLastWin32Error(std::string_view context)
    {
        const DWORD errorCode = GetLastError();

        throw std::system_error(
            static_cast<int>(errorCode),
            std::system_category(),
            std::string(context)
        );
    }
}