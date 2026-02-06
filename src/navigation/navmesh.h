#pragma once

#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"
#include "DetourTileCache.h"
#include "DetourTileCacheBuilder.h"
#include "Recast.h"
#include <raylib.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

#include <raymath.h>
namespace moiras {

// Struttura per identificare una tile
struct TileCoord {
  int x;
  int y;

  bool operator==(const TileCoord &other) const {
    return x == other.x && y == other.y;
  }
};

// Struttura per un ostacolo statico (building)
struct NavMeshObstacle {
  unsigned int id;
  BoundingBox bounds;
};

// Hash per TileCoord
struct TileCoordHash {
  std::size_t operator()(const TileCoord &tc) const {
    return std::hash<int>()(tc.x) ^ (std::hash<int>()(tc.y) << 16);
  }
};

// ============================================
// Tile Cache helper classes
// ============================================

// Linear allocator for tile cache
struct LinearAllocator : public dtTileCacheAlloc {
  unsigned char *buffer;
  size_t capacity;
  size_t top;
  size_t high;

  LinearAllocator(const size_t cap);
  ~LinearAllocator();

  void reset() override;
  void *alloc(const size_t size) override;
  void free(void *ptr) override;
};

// Simple compressor (passthrough - no actual compression)
struct TileCacheCompressor : public dtTileCacheCompressor {
  int maxCompressedSize(const int bufferSize) override;
  dtStatus compress(const unsigned char *buffer, const int bufferSize,
                    unsigned char *compressed, const int maxCompressedSize,
                    int *compressedSize) override;
  dtStatus decompress(const unsigned char *compressed, const int compressedSize,
                      unsigned char *buffer, const int maxBufferSize,
                      int *bufferSize) override;
};

// Mesh processor for tile cache
struct TileCacheMeshProcess : public dtTileCacheMeshProcess {
  void process(struct dtNavMeshCreateParams *params, unsigned char *polyAreas,
               unsigned short *polyFlags) override;
};

class NavMesh {
public:
  using ProgressCallback = std::function<void(int current, int total)>;

  NavMesh();
  ~NavMesh();
  bool build(const Mesh &mesh, Matrix transform = MatrixIdentity());
  bool buildTiled(const Mesh &mesh, Matrix transform = MatrixIdentity(),
                  ProgressCallback progressCallback = nullptr);
  bool buildTile(int tileX, int tileY);
  bool removeTile(int tileX, int tileY);
  bool rebuildTile(int tileX, int tileY);
  TileCoord getTileCoordAt(Vector3 worldPos) const;
  std::vector<Vector3> findPath(Vector3 start, Vector3 end);
  void drawDebug();
  bool saveToFile(const std::string &filename);
  bool loadFromFile(const std::string &filename);
  bool projectPointToNavMesh(Vector3 point, Vector3 &projectedPoint);
  void setParametersForMapSize(float mapSize);
  unsigned int addObstacle(BoundingBox bounds);
  bool removeObstacle(unsigned int obstacleId);
  std::vector<TileCoord> getAffectedTiles(BoundingBox bounds) const;
  float m_cellSize = 0.5f;
  float m_cellHeight = 0.3f;
  float m_agentHeight = 2.0f;
  float m_agentRadius = 0.8f;
  float m_agentMaxClimb = 1.0f;
  float m_agentMaxSlope = 40.0f;
  float m_minRegionArea = 8.0f;
  float m_mergeRegionArea = 20.0f;
  float m_maxSimplificationError = 1.3f;
  float m_tileSize = 64.0f;
  int m_maxTiles = 1024;
  int m_maxPolysPerTile = 4096;
  int getTileCount() const { return m_tileCount; }
  int getTotalPolygons() const { return m_totalPolygons; }
  void getBounds(float *bmin, float *bmax) const;

private:
  rcContext *m_ctx;
  dtNavMesh *m_navMesh;
  dtNavMeshQuery *m_navQuery;
  dtTileCache *m_tileCache;
  LinearAllocator *m_talloc;
  TileCacheCompressor *m_tcomp;
  TileCacheMeshProcess *m_tmproc;
  std::vector<float> m_storedVerts;
  std::vector<int> m_storedTris;
  int m_storedVertCount = 0;
  int m_storedTriCount = 0;
  float m_boundsMin[3] = {0, 0, 0};
  float m_boundsMax[3] = {0, 0, 0};
  rcConfig m_cfg;
  int m_tilesX = 0;
  int m_tilesZ = 0;
  int m_tileCount = 0;
  int m_totalPolygons = 0;
  struct TileDebugData {
    rcPolyMesh *polyMesh = nullptr;
    Model debugModel = {0};
    bool meshBuilt = false;
  };
  std::unordered_map<TileCoord, TileDebugData, TileCoordHash> m_tileDebugData;
  Model m_debugModel = {0};
  bool m_debugMeshBuilt = false;
  std::vector<NavMeshObstacle> m_obstacles;
  unsigned int m_nextObstacleId = 1;
  bool initNavMesh();
  bool initTileCache();
  unsigned char *buildTileData(int tileX, int tileY, int &dataSize);
  int rasterizeTileLayers(int tileX, int tileY, const rcConfig &cfg,
                          struct TileCacheData *tiles, int maxTiles);
  void buildDebugMesh();
  void buildDebugMeshFromNavMesh();
  void buildDebugMeshForTile(int tileX, int tileY, rcPolyMesh *pmesh);
  void cleanupTileDebugData();
};

struct TileCacheData {
  unsigned char *data;
  int dataSize;
};

} // namespace moiras
