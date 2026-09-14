# Gravity Forge → Galaxy Direction Handoff (Post-Prep)

You asked for a clean handoff document so a zero-context agent can continue.
This state is a manual-transition baseline: keep the existing HDR + particle scene,
but remove old debris/brace obligations from runtime and input.

## What I changed in your real repo

Path used: `C:\Users\ridsm\source\repos\GravityForge`

### 1) Reactor geometry cleanup
- `src/scene/ReactorScene.h`
  - Removed `ReactorMeshKind::Brace` from enum.
- `src/scene/ReactorScene.cpp`
  - Removed brace instances from constructor.
  - Reactor now only builds core + ring parts (no ring-like support braces).

### 2) Main render loop / context cleanup
- `src/graphics/d3d12/D3D12Context.cpp`
  - `D3D12Context::CreateReactorMeshes`
    - Removed box mesh creation for brace support.
    - Stops building brace mesh/upload/cleanup; now uploads only core + ring geometry.
  - Constructor init path
    - Removed calls:
      - `CreateDebrisField()`
      - `CreateDebrisInstanceView()`
      - `CreateDebrisCullingResources()`
      - `CreateDebrisIndirectResources()`
  - Render assembly (`RecordFrame`)
    - Removed debris draw pass from scene pass list.
    - Removed debris cull/compact pass invocation (left placeholder comment for future).
  - `MeshFor()`
    - Removed brace case; mapping now only core/ring from active scene mesh enum.

### 3) App controls + debug status cleanup
- `src/app/Application.cpp`
  - Removed input edge-detect state for:
    - N / M / C / X / debris-indirect key path
  - Removed bindings and actions for:
    - Debris draw count cycling
    - Debris animation freeze
    - Debris culling toggle
    - Debris debug toggle
    - Debris indirect toggle
  - Status HUD format no longer prints debris counters/modes.
- `src/app/Window.cpp`
  - Removed last-input text mappings for:
    - N, M, C, X debris actions

---

## Important note

Debris system code is still present in:
- `src/graphics/d3d12/D3D12Context.*`
- `src/scene/DebrisField.*`

It is not removed from the codebase yet, only disconnected from the active startup/render/input path. That means the feature exists for re-use later, but is not executed.

---

## Why this helps for galaxy direction

You now have a simpler baseline:
- Reactor is structurally cleaner (no brace mesh assumptions).
- Scene render path is focused on:
  1) HDR pass,
  2) particle compute/visuals,
  3) bloom extraction/blur/presentation.
- Input/status are no longer cluttered by retired debris toggles, which lets you repurpose those controls.

This directly supports:
- Implementing a galaxy disk/arm particle field,
- Re-using particle billboards + instancing logic,
- Later adding a dedicated procedural galaxy mesh field with separate controls.

---

## Suggested continuation milestones (galaxy direction)

You previously asked about “around 6 more milestones” — this plan keeps that:

### Milestone A — Core galactic compose
1. Add black base clear + soft volumetric core falloff.
2. Add orbit layer with many points.
3. Add arm emitter curves (parametric).

### Milestone B — Color + lighting
4. Add temperature-based gradients (blue/yellow/white core to blue/white/teal arms).
5. Add radial emission masks and per-point intensity curves.

### Milestone C — Motion + control
6. Add spin shear + differential rotation.
7. Add global controls for arm scale/width/pitch/rotation speed.

### Milestone D — Validation polish
8. Add camera path presets around core.
9. Add diagnostics pass (FPS + particle counts + culling status).
10. Keep previous milestones cleanup/debug untouched until stable visuals are acceptable.

Use this as your “new docs baseline” for the next assistant or your own continuation.

---

## Next agent instructions (copy/paste)

Start from this exact path and this current state:

- Active repo:
  - `C:\Users\ridsm\source\repos\GravityForge`
- Key changed files:
  - `src\scene\ReactorScene.h`
  - `src\scene\ReactorScene.cpp`
  - `src\graphics\d3d12\D3D12Context.cpp`
  - `src\app\Application.cpp`
  - `src\app\Window.cpp`

Build caveat in this environment:
- `cmake` is not available on PATH, so I could not run a full compile/test here.
- Please run at least:
  - configure + build in your machine
  - quick run smoke test for no crash at startup, controls, and HUD
  - confirm no brace/debris draw/keys appear in render/input behavior.

---

## New Milestone addendum (zero-code, docs-only): Galaxy Direction — Milestone 18 / Part 1

You asked to keep this as documentation only for handoff continuity.  
This milestone is intentionally scoped to planning so no implementation is changed here.

### Milestone 18 — Galactic Core + Spiral Arms foundation

- Goal:
  - Keep existing working pipeline (HDR scene + particles + tone/bloom chain).
  - Replace the existing “reactor centerpiece” look with a galaxy-consistent visual identity:
    - black/dark core represented by dense, warm (yellow/orange/white) particle density,
    - visible bright core glow bloom anchor,
    - 2–4 spiral arms built from particle or instanced fragments.

- Why this milestone now:
  - It provides a clearer portfolio centerpiece than the current reactor-only scene.
  - It uses systems already built (particle emitters, compute updates, HLSL shading, post) and delays risky engine rewrites.

- Scope:
  1) Art direction lock
    - Define target parameters only:
      - core radius falloff, arm pitch, arm armature width, hue range, emission curves.
    - Keep swapchain clear to black and avoid heavy mesh-heavy additions in this part.
  2) Core representation decision
    - Yes: core should move toward a particle+glow representation.
    - Do not keep a hard emissive sphere as the main “black hole” identity.
    - Keep a tiny emissive fallback mesh only if needed for horizon marker, but do not lead.
  3) Shader/data plan (no code yet, just requirements)
    - Extend particle/instance update with new attractor mode:
      - orbiting + radial drift,
      - per-particle age / arm index / hue seed.
    - Add optional per-arm phase so particles read as continuous arms.
  4) Controls to introduce later
    - Core brightness, arm strength, arm pitch/spread, particle density, spin speed.
  5) Validation criteria (visual-only)
    - Single bright center glow visible in HDR/bloom.
    - Spiral structure visible and coherent while camera moves.
    - No hard geometric “sphere” language in final composition.

- Suggested decomposition for ~6 milestones
  - M18-P1: Parameterized galaxy field math (CPU seed + GPU update path definition).
  - M18-P2: Arm generation + core particle halo in-shader and tuned emission curves.
  - M18-P3: Camera motion presets and composition polish.
  - M18-P4: Runtime controls + on-screen diagnostics for galactic fields.
  - M18-P5: Lighting/color grading pass (soft bloom/color ramp tuning).
  - M18-P6: cleanup (dead controls removed, debug labels, PIX naming, perf sanity pass).

- Hand-off notes for next agent:
  - Keep `C:\\Users\\ridsm\\source\\repos\\GravityForge` as the working root.
  - You do not need to undo prior code edits before this milestone.
  - Do not reintroduce old braces/debris dependencies in this first phase.
  - Treat all pre-existing debris code as dormant; reuse only if it directly supports spiral particles and does not re-complicate the pass graph.
