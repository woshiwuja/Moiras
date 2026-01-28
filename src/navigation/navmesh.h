#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "Recast.h"
#include <raylib.h>
#include <vector>
#include <unordered_map>
#include <string>

#include <raymath.h>
namespace moiras {

// Struttura per identificare una tile
struct TileCoord {
  int x;
  int y;

  bool operator==(const TileCoord& other) const {
    return x == other.x && y == other.y;
  }
};

// Hash per TileCoord
struct TileCoordHash {
  std::size_t operator()(const TileCoord& tc) const {
    return std::hash<int>()(tc.x) ^ (std::hash<int>()(tc.y) << 16);
  }
};

// Obstacle data for navmesh
struct NavMeshObstacle {
  BoundingBox bounds;
  int id;
};

class NavMesh {
public:
  NavMesh();
  ~NavMesh();

  // Build monolitico (legacy - ora chiama buildTiled internamente)
  bool build(const Mesh &mesh, Matrix transform = MatrixIdentity());

  // Build tiled - divide la mesh in tile e le costruisce
  bool buildTiled(const Mesh &mesh, Matrix transform = MatrixIdentity());

  // Costruisce una singola tile
  bool buildTile(int tileX, int tileY);

  // Rimuove una tile
  bool removeTile(int tileX, int tileY);

  // Ricostruisce una tile (utile per aggiornamenti dinamici)
  bool rebuildTile(int tileX, int tileY);

  // Ottiene le coordinate tile da una posizione world
  TileCoord getTileCoordAt(Vector3 worldPos) const;

  // Pathfinding
  std::vector<Vector3> findPath(Vector3 start, Vector3 end);

  // Debug
  void drawDebug();

  // Salvataggio/caricamento binario
  bool saveToFile(const std::string& filename);
  bool loadFromFile(const std::string& filename);

  // Proietta un punto sulla navmesh
  bool projectPointToNavMesh(Vector3 point, Vector3& projectedPoint);

  // Configura parametri per mappe di diverse dimensioni
  void setParametersForMapSize(float mapSize);

  // ============================================
  // Obstacle management
  // ============================================

  // Add an obstacle and rebuild affected tiles
  int addObstacle(BoundingBox bounds);

  // Remove an obstacle and rebuild affected tiles
  bool removeObstacle(int obstacleId);

  // Get tiles affected by a bounding box
  std::vector<TileCoord> getAffectedTiles(BoundingBox bounds) const;

  // Rebuild tiles affected by a bounding box
  void rebuildAffectedTiles(BoundingBox bounds);

  // ============================================
  // Parametri navmesh
  // ============================================

  // Parametri celle
  float m_cellSize = 0.5f;
  float m_cellHeight = 0.3f;

  // Parametri agente
  float m_agentHeight = 2.0f;
  float m_agentRadius = 0.8f;
  float m_agentMaxClimb = 1.0f;
  float m_agentMaxSlope = 40.0f;

  // Parametri filtraggio regioni
  float m_minRegionArea = 8.0f;
  float m_mergeRegionArea = 20.0f;
  float m_maxSimplificationError = 1.3f;

  // ============================================
  // Parametri tiling
  // ============================================

  // Dimensione di ogni tile in world units
  float m_tileSize = 64.0f;

  // Numero massimo di tile (per allocazione navmesh)
  int m_maxTiles = 1024;

  // Numero massimo di poligoni per tile
  int m_maxPolysPerTile = 4096;

  // Statistiche
  int getTileCount() const { return m_tileCount; }
  int getTotalPolygons() const { return m_totalPolygons; }

  // Accesso al bounding box
  void getBounds(float* bmin, float* bmax) const;

private:
  rcContext *m_ctx;
  dtNavMesh *m_navMesh;
  dtNavMeshQuery *m_navQuery;

  // Dati della mesh originale (per rebuild delle tile)
  std::vector<float> m_storedVerts;
  std::vector<int> m_storedTris;
  int m_storedVertCount = 0;
  int m_storedTriCount = 0;

  // Bounding box totale
  float m_boundsMin[3] = {0, 0, 0};
  float m_boundsMax[3] = {0, 0, 0};

  // Configurazione Recast salvata
  rcConfig m_cfg;

  // Numero tile in ogni direzione
  int m_tilesX = 0;
  int m_tilesZ = 0;

  // Statistiche tile
  int m_tileCount = 0;
  int m_totalPolygons = 0;

  // Per debug drawing - una mesh per tile
  struct TileDebugData {
    rcPolyMesh* polyMesh = nullptr;
    Model debugModel = {0};
    bool meshBuilt = false;
  };

  std::unordered_map<TileCoord, TileDebugData, TileCoordHash> m_tileDebugData;

  // Debug mesh globale (combinata)
  Model m_debugModel = {0};
  bool m_debugMeshBuilt = false;

  // Metodi privati
  bool initNavMesh();
  unsigned char* buildTileData(int tileX, int tileY, int& dataSize);
  void buildDebugMesh();
  void buildDebugMeshFromNavMesh();  // Costruisce debug mesh da dtNavMesh
  void buildDebugMeshForTile(int tileX, int tileY, rcPolyMesh* pmesh);
  void cleanupTileDebugData();

  // Mark obstacle areas in compact heightfield
  void markObstacleAreas(rcCompactHeightfield* chf, float tileBmin[3], float tileBmax[3]);

  // Obstacle storage
  std::vector<NavMeshObstacle> m_obstacles;
  int m_nextObstacleId = 1;
};

} // namespace moiras
