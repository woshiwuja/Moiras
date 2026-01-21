#include "models.h"
#include <iostream>
#include <raymath.h>
#include <vector>

namespace moiras {

void smoothMesh(Mesh *mesh, int iterations, float lambda) {
  if (!mesh || !mesh->vertices) {
    std::cerr << "ERROR: Invalid mesh" << std::endl;
    return;
  }

  std::cout << "Smoothing mesh: " << mesh->vertexCount << " vertices, "
            << mesh->triangleCount << " triangles" << std::endl;

  bool isIndexed = (mesh->indices != nullptr);
  std::cout << "Mesh type: " << (isIndexed ? "Indexed" : "Non-indexed")
            << std::endl;

  if (!isIndexed && mesh->vertexCount % 3 != 0) {
    std::cerr << "WARNING: Non-indexed mesh has vertex count not divisible by 3"
              << std::endl;
  }

  for (int iter = 0; iter < iterations; iter++) {
    std::vector<float> newVertices(mesh->vertexCount * 3, 0.0f);
    std::vector<int> neighborCount(mesh->vertexCount, 0);

    // Accumula posizioni dei vicini
    for (int i = 0; i < mesh->triangleCount; i++) {
      int idx0, idx1, idx2;

      if (isIndexed) {
        idx0 = mesh->indices[i * 3];
        idx1 = mesh->indices[i * 3 + 1];
        idx2 = mesh->indices[i * 3 + 2];
      } else {
        idx0 = i * 3;
        idx1 = i * 3 + 1;
        idx2 = i * 3 + 2;
      }

      // Bounds check
      if (idx0 >= mesh->vertexCount || idx1 >= mesh->vertexCount ||
          idx2 >= mesh->vertexCount) {
        std::cerr << "ERROR: Index out of bounds at triangle " << i << " ("
                  << idx0 << ", " << idx1 << ", " << idx2
                  << " max: " << mesh->vertexCount - 1 << ")" << std::endl;
        continue;
      }

      // Per ogni componente (x, y, z)
      for (int j = 0; j < 3; j++) {
        // Accumula per idx0
        newVertices[idx0 * 3 + j] +=
            mesh->vertices[idx1 * 3 + j] + mesh->vertices[idx2 * 3 + j];
        // Accumula per idx1
        newVertices[idx1 * 3 + j] +=
            mesh->vertices[idx0 * 3 + j] + mesh->vertices[idx2 * 3 + j];
        // Accumula per idx2
        newVertices[idx2 * 3 + j] +=
            mesh->vertices[idx0 * 3 + j] + mesh->vertices[idx1 * 3 + j];
      }

      neighborCount[idx0] += 2;
      neighborCount[idx1] += 2;
      neighborCount[idx2] += 2;
    }

    // Applica smoothing con Laplacian
    for (int i = 0; i < mesh->vertexCount; i++) {
      if (neighborCount[i] > 0) {
        for (int j = 0; j < 3; j++) {
          float avg = newVertices[i * 3 + j] / neighborCount[i];
          float current = mesh->vertices[i * 3 + j];
          mesh->vertices[i * 3 + j] = current + lambda * (avg - current);
        }
      }
    }

    if ((iter + 1) % 1 == 0 || iter == iterations - 1) {
      std::cout << "  Iteration " << (iter + 1) << "/" << iterations
                << " completed" << std::endl;
    }
  }

  std::cout << "Smoothing completed successfully" << std::endl;
}
Mesh subdivideMeshNonIndexed(const Mesh &original) {
  Mesh result = {0};

  std::vector<float> newVertices;
  std::vector<float> newTexcoords;

  // Ogni triangolo diventa 4 triangoli
  for (int i = 0; i < original.triangleCount; i++) {
    int baseIdx = i * 3;

    // Vertici originali del triangolo
    Vector3 v0 = {original.vertices[baseIdx * 3],
                  original.vertices[baseIdx * 3 + 1],
                  original.vertices[baseIdx * 3 + 2]};
    Vector3 v1 = {original.vertices[(baseIdx + 1) * 3],
                  original.vertices[(baseIdx + 1) * 3 + 1],
                  original.vertices[(baseIdx + 1) * 3 + 2]};
    Vector3 v2 = {original.vertices[(baseIdx + 2) * 3],
                  original.vertices[(baseIdx + 2) * 3 + 1],
                  original.vertices[(baseIdx + 2) * 3 + 2]};

    // Punti medi
    Vector3 m01 = {(v0.x + v1.x) * 0.5f, (v0.y + v1.y) * 0.5f,
                   (v0.z + v1.z) * 0.5f};
    Vector3 m12 = {(v1.x + v2.x) * 0.5f, (v1.y + v2.y) * 0.5f,
                   (v1.z + v2.z) * 0.5f};
    Vector3 m20 = {(v2.x + v0.x) * 0.5f, (v2.y + v0.y) * 0.5f,
                   (v2.z + v0.z) * 0.5f};

    // Texcoords se presenti
    Vector2 t0 = {0, 0}, t1 = {0, 0}, t2 = {0, 0};
    Vector2 tm01, tm12, tm20;

    if (original.texcoords) {
      t0 = {original.texcoords[baseIdx * 2],
            original.texcoords[baseIdx * 2 + 1]};
      t1 = {original.texcoords[(baseIdx + 1) * 2],
            original.texcoords[(baseIdx + 1) * 2 + 1]};
      t2 = {original.texcoords[(baseIdx + 2) * 2],
            original.texcoords[(baseIdx + 2) * 2 + 1]};

      tm01 = {(t0.x + t1.x) * 0.5f, (t0.y + t1.y) * 0.5f};
      tm12 = {(t1.x + t2.x) * 0.5f, (t1.y + t2.y) * 0.5f};
      tm20 = {(t2.x + t0.x) * 0.5f, (t2.y + t0.y) * 0.5f};
    }

    // Helper per aggiungere vertice
    auto addVertex = [&](Vector3 v, Vector2 t) {
      newVertices.push_back(v.x);
      newVertices.push_back(v.y);
      newVertices.push_back(v.z);
      if (original.texcoords) {
        newTexcoords.push_back(t.x);
        newTexcoords.push_back(t.y);
      }
    };

    // 4 nuovi triangoli
    // Triangolo 1: v0, m01, m20
    addVertex(v0, t0);
    addVertex(m01, tm01);
    addVertex(m20, tm20);

    // Triangolo 2: m01, v1, m12
    addVertex(m01, tm01);
    addVertex(v1, t1);
    addVertex(m12, tm12);

    // Triangolo 3: m20, m12, v2
    addVertex(m20, tm20);
    addVertex(m12, tm12);
    addVertex(v2, t2);

    // Triangolo 4: m01, m12, m20
    addVertex(m01, tm01);
    addVertex(m12, tm12);
    addVertex(m20, tm20);
  }

  // Alloca nuova mesh
  result.vertexCount = newVertices.size() / 3;
  result.triangleCount = result.vertexCount / 3;

  result.vertices = (float *)RL_MALLOC(newVertices.size() * sizeof(float));
  memcpy(result.vertices, newVertices.data(),
         newVertices.size() * sizeof(float));

  if (!newTexcoords.empty()) {
    result.texcoords = (float *)RL_MALLOC(newTexcoords.size() * sizeof(float));
    memcpy(result.texcoords, newTexcoords.data(),
           newTexcoords.size() * sizeof(float));
  }

  std::cout << "Subdivision: " << original.triangleCount << " -> "
            << result.triangleCount << " triangles" << std::endl;

  return result;
}
void UpdateModelAnimationBonesLerp(Model model, ModelAnimation animA,
                                   int frameA, ModelAnimation animB, int frameB,
                                   float value) {
  if ((animA.frameCount > 0) && (animA.bones != NULL) &&
      (animA.framePoses != NULL) && (animB.frameCount > 0) &&
      (animB.bones != NULL) && (animB.framePoses != NULL) && (value >= 0.0f) &&
      (value <= 1.0f)) {
    frameA = frameA % animA.frameCount;
    frameB = frameB % animB.frameCount;

    for (int i = 0; i < model.meshCount; i++) {
      if (model.meshes[i].boneMatrices) {
        for (int boneId = 0; boneId < model.meshes[i].boneCount; boneId++) {
          Vector3 inTranslation = model.bindPose[boneId].translation;
          Quaternion inRotation = model.bindPose[boneId].rotation;
          Vector3 inScale = model.bindPose[boneId].scale;

          Vector3 outATranslation =
              animA.framePoses[frameA][boneId].translation;
          Quaternion outARotation = animA.framePoses[frameA][boneId].rotation;
          Vector3 outAScale = animA.framePoses[frameA][boneId].scale;

          Vector3 outBTranslation =
              animB.framePoses[frameB][boneId].translation;
          Quaternion outBRotation = animB.framePoses[frameB][boneId].rotation;
          Vector3 outBScale = animB.framePoses[frameB][boneId].scale;

          Vector3 outTranslation =
              Vector3Lerp(outATranslation, outBTranslation, value);
          Quaternion outRotation =
              QuaternionSlerp(outARotation, outBRotation, value);
          Vector3 outScale = Vector3Lerp(outAScale, outBScale, value);

          Vector3 invTranslation = Vector3RotateByQuaternion(
              Vector3Negate(inTranslation), QuaternionInvert(inRotation));
          Quaternion invRotation = QuaternionInvert(inRotation);
          Vector3 invScale =
              Vector3Divide((Vector3){1.0f, 1.0f, 1.0f}, inScale);

          Vector3 boneTranslation = Vector3Add(
              Vector3RotateByQuaternion(
                  Vector3Multiply(outScale, invTranslation), outRotation),
              outTranslation);
          Quaternion boneRotation =
              QuaternionMultiply(outRotation, invRotation);
          Vector3 boneScale = Vector3Multiply(outScale, invScale);

          Matrix boneMatrix = MatrixMultiply(
              MatrixMultiply(QuaternionToMatrix(boneRotation),
                             MatrixTranslate(boneTranslation.x,
                                             boneTranslation.y,
                                             boneTranslation.z)),
              MatrixScale(boneScale.x, boneScale.y, boneScale.z));

          model.meshes[i].boneMatrices[boneId] = boneMatrix;
        }
      }
    }
  }
}
} // namespace moiras
