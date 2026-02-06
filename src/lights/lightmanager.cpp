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

    // Shadow uniform locations
    pbrShadowEnabledLoc = GetShaderLocation(shader, "shadowsEnabled");
    pbrLightSpaceMatrixLoc = GetShaderLocation(shader, "lightSpaceMatrix");
    pbrShadowMapLoc = GetShaderLocation(shader, "shadowMap");
    pbrShadowBiasLoc = GetShaderLocation(shader, "shadowBias");

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

void LightManager::setupShadowMap(const std::string &depthVsPath,
                                   const std::string &depthFsPath) {
    // Load depth shader
    shadowDepthShader = LoadShader(depthVsPath.c_str(), depthFsPath.c_str());
    if (shadowDepthShader.id == 0) {
        TraceLog(LOG_ERROR, "LightManager: Failed to load shadow depth shader!");
        return;
    }

    // Create shadow material
    shadowMaterial = LoadMaterialDefault();
    shadowMaterial.shader = shadowDepthShader;

    // Create shadow map FBO
    shadowMapFBO = rlLoadFramebuffer();
    if (shadowMapFBO == 0) {
        TraceLog(LOG_ERROR, "LightManager: Failed to create shadow FBO!");
        return;
    }

    // Create depth texture (not renderbuffer, so it's sampleable)
    rlEnableFramebuffer(shadowMapFBO);
    shadowMapDepthTex = rlLoadTextureDepth(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, false);
    rlFramebufferAttach(shadowMapFBO, shadowMapDepthTex, RL_ATTACHMENT_DEPTH,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(shadowMapFBO)) {
        TraceLog(LOG_ERROR, "LightManager: Shadow FBO is not complete!");
        rlUnloadFramebuffer(shadowMapFBO);
        shadowMapFBO = 0;
        return;
    }
    rlDisableFramebuffer();

    // Set depth texture filtering
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);

    // Bind shadow map sampler to PBR shader
    if (shader.id > 0 && pbrShadowMapLoc >= 0) {
        rlEnableShader(shader.id);
        rlActiveTextureSlot(SHADOW_TEXTURE_SLOT);
        rlEnableTexture(shadowMapDepthTex);
        rlSetUniformSampler(pbrShadowMapLoc, SHADOW_TEXTURE_SLOT);
        rlActiveTextureSlot(0);
    }

    shadowMapReady = true;
    TraceLog(LOG_INFO, "LightManager: Shadow map initialized (%dx%d, FBO: %u, Depth: %u)",
             SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, shadowMapFBO, shadowMapDepthTex);
}

void LightManager::updateLightSpaceMatrix(Vector3 cameraPos) {
    if (!shadowMapReady) return;

    // Find the first enabled directional light
    Light *dirLight = nullptr;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i] && lights[i]->enabled &&
            lights[i]->getType() == LightType::DIRECTIONAL) {
            dirLight = lights[i];
            break;
        }
    }

    if (!dirLight) return;

    // Compute light direction (from light position toward target)
    Vector3 lightDir = Vector3Normalize(Vector3Subtract(dirLight->target, dirLight->position));

    // Position the shadow camera behind the scene, centered on the player area
    Vector3 shadowCenter = cameraPos;
    shadowCenter.y = 0.0f; // Focus at ground level
    Vector3 lightPos = Vector3Subtract(shadowCenter, Vector3Scale(lightDir, shadowFar * 0.5f));

    // Handle edge case: light direction parallel to up vector
    Vector3 up = {0.0f, 1.0f, 0.0f};
    if (fabsf(Vector3DotProduct(lightDir, up)) > 0.99f) {
        up = {0.0f, 0.0f, 1.0f};
    }

    Matrix lightView = MatrixLookAt(lightPos, shadowCenter, up);
    Matrix lightProj = MatrixOrtho(-shadowOrthoSize, shadowOrthoSize,
                                    -shadowOrthoSize, shadowOrthoSize,
                                    shadowNear, shadowFar);

    lightSpaceMatrix = MatrixMultiply(lightView, lightProj);

    // Upload to PBR shader
    if (shader.id > 0 && pbrLightSpaceMatrixLoc >= 0) {
        SetShaderValueMatrix(shader, pbrLightSpaceMatrixLoc, lightSpaceMatrix);
    }

    // Upload shadow enabled and bias to PBR shader
    if (shader.id > 0) {
        int enabled = shadowsEnabled ? 1 : 0;
        SetShaderValue(shader, pbrShadowEnabledLoc, &enabled, SHADER_UNIFORM_INT);
        SetShaderValue(shader, pbrShadowBiasLoc, &shadowBias, SHADER_UNIFORM_FLOAT);
    }
}

void LightManager::beginShadowPass() {
    if (!shadowMapReady || !shadowsEnabled) return;

    // Save current matrices
    savedProjection = rlGetMatrixProjection();
    savedModelview = rlGetMatrixModelview();

    // Enable shadow FBO
    rlEnableFramebuffer(shadowMapFBO);
    rlViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    rlClearColor(255, 255, 255, 255);
    rlClearScreenBuffers();

    // Find the first enabled directional light to get matrices
    Light *dirLight = nullptr;
    for (int i = 0; i < MAX_LIGHTS; i++) {
        if (lights[i] && lights[i]->enabled &&
            lights[i]->getType() == LightType::DIRECTIONAL) {
            dirLight = lights[i];
            break;
        }
    }

    if (!dirLight) return;

    // Set combined light space matrix as modelview, identity as projection.
    // rlgl computes MVP = matModel * matView * matProjection internally,
    // so MVP = matModel * lightSpaceMatrix * I = matModel * lightSpaceMatrix.
    rlSetMatrixModelview(lightSpaceMatrix);
    rlSetMatrixProjection(MatrixIdentity());

    rlEnableDepthTest();
    rlEnableDepthMask();
}

void LightManager::endShadowPass() {
    if (!shadowMapReady || !shadowsEnabled) return;

    rlDisableFramebuffer();

    // Restore viewport
    rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());

    // Restore matrices
    rlSetMatrixProjection(savedProjection);
    rlSetMatrixModelview(savedModelview);
}

void LightManager::bindShadowMap() {
    if (!shadowMapReady) return;

    // Bind shadow depth texture to the designated slot
    rlActiveTextureSlot(SHADOW_TEXTURE_SLOT);
    rlEnableTexture(shadowMapDepthTex);
    rlActiveTextureSlot(0);
}

void LightManager::bindShadowMapToShader(Shader targetShader) {
    if (!shadowMapReady || targetShader.id == 0) return;

    // Get shadow uniform locations for the target shader
    int shadowEnabledLoc = GetShaderLocation(targetShader, "shadowsEnabled");
    int lightSpaceMatLoc = GetShaderLocation(targetShader, "lightSpaceMatrix");
    int shadowMapLoc = GetShaderLocation(targetShader, "shadowMap");
    int biasLoc = GetShaderLocation(targetShader, "shadowBias");

    // Set lightSpaceMatrix
    if (lightSpaceMatLoc >= 0) {
        SetShaderValueMatrix(targetShader, lightSpaceMatLoc, lightSpaceMatrix);
    }

    // Set shadow enabled
    int enabled = (shadowsEnabled && shadowMapReady) ? 1 : 0;
    if (shadowEnabledLoc >= 0) {
        SetShaderValue(targetShader, shadowEnabledLoc, &enabled, SHADER_UNIFORM_INT);
    }

    // Set shadow bias
    if (biasLoc >= 0) {
        SetShaderValue(targetShader, biasLoc, &shadowBias, SHADER_UNIFORM_FLOAT);
    }

    // Bind shadow map sampler
    if (shadowMapLoc >= 0) {
        rlEnableShader(targetShader.id);
        rlActiveTextureSlot(SHADOW_TEXTURE_SLOT);
        rlEnableTexture(shadowMapDepthTex);
        rlSetUniformSampler(shadowMapLoc, SHADOW_TEXTURE_SLOT);
        rlActiveTextureSlot(0);
    }
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

    // Shadow settings
    if (ImGui::CollapsingHeader("Shadows")) {
        ImGui::Checkbox("Enable Shadows", &shadowsEnabled);
        ImGui::SliderFloat("Shadow Ortho Size", &shadowOrthoSize, 50.0f, 500.0f);
        ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0001f, 0.05f, "%.4f");
        ImGui::SliderFloat("Shadow Near", &shadowNear, 0.1f, 10.0f);
        ImGui::SliderFloat("Shadow Far", &shadowFar, 100.0f, 2000.0f);
        if (shadowMapReady) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Shadow Map: %dx%d", SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Shadow Map: Not initialized");
        }
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

    if (shadowDepthShader.id > 0) {
        UnloadShader(shadowDepthShader);
        shadowDepthShader = {0};
    }
    if (shadowMaterial.shader.id > 0) {
        // Don't unload default material maps, just clear the reference
        shadowMaterial = {0};
    }
    if (shadowMapFBO > 0) {
        rlUnloadFramebuffer(shadowMapFBO);
        shadowMapFBO = 0;
    }
    if (shadowMapDepthTex > 0) {
        rlUnloadTexture(shadowMapDepthTex);
        shadowMapDepthTex = 0;
    }
    shadowMapReady = false;

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
