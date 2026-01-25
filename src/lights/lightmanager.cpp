#include "lightmanager.h"
#include "imgui.h"
#include <raylib.h>

namespace moiras {

LightManager::LightManager() {
    for (int i = 0; i < MAX_LIGHTS; i++) {
        lights[i] = nullptr;
    }
}

void LightManager::loadShader(const std::string &vsPath,
                              const std::string &fsPath) {
    shader = LoadShader(vsPath.c_str(), fsPath.c_str());

    if (shader.id == 0) {
        TraceLog(LOG_ERROR, "LightManager: Failed to load shader!");
        return;
    }

    TraceLog(LOG_INFO, "LightManager: Shader loaded with ID %d", shader.id);

    // Setup locations
    viewPosLoc = GetShaderLocation(shader, "viewPos");
    ambientColorLoc = GetShaderLocation(shader, "ambientColor");
    ambientIntensityLoc = GetShaderLocation(shader, "ambient");

    // PBR locations
    metallicLoc = GetShaderLocation(shader, "metallicValue");
    roughnessLoc = GetShaderLocation(shader, "roughnessValue");
    aoLoc = GetShaderLocation(shader, "aoValue");
    normalLoc = GetShaderLocation(shader, "normalValue");
    emissivePowerLoc = GetShaderLocation(shader, "emissivePower");
    albedoColorLoc = GetShaderLocation(shader, "albedoColor");
    emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
    tilingLoc = GetShaderLocation(shader, "tiling");
    offsetLoc = GetShaderLocation(shader, "offset");
    useTexAlbedoLoc = GetShaderLocation(shader, "useTexAlbedo");
    useTexNormalLoc = GetShaderLocation(shader, "useTexNormal");
    useTexMRALoc = GetShaderLocation(shader, "useTexMRA");
    useTexEmissiveLoc = GetShaderLocation(shader, "useTexEmissive");

    // Set number of lights
    int numLights = MAX_LIGHTS;
    SetShaderValue(shader, GetShaderLocation(shader, "numOfLights"), &numLights, SHADER_UNIFORM_INT);

    // Apply initial PBR values
    updatePBRUniforms();
    setAmbient(ambientColor, ambientIntensity);

    TraceLog(LOG_INFO, "LightManager: PBR shader configured");
    tilingLoc = GetShaderLocation(shader, "tiling");
    offsetLoc = GetShaderLocation(shader, "offset");
    useTilingLoc = GetShaderLocation(shader, "useTiling");
    
    // Default: no tiling
    int noTiling = 0;
    SetShaderValue(shader, useTilingLoc, &noTiling, SHADER_UNIFORM_INT);
}

void LightManager::updatePBRUniforms() {
    SetShaderValue(shader, metallicLoc, &metallicValue, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, roughnessLoc, &roughnessValue, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, aoLoc, &aoValue, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, normalLoc, &normalValue, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, emissivePowerLoc, &emissivePower, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, albedoColorLoc, albedoColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, emissiveColorLoc, emissiveColor, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, tilingLoc, tiling, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, offsetLoc, offset, SHADER_UNIFORM_VEC2);
    
    int useAlbedo = useTexAlbedo ? 1 : 0;
    int useNormal = useTexNormal ? 1 : 0;
    int useMRA = useTexMRA ? 1 : 0;
    int useEmissive = useTexEmissive ? 1 : 0;
    SetShaderValue(shader, useTexAlbedoLoc, &useAlbedo, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexNormalLoc, &useNormal, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexMRALoc, &useMRA, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexEmissiveLoc, &useEmissive, SHADER_UNIFORM_INT);
    int tilingEnabled = useTiling ? 1 : 0;
    SetShaderValue(shader, useTilingLoc, &tilingEnabled, SHADER_UNIFORM_INT);
    
    if (useTiling) {
        SetShaderValue(shader, tilingLoc, tiling, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, offsetLoc, offset, SHADER_UNIFORM_VEC2);
    }
}

int LightManager::addLight(Light *light) {
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i] == nullptr) {
            lights[i] = light;
            light->setupShaderLocations(shader, i);
            light->updateShader(shader);
            lightCount++;
            TraceLog(LOG_INFO, "LightManager: Added light at index %d", i);
            return i;
        }
    }
    TraceLog(LOG_WARNING, "LightManager: Max lights reached (%d)", MAX_LIGHTS);
    return -1;
}

void LightManager::removeLight(int index) {
    if (index >= 0 && index < MAX_LIGHTS && lights[index] != nullptr) {
        int disabled = 0;
        SetShaderValue(shader, lights[index]->enabledLoc, &disabled, SHADER_UNIFORM_INT);
        lights[index] = nullptr;
        lightCount--;
    }
}

void LightManager::updateAllLights() {
    // Update PBR uniforms every frame (in case they changed via GUI)
    updatePBRUniforms();
    
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i] != nullptr) {
            lights[i]->updateShader(shader);
        }
    }
}

void LightManager::updateCameraPosition(Vector3 cameraPos) {
    float pos[3] = {cameraPos.x, cameraPos.y, cameraPos.z};
    SetShaderValue(shader, viewPosLoc, pos, SHADER_UNIFORM_VEC3);
}

void LightManager::setAmbient(Color color, float intensity) {
    ambientColor = color;
    ambientIntensity = intensity;
    Vector3 colorNorm = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f};
    SetShaderValue(shader, ambientColorLoc, &colorNorm, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, ambientIntensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
}

void LightManager::gui() {
    // Ambient settings
    if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
        float ambCol[3] = {ambientColor.r / 255.0f, ambientColor.g / 255.0f, ambientColor.b / 255.0f};
        if (ImGui::ColorEdit3("Ambient Color", ambCol)) {
            ambientColor.r = (unsigned char)(ambCol[0] * 255);
            ambientColor.g = (unsigned char)(ambCol[1] * 255);
            ambientColor.b = (unsigned char)(ambCol[2] * 255);
            setAmbient(ambientColor, ambientIntensity);
        }
        if (ImGui::SliderFloat("Ambient Intensity", &ambientIntensity, 0.0f, 2.0f)) {
            setAmbient(ambientColor, ambientIntensity);
        }
    }
    
    // PBR Material settings
    if (ImGui::CollapsingHeader("PBR Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("Metallic", &metallicValue, 0.0f, 1.0f);
        ImGui::SliderFloat("Roughness", &roughnessValue, 0.0f, 1.0f);
        ImGui::SliderFloat("AO", &aoValue, 0.0f, 1.0f);
        ImGui::SliderFloat("Normal Strength", &normalValue, 0.0f, 2.0f);
        
        ImGui::Spacing();
        ImGui::ColorEdit4("Albedo Color", albedoColor);
        
        ImGui::Spacing();
        ImGui::ColorEdit4("Emissive Color", emissiveColor);
        ImGui::SliderFloat("Emissive Power", &emissivePower, 0.0f, 10.0f);
    }
    
    // Texture settings
    if (ImGui::CollapsingHeader("Textures")) {
        ImGui::Checkbox("Use Albedo Map", &useTexAlbedo);
        ImGui::Checkbox("Use Normal Map", &useTexNormal);
        ImGui::Checkbox("Use MRA Map", &useTexMRA);
        ImGui::Checkbox("Use Emissive Map", &useTexEmissive);
        
        ImGui::Spacing();
        ImGui::DragFloat2("Tiling", tiling, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat2("Offset", offset, 0.01f, -1.0f, 1.0f);
    }
    
    // Lights
    if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Active Lights: %d / %d", lightCount, MAX_LIGHTS);
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

void LightManager::unload() {
    UnloadShader(shader);
    shader = {0};
    for (int i = 0; i < MAX_LIGHTS; i++) {
        lights[i] = nullptr;
    }
    lightCount = 0;
}
void LightManager::applyMaterial(const Material& material) {
    // Albedo color dal materiale
    Color albedo = material.maps[MATERIAL_MAP_ALBEDO].color;
    float albedoCol[4] = {
        albedo.r / 255.0f,
        albedo.g / 255.0f,
        albedo.b / 255.0f,
        albedo.a / 255.0f
    };
    SetShaderValue(shader, albedoColorLoc, albedoCol, SHADER_UNIFORM_VEC4);
    
    // Metallic e roughness dai valori del materiale
    float metallic = material.maps[MATERIAL_MAP_METALNESS].value;
    float roughness = material.maps[MATERIAL_MAP_ROUGHNESS].value;
    float ao = material.maps[MATERIAL_MAP_OCCLUSION].value;
    
    SetShaderValue(shader, metallicLoc, &metallic, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, roughnessLoc, &roughness, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, aoLoc, &ao, SHADER_UNIFORM_FLOAT);
    
    // Controlla se le texture sono presenti
    int hasAlbedo = (material.maps[MATERIAL_MAP_ALBEDO].texture.id > 0) ? 1 : 0;
    int hasNormal = (material.maps[MATERIAL_MAP_NORMAL].texture.id > 0) ? 1 : 0;
    int hasMRA = (material.maps[MATERIAL_MAP_METALNESS].texture.id > 0) ? 1 : 0;
    int hasEmissive = (material.maps[MATERIAL_MAP_EMISSION].texture.id > 0) ? 1 : 0;
    
    SetShaderValue(shader, useTexAlbedoLoc, &hasAlbedo, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexNormalLoc, &hasNormal, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexMRALoc, &hasMRA, SHADER_UNIFORM_INT);
    SetShaderValue(shader, useTexEmissiveLoc, &hasEmissive, SHADER_UNIFORM_INT);
    
    // Emissive
    if (hasEmissive) {
        Color emissive = material.maps[MATERIAL_MAP_EMISSION].color;
        float emissiveCol[4] = {
            emissive.r / 255.0f,
            emissive.g / 255.0f,
            emissive.b / 255.0f,
            emissive.a / 255.0f
        };
        SetShaderValue(shader, emissiveColorLoc, emissiveCol, SHADER_UNIFORM_VEC4);
    }
}

} // namespace moiras
