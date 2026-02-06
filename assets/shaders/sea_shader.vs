#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

uniform float time;
uniform sampler2D perlinNoiseMap;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out float height;

void main()
{
    // Multi-octave wave displacement using perlin noise
    vec2 uv1 = vertexTexCoord * 0.8 + vec2(time * 0.03, time * 0.02);
    vec2 uv2 = vertexTexCoord * 1.5 + vec2(-time * 0.02, time * 0.04);
    vec2 uv3 = vertexTexCoord * 3.0 + vec2(time * 0.05, -time * 0.01);

    float wave1 = texture(perlinNoiseMap, uv1).r;
    float wave2 = texture(perlinNoiseMap, uv2).r * 0.5;
    float wave3 = texture(perlinNoiseMap, uv3).r * 0.25;

    float displacement = (wave1 + wave2 + wave3) * 4.0;

    // Displace vertex position
    vec3 displacedPosition = vertexPosition + vec3(0.0, displacement, 0.0);

    // Approximate normals from displacement gradient
    float eps = 0.02;
    vec2 uvDx = vertexTexCoord + vec2(eps, 0.0);
    vec2 uvDz = vertexTexCoord + vec2(0.0, eps);

    float hDx = (texture(perlinNoiseMap, uvDx * 0.8 + vec2(time * 0.03, time * 0.02)).r
               + texture(perlinNoiseMap, uvDx * 1.5 + vec2(-time * 0.02, time * 0.04)).r * 0.5
               + texture(perlinNoiseMap, uvDx * 3.0 + vec2(time * 0.05, -time * 0.01)).r * 0.25) * 4.0;

    float hDz = (texture(perlinNoiseMap, uvDz * 0.8 + vec2(time * 0.03, time * 0.02)).r
               + texture(perlinNoiseMap, uvDz * 1.5 + vec2(-time * 0.02, time * 0.04)).r * 0.5
               + texture(perlinNoiseMap, uvDz * 3.0 + vec2(time * 0.05, -time * 0.01)).r * 0.25) * 4.0;

    vec3 waveNormal = normalize(vec3(displacement - hDx, eps * 200.0, displacement - hDz));

    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel * vec4(displacedPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(waveNormal, 0.0)));
    height = displacement / 7.0;

    // Calculate final vertex position
    gl_Position = mvp * vec4(displacedPosition, 1.0);
}
