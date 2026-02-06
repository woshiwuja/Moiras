#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec4 vertexBoneIds;
in vec4 vertexBoneWeights;

// Uniforms
uniform mat4 mvp;

// GPU skinning
#define MAX_BONE_NUM 128
uniform mat4 boneMatrices[MAX_BONE_NUM];

void main()
{
    vec4 pos;

    // Check if vertex has bone skinning data
    float skinWeight = vertexBoneWeights.x + vertexBoneWeights.y + vertexBoneWeights.z;

    if (skinWeight > 0.0) {
        int b0 = int(vertexBoneIds.x);
        int b1 = int(vertexBoneIds.y);
        int b2 = int(vertexBoneIds.z);
        int b3 = int(vertexBoneIds.w);

        pos = vertexBoneWeights.x * (boneMatrices[b0] * vec4(vertexPosition, 1.0)) +
              vertexBoneWeights.y * (boneMatrices[b1] * vec4(vertexPosition, 1.0)) +
              vertexBoneWeights.z * (boneMatrices[b2] * vec4(vertexPosition, 1.0)) +
              vertexBoneWeights.w * (boneMatrices[b3] * vec4(vertexPosition, 1.0));
    } else {
        pos = vec4(vertexPosition, 1.0);
    }

    gl_Position = mvp * pos;
}
