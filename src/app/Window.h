#pragma once

#include <Windows.h>
#include <string_view>
#include <chrono>
#include <cstdint>
#include <string>
#include <array>

namespace gf::app
{
    class Window final
    {
    public:
        Window(
            std::wstring title,
            std::uint32_t clientWidth,
            std::uint32_t clientHeight
        );

        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        void Show(int showCommand);

        [[nodiscard]] bool ProcessMessages();

        void UpdateStatusTitle(
            std::chrono::steady_clock::duration uptime,
            std::wstring_view graphicsStatus
        );

        [[nodiscard]] HWND NativeHandle() const noexcept
        {
            return m_handle;
        }

        [[nodiscard]] std::uint32_t ClientWidth() const noexcept
        {
            return m_clientWidth;
        }

        [[nodiscard]] std::uint32_t ClientHeight() const noexcept
        {
            return m_clientHeight;
        }

        [[nodiscard]] bool IsMinimized() const noexcept
        {
            return m_minimized;
        }

        [[nodiscard]]
        bool IsKeyDown(UINT virtualKey) const noexcept
        {
            if (virtualKey >= m_keyStates.size())
            {
                return false;
            }

            return m_keyStates[virtualKey];
        }

    private:
        static LRESULT CALLBACK WindowProcedure(
            HWND windowHandle,
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        );

        LRESULT HandleMessage(
            UINT message,
            WPARAM wParam,
            LPARAM lParam
        );

        HINSTANCE m_instance = nullptr;
        HWND m_handle = nullptr;

        std::wstring m_baseTitle;
        std::wstring m_lastInput = L"none";

        std::uint32_t m_clientWidth = 0;
        std::uint32_t m_clientHeight = 0;

        bool m_classRegistered = false;
        bool m_minimized = false;

        std::array<bool, 256> m_keyStates{};
    };
}