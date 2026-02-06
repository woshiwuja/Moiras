#include "lightmanager.h"
#include "imgui.h"
#include <cmath>
#include <cstdio>
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

    // Register PBR shader for shadow uniforms (cached locations)
    registerShadowShader(shader);

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

    // Create shadow atlas FBO (SHADOW_ATLAS_SIZE x SHADOW_ATLAS_SIZE)
    shadowMapFBO = rlLoadFramebuffer();
    if (shadowMapFBO == 0) {
        TraceLog(LOG_ERROR, "LightManager: Failed to create shadow FBO!");
        return;
    }

    // Create depth texture atlas
    rlEnableFramebuffer(shadowMapFBO);
    shadowMapDepthTex = rlLoadTextureDepth(SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE, false);
    rlFramebufferAttach(shadowMapFBO, shadowMapDepthTex, RL_ATTACHMENT_DEPTH,
                        RL_ATTACHMENT_TEXTURE2D, 0);

    if (!rlFramebufferComplete(shadowMapFBO)) {
        TraceLog(LOG_ERROR, "LightManager: Shadow FBO is not complete!");
        rlUnloadFramebuffer(shadowMapFBO);
        shadowMapFBO = 0;
        return;
    }
    rlDisableFramebuffer();

    // LINEAR filtering enables hardware interpolation for smoother PCF
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_LINEAR);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_LINEAR);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(shadowMapDepthTex, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);

    shadowMapReady = true;

    // Bind shadow map sampler to all already-registered shadow shaders
    for (int i = 0; i < shadowShaderCount; i++) {
        if (shadowShaders[i].shader.id > 0 && shadowShaders[i].shadowMapLoc >= 0) {
            rlEnableShader(shadowShaders[i].shader.id);
            rlActiveTextureSlot(SHADOW_TEXTURE_SLOT);
            rlEnableTexture(shadowMapDepthTex);
            rlSetUniformSampler(shadowShaders[i].shadowMapLoc, SHADOW_TEXTURE_SLOT);
            rlActiveTextureSlot(0);
        }
    }

    // Push initial shadow state
    updateShadowUniforms();

    TraceLog(LOG_INFO, "LightManager: Shadow atlas initialized (%dx%d, %d cascades, FBO: %u, Depth: %u)",
             SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE, NUM_CASCADES, shadowMapFBO, shadowMapDepthTex);
}

void LightManager::updateCascadeMatrices(const Camera3D &camera, float cameraNear, float screenAspect) {
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

    Vector3 lightDir = Vector3Normalize(Vector3Subtract(dirLight->target, dirLight->position));

    // Practical split scheme: blend between logarithmic and uniform splits
    float splits[NUM_CASCADES + 1];
    splits[0] = cameraNear;
    for (int i = 1; i <= NUM_CASCADES; i++) {
        float p = (float)i / (float)NUM_CASCADES;
        float uniform_split = cameraNear + (shadowFar - cameraNear) * p;
        float log_split = cameraNear * powf(shadowFar / cameraNear, p);
        splits[i] = cascadeLambda * log_split + (1.0f - cascadeLambda) * uniform_split;
    }

    // Store cascade far distances for fragment shader
    for (int i = 0; i < NUM_CASCADES; i++) {
        cascadeSplits[i] = splits[i + 1];
    }

    // Camera parameters
    float fovY = camera.fovy * DEG2RAD;
    float tanHalfFovY = tanf(fovY * 0.5f);
    float tanHalfFovX = tanHalfFovY * screenAspect;

    Vector3 camPos = camera.position;
    Vector3 camFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camFwd, camera.up));
    Vector3 camUp = Vector3CrossProduct(camRight, camFwd);

    // Handle light direction parallel to up vector
    Vector3 up = {0.0f, 1.0f, 0.0f};
    if (fabsf(Vector3DotProduct(lightDir, up)) > 0.99f) {
        up = {0.0f, 0.0f, 1.0f};
    }

    for (int c = 0; c < NUM_CASCADES; c++) {
        float nearDist = splits[c];
        float farDist = splits[c + 1];

        // Compute 8 frustum corners for this cascade slice
        float xn = nearDist * tanHalfFovX;
        float yn = nearDist * tanHalfFovY;
        float xf = farDist * tanHalfFovX;
        float yf = farDist * tanHalfFovY;

        Vector3 nc = Vector3Add(camPos, Vector3Scale(camFwd, nearDist));
        Vector3 fc = Vector3Add(camPos, Vector3Scale(camFwd, farDist));

        Vector3 corners[8];
        // Near plane corners
        corners[0] = Vector3Add(Vector3Add(nc, Vector3Scale(camRight, -xn)), Vector3Scale(camUp,  yn));
        corners[1] = Vector3Add(Vector3Add(nc, Vector3Scale(camRight,  xn)), Vector3Scale(camUp,  yn));
        corners[2] = Vector3Add(Vector3Add(nc, Vector3Scale(camRight,  xn)), Vector3Scale(camUp, -yn));
        corners[3] = Vector3Add(Vector3Add(nc, Vector3Scale(camRight, -xn)), Vector3Scale(camUp, -yn));
        // Far plane corners
        corners[4] = Vector3Add(Vector3Add(fc, Vector3Scale(camRight, -xf)), Vector3Scale(camUp,  yf));
        corners[5] = Vector3Add(Vector3Add(fc, Vector3Scale(camRight,  xf)), Vector3Scale(camUp,  yf));
        corners[6] = Vector3Add(Vector3Add(fc, Vector3Scale(camRight,  xf)), Vector3Scale(camUp, -yf));
        corners[7] = Vector3Add(Vector3Add(fc, Vector3Scale(camRight, -xf)), Vector3Scale(camUp, -yf));

        // Find bounding sphere: centroid + max radius (rotationally stable)
        Vector3 center = {0, 0, 0};
        for (int i = 0; i < 8; i++) {
            center = Vector3Add(center, corners[i]);
        }
        center = Vector3Scale(center, 1.0f / 8.0f);

        float radius = 0.0f;
        for (int i = 0; i < 8; i++) {
            float dist = Vector3Length(Vector3Subtract(corners[i], center));
            if (dist > radius) radius = dist;
        }
        // Round up radius to reduce jitter
        radius = ceilf(radius * 16.0f) / 16.0f;

        // Build light view matrix: position the light just far enough behind
        // the cascade center to capture objects within the bounding sphere.
        // Using a tight offset (2x radius) instead of shadowFar gives much
        // better depth resolution, which is critical for small shadow casters
        // like characters (scale 0.05 = ~5 world units tall).
        float lightOffset = radius * 2.0f;
        Vector3 lightPos = Vector3Subtract(center, Vector3Scale(lightDir, lightOffset));
        Matrix lightView = MatrixLookAt(lightPos, center, up);

        // Tight near/far: objects in the sphere span [offset-radius, offset+radius]
        // from the light. Add margin for objects casting shadows from outside the sphere.
        float projNear = 0.1f;
        float projFar = lightOffset + radius + 100.0f;
        Matrix lightProj = MatrixOrtho(-radius, radius, -radius, radius,
                                        projNear, projFar);

        // Texel snapping: quantize the light-space translation to whole texel increments
        // to prevent shadow swimming when the camera moves
        float texelSize = (2.0f * radius) / (float)CASCADE_SIZE;
        float cx = lightView.m0 * center.x + lightView.m4 * center.y +
                   lightView.m8 * center.z + lightView.m12;
        float cy = lightView.m1 * center.x + lightView.m5 * center.y +
                   lightView.m9 * center.z + lightView.m13;
        float dx = fmodf(cx, texelSize);
        float dy = fmodf(cy, texelSize);
        lightView.m12 -= dx;
        lightView.m13 -= dy;

        cascadeMatrices[c] = MatrixMultiply(lightView, lightProj);
    }
}

void LightManager::beginShadowPass() {
    if (!shadowMapReady || !shadowsEnabled) return;

    // Save current matrices
    savedProjection = rlGetMatrixProjection();
    savedModelview = rlGetMatrixModelview();

    // Enable shadow atlas FBO and clear entire atlas once
    rlEnableFramebuffer(shadowMapFBO);
    rlViewport(0, 0, SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE);
    rlClearColor(255, 255, 255, 255);
    rlClearScreenBuffers();

    rlEnableDepthTest();
    rlEnableDepthMask();
    rlDisableColorBlend();

    // Disable face culling so skinned meshes with varied winding still write depth
    rlDisableBackfaceCulling();
}

void LightManager::setCascade(int cascade) {
    if (cascade < 0 || cascade >= NUM_CASCADES) return;

    // Set viewport to correct quadrant in the 2x2 atlas
    int x = (cascade % 2) * CASCADE_SIZE;
    int y = (cascade / 2) * CASCADE_SIZE;
    rlViewport(x, y, CASCADE_SIZE, CASCADE_SIZE);

    // Set the light-space matrix for this cascade
    rlSetMatrixModelview(cascadeMatrices[cascade]);
    rlSetMatrixProjection(MatrixIdentity());
}

void LightManager::endShadowPass() {
    if (!shadowMapReady || !shadowsEnabled) return;

    // Restore culling and blending state
    rlEnableBackfaceCulling();
    rlEnableColorBlend();

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

void LightManager::registerShadowShader(Shader targetShader) {
    if (targetShader.id == 0 || shadowShaderCount >= MAX_SHADOW_SHADERS) return;

    // Check if already registered
    for (int i = 0; i < shadowShaderCount; i++) {
        if (shadowShaders[i].shader.id == targetShader.id) return;
    }

    ShadowShaderLocs &locs = shadowShaders[shadowShaderCount];
    locs.shader = targetShader;
    locs.shadowEnabledLoc = GetShaderLocation(targetShader, "shadowsEnabled");

    // CSM: look up individual cascade matrix locations
    for (int c = 0; c < NUM_CASCADES; c++) {
        char name[64];
        snprintf(name, sizeof(name), "cascadeMatrices[%d]", c);
        locs.cascadeMatricesLoc[c] = GetShaderLocation(targetShader, name);
    }
    locs.cascadeSplitsLoc = GetShaderLocation(targetShader, "cascadeSplits");

    locs.shadowMapLoc = GetShaderLocation(targetShader, "shadowMap");
    locs.shadowBiasLoc = GetShaderLocation(targetShader, "shadowBias");
    locs.shadowNormalOffsetLoc = GetShaderLocation(targetShader, "shadowNormalOffset");
    shadowShaderCount++;

    // Bind shadow map sampler once (texture unit doesn't change)
    if (shadowMapReady && locs.shadowMapLoc >= 0) {
        rlEnableShader(targetShader.id);
        rlActiveTextureSlot(SHADOW_TEXTURE_SLOT);
        rlEnableTexture(shadowMapDepthTex);
        rlSetUniformSampler(locs.shadowMapLoc, SHADOW_TEXTURE_SLOT);
        rlActiveTextureSlot(0);
    }

    TraceLog(LOG_INFO, "LightManager: Registered shadow shader ID %d (slot %d)",
             targetShader.id, shadowShaderCount - 1);
}

void LightManager::updateShadowUniforms() {
    if (!shadowMapReady) return;

    int enabled = shadowsEnabled ? 1 : 0;

    for (int i = 0; i < shadowShaderCount; i++) {
        ShadowShaderLocs &locs = shadowShaders[i];
        if (locs.shader.id == 0) continue;

        // Push cascade matrices
        for (int c = 0; c < NUM_CASCADES; c++) {
            if (locs.cascadeMatricesLoc[c] >= 0)
                SetShaderValueMatrix(locs.shader, locs.cascadeMatricesLoc[c], cascadeMatrices[c]);
        }

        // Push cascade split distances as vec4
        if (locs.cascadeSplitsLoc >= 0)
            SetShaderValue(locs.shader, locs.cascadeSplitsLoc, cascadeSplits, SHADER_UNIFORM_VEC4);

        if (locs.shadowEnabledLoc >= 0)
            SetShaderValue(locs.shader, locs.shadowEnabledLoc, &enabled, SHADER_UNIFORM_INT);
        if (locs.shadowBiasLoc >= 0)
            SetShaderValue(locs.shader, locs.shadowBiasLoc, &shadowBias, SHADER_UNIFORM_FLOAT);
        if (locs.shadowNormalOffsetLoc >= 0)
            SetShaderValue(locs.shader, locs.shadowNormalOffsetLoc, &shadowNormalOffset, SHADER_UNIFORM_FLOAT);
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

    // Shadow settings (CSM)
    if (ImGui::CollapsingHeader("Shadows")) {
        ImGui::Checkbox("Enable Shadows", &shadowsEnabled);
        ImGui::SliderInt("Update Interval", &shadowUpdateInterval, 1, 6);
        ImGui::SliderFloat("Cascade Lambda", &cascadeLambda, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0001f, 0.05f, "%.4f");
        ImGui::SliderFloat("Normal Offset", &shadowNormalOffset, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Shadow Far", &shadowFar, 100.0f, 2000.0f);
        if (shadowMapReady) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Shadow Atlas: %dx%d (%d cascades)",
                              SHADOW_ATLAS_SIZE, SHADOW_ATLAS_SIZE, NUM_CASCADES);
            ImGui::Text("Cascade splits: %.1f | %.1f | %.1f | %.1f",
                        cascadeSplits[0], cascadeSplits[1], cascadeSplits[2], cascadeSplits[3]);
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
