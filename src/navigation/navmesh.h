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

  // Parametri configurabili (ottimizzati per mappe 1000-4000 unità)
  // Valori di default conservativi per evitare overflow
  float m_cellSize = 0.5f;           // Default moderato (sovrascritto da map.cpp)
  float m_cellHeight = 0.3f;         // Risoluzione verticale
  float m_agentHeight = 2.0f;
  float m_agentRadius = 0.8f;        // Radius realistico per character umani
  float m_agentMaxClimb = 1.0f;      // 1 metro di salto
  float m_agentMaxSlope = 40.0f;     // Pendenze camminabili (40°)

  // Parametri di filtraggio regioni (adattati per dimensione mappa)
  float m_minRegionArea = 8.0f;      // Minima area regione (default conservativo)
  float m_mergeRegionArea = 20.0f;   // Area per merge regioni
  float m_maxSimplificationError = 1.3f;  // Tolleranza semplificazione


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
