#version 330

// Input fragment attributes
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in float height;

// Uniforms
uniform float time;
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec4 waterDeepColor;
uniform vec4 waterShallowColor;
uniform float foamThreshold;

// Output fragment color
out vec4 finalColor;

// Hash noise
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float smoothNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float n00 = noise(i);
    float n10 = noise(i + vec2(1.0, 0.0));
    float n01 = noise(i + vec2(0.0, 1.0));
    float n11 = noise(i + vec2(1.0, 1.0));

    return mix(mix(n00, n10, f.x), mix(n01, n11, f.x), f.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++) {
        value += amplitude * smoothNoise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 light = normalize(lightDir);

    // Fresnel - edges look darker/more opaque
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 3.0);
    fresnel = clamp(fresnel, 0.0, 1.0);

    // Base water color: blend between deep and shallow based on height
    float heightFactor = clamp(height, 0.0, 1.0);
    vec4 baseColor = mix(waterDeepColor, waterShallowColor, heightFactor);

    // Darken edges with fresnel
    vec4 waterColor = mix(baseColor, waterDeepColor * 0.7, fresnel * 0.6);

    // Diffuse lighting
    float diffuse = max(dot(normal, light), 0.0) * 0.4 + 0.6;
    waterColor.rgb *= diffuse;

    // Specular highlight (sun reflection)
    vec3 halfDir = normalize(light + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 128.0);
    vec3 specular = vec3(1.0, 0.95, 0.85) * spec * 1.2;
    waterColor.rgb += specular;

    // Foam on wave peaks
    float foamNoise = fbm(fragTexCoord * 20.0 + vec2(time * 0.1, time * 0.15));
    float foamMask = smoothstep(foamThreshold - 0.05, foamThreshold + 0.1, heightFactor);
    foamMask *= smoothstep(0.3, 0.6, foamNoise);
    vec3 foamColor = vec3(0.9, 0.95, 1.0);
    waterColor.rgb = mix(waterColor.rgb, foamColor, foamMask * 0.7);

    // Alpha: deeper areas more opaque, shallow more transparent
    waterColor.a = mix(0.75, 0.92, fresnel);

    finalColor = waterColor;
}
