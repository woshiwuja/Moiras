#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Illuminazione direzionale semplice
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(fragNormal, lightDir), 0.0);
    float lighting = 0.35 + 0.65 * NdotL;

    finalColor = texelColor * colDiffuse * vec4(vec3(lighting), 1.0);
}
