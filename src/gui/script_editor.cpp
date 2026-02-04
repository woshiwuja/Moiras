#include "script_editor.h"
#include "gui.h"
#include <raylib.h>
#include <algorithm>

namespace moiras
{

ScriptEditor::ScriptEditor()
    : GameObject("ScriptEditor")
    , m_isOpen(false)
    , m_showFileBrowser(false)
    , m_currentDirectory("../assets/scripts")
    , m_showSettings(false)
    , m_fontSize(16) // Default font size 16px
    , m_currentTheme(0) // Dark theme by default
    , m_wasHovered(false)
{
    // Initialize text editor
    m_textEditor = std::make_unique<TextEditor>();
    
    // Set language definition to Lua
    auto lang = TextEditor::LanguageDefinition::Lua();
    m_textEditor->SetLanguageDefinition(lang);
    
    // Use dark palette
    applyTheme(m_currentTheme);
    
    TraceLog(LOG_INFO, "SCRIPT_EDITOR: TextEditor initialized");
}

ScriptEditor::~ScriptEditor()
{
}

void ScriptEditor::update()
{
    // No update needed for TextEditor
}

void ScriptEditor::loadFile(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        m_textEditor->SetText(buffer.str());
        m_currentScriptPath = filePath;
        TraceLog(LOG_INFO, "SCRIPT_EDITOR: Loaded file: %s", filePath.c_str());
    } else {
        TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to open file: %s", filePath.c_str());
    }
}

void ScriptEditor::saveCurrentFile()
{
    if (m_currentScriptPath.empty()) {
        TraceLog(LOG_WARNING, "SCRIPT_EDITOR: No file to save");
        return;
    }
    
    std::ofstream file(m_currentScriptPath);
    if (file.is_open()) {
        file << m_textEditor->GetText();
        TraceLog(LOG_INFO, "SCRIPT_EDITOR: Saved file: %s", m_currentScriptPath.c_str());
    } else {
        TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to save file: %s", m_currentScriptPath.c_str());
    }
}

void ScriptEditor::applyTheme(int themeIndex)
{
    switch (themeIndex) {
        case 0:
            m_textEditor->SetPalette(TextEditor::GetDarkPalette());
            break;
        case 1:
            m_textEditor->SetPalette(TextEditor::GetLightPalette());
            break;
        case 2:
            m_textEditor->SetPalette(TextEditor::GetRetroBluePalette());
            break;
    }
}

void ScriptEditor::updateDirectoryContents()
{
    m_directoryContents.clear();
    
    try {
        if (std::filesystem::exists(m_currentDirectory)) {
            for (const auto& entry : std::filesystem::directory_iterator(m_currentDirectory)) {
                m_directoryContents.push_back(entry.path());
            }
            
            // Sort: directories first, then files
            std::sort(m_directoryContents.begin(), m_directoryContents.end(),
                [](const std::filesystem::path& a, const std::filesystem::path& b) {
                    bool aIsDir = std::filesystem::is_directory(a);
                    bool bIsDir = std::filesystem::is_directory(b);
                    if (aIsDir != bIsDir) return aIsDir;
                    return a.filename().string() < b.filename().string();
                });
        }
    } catch (const std::filesystem::filesystem_error& e) {
        TraceLog(LOG_ERROR, "SCRIPT_EDITOR: Failed to read directory: %s", e.what());
    }
}

void ScriptEditor::renderFileBrowser()
{
    if (!m_showFileBrowser) return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Open Script File", &m_showFileBrowser)) {
        // Current directory display
        ImGui::Text("Current Directory: %s", m_currentDirectory.c_str());
        ImGui::Separator();
        
        // Parent directory button
        if (ImGui::Button("..") && m_currentDirectory != "/") {
            std::filesystem::path parentPath = std::filesystem::path(m_currentDirectory).parent_path();
            m_currentDirectory = parentPath.string();
            updateDirectoryContents();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh")) {
            updateDirectoryContents();
        }
        
        ImGui::Separator();
        
        // File list
        ImGui::BeginChild("FileList", ImVec2(0, -30), true);
        
        for (const auto& path : m_directoryContents) {
            bool isDir = std::filesystem::is_directory(path);
            std::string filename = path.filename().string();
            std::string ext = path.extension().string();
            
            // Show directories and .lua files
            if (isDir) {
                if (ImGui::Selectable((std::string("📁 ") + filename).c_str(), false, ImGuiSelectableFlags_DontClosePopups)) {
                    m_currentDirectory = path.string();
                    updateDirectoryContents();
                }
            } else if (ext == ".lua") {
                if (ImGui::Selectable((std::string("📄 ") + filename).c_str())) {
                    loadFile(path.string());
                    m_showFileBrowser = false;
                }
            }
        }
        
        ImGui::EndChild();
        
        // Bottom buttons
        if (ImGui::Button("Cancel")) {
            m_showFileBrowser = false;
        }
    }
    ImGui::End();
}

void ScriptEditor::renderSettings()
{
    if (!m_showSettings) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Editor Settings", &m_showSettings)) {
        ImGui::Text("Appearance Settings");
        ImGui::Separator();
        
        // Font size slider
        int oldSize = m_fontSize;
        if (ImGui::SliderInt("Font Size", &m_fontSize, 8, 32, "%d px")) {
            TraceLog(LOG_INFO, "SCRIPT_EDITOR: Font size changed to %dpx", m_fontSize);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset##FontSize")) {
            m_fontSize = 16;
        }
        
        ImGui::Text("Use Ctrl+= to increase, Ctrl+- to decrease");
        
        ImGui::Spacing();
        
        // Theme selector
        const char* themes[] = { "Dark", "Light", "Retro Blue" };
        int oldTheme = m_currentTheme;
        if (ImGui::Combo("Theme", &m_currentTheme, themes, IM_ARRAYSIZE(themes))) {
            applyTheme(m_currentTheme);
            TraceLog(LOG_INFO, "SCRIPT_EDITOR: Theme changed to %s", themes[m_currentTheme]);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // Whitespace visibility
        bool showWhitespace = m_textEditor->IsShowingWhitespaces();
        if (ImGui::Checkbox("Show Whitespace Characters", &showWhitespace)) {
            m_textEditor->SetShowWhitespaces(showWhitespace);
        }
        
        ImGui::Spacing();
        
        // Tab size
        int tabSize = m_textEditor->GetTabSize();
        if (ImGui::SliderInt("Tab Size", &tabSize, 2, 8)) {
            m_textEditor->SetTabSize(tabSize);
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        if (ImGui::Button("Close")) {
            m_showSettings = false;
        }
    }
    ImGui::End();
}

void ScriptEditor::gui()
{
    if (!m_isOpen) {
        m_wasHovered = false;
        return;
    }

    // Show text editor window
    ImGui::SetNextWindowSize(ImVec2(1000, 700), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Script Editor", &m_isOpen, ImGuiWindowFlags_MenuBar)) {
        // Check if window/editor area is hovered
        bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
        bool isWindowFocused = ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows);
        
        // Get ImGui IO for input control
        ImGuiIO& io = ImGui::GetIO();
        bool ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
        
        // Handle keyboard shortcuts for font size (only when window has focus)
        if (isWindowFocused && ctrl) {
            // Ctrl+= or Ctrl++ to increase font
            if (ImGui::IsKeyPressed(ImGuiKey_Equal) || ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)) {
                m_fontSize += 2;
                if (m_fontSize > 32) m_fontSize = 32;
                TraceLog(LOG_INFO, "SCRIPT_EDITOR: Font size increased to %dpx", m_fontSize);
            }
            // Ctrl+- to decrease font
            if (ImGui::IsKeyPressed(ImGuiKey_Minus) || ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract)) {
                m_fontSize -= 2;
                if (m_fontSize < 8) m_fontSize = 8;
                TraceLog(LOG_INFO, "SCRIPT_EDITOR: Font size decreased to %dpx", m_fontSize);
            }
            // Ctrl+0 to reset font
            if (ImGui::IsKeyPressed(ImGuiKey_0) || ImGui::IsKeyPressed(ImGuiKey_Keypad0)) {
                m_fontSize = 16;
                TraceLog(LOG_INFO, "SCRIPT_EDITOR: Font size reset to 16px");
            }
        }
        
        // Enable/disable editor input based on focus state
        // Keyboard: Use focus (so you can type even if mouse moves away)
        // Mouse: Use hover (so clicks outside don't affect editor)
        m_textEditor->SetHandleKeyboardInputs(isWindowFocused);
        m_textEditor->SetHandleMouseInputs(isWindowHovered);
        
        // CRITICAL: Release ImGui input capture when not active
        // This allows camera control and game input to work when editor is not active
        if (!isWindowHovered) {
            io.WantCaptureMouse = false;
        }
        if (!isWindowFocused) {
            io.WantCaptureKeyboard = false;
        }
        
        m_wasHovered = isWindowHovered;
        
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                    m_showFileBrowser = true;
                    updateDirectoryContents();
                }
                if (ImGui::MenuItem("Save", "Ctrl+S", nullptr, !m_currentScriptPath.empty())) {
                    saveCurrentFile();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Close Editor")) {
                    m_isOpen = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                bool canUndo = m_textEditor->CanUndo();
                bool canRedo = m_textEditor->CanRedo();
                
                if (ImGui::MenuItem("Undo", "Ctrl+Z", nullptr, canUndo)) {
                    m_textEditor->Undo();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", nullptr, canRedo)) {
                    m_textEditor->Redo();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                    m_textEditor->Copy();
                }
                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                    m_textEditor->Cut();
                }
                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                    m_textEditor->Paste();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                    m_textEditor->SelectAll();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Settings...")) {
                    m_showSettings = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Increase Font Size", "Ctrl+=")) {
                    m_fontSize += 2;
                    if (m_fontSize > 32) m_fontSize = 32;
                }
                if (ImGui::MenuItem("Decrease Font Size", "Ctrl+-")) {
                    m_fontSize -= 2;
                    if (m_fontSize < 8) m_fontSize = 8;
                }
                if (ImGui::MenuItem("Reset Font Size", "Ctrl+0")) {
                    m_fontSize = 16;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
        
        // Status bar
        auto cpos = m_textEditor->GetCursorPosition();
        ImGui::Text("Line: %d, Column: %d | %d lines | Font: %dpx | %s", 
                    cpos.mLine + 1, 
                    cpos.mColumn + 1, 
                    m_textEditor->GetTotalLines(),
                    m_fontSize,
                    m_currentScriptPath.empty() ? "No file" : m_currentScriptPath.c_str());
        
        ImGui::Separator();
        
        // Get the appropriate font for the current size
        ImFont* editorFont = Gui::getEditorFont(m_fontSize);
        if (editorFont) {
            ImGui::PushFont(editorFont);
        }
        
        // Render text editor with the selected font
        m_textEditor->Render("TextEditor");
        
        // Pop font if we pushed one
        if (editorFont) {
            ImGui::PopFont();
        }
    }
    ImGui::End();
    
    // Render dialogs
    renderFileBrowser();
    renderSettings();
}

void ScriptEditor::openScript(const std::string& filePath)
{
    loadFile(filePath);
    m_isOpen = true;
}

std::string ScriptEditor::getCurrentScriptPath() const
{
    return m_currentScriptPath;
}

} // namespace moiras
