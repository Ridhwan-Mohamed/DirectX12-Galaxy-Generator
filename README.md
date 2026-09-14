# 🌌 DirectX12 Galaxy Generator

![Platform](https://img.shields.io/badge/platform-Windows-0078D4?logo=windows&logoColor=white)
![Graphics API](https://img.shields.io/badge/graphics-DirectX%2012-1F6FEB)
![Build](https://img.shields.io/badge/build-CMake%20%2B%20Visual%20Studio-5C2D91?logo=cmake&logoColor=white)
![License](https://img.shields.io/badge/license-TBD-lightgrey)

<p align="center">
  <video
    src="images/reformationVid.mp4"
    poster="images/image1.png"
    width="1000"
    autoplay
    muted
    loop
    playsinline
    controls
    style="max-width: 100%; height: auto;"
  >
    <a href="images/reformationVid.mp4">
      Reformation demo (video)
    </a>
  </video>
</p>

A real-time particle galaxy built with **C++**, **DirectX 12**, and **HLSL**. The application generates a large field of particles, updates their motion on the GPU, and renders them as a glowing spiral galaxy that can be reshaped and viewed interactively.

The project is designed to make GPU-driven rendering easy to explore: move around the galaxy, change its spiral shape and motion, randomize its colors, and tune bloom and exposure while it is running.

## ✨ Key Features

- **Real-time particle galaxy simulation** updated on the GPU
- **Spiral-arm controls** for spin, winding, concentration, and differential rotation
- **Interactive 3D camera** with free movement and preset viewpoints
- **Galaxy color randomization** plus adjustable color gradients
- **HDR rendering and bloom** with live intensity, threshold, and exposure controls
- **Depth and debug controls** for inspecting particle rendering behaviour
- **Particle explosion and reformation** effects for reshaping the galaxy in real time
- **Frame-resource and fence synchronization** keeps CPU and GPU work coordinated safely across frames

## 🧭 How It Works

The rendering flow is intentionally straightforward at a high level:

```text
Initial particle field
        ↓
CPU setup + galaxy parameters
        ↓
GPU compute shader updates particle motion
        ↓
Particle rendering into the HDR scene
        ↓
Bloom extraction + blur
        ↓
Presentation / final image to the window
```

## 🖼️ Gallery

| | |
|:---:|:---:|
| [![Reformation animation](images/image1.png)](images/reformationGif.gif) | ![Galaxy view 1](images/image1.png) |
| ![Galaxy view 2](images/image2.png) | ![Galaxy view 3](images/image3.png) |
| ![Galaxy view 4](images/image4.png) | ![Galaxy view 5](images/image5.png) |
| ![Galaxy view 6](images/image6.png) |   |

### 🎬 Reformation Demo

- [Watch animated preview (GIF)](images/reformationGif.gif)
- [Watch full video (GIF replacement)](images/reformationVid.mp4)
