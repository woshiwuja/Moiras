#include "lights.h"
#include "raymath.h"
#include "imgui.h"
#include <raylib.h>

namespace moiras {

// ==================== Light Base ====================

Light::Light(const std::string& name) {
    this->name = name;
    normalizeColor();
}

void Light::normalizeColor() {
    colorNormalized[0] = color.r / 255.0f;
    colorNormalized[1] = color.g / 255.0f;
    colorNormalized[2] = color.b / 255.0f;
    colorNormalized[3] = color.a / 255.0f;
}

void Light::setupShaderLocations(Shader shader, int lightIndex) {
    enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightIndex));
    typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightIndex));
    positionLoc = GetShaderLocation(shader, TextFormat("lights[%i].position", lightIndex));
    targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightIndex));
    colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightIndex));
    intensityLoc = GetShaderLocation(shader, TextFormat("lights[%i].intensity", lightIndex));
}

void Light::updateShader(Shader shader) {
    normalizeColor();
    
    int enabledInt = enabled ? 1 : 0;
    int typeInt = static_cast<int>(getType());
    
    SetShaderValue(shader, enabledLoc, &enabledInt, SHADER_UNIFORM_INT);
    SetShaderValue(shader, typeLoc, &typeInt, SHADER_UNIFORM_INT);
    
    float pos[3] = {position.x, position.y, position.z};
    SetShaderValue(shader, positionLoc, pos, SHADER_UNIFORM_VEC3);
    
    float tgt[3] = {target.x, target.y, target.z};
    SetShaderValue(shader, targetLoc, tgt, SHADER_UNIFORM_VEC3);
    
    SetShaderValue(shader, colorLoc, colorNormalized, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, intensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
}

void Light::update() {
    GameObject::update();
}

void Light::draw() {
    // Override nelle sottoclassi
}

void Light::gui(){}
void Light::guiControl() {
    ImGui::PushID(this);
    
    if (ImGui::TreeNode(name.c_str())) {
        ImGui::Checkbox("Enabled", &enabled);
        ImGui::DragFloat3("Position", &position.x, 0.1f);
        ImGui::DragFloat3("Target", &target.x, 0.1f);
        
        float col[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f};
        if (ImGui::ColorEdit4("Color", col)) {
            color.r = (unsigned char)(col[0] * 255);
            color.g = (unsigned char)(col[1] * 255);
            color.b = (unsigned char)(col[2] * 255);
            color.a = (unsigned char)(col[3] * 255);
        }
        
        ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f);
        
        ImGui::TreePop();
    }
    
    ImGui::PopID();
}

// ==================== PointLight ====================

PointLight::PointLight(const std::string& name) : Light(name) {
    this->name = name;
}

void PointLight::draw() {
    if (enabled) {
        DrawSphereEx(position, 0.2f, 8, 8, color);
    } else {
        DrawSphereWires(position, 0.2f, 8, 8, ColorAlpha(color, 0.3f));
    }
}

// ==================== SpotLight ====================

SpotLight::SpotLight(const std::string& name) : Light(name) {
    this->name = name;
}

void SpotLight::draw() {
    if (enabled) {
        DrawSphereEx(position, 0.15f, 8, 8, color);
        // Disegna direzione dello spot
        DrawLine3D(position, target, color);
        DrawCylinderEx(position, target, 0.1f, 0.3f, 8, ColorAlpha(color, 0.3f));
    } else {
        DrawSphereWires(position, 0.15f, 8, 8, ColorAlpha(color, 0.3f));
    }
}

// ==================== DirectionalLight ====================

DirectionalLight::DirectionalLight(const std::string& name) : Light(name) {
    this->name = name;
    position = {0, 10, 0}; // Default alto
    target = {0, 0, 0};
}

void DirectionalLight::draw() {
    if (enabled) {
        // Disegna freccia che indica la direzione
        Vector3 dir = Vector3Normalize(Vector3Subtract(target, position));
        Vector3 arrowEnd = Vector3Add(position, Vector3Scale(dir, 2.0f));
        
        DrawLine3D(position, arrowEnd, color);
        DrawSphereEx(position, 0.1f, 8, 8, color);
        
        // Freccia
        DrawCylinderEx(arrowEnd, Vector3Add(arrowEnd, Vector3Scale(dir, 0.3f)), 0.15f, 0.0f, 8, color);
    } else {
        DrawSphereWires(position, 0.1f, 8, 8, ColorAlpha(color, 0.3f));
    }
}

} // namespace moiras
