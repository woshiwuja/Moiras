#pragma once

#include <SDL2/SDL.h>
#include <string>

namespace moiras {

class SDLWindow {
private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_width;
    int m_height;
    std::string m_title;
    bool m_shouldClose = false;
    bool m_fullscreen;

public:
    SDLWindow();
    SDLWindow(int width, int height, const std::string& title, bool fullscreen = false);
    ~SDLWindow();

    // Initialize window and OpenGL context
    void init(int width, int height, const std::string& title, bool fullscreen = false);

    // Poll SDL events
    void pollEvents();

    // Check if window should close
    bool shouldClose() const { return m_shouldClose; }

    // Swap OpenGL buffers
    void swapBuffers();

    // Get native window handle for Filament SwapChain
    void* getNativeWindow() const;

    // Accessors
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    SDL_Window* getSDLWindow() const { return m_window; }
    SDL_GLContext getGLContext() const { return m_glContext; }

    // Toggle fullscreen
    void toggleFullscreen();
};

} // namespace moiras
