# Gravity Forge — Remaining Milestones (Single Handoff)

Last consolidated handoff target for a zero-context agent.
Repository root: `C:\Users\ridsm\source\repos\GravityForge`

## Context

At this point you have:

- A working HDR + particle + bloom pipeline in the repo.
- Debris/brace runtime paths mostly disconnected from startup/render path.
- Existing `GALAXY_DIRECTION_HANDOFF.md` containing earlier context.

This file is the **one place to read** before continuing.

---

## Global rule for continuation

1. Use `C:\Users\ridsm\source\repos\GravityForge` as source of truth.
2. Keep graphics pipeline GPU-first (no CPU per-object draw submission when instancing/compute can do it).
3. Preserve frame-resources/fences, avoid readback for per-frame logic.
4. Keep existing style: `C++20 + D3D12 + HLSL SM6`.
5. Run build + smoke test before/after each milestone:
   - launch without crash,
   - verify input/diagnostic behavior,
   - verify no stale debris/brace controls remain unless explicitly restored.

---

## Ordered milestones (execution sequence)

You asked for a more “popping” result and a cleaner portfolio presentation.

### 1) Milestone 18 (6-part): Galactic Core + Spiral Arms Foundation

- **P1: Parametric field design**
  - Define all galaxy parameters in one config block:
    - arm count, arm pitch, arm width, spiral spread, core falloff, spin.
  - Keep core clear-color black and tune HDR bloom anchor.

- **P2: Core as particles/glow**
  - Replace hard sphere identity with dense particle core + faint emissive core bloom.
- **P3: Arm population**
  - Seed particles by arm index / radial distance / phase.
  - Render coherent spiral density and brightness falloff.
- **P4: Motion**
  - Add differential rotation and sheared orbit evolution.
  - Add gentle perturbations for natural motion.
- **P5: Color / tone**
  - Implement temperature gradient:
    - warm center,
    - cooler outer arms,
    - optional noise-based hue jitter.
- **P6: Interaction and polish**
  - Runtime controls for color, density, speed, spiral strength.
  - Validate with screenshots and camera presets.

### 2) Milestone 19: Post Pipeline Adaptation

- [ ] Tune bloom threshold, blur strength, and tonemap to keep core pop.
- [ ] Add optional starfield fill pass for depth and background richness.
- [ ] Confirm no geometry artifacts and stable transitions during camera movement.

### 3) Milestone 20: Presentation Prep

- [ ] Record a final reference clip / screenshots.
- [ ] Finalize controls list and onboarding text.
- [ ] Lock a “portfolio camera flythrough” sequence.
- [ ] Final cleanup of shader/resource comments and pass names.

### 4) Milestone 14-P4: Cleanup / Controls / Validation

- [ ] Add explicit debug labels for all render passes and compute queues.
- [ ] Consolidate HUD/status text, including remaining stale fields.
- [ ] Validate all runtime controls (toggle, sliders, modes) are documented and bounded.
- [ ] Remove dead control paths/members for retired features.
- [ ] Ensure PIX-friendly marker names around:
  - particle compute + render,
  - bloom passes,
  - present path.
- [ ] Add a short pre-handoff validation checklist.

### 5) Milestone 15: Debug Views + Runtime Controls

- [ ] Debug views: depth, optional material/normal info, particle masks, bloom extraction target.
- [ ] Runtime controls: camera behavior, particle behavior, emission/brightness thresholds.
- [ ] Optional hotkey legend and robust control ranges.
- [ ] Basic on-screen diagnostics for frame status and key toggles.

### 6) Milestone 16: GPU Timing + PIX Integration

- [ ] Add GPU timestamp queries for:
  - compute
  - particle render
  - bloom extract/blur
  - present
- [ ] Record per-stage timings (overlay or logged output).
- [ ] Add/verify PIX events around:
  - resource uploads,
  - pass transitions,
  - command-list lifecycle.
- [ ] PIX validation of pass ordering, transitions, and cost.

### 7) Milestone 17: Baseline vs Optimized Comparison

- [ ] Add deterministic baseline vs optimized modes.
- [ ] Keep scene output equivalent, change only performance strategy.
- [ ] Capture comparable:
  - GPU ms
  - frame time
  - particle throughput
  - resource update overhead
- [ ] Write one-page tradeoff summary.

---

## Files to treat as canonical handoff inputs

- `GALAXY_DIRECTION_HANDOFF.md`
- `src/graphics/d3d12/D3D12Context.h`
- `src/graphics/d3d12/D3D12Context.cpp`
- `src/app/Application.cpp`
- `src/app/Window.cpp`
- `src/scene/ReactorScene.cpp`
- `src/scene/ReactorScene.h`

---

## Ownership note

No code was changed when creating this document.  
This file is a planning/handoff artifact only.
