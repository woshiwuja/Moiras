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

  // Proietta un punto sulla navmesh (trova il punto più vicino sulla navmesh)
  bool projectPointToNavMesh(Vector3 point, Vector3& projectedPoint);

  // Configura parametri per mappe di diverse dimensioni
  void setParametersForMapSize(float mapSize);

  // Parametri configurabili per mappe grandi (4000x4000)
  // Aumentati per performance e gestione migliore delle pendenze
  float m_cellSize = 0.3f;           // Era 0.1f - riduce celle di 9x per performance
  float m_cellHeight = 0.3f;         // Era 0.2f - migliore risoluzione verticale
  float m_agentHeight = 2.0f;
  float m_agentRadius = 0.6f;        // Era 0.3f - più realistico per character
  float m_agentMaxClimb = 0.9f;      // Era 2.9f - più realistico (90cm)
  float m_agentMaxSlope = 45.0f;     // Era 65.0f - pendenze camminabili realistiche


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
