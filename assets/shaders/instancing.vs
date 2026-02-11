#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Instance transform matrix (per-instance attribute)
in mat4 instanceTransform;

// Input uniform values
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(instanceTransform * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(mat3(instanceTransform) * vertexNormal);

    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
}
