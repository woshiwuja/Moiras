#version 330
// Input fragment attributes
in vec2 fragTexCoord;
in float height;

// Uniforms
uniform sampler2D tex0;      // The texture behind the water
uniform float time;          // For animating the distortion
uniform float distortAmount; // Control distortion intensity

// Output fragment color
out vec4 finalColor;

// Simplex-like noise function
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Smoother noise with interpolation
float smoothNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f); // smoothstep
    
    float n00 = noise(i);
    float n10 = noise(i + vec2(1.0, 0.0));
    float n01 = noise(i + vec2(0.0, 1.0));
    float n11 = noise(i + vec2(1.0, 1.0));
    
    float nx0 = mix(n00, n10, f.x);
    float nx1 = mix(n01, n11, f.x);
    return mix(nx0, nx1, f.y);
}

void main()
{
    vec4 darkblue = vec4(0.0, 0.13, 0.2, 0.8);
    vec4 lightblue = vec4(1.0, 1.0, 1.0, 0.8);
    
    // Create animated wave pattern
    vec2 wave1 = vec2(
        smoothNoise(fragTexCoord * 3.0 + vec2(time * 0.3, 0.0)),
        smoothNoise(fragTexCoord * 3.0 + vec2(0.0, time * 0.3))
    );
    
    vec2 wave2 = vec2(
        smoothNoise(fragTexCoord * 5.0 + vec2(time * 0.5, time * 0.2)),
        smoothNoise(fragTexCoord * 5.0 + vec2(time * 0.2, time * 0.5))
    );
    
    // Combine wave patterns
    vec2 distortion = (wave1 + wave2 - 1.0) * distortAmount;
    
    // Apply distortion to texture coordinates
    vec2 distortedCoord = fragTexCoord + distortion;
    
    // Sample the texture with distorted coordinates
    vec4 distortedColor = texture(tex0, distortedCoord);
    
    // Blend water color with distorted background based on height
    vec4 waterColor = mix(darkblue, lightblue, height);
    finalColor = mix(distortedColor, waterColor, 0.4);
}
