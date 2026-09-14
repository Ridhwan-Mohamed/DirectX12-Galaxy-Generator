#pragma once

#include "core/HResult.h"

#include <objbase.h>

namespace gf::core
{
    class ComApartment final
    {
    public:
        ComApartment()
        {
            const HRESULT result =
                CoInitializeEx(
                    nullptr,
                    COINIT_MULTITHREADED
                );

            ThrowIfFailed(
                result,
                "Failed to initialize COM"
            );

            m_initialized = true;
        }

        ~ComApartment() noexcept
        {
            if (m_initialized)
            {
                CoUninitialize();
            }
        }

        ComApartment(const ComApartment&) = delete;
        ComApartment& operator=(
            const ComApartment&) = delete;

        ComApartment(ComApartment&&) = delete;
        ComApartment& operator=(
            ComApartment&&) = delete;

    private:
        bool m_initialized = false;
    };
}