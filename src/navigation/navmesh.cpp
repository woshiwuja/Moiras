#include "navmesh.h"
#include <rlgl.h>
#include "DetourNavMeshBuilder.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <fstream>

namespace moiras {

// ============================================
// Linear Allocator implementation
// ============================================

LinearAllocator::LinearAllocator(const size_t cap) : buffer(nullptr), capacity(0), top(0), high(0) {
    buffer = (unsigned char*)dtAlloc(cap, DT_ALLOC_PERM);
    capacity = cap;
}

LinearAllocator::~LinearAllocator() {
    dtFree(buffer);
}

void LinearAllocator::reset() {
    high = (high > top) ? high : top;
    top = 0;
}

void* LinearAllocator::alloc(const size_t size) {
    if (!buffer) return nullptr;
    if (top + size > capacity) return nullptr;
    unsigned char* mem = &buffer[top];
    top += size;
    return mem;
}

void LinearAllocator::free(void* /*ptr*/) {
    // Empty - linear allocator doesn't free individual allocations
}

// ============================================
// Tile Cache Compressor implementation (passthrough)
// ============================================

int TileCacheCompressor::maxCompressedSize(const int bufferSize) {
    return bufferSize;
}

dtStatus TileCacheCompressor::compress(const unsigned char* buffer, const int bufferSize,
                                       unsigned char* compressed, const int /*maxCompressedSize*/, int* compressedSize) {
    memcpy(compressed, buffer, bufferSize);
    *compressedSize = bufferSize;
    return DT_SUCCESS;
}

dtStatus TileCacheCompressor::decompress(const unsigned char* compressed, const int compressedSize,
                                         unsigned char* buffer, const int maxBufferSize, int* bufferSize) {
    if (compressedSize > maxBufferSize) return DT_FAILURE;
    memcpy(buffer, compressed, compressedSize);
    *bufferSize = compressedSize;
    return DT_SUCCESS;
}

// ============================================
// Tile Cache Mesh Process implementation
// ============================================

void TileCacheMeshProcess::process(struct dtNavMeshCreateParams* params,
                                   unsigned char* polyAreas, unsigned short* polyFlags) {
    // Set all polygons as walkable
    for (int i = 0; i < params->polyCount; ++i) {
        polyAreas[i] = DT_TILECACHE_WALKABLE_AREA;
        polyFlags[i] = 1; // Walkable flag
    }
}

// ============================================
// NavMesh implementation
// ============================================

NavMesh::NavMesh() : m_ctx(nullptr), m_navMesh(nullptr), m_navQuery(nullptr),
                     m_tileCache(nullptr), m_talloc(nullptr), m_tcomp(nullptr), m_tmproc(nullptr) {
    m_ctx = new rcContext();
    m_navQuery = dtAllocNavMeshQuery();
    memset(&m_cfg, 0, sizeof(m_cfg));
}

NavMesh::~NavMesh() {
    cleanupTileDebugData();
    if (m_tileCache) dtFreeTileCache(m_tileCache);
    if (m_navMesh) dtFreeNavMesh(m_navMesh);
    if (m_navQuery) dtFreeNavMeshQuery(m_navQuery);
    if (m_debugModel.meshCount > 0) {
        UnloadModel(m_debugModel);
    }
    delete m_talloc;
    delete m_tcomp;
    delete m_tmproc;
    delete m_ctx;
}

void NavMesh::cleanupTileDebugData() {
    for (auto& pair : m_tileDebugData) {
        if (pair.second.polyMesh) {
            rcFreePolyMesh(pair.second.polyMesh);
        }
        if (pair.second.debugModel.meshCount > 0) {
            UnloadModel(pair.second.debugModel);
        }
    }
    m_tileDebugData.clear();
}

void NavMesh::getBounds(float* bmin, float* bmax) const {
    if (bmin) {
        bmin[0] = m_boundsMin[0];
        bmin[1] = m_boundsMin[1];
        bmin[2] = m_boundsMin[2];
    }
    if (bmax) {
        bmax[0] = m_boundsMax[0];
        bmax[1] = m_boundsMax[1];
        bmax[2] = m_boundsMax[2];
    }
}

TileCoord NavMesh::getTileCoordAt(Vector3 worldPos) const {
    TileCoord tc;
    tc.x = (int)floorf((worldPos.x - m_boundsMin[0]) / m_tileSize);
    tc.y = (int)floorf((worldPos.z - m_boundsMin[2]) / m_tileSize);
    return tc;
}

// Build legacy - ora usa buildTiled internamente
bool NavMesh::build(const Mesh& mesh, Matrix transform) {
    return buildTiled(mesh, transform);
}

bool NavMesh::initNavMesh() {
    // Pulisci navmesh esistente
    if (m_navMesh) {
        dtFreeNavMesh(m_navMesh);
        m_navMesh = nullptr;
    }

    m_navMesh = dtAllocNavMesh();
    if (!m_navMesh) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate navmesh");
        return false;
    }

    // Parametri per navmesh tiled
    dtNavMeshParams params;
    memset(&params, 0, sizeof(params));
    rcVcopy(params.orig, m_boundsMin);
    params.tileWidth = m_tileSize;
    params.tileHeight = m_tileSize;
    params.maxTiles = m_maxTiles;
    params.maxPolys = m_maxPolysPerTile;

    dtStatus status = m_navMesh->init(&params);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init tiled navmesh");
        return false;
    }

    // Init query
    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init navmesh query");
        return false;
    }

    return true;
}

bool NavMesh::initTileCache() {
    // Clean up existing tile cache
    if (m_tileCache) {
        dtFreeTileCache(m_tileCache);
        m_tileCache = nullptr;
    }

    // Create allocator, compressor and mesh processor
    delete m_talloc;
    delete m_tcomp;
    delete m_tmproc;

    // Allocator needs enough memory for tile operations
    // Larger maps need more memory
    const size_t allocSize = 64 * 1024; // 64KB should be sufficient
    m_talloc = new LinearAllocator(allocSize);
    m_tcomp = new TileCacheCompressor();
    m_tmproc = new TileCacheMeshProcess();

    // Allocate tile cache
    m_tileCache = dtAllocTileCache();
    if (!m_tileCache) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate tile cache");
        return false;
    }

    // Setup tile cache params
    dtTileCacheParams tcparams;
    memset(&tcparams, 0, sizeof(tcparams));
    rcVcopy(tcparams.orig, m_boundsMin);
    tcparams.cs = m_cellSize;
    tcparams.ch = m_cellHeight;
    tcparams.width = m_cfg.tileSize;  // Tile size in cells
    tcparams.height = m_cfg.tileSize; // Tile size in cells
    tcparams.walkableHeight = m_agentHeight;
    tcparams.walkableRadius = m_agentRadius;
    tcparams.walkableClimb = m_agentMaxClimb;
    tcparams.maxSimplificationError = m_maxSimplificationError;

    // Calculate max tiles - need extra for layers
    const int EXPECTED_LAYERS_PER_TILE = 4;
    tcparams.maxTiles = m_tilesX * m_tilesZ * EXPECTED_LAYERS_PER_TILE;
    tcparams.maxObstacles = 256; // Increase max obstacles

    dtStatus status = m_tileCache->init(&tcparams, m_talloc, m_tcomp, m_tmproc);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init tile cache");
        return false;
    }

    TraceLog(LOG_INFO, "NavMesh: Tile cache initialized:");
    TraceLog(LOG_INFO, "  - origin: (%.2f, %.2f, %.2f)", tcparams.orig[0], tcparams.orig[1], tcparams.orig[2]);
    TraceLog(LOG_INFO, "  - cellSize: %.3f, cellHeight: %.3f", tcparams.cs, tcparams.ch);
    TraceLog(LOG_INFO, "  - tileSize: %d x %d cells", tcparams.width, tcparams.height);
    TraceLog(LOG_INFO, "  - walkableHeight: %.2f, radius: %.2f, climb: %.2f",
             tcparams.walkableHeight, tcparams.walkableRadius, tcparams.walkableClimb);
    TraceLog(LOG_INFO, "  - maxTiles: %d, maxObstacles: %d", tcparams.maxTiles, tcparams.maxObstacles);

    return true;
}

int NavMesh::rasterizeTileLayers(int tileX, int tileY, const rcConfig& cfg,
                                  TileCacheData* tiles, int maxTiles) {
    // Calculate tile bounds with border
    float tileBmin[3], tileBmax[3];

    tileBmin[0] = m_boundsMin[0] + tileX * m_tileSize;
    tileBmin[1] = m_boundsMin[1];
    tileBmin[2] = m_boundsMin[2] + tileY * m_tileSize;

    tileBmax[0] = m_boundsMin[0] + (tileX + 1) * m_tileSize;
    tileBmax[1] = m_boundsMax[1];
    tileBmax[2] = m_boundsMin[2] + (tileY + 1) * m_tileSize;

    // Expand for border
    float borderSize = cfg.borderSize * cfg.cs;
    tileBmin[0] -= borderSize;
    tileBmin[2] -= borderSize;
    tileBmax[0] += borderSize;
    tileBmax[2] += borderSize;

    // Setup config for this tile
    rcConfig tileCfg = cfg;
    rcVcopy(tileCfg.bmin, tileBmin);
    rcVcopy(tileCfg.bmax, tileBmax);

    rcCalcGridSize(tileCfg.bmin, tileCfg.bmax, tileCfg.cs, &tileCfg.width, &tileCfg.height);

    // If tile is too small, skip
    if (tileCfg.width < 3 || tileCfg.height < 3) {
        return 0;
    }

    // 1. Create heightfield
    rcHeightfield* hf = rcAllocHeightfield();
    if (!hf) return 0;

    if (!rcCreateHeightfield(m_ctx, *hf, tileCfg.width, tileCfg.height,
                             tileCfg.bmin, tileCfg.bmax, tileCfg.cs, tileCfg.ch)) {
        rcFreeHeightField(hf);
        return 0;
    }

    // Rasterize triangles
    unsigned char* areas = new unsigned char[m_storedTriCount];
    memset(areas, 0, m_storedTriCount);

    rcMarkWalkableTriangles(m_ctx, tileCfg.walkableSlopeAngle,
                            m_storedVerts.data(), m_storedVertCount,
                            m_storedTris.data(), m_storedTriCount, areas);

    if (!rcRasterizeTriangles(m_ctx, m_storedVerts.data(), m_storedVertCount,
                              m_storedTris.data(), areas, m_storedTriCount,
                              *hf, tileCfg.walkableClimb)) {
        delete[] areas;
        rcFreeHeightField(hf);
        return 0;
    }
    delete[] areas;

    // 2. Filtering
    rcFilterLowHangingWalkableObstacles(m_ctx, tileCfg.walkableClimb, *hf);
    rcFilterLedgeSpans(m_ctx, tileCfg.walkableHeight, tileCfg.walkableClimb, *hf);
    rcFilterWalkableLowHeightSpans(m_ctx, tileCfg.walkableHeight, *hf);

    // 3. Compact heightfield
    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf) {
        rcFreeHeightField(hf);
        return 0;
    }

    if (!rcBuildCompactHeightfield(m_ctx, tileCfg.walkableHeight, tileCfg.walkableClimb, *hf, *chf)) {
        rcFreeHeightField(hf);
        rcFreeCompactHeightfield(chf);
        return 0;
    }
    rcFreeHeightField(hf);

    // 4. Erode walkable area
    if (!rcErodeWalkableArea(m_ctx, tileCfg.walkableRadius, *chf)) {
        rcFreeCompactHeightfield(chf);
        return 0;
    }

    // 5. Build heightfield layers
    rcHeightfieldLayerSet* lset = rcAllocHeightfieldLayerSet();
    if (!lset) {
        rcFreeCompactHeightfield(chf);
        return 0;
    }

    if (!rcBuildHeightfieldLayers(m_ctx, *chf, tileCfg.borderSize, tileCfg.walkableHeight, *lset)) {
        rcFreeCompactHeightfield(chf);
        rcFreeHeightfieldLayerSet(lset);
        return 0;
    }

    rcFreeCompactHeightfield(chf);

    // Build tile cache layers from heightfield layers
    int ntiles = 0;
    for (int i = 0; i < rcMin(lset->nlayers, maxTiles); ++i) {
        const rcHeightfieldLayer* layer = &lset->layers[i];

        dtTileCacheLayerHeader header;
        header.magic = DT_TILECACHE_MAGIC;
        header.version = DT_TILECACHE_VERSION;

        header.tx = tileX;
        header.ty = tileY;
        header.tlayer = i;
        rcVcopy(header.bmin, layer->bmin);
        rcVcopy(header.bmax, layer->bmax);

        header.width = (unsigned char)layer->width;
        header.height = (unsigned char)layer->height;
        header.minx = (unsigned char)layer->minx;
        header.maxx = (unsigned char)layer->maxx;
        header.miny = (unsigned char)layer->miny;
        header.maxy = (unsigned char)layer->maxy;
        header.hmin = (unsigned short)layer->hmin;
        header.hmax = (unsigned short)layer->hmax;

        dtStatus status = dtBuildTileCacheLayer(m_tcomp, &header, layer->heights, layer->areas, layer->cons,
                                                &tiles[ntiles].data, &tiles[ntiles].dataSize);
        if (dtStatusSucceed(status)) {
            ntiles++;
        }
    }

    rcFreeHeightfieldLayerSet(lset);

    return ntiles;
}

bool NavMesh::buildTiled(const Mesh& mesh, Matrix transform) {
    if (mesh.vertexCount == 0) {
        TraceLog(LOG_ERROR, "NavMesh: Mesh has no vertices");
        return false;
    }

    TraceLog(LOG_INFO, "NavMesh: Building TILED navmesh from mesh with %d vertices, %d triangles",
             mesh.vertexCount, mesh.triangleCount);

    // Cleanup precedente
    cleanupTileDebugData();
    m_debugMeshBuilt = false;
    m_tileCount = 0;
    m_totalPolygons = 0;

    // Trasforma e salva i vertici
    m_storedVertCount = mesh.vertexCount;
    m_storedTriCount = mesh.triangleCount;
    m_storedVerts.resize(mesh.vertexCount * 3);

    for (int i = 0; i < mesh.vertexCount; i++) {
        Vector3 v = {
            mesh.vertices[i * 3 + 0],
            mesh.vertices[i * 3 + 1],
            mesh.vertices[i * 3 + 2]
        };
        v = Vector3Transform(v, transform);
        m_storedVerts[i * 3 + 0] = v.x;
        m_storedVerts[i * 3 + 1] = v.y;
        m_storedVerts[i * 3 + 2] = v.z;
    }

    // Salva gli indici
    m_storedTris.resize(m_storedTriCount * 3);
    if (mesh.indices != nullptr) {
        for (int i = 0; i < m_storedTriCount * 3; i++) {
            m_storedTris[i] = (int)mesh.indices[i];
        }
    } else {
        for (int i = 0; i < m_storedTriCount * 3; i++) {
            m_storedTris[i] = i;
        }
    }

    // Calcola il bounding box totale
    rcCalcBounds(m_storedVerts.data(), m_storedVertCount, m_boundsMin, m_boundsMax);

    float mapWidth = m_boundsMax[0] - m_boundsMin[0];
    float mapLength = m_boundsMax[2] - m_boundsMin[2];

    TraceLog(LOG_INFO, "NavMesh: Bounding box: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
             m_boundsMin[0], m_boundsMin[1], m_boundsMin[2],
             m_boundsMax[0], m_boundsMax[1], m_boundsMax[2]);
    TraceLog(LOG_INFO, "NavMesh: Map dimensions: %.2f x %.2f", mapWidth, mapLength);

    // Calcola la dimensione ottimale delle tile
    // Ogni tile dovrebbe avere una grid di circa 64-128 celle per lato per buone performance
    float targetCellsPerTile = 128.0f;
    m_tileSize = targetCellsPerTile * m_cellSize;

    // Clamp tile size tra 32 e 256 unità
    m_tileSize = fmaxf(32.0f, fminf(256.0f, m_tileSize));

    // Calcola numero di tile
    m_tilesX = (int)ceilf(mapWidth / m_tileSize);
    m_tilesZ = (int)ceilf(mapLength / m_tileSize);

    // Assicurati di avere almeno 1 tile
    m_tilesX = (m_tilesX < 1) ? 1 : m_tilesX;
    m_tilesZ = (m_tilesZ < 1) ? 1 : m_tilesZ;

    // Calcola maxTiles e maxPolys come potenze di 2 (richiesto da Detour)
    // 22 bit totali da dividere tra tiles e polys
    int totalTiles = m_tilesX * m_tilesZ;

    // Helper: calcola prossima potenza di 2
    auto nextPow2 = [](unsigned int v) -> unsigned int {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    };

    // Helper: calcola log2 intero
    auto ilog2 = [](unsigned int v) -> unsigned int {
        unsigned int r = 0;
        while (v >>= 1) r++;
        return r;
    };

    int tileBits = ilog2(nextPow2(totalTiles));
    if (tileBits < 1) tileBits = 1;
    if (tileBits > 14) tileBits = 14;  // Max 14 bit per tiles
    int polyBits = 22 - tileBits;
    m_maxTiles = 1 << tileBits;
    m_maxPolysPerTile = 1 << polyBits;

    TraceLog(LOG_INFO, "NavMesh: Tile size: %.2f, Grid: %d x %d tiles (total: %d)",
             m_tileSize, m_tilesX, m_tilesZ, totalTiles);
    TraceLog(LOG_INFO, "NavMesh: maxTiles: %d (2^%d), maxPolysPerTile: %d (2^%d)",
             m_maxTiles, tileBits, m_maxPolysPerTile, polyBits);

    // Setup configurazione Recast base
    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.cs = m_cellSize;
    m_cfg.ch = m_cellHeight;
    m_cfg.walkableSlopeAngle = m_agentMaxSlope;
    m_cfg.walkableHeight = (int)ceilf(m_agentHeight / m_cfg.ch);
    m_cfg.walkableClimb = (int)floorf(m_agentMaxClimb / m_cfg.ch);
    m_cfg.walkableRadius = (int)ceilf(m_agentRadius / m_cfg.cs);
    m_cfg.maxEdgeLen = (int)(12.0f / m_cfg.cs);
    m_cfg.maxSimplificationError = m_maxSimplificationError;
    m_cfg.minRegionArea = (int)rcSqr(m_minRegionArea);
    m_cfg.mergeRegionArea = (int)rcSqr(m_mergeRegionArea);
    m_cfg.maxVertsPerPoly = 6;
    m_cfg.detailSampleDist = m_cfg.cs * 6.0f;
    m_cfg.detailSampleMaxError = m_cfg.ch * 1.0f;
    // Tile size in celle
    m_cfg.tileSize = (int)(m_tileSize / m_cfg.cs);
    m_cfg.borderSize = m_cfg.walkableRadius + 3; // Border per connessioni tra tile

    TraceLog(LOG_INFO, "NavMesh: Config - cellSize: %.2f, tileSize(cells): %d, border: %d",
             m_cfg.cs, m_cfg.tileSize, m_cfg.borderSize);

    // Inizializza la navmesh tiled
    if (!initNavMesh()) {
        return false;
    }

    // Initialize tile cache for dynamic obstacles
    if (!initTileCache()) {
        return false;
    }

    // Build tile cache layers and navmesh tiles
    int builtTiles = 0;
    double startTime = GetTime();

    const int MAX_LAYERS = 32;
    TileCacheData tiles[MAX_LAYERS];

    for (int y = 0; y < m_tilesZ; y++) {
        for (int x = 0; x < m_tilesX; x++) {
            // Build tile cache layers for this tile
            int ntiles = rasterizeTileLayers(x, y, m_cfg, tiles, MAX_LAYERS);

            // Add layers to tile cache
            for (int i = 0; i < ntiles; ++i) {
                dtStatus status = m_tileCache->addTile(tiles[i].data, tiles[i].dataSize,
                                                       DT_COMPRESSEDTILE_FREE_DATA, 0);
                if (dtStatusFailed(status)) {
                    dtFree(tiles[i].data);
                    tiles[i].data = nullptr;
                }
            }

            builtTiles++;
        }
    }

    // Build initial navmesh tiles from the tile cache
    for (int y = 0; y < m_tilesZ; y++) {
        for (int x = 0; x < m_tilesX; x++) {
            m_tileCache->buildNavMeshTilesAt(x, y, m_navMesh);
        }
    }

    double elapsed = GetTime() - startTime;

    // Count actual polygons
    m_totalPolygons = 0;
    const dtNavMesh* navMesh = m_navMesh;
    int maxTiles = navMesh->getMaxTiles();
    for (int i = 0; i < maxTiles; i++) {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (tile && tile->header) {
            m_totalPolygons += tile->header->polyCount;
        }
    }

    TraceLog(LOG_INFO, "NavMesh: Built %d/%d tiles in %.2f seconds (using TileCache)",
             builtTiles, totalTiles, elapsed);
    TraceLog(LOG_INFO, "NavMesh: Total polygons: %d", m_totalPolygons);

    if (builtTiles == 0) {
        TraceLog(LOG_ERROR, "NavMesh: No tiles were built!");
        return false;
    }

    m_tileCount = builtTiles;
    return true;
}

unsigned char* NavMesh::buildTileData(int tileX, int tileY, int& dataSize) {
    dataSize = 0;

    // Calcola i bounds della tile con bordo
    float tileBmin[3], tileBmax[3];

    tileBmin[0] = m_boundsMin[0] + tileX * m_tileSize;
    tileBmin[1] = m_boundsMin[1];
    tileBmin[2] = m_boundsMin[2] + tileY * m_tileSize;

    tileBmax[0] = m_boundsMin[0] + (tileX + 1) * m_tileSize;
    tileBmax[1] = m_boundsMax[1];
    tileBmax[2] = m_boundsMin[2] + (tileY + 1) * m_tileSize;

    // Espandi per il bordo
    float borderSize = m_cfg.borderSize * m_cfg.cs;
    tileBmin[0] -= borderSize;
    tileBmin[2] -= borderSize;
    tileBmax[0] += borderSize;
    tileBmax[2] += borderSize;

    // Setup config per questa tile
    rcConfig tileCfg = m_cfg;
    rcVcopy(tileCfg.bmin, tileBmin);
    rcVcopy(tileCfg.bmax, tileBmax);

    rcCalcGridSize(tileCfg.bmin, tileCfg.bmax, tileCfg.cs, &tileCfg.width, &tileCfg.height);

    // Se la tile è troppo piccola, skip
    if (tileCfg.width < 3 || tileCfg.height < 3) {
        return nullptr;
    }

    // 1. Heightfield
    rcHeightfield* hf = rcAllocHeightfield();
    if (!hf) return nullptr;

    if (!rcCreateHeightfield(m_ctx, *hf, tileCfg.width, tileCfg.height,
                             tileCfg.bmin, tileCfg.bmax, tileCfg.cs, tileCfg.ch)) {
        rcFreeHeightField(hf);
        return nullptr;
    }

    // Rasterizza i triangoli
    unsigned char* areas = new unsigned char[m_storedTriCount];
    memset(areas, 0, m_storedTriCount);

    rcMarkWalkableTriangles(m_ctx, tileCfg.walkableSlopeAngle,
                            m_storedVerts.data(), m_storedVertCount,
                            m_storedTris.data(), m_storedTriCount, areas);

    if (!rcRasterizeTriangles(m_ctx, m_storedVerts.data(), m_storedVertCount,
                              m_storedTris.data(), areas, m_storedTriCount,
                              *hf, tileCfg.walkableClimb)) {
        delete[] areas;
        rcFreeHeightField(hf);
        return nullptr;
    }
    delete[] areas;

    // 2. Filtraggio
    rcFilterLowHangingWalkableObstacles(m_ctx, tileCfg.walkableClimb, *hf);
    rcFilterLedgeSpans(m_ctx, tileCfg.walkableHeight, tileCfg.walkableClimb, *hf);
    rcFilterWalkableLowHeightSpans(m_ctx, tileCfg.walkableHeight, *hf);

    // 3. Compact heightfield
    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf) {
        rcFreeHeightField(hf);
        return nullptr;
    }

    if (!rcBuildCompactHeightfield(m_ctx, tileCfg.walkableHeight, tileCfg.walkableClimb, *hf, *chf)) {
        rcFreeHeightField(hf);
        rcFreeCompactHeightfield(chf);
        return nullptr;
    }
    rcFreeHeightField(hf);

    // 4. Erosione
    if (!rcErodeWalkableArea(m_ctx, tileCfg.walkableRadius, *chf)) {
        rcFreeCompactHeightfield(chf);
        return nullptr;
    }

    // 5. Distance field e regioni
    if (!rcBuildDistanceField(m_ctx, *chf)) {
        rcFreeCompactHeightfield(chf);
        return nullptr;
    }

    if (!rcBuildRegions(m_ctx, *chf, tileCfg.borderSize, tileCfg.minRegionArea, tileCfg.mergeRegionArea)) {
        rcFreeCompactHeightfield(chf);
        return nullptr;
    }

    // 6. Contours
    rcContourSet* cset = rcAllocContourSet();
    if (!cset) {
        rcFreeCompactHeightfield(chf);
        return nullptr;
    }

    if (!rcBuildContours(m_ctx, *chf, tileCfg.maxSimplificationError, tileCfg.maxEdgeLen, *cset)) {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return nullptr;
    }

    // 7. Poly mesh
    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!pmesh) {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return nullptr;
    }

    if (!rcBuildPolyMesh(m_ctx, *cset, tileCfg.maxVertsPerPoly, *pmesh)) {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return nullptr;
    }

    // 8. Detail mesh
    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    if (!dmesh) {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return nullptr;
    }

    if (!rcBuildPolyMeshDetail(m_ctx, *pmesh, *chf, tileCfg.detailSampleDist, tileCfg.detailSampleMaxError, *dmesh)) {
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return nullptr;
    }

    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(cset);

    // Se non ci sono poligoni, tile vuota
    if (pmesh->npolys == 0) {
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return nullptr;
    }

    // Setup flags
    for (int i = 0; i < pmesh->npolys; i++) {
        pmesh->flags[i] = 1; // Walkable
    }

    // Debug data - store reference to tile coords only (no manual mesh copy)
    TileCoord tc = {tileX, tileY};
    m_tileDebugData[tc].polyMesh = nullptr;
    m_tileDebugData[tc].meshBuilt = false;

    // 9. Crea dati navmesh per Detour
    dtNavMeshCreateParams params;
    memset(&params, 0, sizeof(params));
    params.verts = pmesh->verts;
    params.vertCount = pmesh->nverts;
    params.polys = pmesh->polys;
    params.polyAreas = pmesh->areas;
    params.polyFlags = pmesh->flags;
    params.polyCount = pmesh->npolys;
    params.nvp = pmesh->nvp;
    params.detailMeshes = dmesh->meshes;
    params.detailVerts = dmesh->verts;
    params.detailVertsCount = dmesh->nverts;
    params.detailTris = dmesh->tris;
    params.detailTriCount = dmesh->ntris;
    params.walkableHeight = m_agentHeight;
    params.walkableRadius = m_agentRadius;
    params.walkableClimb = m_agentMaxClimb;
    params.cs = tileCfg.cs;
    params.ch = tileCfg.ch;
    params.buildBvTree = true;
    params.tileX = tileX;
    params.tileY = tileY;
    params.tileLayer = 0;
    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);

    unsigned char* navData = nullptr;
    if (!dtCreateNavMeshData(&params, &navData, &dataSize)) {
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return nullptr;
    }

    m_totalPolygons += pmesh->npolys;

    rcFreePolyMesh(pmesh);
    rcFreePolyMeshDetail(dmesh);

    return navData;
}

bool NavMesh::buildTile(int tileX, int tileY) {
    if (!m_navMesh) {
        TraceLog(LOG_ERROR, "NavMesh: NavMesh not initialized");
        return false;
    }

    // Rimuovi tile esistente
    dtTileRef existingTile = m_navMesh->getTileRefAt(tileX, tileY, 0);
    if (existingTile) {
        m_navMesh->removeTile(existingTile, nullptr, nullptr);
    }

    // Costruisci i dati della tile
    int dataSize = 0;
    unsigned char* data = buildTileData(tileX, tileY, dataSize);

    if (!data) {
        // Tile vuota - nessun poligono walkable
        return false;
    }

    // Aggiungi la tile
    dtStatus status = m_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, nullptr);
    if (dtStatusFailed(status)) {
        dtFree(data);
        TraceLog(LOG_ERROR, "NavMesh: Failed to add tile (%d, %d)", tileX, tileY);
        return false;
    }

    return true;
}

bool NavMesh::removeTile(int tileX, int tileY) {
    if (!m_navMesh) return false;

    dtTileRef ref = m_navMesh->getTileRefAt(tileX, tileY, 0);
    if (ref) {
        m_navMesh->removeTile(ref, nullptr, nullptr);

        // Rimuovi debug data
        TileCoord tc = {tileX, tileY};
        auto it = m_tileDebugData.find(tc);
        if (it != m_tileDebugData.end()) {
            if (it->second.polyMesh) rcFreePolyMesh(it->second.polyMesh);
            if (it->second.debugModel.meshCount > 0) UnloadModel(it->second.debugModel);
            m_tileDebugData.erase(it);
        }

        m_tileCount--;
        m_debugMeshBuilt = false;
        return true;
    }
    return false;
}

bool NavMesh::rebuildTile(int tileX, int tileY) {
    removeTile(tileX, tileY);
    return buildTile(tileX, tileY);
}

std::vector<Vector3> NavMesh::findPath(Vector3 start, Vector3 end) {
    std::vector<Vector3> pathPoints;

    if (!m_navMesh || !m_navQuery) {
        TraceLog(LOG_WARNING, "NavMesh: NavMesh not initialized");
        return pathPoints;
    }

    float sPos[3] = { start.x, start.y, start.z };
    float ePos[3] = { end.x, end.y, end.z };
    float extents[3] = { 10.0f, 50.0f, 10.0f };

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtPolyRef startRef, endRef;
    float nearestStart[3], nearestEnd[3];

    m_navQuery->findNearestPoly(sPos, extents, &filter, &startRef, nearestStart);
    m_navQuery->findNearestPoly(ePos, extents, &filter, &endRef, nearestEnd);

    if (!startRef || !endRef) {
        TraceLog(LOG_WARNING, "NavMesh: Could not find start or end polygon (start: %.2f,%.2f,%.2f, end: %.2f,%.2f,%.2f)",
                 start.x, start.y, start.z, end.x, end.y, end.z);
        return pathPoints;
    }

    static const int MAX_POLYS = 256;
    dtPolyRef pathPolys[MAX_POLYS];
    int pathCount = 0;

    m_navQuery->findPath(startRef, endRef, nearestStart, nearestEnd, &filter, pathPolys, &pathCount, MAX_POLYS);

    if (pathCount > 0) {
        float straightPath[MAX_POLYS * 3];
        unsigned char straightPathFlags[MAX_POLYS];
        dtPolyRef straightPathPolys[MAX_POLYS];
        int straightPathCount = 0;

        m_navQuery->findStraightPath(nearestStart, nearestEnd, pathPolys, pathCount,
                                     straightPath, straightPathFlags, straightPathPolys,
                                     &straightPathCount, MAX_POLYS, 0);

        for (int i = 0; i < straightPathCount; i++) {
            pathPoints.push_back({
                straightPath[i * 3],
                straightPath[i * 3 + 1],
                straightPath[i * 3 + 2]
            });
        }
    }

    return pathPoints;
}

void NavMesh::setParametersForMapSize(float mapSize) {
    if (mapSize < 500.0f) {
        m_cellSize = 0.2f;
        m_cellHeight = 0.2f;
        m_agentRadius = 0.5f;
        m_tileSize = 32.0f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for SMALL map (< 500)");
    } else if (mapSize < 2000.0f) {
        m_cellSize = 0.3f;
        m_cellHeight = 0.3f;
        m_agentRadius = 0.6f;
        m_tileSize = 64.0f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for MEDIUM map (500-2000)");
    } else if (mapSize < 5000.0f) {
        m_cellSize = 0.5f;
        m_cellHeight = 0.4f;
        m_agentRadius = 0.8f;
        m_tileSize = 128.0f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for LARGE map (2000-5000)");
    } else {
        m_cellSize = 0.8f;
        m_cellHeight = 0.5f;
        m_agentRadius = 1.0f;
        m_tileSize = 256.0f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for HUGE map (> 5000)");
    }
}

bool NavMesh::projectPointToNavMesh(Vector3 point, Vector3& projectedPoint) {
    if (!m_navMesh || !m_navQuery) {
        TraceLog(LOG_WARNING, "NavMesh: NavMesh not initialized");
        return false;
    }

    float pos[3] = { point.x, point.y, point.z };
    float extents[3] = { 10.0f, 50.0f, 10.0f };

    dtQueryFilter filter;
    filter.setIncludeFlags(0xFFFF);
    filter.setExcludeFlags(0);

    dtPolyRef polyRef;
    float nearestPoint[3];

    dtStatus status = m_navQuery->findNearestPoly(pos, extents, &filter, &polyRef, nearestPoint);

    if (dtStatusSucceed(status) && polyRef) {
        projectedPoint.x = nearestPoint[0];
        projectedPoint.y = nearestPoint[1];
        projectedPoint.z = nearestPoint[2];
        return true;
    }

    return false;
}

void NavMesh::buildDebugMesh() {
    // Usa il nuovo metodo che legge direttamente da dtNavMesh
    buildDebugMeshFromNavMesh();
}

void NavMesh::buildDebugMeshFromNavMesh() {
    if (!m_navMesh) return;

    std::vector<Vector3> allVertices;
    std::vector<unsigned short> allIndices;
    std::vector<Color> allColors;

    // Colori diversi per tile diverse
    Color tileColors[] = {
        {0, 200, 0, 100},    // Verde
        {0, 150, 200, 100},  // Cyan
        {200, 150, 0, 100},  // Arancione
        {150, 0, 200, 100},  // Viola
        {200, 0, 100, 100},  // Magenta
        {100, 200, 0, 100},  // Lime
    };
    int numColors = sizeof(tileColors) / sizeof(tileColors[0]);

    // Usa const pointer per accedere al metodo getTile pubblico
    const dtNavMesh* navMesh = m_navMesh;
    int maxTiles = navMesh->getMaxTiles();
    int colorIndex = 0;

    for (int i = 0; i < maxTiles; i++) {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (!tile || !tile->header) continue;

        Color tileColor = tileColors[colorIndex % numColors];
        colorIndex++;

        // Itera su tutti i poligoni della tile
        for (int j = 0; j < tile->header->polyCount; j++) {
            const dtPoly* poly = &tile->polys[j];

            // Skip off-mesh connections
            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION) continue;

            // Estrai i vertici del poligono
            std::vector<Vector3> polyVerts;
            for (int k = 0; k < poly->vertCount; k++) {
                const float* v = &tile->verts[poly->verts[k] * 3];
                // Alza leggermente per la visualizzazione
                polyVerts.push_back({v[0], v[1] + 0.2f, v[2]});
            }

            if (polyVerts.size() >= 3) {
                unsigned short baseIndex = (unsigned short)allVertices.size();

                for (const auto& v : polyVerts) {
                    allVertices.push_back(v);
                    allColors.push_back(tileColor);
                }

                // Triangola il poligono (fan triangulation)
                for (size_t k = 1; k < polyVerts.size() - 1; k++) {
                    allIndices.push_back(baseIndex);
                    allIndices.push_back(baseIndex + k);
                    allIndices.push_back(baseIndex + k + 1);
                }
            }
        }
    }

    if (allVertices.empty()) {
        TraceLog(LOG_WARNING, "NavMesh: No polygons found for debug mesh");
        return;
    }

    // Cleanup vecchio model
    if (m_debugModel.meshCount > 0) {
        UnloadModel(m_debugModel);
        m_debugModel = {0};
    }

    Mesh debugMesh = {0};
    debugMesh.vertexCount = allVertices.size();
    debugMesh.triangleCount = allIndices.size() / 3;

    debugMesh.vertices = (float*)MemAlloc(allVertices.size() * 3 * sizeof(float));
    debugMesh.indices = (unsigned short*)MemAlloc(allIndices.size() * sizeof(unsigned short));
    debugMesh.colors = (unsigned char*)MemAlloc(allVertices.size() * 4 * sizeof(unsigned char));

    for (size_t i = 0; i < allVertices.size(); i++) {
        debugMesh.vertices[i * 3 + 0] = allVertices[i].x;
        debugMesh.vertices[i * 3 + 1] = allVertices[i].y;
        debugMesh.vertices[i * 3 + 2] = allVertices[i].z;

        debugMesh.colors[i * 4 + 0] = allColors[i].r;
        debugMesh.colors[i * 4 + 1] = allColors[i].g;
        debugMesh.colors[i * 4 + 2] = allColors[i].b;
        debugMesh.colors[i * 4 + 3] = allColors[i].a;
    }

    for (size_t i = 0; i < allIndices.size(); i++) {
        debugMesh.indices[i] = allIndices[i];
    }

    UploadMesh(&debugMesh, false);
    m_debugModel = LoadModelFromMesh(debugMesh);
    m_debugMeshBuilt = true;

    TraceLog(LOG_INFO, "NavMesh: Debug mesh built with %d vertices, %d triangles",
             (int)allVertices.size(), (int)allIndices.size() / 3);
}

void NavMesh::drawDebug() {
    if (!m_debugMeshBuilt) {
        buildDebugMesh();
    }

    if (m_debugMeshBuilt && m_debugModel.meshCount > 0) {
        rlDisableBackfaceCulling();
        rlEnableColorBlend();
        DrawModel(m_debugModel, {0, 0, 0}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
    }
}

// Header per il file binario navmesh
static const int NAVMESH_FILE_MAGIC = 0x4E4D5348;  // 'NMSH' in hex
static const int NAVMESH_FILE_VERSION = 1;

bool NavMesh::saveToFile(const std::string& filename) {
    if (!m_navMesh) {
        TraceLog(LOG_ERROR, "NavMesh: Cannot save - navmesh not built");
        return false;
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "NavMesh: Cannot open file for writing: %s", filename.c_str());
        return false;
    }

    // Scrivi header
    file.write(reinterpret_cast<const char*>(&NAVMESH_FILE_MAGIC), sizeof(int));
    file.write(reinterpret_cast<const char*>(&NAVMESH_FILE_VERSION), sizeof(int));

    // Scrivi parametri navmesh
    const dtNavMeshParams* params = m_navMesh->getParams();
    file.write(reinterpret_cast<const char*>(params), sizeof(dtNavMeshParams));

    // Scrivi bounds
    file.write(reinterpret_cast<const char*>(m_boundsMin), sizeof(float) * 3);
    file.write(reinterpret_cast<const char*>(m_boundsMax), sizeof(float) * 3);

    // Scrivi numero di tiles
    const dtNavMesh* navMesh = m_navMesh;
    int maxTiles = navMesh->getMaxTiles();
    int numTiles = 0;

    // Conta tiles valide
    for (int i = 0; i < maxTiles; i++) {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (tile && tile->header && tile->dataSize > 0) {
            numTiles++;
        }
    }

    file.write(reinterpret_cast<const char*>(&numTiles), sizeof(int));

    // Scrivi ogni tile
    for (int i = 0; i < maxTiles; i++) {
        const dtMeshTile* tile = navMesh->getTile(i);
        if (!tile || !tile->header || tile->dataSize <= 0) continue;

        // Scrivi tile reference
        dtTileRef tileRef = m_navMesh->getTileRef(tile);
        file.write(reinterpret_cast<const char*>(&tileRef), sizeof(dtTileRef));

        // Scrivi dimensione dati tile
        int dataSize = tile->dataSize;
        file.write(reinterpret_cast<const char*>(&dataSize), sizeof(int));

        // Scrivi dati tile
        file.write(reinterpret_cast<const char*>(tile->data), dataSize);
    }

    file.close();

    TraceLog(LOG_INFO, "NavMesh: Saved to %s (%d tiles)", filename.c_str(), numTiles);
    return true;
}

bool NavMesh::loadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        TraceLog(LOG_INFO, "NavMesh: Cache file not found: %s", filename.c_str());
        return false;
    }

    // Leggi e verifica header
    int magic, version;
    file.read(reinterpret_cast<char*>(&magic), sizeof(int));
    file.read(reinterpret_cast<char*>(&version), sizeof(int));

    if (magic != NAVMESH_FILE_MAGIC) {
        TraceLog(LOG_ERROR, "NavMesh: Invalid file format");
        file.close();
        return false;
    }

    if (version != NAVMESH_FILE_VERSION) {
        TraceLog(LOG_WARNING, "NavMesh: Version mismatch (file: %d, expected: %d)", version, NAVMESH_FILE_VERSION);
        file.close();
        return false;
    }

    // Leggi parametri navmesh
    dtNavMeshParams params;
    file.read(reinterpret_cast<char*>(&params), sizeof(dtNavMeshParams));

    // Leggi bounds
    file.read(reinterpret_cast<char*>(m_boundsMin), sizeof(float) * 3);
    file.read(reinterpret_cast<char*>(m_boundsMax), sizeof(float) * 3);

    // Cleanup navmesh esistente
    if (m_navMesh) {
        dtFreeNavMesh(m_navMesh);
        m_navMesh = nullptr;
    }

    // Crea nuova navmesh
    m_navMesh = dtAllocNavMesh();
    if (!m_navMesh) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate navmesh");
        file.close();
        return false;
    }

    dtStatus status = m_navMesh->init(&params);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init navmesh from file");
        dtFreeNavMesh(m_navMesh);
        m_navMesh = nullptr;
        file.close();
        return false;
    }

    // Init query
    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init navmesh query");
        dtFreeNavMesh(m_navMesh);
        m_navMesh = nullptr;
        file.close();
        return false;
    }

    // Leggi numero di tiles
    int numTiles;
    file.read(reinterpret_cast<char*>(&numTiles), sizeof(int));

    m_tileCount = 0;
    m_totalPolygons = 0;

    // Leggi ogni tile
    for (int i = 0; i < numTiles; i++) {
        // Leggi tile reference (non usata per l'aggiunta)
        dtTileRef tileRef;
        file.read(reinterpret_cast<char*>(&tileRef), sizeof(dtTileRef));

        // Leggi dimensione dati
        int dataSize;
        file.read(reinterpret_cast<char*>(&dataSize), sizeof(int));

        if (dataSize <= 0) {
            TraceLog(LOG_WARNING, "NavMesh: Invalid tile data size");
            continue;
        }

        // Alloca e leggi dati tile
        unsigned char* data = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        if (!data) {
            TraceLog(LOG_ERROR, "NavMesh: Failed to allocate tile data");
            continue;
        }

        file.read(reinterpret_cast<char*>(data), dataSize);

        // Aggiungi tile
        dtTileRef resultRef;
        status = m_navMesh->addTile(data, dataSize, DT_TILE_FREE_DATA, 0, &resultRef);
        if (dtStatusFailed(status)) {
            dtFree(data);
            TraceLog(LOG_WARNING, "NavMesh: Failed to add tile %d", i);
            continue;
        }

        m_tileCount++;

        // Conta poligoni
        const dtMeshTile* tile = m_navMesh->getTileByRef(resultRef);
        if (tile && tile->header) {
            m_totalPolygons += tile->header->polyCount;
        }
    }

    file.close();

    // Reset debug mesh
    m_debugMeshBuilt = false;
    cleanupTileDebugData();

    TraceLog(LOG_INFO, "NavMesh: Loaded from %s (%d tiles, %d polygons)",
             filename.c_str(), m_tileCount, m_totalPolygons);

    return m_tileCount > 0;
}

// ============================================
// Obstacle management (using dtTileCache)
// ============================================

dtObstacleRef NavMesh::addObstacle(BoundingBox bounds) {
    if (!m_tileCache) {
        TraceLog(LOG_ERROR, "NavMesh: Tile cache not initialized - cannot add obstacle");
        return 0;
    }

    // Convert raylib BoundingBox to float arrays
    float bmin[3] = { bounds.min.x, bounds.min.y, bounds.min.z };
    float bmax[3] = { bounds.max.x, bounds.max.y, bounds.max.z };

    // Verify the obstacle is within the navmesh bounds
    if (bmax[0] < m_boundsMin[0] || bmin[0] > m_boundsMax[0] ||
        bmax[2] < m_boundsMin[2] || bmin[2] > m_boundsMax[2]) {
        TraceLog(LOG_WARNING, "NavMesh: Obstacle at (%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f) is outside navmesh bounds (%.1f,%.1f,%.1f)-(%.1f,%.1f,%.1f)",
                 bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2],
                 m_boundsMin[0], m_boundsMin[1], m_boundsMin[2],
                 m_boundsMax[0], m_boundsMax[1], m_boundsMax[2]);
    }

    // Log current obstacle count
    int obstacleCount = m_tileCache->getObstacleCount();
    TraceLog(LOG_INFO, "NavMesh: Adding obstacle (current count: %d, max: %d)",
             obstacleCount, m_tileCache->getParams()->maxObstacles);

    // Log which tiles this obstacle will affect
    std::vector<TileCoord> affectedTiles = getAffectedTiles(bounds);
    TraceLog(LOG_INFO, "NavMesh: Obstacle will affect %d tiles:", (int)affectedTiles.size());
    for (const auto& tc : affectedTiles) {
        TraceLog(LOG_INFO, "  - Tile (%d, %d)", tc.x, tc.y);
    }

    dtObstacleRef ref = 0;
    dtStatus status = m_tileCache->addBoxObstacle(bmin, bmax, &ref);

    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to add obstacle at (%.1f,%.1f,%.1f) - (%.1f,%.1f,%.1f), status=%u",
                 bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2], status);
        return 0;
    }

    TraceLog(LOG_INFO, "NavMesh: Added obstacle ref=%u at (%.1f,%.1f,%.1f) - (%.1f,%.1f,%.1f)",
             ref, bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);

    // Check obstacle state
    const dtTileCacheObstacle* ob = m_tileCache->getObstacleByRef(ref);
    if (ob) {
        const char* stateStr = "unknown";
        switch (ob->state) {
            case DT_OBSTACLE_EMPTY: stateStr = "EMPTY"; break;
            case DT_OBSTACLE_PROCESSING: stateStr = "PROCESSING"; break;
            case DT_OBSTACLE_PROCESSED: stateStr = "PROCESSED"; break;
            case DT_OBSTACLE_REMOVING: stateStr = "REMOVING"; break;
        }
        TraceLog(LOG_INFO, "NavMesh: Obstacle state=%s, type=%d, touched=%d, pending=%d",
                 stateStr, ob->type, ob->ntouched, ob->npending);
    }

    // Invalidate debug mesh
    m_debugMeshBuilt = false;

    return ref;
}

bool NavMesh::removeObstacle(dtObstacleRef ref) {
    if (!m_tileCache) {
        TraceLog(LOG_ERROR, "NavMesh: Tile cache not initialized - cannot remove obstacle");
        return false;
    }

    dtStatus status = m_tileCache->removeObstacle(ref);

    if (dtStatusFailed(status)) {
        TraceLog(LOG_WARNING, "NavMesh: Failed to remove obstacle ref=%u", ref);
        return false;
    }

    TraceLog(LOG_INFO, "NavMesh: Removed obstacle ref=%u", ref);

    // Invalidate debug mesh
    m_debugMeshBuilt = false;

    return true;
}

void NavMesh::update(float dt) {
    if (!m_tileCache || !m_navMesh) {
        return;
    }

    // Process all pending tile cache operations
    // The update function only processes one request at a time, so we loop
    const int MAX_UPDATES = 100; // Safety limit
    int updateCount = 0;
    bool upToDate = false;

    while (!upToDate && updateCount < MAX_UPDATES) {
        dtStatus status = m_tileCache->update(dt, m_navMesh, &upToDate);

        if (dtStatusFailed(status)) {
            TraceLog(LOG_WARNING, "NavMesh: TileCache update failed with status %u", status);
            break;
        }

        updateCount++;
    }

    // Log if we processed any updates
    if (updateCount > 0) {
        TraceLog(LOG_INFO, "NavMesh: Processed %d tile cache updates (upToDate: %s)",
                 updateCount, upToDate ? "yes" : "no");
        m_debugMeshBuilt = false; // Force debug mesh rebuild
    }
}

std::vector<TileCoord> NavMesh::getAffectedTiles(BoundingBox bounds) const {
    std::vector<TileCoord> tiles;

    if (m_tileSize <= 0) return tiles;

    // Get tile coordinates for min and max corners
    TileCoord minTile = getTileCoordAt(bounds.min);
    TileCoord maxTile = getTileCoordAt(bounds.max);

    // Enumerate all tiles in the range
    for (int x = minTile.x; x <= maxTile.x; x++) {
        for (int y = minTile.y; y <= maxTile.y; y++) {
            // Check if tile is within valid range
            if (x >= 0 && x < m_tilesX && y >= 0 && y < m_tilesZ) {
                tiles.push_back({x, y});
            }
        }
    }

    return tiles;
}

} // namespace moiras
