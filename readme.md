# armedium

a chill external overlay for roblox. it's just a window that sits on top and reads some memory, that's about it. nothing to see here. 🚶

built with c++, imgui, and a directx 11 overlay, all on top of a simple external memory reader — no injections, no dlls, no touching the game process beyond a couple of reads and writes.

## features

- **overlay** — clean imgui menu with themes, keybind list, fps counter, notifications, and a config autosave system
- **esp** — boxes, 3d boxes, tracers, skeleton, health, names, distance, corner esp, head circles, offscreen arrows
- **aiming** — camera / mouse / viewport methods, fov circle, smoothing curves, prediction, sticky aim, shake, stutter
- **silent aiming** — mouse hit overwrite methods with optional hitbox on fire
- **triggerbot** — radius or advanced per-part fov, delay control
- **chams** — part chams and hitbox chams (external, rendered client-side)
- **misc** — fly, walkspeed, noclip, fling, gravity mods, teleport, macro, wallcheck
- **configs** — save/load via json, autosave on exit

## building

- visual studio 2022 with the c++ workload
- open `armedium/armedium.sln` and build in **x64 release**
- dependencies are bundled (imgui, directx headers) — nothing to fetch

## usage

1. join a roblox game and wait a second
2. run the exe — it'll attach to the roblox process by itself
3. `insert` to toggle the menu

> offsets are tied to a specific client version and may need updating after roblox updates. check `src/rbx/offsets.h`.

## structure

```
armedium/src/
├── features/    # esp, aiming, triggerbot, chams, misc...
├── overlay/     # imgui menu + dx11 renderer
├── rbx/         # offsets, globals, configs, caches
└── Memory/      # external memory manager
```

## disclaimer

this is a personal project for learning about external memory reading, rendering, and reverse engineering. it's not affiliated with roblox. don't use it in ways that'd get you banned. probably.
