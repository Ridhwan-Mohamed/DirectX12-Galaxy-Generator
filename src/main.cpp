#include "app/Application.h"
#include "core/Log.h"

#include <Windows.h>

#include <exception>
#include "core/ComApartment.h"

int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    PWSTR,
    int showCommand)
{
    try
    {
        gf::core::ComApartment comApartment;
        gf::app::Application application;
        return application.Run(showCommand);
    }
    catch (const std::exception& error)
    {
        gf::core::LogError(error.what());

        MessageBoxA(
            nullptr,
            error.what(),
            "Gravity Forge startup failure",
            MB_OK | MB_ICONERROR
        );

        return EXIT_FAILURE;
    }
}