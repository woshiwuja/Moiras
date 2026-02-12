#include "sdl_window.h"
#include "../input/input_manager.h"
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <stdexcept>
#include <iostream>

namespace moiras {

SDLWindow::SDLWindow()
    : m_window(nullptr)
    , m_glContext(nullptr)
    , m_width(800)
    , m_height(600)
    , m_title("Moiras")
    , m_shouldClose(false)
    , m_fullscreen(false)
{
}

SDLWindow::SDLWindow(int width, int height, const std::string& title, bool fullscreen)
    : m_window(nullptr)
    , m_glContext(nullptr)
    , m_width(width)
    , m_height(height)
    , m_title(title)
    , m_shouldClose(false)
    , m_fullscreen(fullscreen)
{
}

SDLWindow::~SDLWindow() {
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

void SDLWindow::init(int width, int height, const std::string& title, bool fullscreen) {
    m_width = width;
    m_height = height;
    m_title = title;
    m_fullscreen = fullscreen;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    // Request OpenGL 4.5 Core Profile (compatible with Filament)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Enable double buffering
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // MSAA 4X (match Raylib's FLAG_MSAA_4X_HINT)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    // Create window
    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (fullscreen) {
        windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        windowFlags
    );

    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    // Create OpenGL context
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    // Enable VSync (match SetTargetFPS behavior)
    SDL_GL_SetSwapInterval(1);

    std::cout << "SDL Window initialized: " << width << "x" << height << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
}

void SDLWindow::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Forward to ImGui
        ImGui_ImplSDL2_ProcessEvent(&event);

        switch (event.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                }
                break;

            case SDL_KEYDOWN:
                // Handle ESC to close (match Raylib's SetExitKey behavior)
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_shouldClose = true;
                }
                // Forward to InputManager
                InputManager::getInstance().handleSDLKeyDown(event.key);
                break;

            case SDL_KEYUP:
                InputManager::getInstance().handleSDLKeyUp(event.key);
                break;

            case SDL_MOUSEMOTION:
                InputManager::getInstance().handleSDLMouseMotion(event.motion);
                break;

            case SDL_MOUSEBUTTONDOWN:
                InputManager::getInstance().handleSDLMouseButtonDown(event.button);
                break;

            case SDL_MOUSEBUTTONUP:
                InputManager::getInstance().handleSDLMouseButtonUp(event.button);
                break;

            case SDL_MOUSEWHEEL:
                InputManager::getInstance().handleSDLMouseWheel(event.wheel);
                break;
        }
    }
}

void SDLWindow::swapBuffers() {
    SDL_GL_SwapWindow(m_window);
}

void* SDLWindow::getNativeWindow() const {
    // For Filament SwapChain on Linux/X11
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(m_window, &wmInfo)) {
        if (wmInfo.subsystem == SDL_SYSWM_X11) {
            return (void*)(uintptr_t)wmInfo.info.x11.window;
        }
    }
    return nullptr;
}

void SDLWindow::toggleFullscreen() {
    m_fullscreen = !m_fullscreen;
    Uint32 flag = m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(m_window, flag);
}

} // namespace moiras
