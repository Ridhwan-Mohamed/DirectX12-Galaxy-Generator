#pragma once

#include <Windows.h>

#include <string>
#include <string_view>
#include <system_error>

namespace gf::core
{
    inline void ThrowIfFailed(
        HRESULT result,
        std::string_view context)
    {
        if (FAILED(result))
        {
            throw std::system_error(
                static_cast<int>(result),
                std::system_category(),
                std::string(context)
            );
        }
    }
}