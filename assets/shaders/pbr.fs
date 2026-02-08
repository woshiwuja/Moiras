#version 330

#define MAX_LIGHTS              4
#define LIGHT_DIRECTIONAL       0
#define LIGHT_POINT              1
#define PI 3.14159265358979323846
#define NUM_CASCADES 4

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
    float intensity;
};

// Input dal vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
in mat3 TBN;

out vec4 finalColor;

// Raylib default uniforms
uniform sampler2D texture0;     // Albedo
uniform sampler2D texture1;     // Normal
uniform sampler2D texture2;     // MRA (Metallic, Roughness, AO)
uniform vec4 colDiffuse;

// Custom uniforms
uniform int numOfLights;
uniform Light lights[MAX_LIGHTS];
uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform float ambient;

uniform int useTexAlbedo;
uniform int useTexNormal;
uniform int useTexMRA;

uniform float metallicValue;
uniform float roughnessValue;
uniform float aoValue;

// Shadow mapping (CSM)
uniform sampler2D shadowMap;
uniform int shadowsEnabled;
uniform float shadowBias;
uniform float shadowNormalOffset;
uniform mat4 cascadeMatrices[NUM_CASCADES];
uniform vec4 cascadeSplits;

// Atlas quadrant offsets for 2x2 cascade layout
const vec2 cascadeOffsets[NUM_CASCADES] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.5, 0.0),
    vec2(0.0, 0.5),
    vec2(0.5, 0.5)
);

// 16-sample Poisson disk for soft shadow PCF
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

// --- PBR Functions ---
vec3 SchlickFresnel(float hDotV, vec3 refl) {
    return refl + (1.0 - refl) * pow(1.0 - hDotV, 5.0);
}

float GgxDistribution(float nDotH, float roughness) {
    float a = roughness * roughness * roughness * roughness;
    float d = nDotH * nDotH * (a - 1.0) + 1.0;
    return a / max(PI * d * d, 0.0000001);
}

float GeomSmith(float nDotV, float nDotL, float roughness) {
    float r = roughness + 1.0;
    float k = r * r / 8.0;
    float ik = 1.0 - k;
    return (nDotV / (nDotV * ik + k)) * (nDotL / (nDotL * ik + k));
}

float CSMShadowCalculation(vec3 fragPos, vec3 normal, vec3 lightDir) {
    // Select cascade based on distance from camera
    float dist = length(viewPos - fragPos);

    int cascade = NUM_CASCADES - 1;
    if (dist < cascadeSplits.x) cascade = 0;
    else if (dist < cascadeSplits.y) cascade = 1;
    else if (dist < cascadeSplits.z) cascade = 2;

    // Apply normal offset and transform to light space
    vec3 shadowPos = fragPos + normal * shadowNormalOffset;
    vec4 fragPosLS = cascadeMatrices[cascade] * vec4(shadowPos, 1.0);

    // Perspective divide (ortho projection, but still need w-divide for correctness)
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map range - no shadow
    if (projCoords.z > 1.0) return 0.0;

    // Smooth fade at cascade edges to avoid hard cutoff
    vec2 edgeDist = min(projCoords.xy, 1.0 - projCoords.xy);
    float edgeFade = smoothstep(0.0, 0.05, min(edgeDist.x, edgeDist.y));
    if (edgeFade <= 0.0) return 0.0;

    // Remap UV to the correct atlas quadrant
    vec2 atlasUV = projCoords.xy * 0.5 + cascadeOffsets[cascade];

    float currentDepth = projCoords.z;

    // Slope-scaled bias: steeper angles get more bias to prevent acne
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float bias = shadowBias * sqrt(1.0 - cosTheta * cosTheta) / max(cosTheta, 0.001);
    bias = clamp(bias, shadowBias * 0.5, shadowBias * 5.0);

    // Per-pixel rotation angle to break up banding
    float angle = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;
    float sa = sin(angle);
    float ca = cos(angle);
    mat2 rotation = mat2(ca, sa, -sa, ca);

    // 16-sample rotated Poisson disk PCF (texel size relative to full atlas)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float spread = 2.0;

    for (int i = 0; i < 16; i++) {
        vec2 off = rotation * poissonDisk[i] * texelSize * spread;
        float sampleDepth = texture(shadowMap, atlasUV + off).r;
        shadow += currentDepth - bias > sampleDepth ? 1.0 : 0.0;
    }
    shadow /= 16.0;

    // Apply edge fade
    shadow *= edgeFade;

    // Fade out shadow at the far edge of the last cascade
    if (cascade == NUM_CASCADES - 1) {
        float fadeStart = cascadeSplits.w * 0.9;
        float fadeFactor = 1.0 - smoothstep(fadeStart, cascadeSplits.w, dist);
        shadow *= fadeFactor;
    }

    return shadow;
}

void main()
{
    // 1. Albedo
    vec3 albedo = colDiffuse.rgb;
    if (useTexAlbedo == 1) {
        albedo *= texture(texture0, fragTexCoord).rgb;
    }

    // 2. Metallic, Roughness, AO
    float metallic = metallicValue;
    float roughness = roughnessValue;
    float ao = aoValue;

    if (useTexMRA == 1) {
        vec4 mra = texture(texture2, fragTexCoord);
        metallic = clamp(mra.r + metallicValue, 0.0, 1.0);
        roughness = clamp(mra.g + roughnessValue, 0.04, 1.0);
        ao *= mra.b;
    }

    // 3. Normal Mapping
    vec3 N = normalize(fragNormal);
    if (useTexNormal == 1) {
        vec3 normalTex = texture(texture1, fragTexCoord).rgb;
        normalTex = normalize(normalTex * 2.0 - 1.0);
        N = normalize(TBN * normalTex);
    }

    vec3 V = normalize(viewPos - fragPosition);
    vec3 baseRefl = mix(vec3(0.04), albedo, metallic);
    vec3 lightAccum = vec3(0.0);

    // 4. Lighting Loop
    for (int i = 0; i < numOfLights; i++) {
        if (lights[i].enabled == 0) continue;

        vec3 L;
        float attenuation;

        if (lights[i].type == LIGHT_DIRECTIONAL) {
            L = normalize(lights[i].position - lights[i].target);
            attenuation = lights[i].intensity;
        } else {
            L = normalize(lights[i].position - fragPosition);
            float dist = length(lights[i].position - fragPosition);
            attenuation = lights[i].intensity / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        }

        vec3 H = normalize(V + L);
        vec3 radiance = lights[i].color.rgb * attenuation;

        float nDotV = max(dot(N, V), 0.0000001);
        float nDotL = max(dot(N, L), 0.0000001);
        float hDotV = max(dot(H, V), 0.0);
        float nDotH = max(dot(N, H), 0.0);

        float D = GgxDistribution(nDotH, roughness);
        float G = GeomSmith(nDotV, nDotL, roughness);
        vec3 F = SchlickFresnel(hDotV, baseRefl);

        vec3 spec = (D * G * F) / (4.0 * nDotV * nDotL);
        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        // Apply CSM shadow only to directional lights
        float shadow = 0.0;
        if (shadowsEnabled == 1 && lights[i].type == LIGHT_DIRECTIONAL) {
            shadow = CSMShadowCalculation(fragPosition, N, L);
        }

        lightAccum += (kD * albedo / PI + spec) * radiance * nDotL * (1.0 - shadow);
    }

    // 5. Final Composition
    vec3 ambientFinal = ambientColor * ambient * albedo * ao;
    vec3 color = ambientFinal + lightAccum;

    // HDR Tonemapping & Gamma Correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    finalColor = vec4(color, colDiffuse.a);
}
