#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform float ambient;

#define MAX_LIGHTS 4

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
    float intensity;
};

uniform Light lights[MAX_LIGHTS];

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    
    // Ambient più forte
    vec3 result = vec3(0.3);  // Luce base fissa
    
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 lightDir;
            float attenuation = 1.0;
            
            if (lights[i].type == 0) // Directional
            {
                lightDir = normalize(lights[i].position - lights[i].target);
                attenuation = lights[i].intensity;
            }
            else // Point or Spot
            {
                lightDir = normalize(lights[i].position - fragPosition);
                float distance = length(lights[i].position - fragPosition);
                // Attenuation meno aggressiva
                attenuation = lights[i].intensity / (1.0 + 0.01 * distance + 0.001 * distance * distance);
            }
            
            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * lights[i].color.rgb;
            
            result += diffuse * attenuation;
        }
    }
    
    finalColor = vec4(result, 1.0) * texelColor * colDiffuse;
}
