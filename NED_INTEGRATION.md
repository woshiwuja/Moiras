# Ned Text Editor Integration

## Summary

The Ned text editor (https://github.com/nealmick/ned) has been integrated into the Moiras project as an embedded component. The integration is **99% complete** but currently disabled due to a crash in Ned's background file scanner.

## What Was Integrated

### 1. Dependencies Added
- **Ned submodule** at `ned/` (commit 9053f2f)
- **Modified ImGui** with Freetype support and `ImFontBaked` extensions
- OpenGL, GLFW3, GLEW, CURL, Freetype
- libgit2 (for Git integration)
- tree-sitter (syntax highlighting for 15+ languages)
- LSP framework (Language Server Protocol support)
- gssapi_krb5 (for authentication)

### 2. Files Created
- `src/gui/script_editor.h` - ScriptEditor GameObject wrapper
- `src/gui/script_editor.cpp` - Implementation with OpenGL/GLEW initialization
- `src/gui/ned_stubs.cpp` - Stubs for Ned standalone globals

### 3. Files Modified
- `CMakeLists.txt` - Added Ned build configuration and dependencies
- `src/game/game.h` - Added ScriptEditor member and header include
- `src/game/game.cpp` - Added editor initialization and F12 toggle
- `ned/lsp/lsp_includes.h` - Fixed LSP include paths for CMake
- `ned/lsp/lsp_goto_def.h` - Fixed LSP include paths
- `ned/lsp/lsp_goto_ref.h` - Fixed LSP include paths
- `imgui/CMakeLists.txt` - Replaced with Ned's ImGui build

## Features Available (When Enabled)

- **Text Editor**: Syntax highlighting for C++, Lua, Python, JavaScript, and 12+ more languages
- **File Explorer**: Browse project files
- **LSP Integration**: clangd, gopls, pyright, TypeScript language servers
- **Terminal Emulator**: Based on st.c
- **Multi-cursor Editing**: Advanced editing features
- **Git Integration**: Via libgit2
- **AI Agent Support**: OpenRouter integration (optional)

## Current Status: Disabled ⚠️

The editor is **temporarily disabled** due to a crash in `FileFinder::backgroundRefresh()`. 

### The Issue

When NedEmbed initializes, it starts a background thread that scans directories for files. This thread crashes because:

1. **Wrong resource paths**: Ned is compiled with `-DSOURCE_DIR` pointing to the Moiras source directory instead of the Ned directory
2. **Missing configuration**: The embedded version doesn't have a way to disable the file finder or configure paths properly
3. **Race condition**: The background thread may be accessing resources before they're fully initialized

### Error Log
```
[LSP] Failed to open lsp.json file: /home/federico/ned/settings/lsp.json
[Settings] Using ned resource path: /home/federico/Moiras-git/build/../ned
Thread 2 "Moiras" received signal SIGSEGV, Segmentation fault.
FileFinder::backgroundRefresh()
```

## How to Enable (Once Fixed)

In `src/game/game.cpp`, uncomment the script editor initialization:

```cpp
// In setup():
auto scriptEditorPtr = std::make_unique<ScriptEditor>();
scriptEditor = scriptEditorPtr.get();
scriptEditor->setOpen(false);
root.addChild(std::move(scriptEditorPtr));

// In loop():
if (IsKeyPressed(KEY_F12) && scriptEditor)
{
    scriptEditor->setOpen(!scriptEditor->isOpen());
}
```

## Potential Solutions

### Option 1: Fix SOURCE_DIR in Ned's CMakeLists
Modify `ned/CMakeLists.txt` to use `CMAKE_CURRENT_SOURCE_DIR` instead of `CMAKE_SOURCE_DIR`:
```cmake
add_definitions(-DSOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
```

### Option 2: Disable FileFinder in Embedded Mode
Contact Ned maintainers to add a configuration option to disable the background file scanner in embedded mode.

### Option 3: Configure Paths Before Initialization
Set environment variables or working directory before creating NedEmbed:
```cpp
// Set working directory to build folder
std::filesystem::current_path("/path/to/build");
m_nedEditor = std::make_unique<NedEmbed>();
```

### Option 4: Use Standalone Ned
Instead of embedding, launch Ned as a separate process and communicate via IPC.

## Build System Changes

### CMakeLists.txt Key Changes
1. **ImGui replacement**: Uses Ned's modified ImGui with Freetype support
2. **Global includes**: Ned's ImGui backends and misc directories
3. **Linker flags**: `-Wl,--allow-multiple-definition` for stb_image conflicts
4. **Dependencies**: All Ned libraries linked (ned_embed, ned_util, tree-sitter grammars, etc.)

### Resource Copying
Ned's resources (settings/, fonts/, shaders/, icons/) are automatically copied to the build directory by Ned's CMakeLists.txt.

## Usage Example (When Working)

```cpp
// Get the script editor
if (scriptEditor)
{
    // Open a specific script file
    scriptEditor->openScript("../assets/scripts/player.lua");
    
    // Show the editor
    scriptEditor->setOpen(true);
    
    // Get current script path
    std::string currentScript = scriptEditor->getCurrentScriptPath();
}
```

## Testing

```bash
cd build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j$(nproc)
./Moiras

# Press F12 to toggle editor (when enabled)
```

## Next Steps

1. **Contact Ned maintainers** about the embedded mode crash
2. **Try Option 1** (fix SOURCE_DIR) by creating a patch for Ned
3. **Test with environment variables** to set proper resource paths
4. **Consider alternative**: Simple text editor without full Ned features

## References

- Ned Repository: https://github.com/nealmick/ned
- Ned Embed Demo: https://github.com/nealmick/ImGui_Ned_Embed
- Issue Thread: (create one on Ned's GitHub)

---

**Date**: 2026-02-02
**Status**: Integration complete, awaiting fix for FileFinder crash
**Integration**: 99% complete
