#pragma once

#include <imgui.h>
#include <string>

namespace moiras {

/**
 * @brief Focusable text input widget that automatically grabs keyboard focus when clicked
 * 
 * This wrapper around ImGui::InputText provides automatic focus management,
 * ensuring that when the user clicks on the input field, it immediately
 * captures keyboard focus for typing.
 * 
 * @param label The label for the input field (use "##name" for invisible labels)
 * @param str Pointer to std::string to edit
 * @param flags ImGui input text flags (default: 0)
 * @return true if the text was modified
 */
inline bool FocusableInputText(const char* label, std::string* str, ImGuiInputTextFlags flags = 0) {
    // Helper struct for std::string with ImGui::InputText
    struct InputTextCallback_UserData {
        std::string* Str;
        ImGuiInputTextCallback ChainCallback;
        void* ChainCallbackUserData;
    };
    
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* str = user_data->Str;
            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }
        else if (user_data->ChainCallback) {
            data->UserData = user_data->ChainCallbackUserData;
            return user_data->ChainCallback(data);
        }
        return 0;
    };
    
    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = nullptr;
    cb_user_data.ChainCallbackUserData = nullptr;
    
    flags |= ImGuiInputTextFlags_CallbackResize;
    
    // Check if the widget was clicked
    ImGui::PushID(label);
    bool clicked = false;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        clicked = true;
    }
    ImGui::PopID();
    
    // Set keyboard focus if clicked
    if (clicked) {
        ImGui::SetKeyboardFocusHere();
    }
    
    // Render the input text
    bool modified = ImGui::InputText(label, (char*)str->c_str(), str->capacity() + 1, 
                                     flags, callback, &cb_user_data);
    
    return modified;
}

/**
 * @brief Focusable multi-line text input widget
 * 
 * @param label The label for the input field
 * @param str Pointer to std::string to edit
 * @param size Size of the text input area
 * @param flags ImGui input text flags (default: 0)
 * @return true if the text was modified
 */
inline bool FocusableInputTextMultiline(const char* label, std::string* str, 
                                       const ImVec2& size = ImVec2(0, 0), 
                                       ImGuiInputTextFlags flags = 0) {
    // Similar implementation as above but for multiline
    struct InputTextCallback_UserData {
        std::string* Str;
        ImGuiInputTextCallback ChainCallback;
        void* ChainCallbackUserData;
    };
    
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        InputTextCallback_UserData* user_data = (InputTextCallback_UserData*)data->UserData;
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
            std::string* str = user_data->Str;
            str->resize(data->BufTextLen);
            data->Buf = (char*)str->c_str();
        }
        else if (user_data->ChainCallback) {
            data->UserData = user_data->ChainCallbackUserData;
            return user_data->ChainCallback(data);
        }
        return 0;
    };
    
    InputTextCallback_UserData cb_user_data;
    cb_user_data.Str = str;
    cb_user_data.ChainCallback = nullptr;
    cb_user_data.ChainCallbackUserData = nullptr;
    
    flags |= ImGuiInputTextFlags_CallbackResize;
    
    // Check if the widget was clicked
    ImGui::PushID(label);
    bool clicked = false;
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        clicked = true;
    }
    ImGui::PopID();
    
    // Set keyboard focus if clicked
    if (clicked) {
        ImGui::SetKeyboardFocusHere();
    }
    
    // Render the input text
    bool modified = ImGui::InputTextMultiline(label, (char*)str->c_str(), 
                                             str->capacity() + 1, size, flags, 
                                             callback, &cb_user_data);
    
    return modified;
}

} // namespace moiras
