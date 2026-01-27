# CLAUDE.md - AI Assistant Guide for Moiras

## Project Overview

**Moiras** is a Kenshi-like open-world squad-based RPG game written in **C++20** using **Raylib 5.5**. The project focuses on character movement, pathfinding, terrain interaction, and PBR rendering.

## Quick Reference

```bash
# Build the project
mkdir -p build && cd build
cmake .. && make

# Run the game
./build/Moiras

# Build with Docker
docker build -t moiras .
docker run -it -v $(pwd):/app moiras
```

## Technology Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| Rendering | Raylib | 5.5 |
| Physics | Jolt Physics | 5.5.0 |
| Navigation | Recast/Detour | 1.6.0 |
| UI | ImGui | (bundled) |
| Build | CMake | 3.1+ |
| Standard | C++ | C++20 |
| Platform | Linux | X11 (Wayland disabled) |

## Project Structure

```
Moiras/
├── src/
│   ├── main.cpp              # Entry point
│   ├── game/
│   │   ├── game.h/cpp        # Main game loop, scene management
│   │   └── game_object.h/cpp # Base class for all scene objects
│   ├── character/
│   │   ├── character.h/cpp   # Character entity (health, model, rendering)
│   │   └── controller.h/cpp  # Input handling, pathfinding movement
│   ├── camera/
│   │   └── camera.h/cpp      # GameCamera with raycast support
│   ├── map/
│   │   └── map.h/cpp         # Terrain, heightmap, sea, skybox
│   ├── navigation/
│   │   └── navmesh.h/cpp     # Recast/Detour pathfinding
│   ├── lights/
│   │   ├── lights.h/cpp      # Light base class and types
│   │   └── lightmanager.h/cpp # PBR lighting system
│   ├── gui/
│   │   ├── gui.h/cpp         # ImGui integration
│   │   ├── sidebar.h/cpp     # Debug controls
│   │   └── asset_spawner.h   # Runtime model loading
│   ├── models/
│   │   └── models.h/cpp      # Mesh utilities, animation
│   ├── enemy/                # Enemy AI (stub)
│   ├── ground/               # Ground collision (stub)
│   ├── tree/                 # Vegetation system (stub)
│   ├── window/               # Window wrapper
│   └── commons/              # Shared utilities (stub)
├── assets/
│   ├── shaders/              # GLSL shaders (PBR, outline, sea, skybox)
│   ├── fonts/                # Font files
│   ├── Gothic Furniture/     # Environment props
│   ├── *.glb, *.fbx          # 3D models
│   └── heightmap.png         # Terrain heightmap
├── imgui/                    # ImGui source
├── rlImGui/                  # Raylib-ImGui bridge
├── CMakeLists.txt            # Build configuration
└── Dockerfile                # Container build environment
```

## Architecture

### Scene Graph Hierarchy

All game entities inherit from `GameObject`:

```
Game (root)
├── GameCamera
├── Map (terrain/environment)
├── Character (player)
├── Lights (Directional, Point, Spot)
└── Other GameObjects
```

### Key Classes

- **GameObject** (`src/game/game_object.h`): Base class with parent/child hierarchy, virtual `update()`, `draw()`, `gui()` methods
- **Game** (`src/game/game.h`): Main loop, render target, LightManager, CharacterController
- **Character** (`src/character/character.h`): Entity with health, model, position, rotation
- **CharacterController** (`src/character/controller.h`): Mouse-based movement via NavMesh raycast
- **NavMesh** (`src/navigation/navmesh.h`): Pathfinding with Recast/Detour
- **LightManager** (`src/lights/lightmanager.h`): PBR lighting (up to 4 lights)
- **Map** (`src/map/map.h`): Terrain, NavMesh building, sea shader, fog

### Memory Management

- `std::unique_ptr` for ownership (children, controllers)
- Raw pointers for cross-references (parent pointers)
- Move semantics for large objects (Models, Meshes)
- Copy constructors deleted on resource-owning classes

## Code Conventions

### Naming

| Element | Convention | Example |
|---------|-----------|---------|
| Classes | PascalCase | `CharacterController`, `GameCamera` |
| Methods | camelCase | `findPath()`, `snapToGround()` |
| Member variables | m_ prefix (private) | `m_navMesh`, `m_character` |
| Constants | UPPER_CASE | `MAX_LIGHTS = 4` |

### File Organization

```cpp
#pragma once                    // Header guard
#include <system_headers>       // Standard library first
#include <raylib.h>             // Third-party next
#include "../local/headers.h"   // Local headers last

namespace moiras {              // All code in moiras namespace

class MyClass : public GameObject {
private:
    Type m_privateMember;       // Private with m_ prefix

public:
    Type publicMember;          // Public without prefix

    void update() override;     // Virtual overrides
    void draw() override;
    void gui() override;        // ImGui controls
};

} // namespace moiras
```

### Shader Paths

Shaders are loaded relative to the working directory:
```cpp
LoadShader("../assets/shaders/pbr.vs", "../assets/shaders/pbr.fs")
```

### Resource Cleanup

Always call explicit `Unload*()` for Raylib resources:
```cpp
~Character() {
    if (model.meshCount > 0) {
        UnloadModel(model);
    }
}
```

## Development Guidelines

### Adding New GameObjects

1. Create header/cpp in appropriate `src/` subdirectory
2. Inherit from `GameObject`
3. Override `update()`, `draw()`, `gui()` as needed
4. Add to `CMakeLists.txt` source list
5. Use `std::unique_ptr` when adding to scene:
   ```cpp
   root.addChild(std::make_unique<MyObject>());
   ```

### NavMesh Configuration

NavMesh parameters are adaptive based on map size:
- `m_cellSize`: Grid resolution (0.5-2.0 for maps 1000-4000 units)
- `m_agentHeight/Radius`: Character dimensions
- `m_agentMaxSlope`: Maximum walkable angle (default 40°)

### Debugging

- **NavMesh**: Call `navMesh.drawDebug()` for visualization
- **Paths**: CharacterController draws current path
- **Lights**: Debug spheres/rays available
- **ImGui**: Use `gui()` methods for live parameter tweaking

## Build System

### CMake Targets

```bash
make              # Build everything
make Moiras       # Build executable only
make copy_assets  # Copy assets to build dir
make clean        # Clean build
```

### Dependencies (auto-fetched)

- raylib 5.5 (rendering, input)
- JoltPhysics v5.5.0 (physics)
- RecastNavigation v1.6.0 (pathfinding)

### Docker Build

```bash
docker build -t moiras .
docker run -it -v $(pwd):/app moiras
cd /app && mkdir build && cd build && cmake .. && make
```

## Shaders

| Shader | Purpose |
|--------|---------|
| `pbr.vs/fs` | Physically Based Rendering |
| `outline.fs` | Character outline effect |
| `sea_shader.vs/fs` | Water rendering |
| `basic_lighting.vs/fs` | Simple lighting |

## Common Patterns

### Raycast to NavMesh (click-to-move)

```cpp
Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);
RayCollision collision = GetRayCollisionMesh(ray, mapMesh, transform);
if (collision.hit) {
    Vector3 target;
    if (navMesh.projectPointToNavMesh(collision.point, target)) {
        path = navMesh.findPath(character.position, target);
    }
}
```

### Adding ImGui Controls

```cpp
void MyClass::gui() {
    if (ImGui::CollapsingHeader("My Class")) {
        ImGui::SliderFloat("Value", &m_value, 0.0f, 1.0f);
        ImGui::Checkbox("Enabled", &m_enabled);
    }
}
```

## Notes for AI Assistants

1. **Language**: Code comments are in Italian; maintain consistency or use English for new comments
2. **Testing**: No formal test framework; test through runtime execution
3. **Stub systems**: `ground/`, `tree/`, `enemy/` directories contain minimal stubs for future expansion
4. **Assets**: Large GLB models (up to 60MB); avoid committing new large assets without discussion
5. **Platform**: Currently Linux/X11 only; Wayland explicitly disabled
6. **Performance**: SSE4.1, SSE4.2, AVX, AVX2 optimizations enabled; consider SIMD-friendly code
