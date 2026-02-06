#version 330 core
in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;
in vec4 FragPosLightSpace;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 lightPos;
uniform vec3 viewPos;

// Shadow mapping
uniform sampler2D shadowMap;
uniform int shadowsEnabled;
uniform float shadowBias;

float ShadowCalculation(vec4 fragPosLS, vec3 normal, vec3 lightDir) {
    // Perspective divide
    vec3 projCoords = fragPosLS.xyz / fragPosLS.w;
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map range - no shadow
    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    // Adaptive bias
    float bias = max(shadowBias * 5.0 * (1.0 - dot(normal, lightDir)), shadowBias);

    // 4-sample PCF for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    shadow += currentDepth - bias > texture(shadowMap, projCoords.xy + vec2(-0.5, -0.5) * texelSize).r ? 1.0 : 0.0;
    shadow += currentDepth - bias > texture(shadowMap, projCoords.xy + vec2( 0.5, -0.5) * texelSize).r ? 1.0 : 0.0;
    shadow += currentDepth - bias > texture(shadowMap, projCoords.xy + vec2(-0.5,  0.5) * texelSize).r ? 1.0 : 0.0;
    shadow += currentDepth - bias > texture(shadowMap, projCoords.xy + vec2( 0.5,  0.5) * texelSize).r ? 1.0 : 0.0;
    shadow *= 0.25;

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

    // Apply shadow
    float shadow = 0.0;
    if (shadowsEnabled == 1) {
        shadow = ShadowCalculation(FragPosLightSpace, Normal, lightDir);
        // Reduce intensity in shadow, but keep minimum ambient
        intensity *= (1.0 - shadow * 0.7);
    }

    finalColor = vec4(texelColor.rgb * intensity, texelColor.a);
}
