#include "lightmanager.h"
#include "imgui.h"
#include <raylib.h>
#include <r3d/r3d.h>

namespace moiras {

LightManager::LightManager() {
    for (int i = 0; i < MAX_LIGHTS; i++) {
        lights[i] = nullptr;
    }
}

int LightManager::addLight(Light *light) {
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i] == nullptr) {
            lights[i] = light;
            lightCount++;
            TraceLog(LOG_INFO, "LightManager: Registered light at index %d", i);
            return i;
        }
    }
    TraceLog(LOG_WARNING, "LightManager: Max lights reached (%d)", MAX_LIGHTS);
    return -1;
}

void LightManager::removeLight(int index) {
    if (index >= 0 && index < MAX_LIGHTS && lights[index] != nullptr) {
        lights[index] = nullptr;
        lightCount--;
    }
}

void LightManager::gui() {
    // r3d environment ambient settings
    if (ImGui::CollapsingHeader("Ambient (r3d)", ImGuiTreeNodeFlags_DefaultOpen)) {
        R3D_Environment *env = R3D_GetEnvironment();
        if (env) {
            float col[4] = {
                env->ambient.color.r / 255.0f,
                env->ambient.color.g / 255.0f,
                env->ambient.color.b / 255.0f,
                env->ambient.color.a / 255.0f
            };
            if (ImGui::ColorEdit3("Ambient Color", col)) {
                env->ambient.color.r = (unsigned char)(col[0] * 255);
                env->ambient.color.g = (unsigned char)(col[1] * 255);
                env->ambient.color.b = (unsigned char)(col[2] * 255);
            }
            ImGui::SliderFloat("Ambient Energy", &env->ambient.energy, 0.0f, 5.0f);
        }
    }

    // Background settings
    if (ImGui::CollapsingHeader("Background (r3d)")) {
        R3D_Environment *env = R3D_GetEnvironment();
        if (env) {
            float col[4] = {
                env->background.color.r / 255.0f,
                env->background.color.g / 255.0f,
                env->background.color.b / 255.0f,
                env->background.color.a / 255.0f
            };
            if (ImGui::ColorEdit3("Background Color", col)) {
                env->background.color.r = (unsigned char)(col[0] * 255);
                env->background.color.g = (unsigned char)(col[1] * 255);
                env->background.color.b = (unsigned char)(col[2] * 255);
            }
            ImGui::SliderFloat("Background Energy", &env->background.energy, 0.0f, 5.0f);
        }
    }

    // Registered scene lights (display only)
    if (ImGui::CollapsingHeader("Scene Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Registered lights: %d", lightCount);
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           "Lighting handled by r3d pipeline");
        ImGui::Separator();
        for (int i = 0; i < MAX_LIGHTS; i++) {
            if (lights[i] != nullptr) {
                ImGui::PushID(i);
                lights[i]->guiControl();
                ImGui::PopID();
            }
        }
    }
}

} // namespace moiras
