#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include "../../external/ImGuiColorTextEdit/TextEditor.h"
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace moiras
{

class ScriptEditor : public GameObject
{
public:
    ScriptEditor();
    ~ScriptEditor();

    void update() override;
    void gui() override;

    // Open a script file in the editor
    void openScript(const std::string& filePath);
    
    // Save the current file
    void saveCurrentFile();
    
    // Get the currently open script path
    std::string getCurrentScriptPath() const;
    
    // Check if editor is open
    bool isOpen() const { return m_isOpen; }
    
    // Set editor visibility
    void setOpen(bool open) { m_isOpen = open; }

private:
    std::unique_ptr<TextEditor> m_textEditor;
    bool m_isOpen;
    std::string m_currentScriptPath;
    
    // File browser state
    bool m_showFileBrowser;
    std::string m_currentDirectory;
    std::vector<std::filesystem::path> m_directoryContents;
    
    // Settings state
    bool m_showSettings;
    int m_fontSize;     // Actual font size in pixels (8-32)
    int m_currentTheme; // 0=Dark, 1=Light, 2=Retro Blue
    bool m_wasHovered;  // Track if editor was hovered in previous frame
    
    // Helper methods
    void loadFile(const std::string& filePath);
    void renderFileBrowser();
    void renderSettings();
    void updateDirectoryContents();
    void applyTheme(int themeIndex);
};

} // namespace moiras
