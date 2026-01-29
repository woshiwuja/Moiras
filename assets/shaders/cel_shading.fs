#version 330 core
in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 lightPos; // We'll map Light1 here
uniform vec3 viewPos;

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

    finalColor = vec4(texelColor.rgb * intensity, texelColor.a);
}