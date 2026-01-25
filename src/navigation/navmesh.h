#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "Recast.h"
#include <raylib.h>
#include <vector>

#include <raymath.h>
namespace moiras {

class NavMesh {
public:
  NavMesh();
  ~NavMesh();

  bool build(const Mesh &mesh, Matrix transform = MatrixIdentity());
  std::vector<Vector3> findPath(Vector3 start, Vector3 end);
  void drawDebug();

  // Parametri configurabili
  float m_cellSize = 0.1f;
  float m_cellHeight = 0.2f;
  float m_agentHeight = 2.0f;
  float m_agentRadius = 0.3f;
  float m_agentMaxClimb = 2.9f;
  float m_agentMaxSlope = 65.0f;


private:
  rcContext *m_ctx;
  dtNavMesh *m_navMesh;
  dtNavMeshQuery *m_navQuery;

  // Per debug drawing
  rcPolyMesh *m_polyMesh = nullptr;
  rcPolyMeshDetail *m_polyMeshDetail = nullptr;

  // Debug mesh pre-generata
  Model m_debugModel = {0};
  bool m_debugMeshBuilt = false;

  void buildDebugMesh();
};

} // namespace moiras
