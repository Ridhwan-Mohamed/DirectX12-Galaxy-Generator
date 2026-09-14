#include "Window.h"

#include "../core/Log.h"
#include "../core/Win32Error.h"

#include <cassert>
#include <format>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr wchar_t WindowClassName[] = L"GravityForgeWindowClass";
}

namespace gf::app
{
    Window::Window(
        std::wstring title,
        std::uint32_t clientWidth,
        std::uint32_t clientHeight
    )
        : m_baseTitle(std::move(title)),
        m_clientWidth(clientWidth),
        m_clientHeight(clientHeight)
    {
        assert(clientWidth > 0);
        assert(clientHeight > 0);

        if (clientWidth == 0 || clientHeight == 0)
        {
            throw std::invalid_argument(
                "Window dimensions must be greater than zero"
            );
        }

        m_instance = GetModuleHandleW(nullptr);

        if (m_instance == nullptr)
        {
            core::ThrowLastWin32Error("GetModuleHandleW failed");
        }

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Window::WindowProcedure;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
        windowClass.lpszClassName = WindowClassName;

        if (RegisterClassExW(&windowClass) == 0)
        {
            core::ThrowLastWin32Error("RegisterClassExW failed");
        }

        m_classRegistered = true;

        RECT windowRectangle{
            0,
            0,
            static_cast<LONG>(clientWidth),
            static_cast<LONG>(clientHeight)
        };

        constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW;

        if (!AdjustWindowRectEx(
            &windowRectangle,
            windowStyle,
            FALSE,
            0))
        {
            UnregisterClassW(WindowClassName, m_instance);
            m_classRegistered = false;

            core::ThrowLastWin32Error("AdjustWindowRectEx failed");
        }

        const int outerWidth =
            windowRectangle.right - windowRectangle.left;

        const int outerHeight =
            windowRectangle.bottom - windowRectangle.top;

        m_handle = CreateWindowExW(
            0,
            WindowClassName,
            m_baseTitle.c_str(),
            windowStyle,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            outerWidth,
            outerHeight,
            nullptr,
            nullptr,
            m_instance,
            this
        );

        if (m_handle == nullptr)
        {
            UnregisterClassW(WindowClassName, m_instance);
            m_classRegistered = false;

            core::ThrowLastWin32Error("CreateWindowExW failed");
        }

        core::LogInfo("Window created");
    }

    Window::~Window()
    {
        if (m_handle != nullptr && IsWindow(m_handle))
        {
            DestroyWindow(m_handle);
            m_handle = nullptr;
        }

        if (m_classRegistered)
        {
            UnregisterClassW(WindowClassName, m_instance);
            m_classRegistered = false;
        }

        core::LogInfo("Window destroyed");
    }

    void Window::Show(int showCommand)
    {
        ShowWindow(m_handle, showCommand);
        UpdateWindow(m_handle);
    }

    bool Window::ProcessMessages()
    {
        MSG message{};

        while (PeekMessageW(
            &message,
            nullptr,
            0,
            0,
            PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                return false;
            }

            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        return true;
    }

    void Window::UpdateStatusTitle(
        std::chrono::steady_clock::duration uptime,
        std::wstring_view graphicsStatus)
    {
        if (m_handle == nullptr)
        {
            return;
        }

        const double uptimeSeconds =
            std::chrono::duration<double>(
                uptime
            ).count();

        const std::wstring status = std::format(
            L"{} | {}x{} | {} | {:.1f}s | "
            L"last input: {} | {}",
            m_baseTitle,
            m_clientWidth,
            m_clientHeight,
            m_minimized ? L"minimized" : L"running",
            uptimeSeconds,
            m_lastInput,
            graphicsStatus
        );

        SetWindowTextW(
            m_handle,
            status.c_str()
        );
    }

    LRESULT CALLBACK Window::WindowProcedure(
        HWND windowHandle,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        Window* window = nullptr;

        if (message == WM_NCCREATE)
        {
            auto* creationData =
                reinterpret_cast<CREATESTRUCTW*>(lParam);

            window = static_cast<Window*>(
                creationData->lpCreateParams
                );

            window->m_handle = windowHandle;

            SetWindowLongPtrW(
                windowHandle,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(window)
            );
        }
        else
        {
            window = reinterpret_cast<Window*>(
                GetWindowLongPtrW(
                    windowHandle,
                    GWLP_USERDATA
                )
                );
        }

        if (window != nullptr)
        {
            return window->HandleMessage(
                message,
                wParam,
                lParam
            );
        }

        return DefWindowProcW(
            windowHandle,
            message,
            wParam,
            lParam
        );
    }

    LRESULT Window::HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (message)
        {
        case WM_SIZE:
            m_clientWidth =
                static_cast<std::uint32_t>(LOWORD(lParam));

            m_clientHeight =
                static_cast<std::uint32_t>(HIWORD(lParam));

            m_minimized = wParam == SIZE_MINIMIZED;
            return 0;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < m_keyStates.size())
            {
                m_keyStates[
                    static_cast<std::size_t>(wParam)
                ] = true;
            }

            switch (wParam)
            {
            case 'W':
                m_lastInput = L"W / forward";
                break;

            case 'A':
                m_lastInput = L"A / left";
                break;

            case 'S':
                m_lastInput = L"S / backward";
                break;

            case 'D':
                m_lastInput = L"D / right";
                break;

            case 'Q':
                m_lastInput =
                    L"Q / move down";
                break;

            case 'E':
                m_lastInput =
                    L"E / move up";
                break;
            case VK_LEFT:
                m_lastInput =
                    L"Left / turn left";
                break;

            case VK_RIGHT:
                m_lastInput =
                    L"Right / turn right";
                break;

            case VK_UP:
                m_lastInput =
                    L"Up / look up";
                break;

            case VK_DOWN:
                m_lastInput =
                    L"Down / look down";
                break;
            case 'Z':
                m_lastInput =
                    L"Z / roll left";
                break;
            case 'C':
                m_lastInput =
                    L"C / roll right";
                break;
            case VK_ESCAPE:
                m_lastInput = L"Escape / quit";
                PostMessageW(m_handle, WM_CLOSE, 0, 0);
                break;

            case '1':
                m_lastInput =
                    L"1 / toggle depth";
                break;

            case '2':
                m_lastInput =
                    L"2 / emission down";
                break;

            case '3':
                m_lastInput =
                    L"3 / emission up";
                break;

            case '4':
                m_lastInput =
                    L"4 / spin down";
                break;

            case '5':
                m_lastInput =
                    L"5 / spin up";
                break;

            case '6':
                m_lastInput =
                    L"6 / exposure down";
                break;

            case '7':
                m_lastInput =
                    L"7 / exposure up";
                break;

            case '8':
                m_lastInput =
                    L"8 / particle pause";
                break;

            case '9':
                m_lastInput =
                    L"9 / differential rotation down";
                break;

            case '0':
                m_lastInput =
                    L"0 / differential rotation up";
                break;

            case 'P':
                m_lastInput =
                    L"P / particle density cycle";
                break;

            case 'B':
                m_lastInput =
                    L"B / particle rendering";
                break;

            case 'V':
                m_lastInput =
                    L"V / particle depth debug";
                break;

            case 'G':
                m_lastInput =
                    L"G / gravity source toggle";
                break;

            case 'J':
                m_lastInput =
                    L"J / gravity source X-";
                break;

            case 'L':
                m_lastInput =
                    L"L / gravity source X+";
                break;

            case 'U':
                m_lastInput =
                    L"U / gravity source Y+";
                break;

            case 'O':
                m_lastInput =
                    L"O / gravity source Y-";
                break;

            case 'I':
                m_lastInput =
                    L"I / gravity source Z+";
                break;

            case 'K':
                m_lastInput =
                    L"K / gravity source Z-";
                break;

            case 'T':
                m_lastInput =
                    L"T / gravity source count";
                break;

            case 'H':
                m_lastInput =
                    L"H / select gravity source";
                break;

            case 'F':
                m_lastInput =
                    L"F / particle explosion";
                break;

            case 'R':
                m_lastInput =
                    L"R / particle reformation";
                break;

            case 'Y':
                m_lastInput =
                    L"Y / bloom toggle";
                break;

            case VK_F2:
                m_lastInput =
                    L"F2 / particle speed down";
                break;

            case VK_F3:
                m_lastInput =
                    L"F3 / particle speed up";
                break;

            case VK_OEM_4:
                m_lastInput =
                    L"[ / bloom intensity down";
                break;

            case VK_OEM_6:
                m_lastInput =
                    L"] / bloom intensity up";
                break;

            case VK_OEM_1:
                m_lastInput =
                    L"; / bloom threshold down";
                break;

            case VK_OEM_7:
                m_lastInput =
                    L"' / bloom threshold up";
                break;
            case VK_OEM_2:
                m_lastInput =
                    L"/ / randomize galaxy colors";
                break;
            case VK_DIVIDE:
                m_lastInput =
                    L"Numpad / / randomize galaxy colors";
                break;

            case VK_OEM_MINUS:
                m_lastInput =
                    L"- / bloom exposure down";
                break;

            case VK_OEM_PLUS:
                m_lastInput =
                    L"= / bloom exposure up";
                break;

            default:
                m_lastInput = std::format(
                    L"virtual key {}",
                    wParam
                );
                break;
            }

            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT paint{};
            BeginPaint(m_handle, &paint);
            EndPaint(m_handle, &paint);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(m_handle);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_NCDESTROY:
        {
            const HWND destroyedHandle = m_handle;

            SetWindowLongPtrW(
                destroyedHandle,
                GWLP_USERDATA,
                0
            );

            const LRESULT result = DefWindowProcW(
                destroyedHandle,
                message,
                wParam,
                lParam
            );

            m_handle = nullptr;
            return result;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < m_keyStates.size())
            {
                m_keyStates[
                    static_cast<std::size_t>(
                        wParam
                        )
                ] = false;
            }
            return 0;
        case WM_SYSCOMMAND:
            if (
                (wParam & 0xFFF0) ==
                SC_KEYMENU)
            {
                return 0;
            }
            break;
        case WM_KILLFOCUS:
            m_keyStates.fill(false);
            return 0;

        default:
            break;
        }
        return DefWindowProcW(
            m_handle,
            message,
            wParam,
            lParam
        );
    }
}
