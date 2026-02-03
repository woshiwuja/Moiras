#include "script_editor.h"
#include <raylib.h>
#include <GL/glew.h>

namespace moiras
{

ScriptEditor::ScriptEditor()
    : GameObject("ScriptEditor")
    , m_isOpen(false)
    , m_initialized(false)
{
}

ScriptEditor::~ScriptEditor()
{
    if (m_nedEditor) {
        m_nedEditor->cleanup();
    }
}

void ScriptEditor::initializeEditor()
{
    if (m_initialized) return;
    
    // Initialize GLEW if not already initialized (needed for Ned's OpenGL features)
    static bool glewInitialized = false;
    if (!glewInitialized) {
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to initialize GLEW: %s", 
                    glewGetErrorString(err));
            return;
        }
        glewInitialized = true;
        TraceLog(LOG_INFO, "SCRIPT_EDITOR: GLEW initialized successfully");
    }
    
    try {
        // Create Ned editor (it initializes automatically in constructor)
        m_nedEditor = std::make_unique<NedEmbed>();
        m_initialized = true;
        
        TraceLog(LOG_INFO, "SCRIPT_EDITOR: Ned editor initialized");
    }
    catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to initialize Ned editor: %s", e.what());
        m_initialized = false;
        m_isOpen = false;
    }
    catch (...) {
        TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to initialize Ned editor: Unknown error");
        m_initialized = false;
        m_isOpen = false;
    }
}

void ScriptEditor::update()
{
    // Only initialize when editor is opened for the first time
    if (m_isOpen && !m_initialized) {
        initializeEditor();
    }
    
    // Check for font reloading before ImGui frame (only if editor is open)
    if (m_isOpen && m_nedEditor) {
        m_nedEditor->checkForFontReload();
    }
}

void ScriptEditor::gui()
{
    if (!m_isOpen) {
        m_wasOpen = false;
        return;
    }

    if (!m_initialized) {
        ImGui::Begin("Script Editor", &m_isOpen);
        ImGui::Text("Initializing editor...");
        ImGui::End();
        return;
    }

    // Show Ned editor window
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);

    // Track if window state changed
    if (!m_wasOpen) {
        m_wasOpen = true;
    }
    
    // Request focus if needed
    if (m_shouldGrabFocus) {
        ImGui::SetNextWindowFocus();
        m_shouldGrabFocus = false;
    }

    if (ImGui::Begin("Script Editor", &m_isOpen, ImGuiWindowFlags_MenuBar)) {
        // Detect if user clicked in the window to grab focus
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_shouldGrabFocus = true;
        }
        
        // Handle ESC to release focus from the editor window
        if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            // Let the embedded editor handle ESC first
            // If it doesn't consume it, we could close the window or release focus
        }
        // Optional: Add menu bar for quick actions
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Script Folder")) {
                    // Could trigger file browser here
                }
                if (ImGui::MenuItem("Reload Current")) {
                    // Reload current script
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Editor")) {
                    m_isOpen = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Show Project Tree", nullptr, true);
                ImGui::MenuItem("Show Terminal", nullptr, false);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        // Render Ned editor
        if (m_nedEditor) {
            m_nedEditor->render();
        }
    }
    ImGui::End();
}

void ScriptEditor::openScript(const std::string& filePath)
{
    m_currentScriptPath = filePath;
    m_isOpen = true;
    
    TraceLog(LOG_INFO, "SCRIPT_EDITOR: Opening script: %s", filePath.c_str());
}

std::string ScriptEditor::getCurrentScriptPath() const
{
    return m_currentScriptPath;
}

} // namespace moiras
