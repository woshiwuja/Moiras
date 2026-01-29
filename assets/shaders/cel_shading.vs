#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

uniform mat4 mvp;
uniform mat4 matModel;

void main() {
    FragPos = vec3(matModel * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    Normal = normalize(mat3(transpose(inverse(matModel))) * aNormal);
    gl_Position = mvp * vec4(aPos, 1.0);
}