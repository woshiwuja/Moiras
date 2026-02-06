#version 330 core

#define NUM_CASCADES 4

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 lightPos;
uniform vec3 viewPos;

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

    // Perspective divide
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map range - no shadow
    if (projCoords.z > 1.0) return 0.0;

    // Smooth fade at cascade edges
    vec2 edgeDist = min(projCoords.xy, 1.0 - projCoords.xy);
    float edgeFade = smoothstep(0.0, 0.05, min(edgeDist.x, edgeDist.y));
    if (edgeFade <= 0.0) return 0.0;

    // Remap UV to the correct atlas quadrant
    vec2 atlasUV = projCoords.xy * 0.5 + cascadeOffsets[cascade];

    float currentDepth = projCoords.z;

    // Slope-scaled bias
    float cosTheta = max(dot(normal, lightDir), 0.0);
    float bias = shadowBias * sqrt(1.0 - cosTheta * cosTheta) / max(cosTheta, 0.001);
    bias = clamp(bias, shadowBias * 0.5, shadowBias * 5.0);

    // Per-pixel rotation to break up banding
    float angle = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;
    float sa = sin(angle);
    float ca = cos(angle);
    mat2 rotation = mat2(ca, sa, -sa, ca);

    // 16-sample rotated Poisson disk PCF
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

void main() {
    vec4 texelColor = texture(texture0, TexCoord);
    vec3 lightDir = normalize(lightPos - FragPos);

    // Calculate diffuse intensity
    float diff = max(dot(Normal, lightDir), 0.0);

    // Apply stepping (Cel effect)
    float intensity;
    if (diff > 0.8)      intensity = 1.0;
    else if (diff > 0.4) intensity = 0.6;
    else                 intensity = 0.2;

    // Apply CSM shadow
    float shadow = 0.0;
    if (shadowsEnabled == 1) {
        shadow = CSMShadowCalculation(FragPos, Normal, lightDir);
        // Reduce intensity in shadow, but keep minimum ambient
        intensity *= (1.0 - shadow * 0.7);
    }

    finalColor = vec4(texelColor.rgb * intensity, texelColor.a);
}
