#pragma once

#include "../game/game_object.h"
#include "imgui.h"
#include "ned_embed.h"
#include <string>
#include <memory>

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
    
    // Get the currently open script path
    std::string getCurrentScriptPath() const;
    
    // Check if editor is open
    bool isOpen() const { return m_isOpen; }
    
    // Set editor visibility
    void setOpen(bool open) { m_isOpen = open; }

private:
    std::unique_ptr<NedEmbed> m_nedEditor;
    bool m_isOpen;
    bool m_initialized;
    bool m_wasOpen = false;  // Track if window was open in the previous frame
    bool m_shouldGrabFocus = false;  // Flag to request focus on next frame
    std::string m_currentScriptPath;

    void initializeEditor();
};

} // namespace moiras
