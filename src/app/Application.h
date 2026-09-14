#pragma once

#include "Window.h"
#include "../graphics/d3d12/D3D12Context.h"
#include "../scene/Camera.h"

namespace gf::app
{
    class Application final
    {
    public:
        Application();

        [[nodiscard]] int Run(int showCommand);

    private:
        Window m_window;
        gf::scene::Camera m_camera;
        gf::graphics::d3d12::D3D12Context m_graphics;
    };
}