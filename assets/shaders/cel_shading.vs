#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 7) in vec4 vertexBoneIds;
layout (location = 8) in vec4 vertexBoneWeights;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

uniform mat4 mvp;
uniform mat4 matModel;

// GPU skinning: bone matrices uploaded by raylib during DrawMesh
#define MAX_BONE_NUM 128
uniform mat4 boneMatrices[MAX_BONE_NUM];

void main() {
    vec4 pos;
    vec3 norm;

    // Check if vertex has bone skinning data.
    float skinWeight = vertexBoneWeights.x + vertexBoneWeights.y + vertexBoneWeights.z;

    if (skinWeight > 0.0) {
        int boneIndex0 = int(vertexBoneIds.x);
        int boneIndex1 = int(vertexBoneIds.y);
        int boneIndex2 = int(vertexBoneIds.z);
        int boneIndex3 = int(vertexBoneIds.w);

        pos = vertexBoneWeights.x * (boneMatrices[boneIndex0] * vec4(aPos, 1.0)) +
              vertexBoneWeights.y * (boneMatrices[boneIndex1] * vec4(aPos, 1.0)) +
              vertexBoneWeights.z * (boneMatrices[boneIndex2] * vec4(aPos, 1.0)) +
              vertexBoneWeights.w * (boneMatrices[boneIndex3] * vec4(aPos, 1.0));

        norm = vertexBoneWeights.x * (mat3(boneMatrices[boneIndex0]) * aNormal) +
               vertexBoneWeights.y * (mat3(boneMatrices[boneIndex1]) * aNormal) +
               vertexBoneWeights.z * (mat3(boneMatrices[boneIndex2]) * aNormal) +
               vertexBoneWeights.w * (mat3(boneMatrices[boneIndex3]) * aNormal);
    } else {
        pos = vec4(aPos, 1.0);
        norm = aNormal;
    }

    FragPos = vec3(matModel * pos);
    TexCoord = aTexCoord;
    Normal = normalize(mat3(transpose(inverse(matModel))) * norm);
    gl_Position = mvp * pos;
}
