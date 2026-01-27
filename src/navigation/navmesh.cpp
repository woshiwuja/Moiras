#include "navmesh.h"
	#include <rlgl.h>
#include "DetourNavMeshBuilder.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace moiras {

NavMesh::NavMesh() : m_ctx(nullptr), m_navMesh(nullptr), m_navQuery(nullptr) {
    m_ctx = new rcContext();
    m_navQuery = dtAllocNavMeshQuery();
}

NavMesh::~NavMesh() {
    if (m_navMesh) dtFreeNavMesh(m_navMesh);
    if (m_navQuery) dtFreeNavMeshQuery(m_navQuery);
    delete m_ctx;
}

bool NavMesh::build(const Mesh& mesh, Matrix transform) {
    if (mesh.vertexCount == 0) {
        TraceLog(LOG_ERROR, "NavMesh: Mesh has no vertices");
        return false;
    }

    TraceLog(LOG_INFO, "NavMesh: Building from mesh with %d vertices, %d triangles", 
             mesh.vertexCount, mesh.triangleCount);

    // Trasforma i vertici
    std::vector<float> transformedVerts(mesh.vertexCount * 3);
    for (int i = 0; i < mesh.vertexCount; i++) {
        Vector3 v = {
            mesh.vertices[i * 3 + 0],
            mesh.vertices[i * 3 + 1],
            mesh.vertices[i * 3 + 2]
        };
        
        // Applica la trasformazione
        v = Vector3Transform(v, transform);
        
        transformedVerts[i * 3 + 0] = v.x;
        transformedVerts[i * 3 + 1] = v.y;
        transformedVerts[i * 3 + 2] = v.z;
    }
    rcConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.cs = m_cellSize;
    cfg.ch = m_cellHeight;
    cfg.walkableSlopeAngle = m_agentMaxSlope;
    cfg.walkableHeight = (int)ceilf(m_agentHeight / cfg.ch);
    cfg.walkableClimb = (int)floorf(m_agentMaxClimb / cfg.ch);
    cfg.walkableRadius = (int)ceilf(m_agentRadius / cfg.cs);
    cfg.maxEdgeLen = (int)(12.0f / cfg.cs);
    cfg.maxSimplificationError = 1.5f;           // Era 1.3f - più tolleranza per mappe grandi
    cfg.minRegionArea = (int)rcSqr(12.0f);      // Era 8.0f (64) - ora 144 per filtrare regioni piccole
    cfg.mergeRegionArea = (int)rcSqr(30.0f);    // Era 20.0f (400) - ora 900 per merge migliore
    cfg.maxVertsPerPoly = 6;
    cfg.detailSampleDist = cfg.cs * 6.0f;
    cfg.detailSampleMaxError = cfg.ch * 1.5f;    // Era 1.0f - più tolleranza

    // 2. Calcolo Bounding Box
    float bmin[3], bmax[3];
    rcCalcBounds(transformedVerts.data(), mesh.vertexCount, bmin, bmax);
    rcVcopy(cfg.bmin, bmin);
    rcVcopy(cfg.bmax, bmax);
    rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

    TraceLog(LOG_INFO, "NavMesh: Bounding box: (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
             bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]);
    TraceLog(LOG_INFO, "NavMesh: Map dimensions: %.2f x %.2f x %.2f",
             bmax[0]-bmin[0], bmax[1]-bmin[1], bmax[2]-bmin[2]);
    TraceLog(LOG_INFO, "NavMesh: Grid size %d x %d (total cells: %d)",
             cfg.width, cfg.height, cfg.width * cfg.height);
    TraceLog(LOG_INFO, "NavMesh: Cell size: %.2f, Cell height: %.2f", cfg.cs, cfg.ch);
    TraceLog(LOG_INFO, "NavMesh: Agent - Height: %.2f, Radius: %.2f, MaxClimb: %.2f, MaxSlope: %.2f°",
             m_agentHeight, m_agentRadius, m_agentMaxClimb, m_agentMaxSlope);

    // 3. Conversione Indici
    int triCount = mesh.triangleCount;
    std::vector<int> tris(triCount * 3);
    
    if (mesh.indices != nullptr) {
        for (int i = 0; i < triCount * 3; i++) {
            tris[i] = (int)mesh.indices[i];
        }
    } else {
        // Se non ci sono indici, usa i vertici in ordine
        for (int i = 0; i < triCount * 3; i++) {
            tris[i] = i;
        }
    }

    // 4. Voxelizzazione
    rcHeightfield* hf = rcAllocHeightfield();
    if (!hf) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate heightfield");
        return false;
    }
    
    if (!rcCreateHeightfield(m_ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to create heightfield");
        rcFreeHeightField(hf);
        return false;
    }

    unsigned char* areas = new unsigned char[triCount];
    memset(areas, 0, triCount);
    
    rcMarkWalkableTriangles(m_ctx, cfg.walkableSlopeAngle, transformedVerts.data(), mesh.vertexCount, tris.data(), triCount, areas);
    
    if (!rcRasterizeTriangles(m_ctx, transformedVerts.data(), mesh.vertexCount, tris.data(), areas, triCount, *hf, cfg.walkableClimb)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to rasterize triangles");
        delete[] areas;
        rcFreeHeightField(hf);
        return false;
    }
    delete[] areas;

    // 5. Filtraggio
    rcFilterLowHangingWalkableObstacles(m_ctx, cfg.walkableClimb, *hf);
    rcFilterLedgeSpans(m_ctx, cfg.walkableHeight, cfg.walkableClimb, *hf);
    rcFilterWalkableLowHeightSpans(m_ctx, cfg.walkableHeight, *hf);

    // 6. Compact Heightfield
    rcCompactHeightfield* chf = rcAllocCompactHeightfield();
    if (!chf) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate compact heightfield");
        rcFreeHeightField(hf);
        return false;
    }
    
    if (!rcBuildCompactHeightfield(m_ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build compact heightfield");
        rcFreeHeightField(hf);
        rcFreeCompactHeightfield(chf);
        return false;
    }
    rcFreeHeightField(hf);

    if (!rcErodeWalkableArea(m_ctx, cfg.walkableRadius, *chf)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to erode walkable area");
        rcFreeCompactHeightfield(chf);
        return false;
    }
    
    if (!rcBuildDistanceField(m_ctx, *chf)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build distance field");
        rcFreeCompactHeightfield(chf);
        return false;
    }
    
    if (!rcBuildRegions(m_ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build regions");
        rcFreeCompactHeightfield(chf);
        return false;
    }

    // 7. Contours
    rcContourSet* cset = rcAllocContourSet();
    if (!cset) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate contour set");
        rcFreeCompactHeightfield(chf);
        return false;
    }
    
    if (!rcBuildContours(m_ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build contours");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return false;
    }

    // 8. PolyMesh
    rcPolyMesh* pmesh = rcAllocPolyMesh();
    if (!pmesh) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate poly mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        return false;
    }
    
    if (!rcBuildPolyMesh(m_ctx, *cset, cfg.maxVertsPerPoly, *pmesh)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build poly mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return false;
    }

    // 9. Detail Mesh
    rcPolyMeshDetail* dmesh = rcAllocPolyMeshDetail();
    if (!dmesh) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate detail mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        return false;
    }
    
    if (!rcBuildPolyMeshDetail(m_ctx, *pmesh, *chf, cfg.detailSampleDist, cfg.detailSampleMaxError, *dmesh)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to build detail mesh");
        rcFreeCompactHeightfield(chf);
        rcFreeContourSet(cset);
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    rcFreeCompactHeightfield(chf);
    rcFreeContourSet(cset);

    // 10. Setup polygon flags
    for (int i = 0; i < pmesh->npolys; i++) {
        pmesh->flags[i] = 1;  // Walkable
    }

    // 11. Creazione NavMesh Data per Detour
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
    rcVcopy(params.bmin, pmesh->bmin);
    rcVcopy(params.bmax, pmesh->bmax);
    params.cs = cfg.cs;
    params.ch = cfg.ch;
    params.buildBvTree = true;

    unsigned char* navData = nullptr;
    int navDataSize = 0;
    
    if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to create Detour navmesh data");
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    // 12. Inizializza NavMesh
    m_navMesh = dtAllocNavMesh();
    if (!m_navMesh) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to allocate Detour navmesh");
        dtFree(navData);
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }
    
    dtStatus status = m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init Detour navmesh");
        dtFree(navData);
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    // 13. Inizializza Query
    status = m_navQuery->init(m_navMesh, 2048);
    if (dtStatusFailed(status)) {
        TraceLog(LOG_ERROR, "NavMesh: Failed to init Detour query");
        rcFreePolyMesh(pmesh);
        rcFreePolyMeshDetail(dmesh);
        return false;
    }

    // Salva per debug drawing
    m_polyMesh = pmesh;
    m_polyMeshDetail = dmesh;

    TraceLog(LOG_INFO, "NavMesh: Built successfully with %d polys", pmesh->npolys);
    return true;
}

std::vector<Vector3> NavMesh::findPath(Vector3 start, Vector3 end) {
    std::vector<Vector3> pathPoints;

    if (!m_navMesh || !m_navQuery) {
        TraceLog(LOG_WARNING, "NavMesh: NavMesh not initialized");
        return pathPoints;
    }

    float sPos[3] = { start.x, start.y, start.z };
    float ePos[3] = { end.x, end.y, end.z };
    // Aumentati gli extents per trovare poligoni più lontani, specialmente in verticale
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
        TraceLog(LOG_INFO, "NavMesh: Start polygon found: %s, End polygon found: %s",
                 startRef ? "YES" : "NO", endRef ? "YES" : "NO");
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
        // Mappe piccole (< 500 unità)
        m_cellSize = 0.2f;
        m_cellHeight = 0.2f;
        m_agentRadius = 0.5f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for SMALL map (< 500)");
    } else if (mapSize < 2000.0f) {
        // Mappe medie (500-2000 unità)
        m_cellSize = 0.3f;
        m_cellHeight = 0.3f;
        m_agentRadius = 0.6f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for MEDIUM map (500-2000)");
    } else if (mapSize < 5000.0f) {
        // Mappe grandi (2000-5000 unità)
        m_cellSize = 0.5f;
        m_cellHeight = 0.4f;
        m_agentRadius = 0.8f;
        TraceLog(LOG_INFO, "NavMesh: Parameters set for LARGE map (2000-5000)");
    } else {
        // Mappe molto grandi (> 5000 unità)
        m_cellSize = 0.8f;
        m_cellHeight = 0.5f;
        m_agentRadius = 1.0f;
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
        TraceLog(LOG_INFO, "NavMesh: Point projected from (%.2f,%.2f,%.2f) to (%.2f,%.2f,%.2f)",
                 point.x, point.y, point.z, projectedPoint.x, projectedPoint.y, projectedPoint.z);
        return true;
    }

    TraceLog(LOG_WARNING, "NavMesh: Could not project point (%.2f,%.2f,%.2f) to navmesh",
             point.x, point.y, point.z);
    return false;
}

void NavMesh::buildDebugMesh() {
    if (!m_polyMesh || m_polyMesh->npolys == 0) return;
    
    // Conta i vertici e triangoli necessari
    std::vector<Vector3> vertices;
    std::vector<unsigned short> indices;
    std::vector<Color> colors;
    
    for (int i = 0; i < m_polyMesh->npolys; i++) {
        const unsigned short* p = &m_polyMesh->polys[i * m_polyMesh->nvp * 2];
        
        // Trova i vertici del poligono
        std::vector<Vector3> polyVerts;
        for (int j = 0; j < m_polyMesh->nvp; j++) {
            if (p[j] == RC_MESH_NULL_IDX) break;
            
            const unsigned short* v = &m_polyMesh->verts[p[j] * 3];
            float x = m_polyMesh->bmin[0] + v[0] * m_polyMesh->cs;
            float y = m_polyMesh->bmin[1] + (v[1] + 1) * m_polyMesh->ch + 0.2f;
            float z = m_polyMesh->bmin[2] + v[2] * m_polyMesh->cs;
            polyVerts.push_back({x, y, z});
        }
        
        // Triangola il poligono (fan triangulation)
        if (polyVerts.size() >= 3) {
            unsigned short baseIndex = (unsigned short)vertices.size();
            
            for (const auto& v : polyVerts) {
                vertices.push_back(v);
                // Colore verde semi-trasparente
                colors.push_back({0, 200, 0, 100});
            }
            
            // Fan triangulation
            for (size_t j = 1; j < polyVerts.size() - 1; j++) {
                indices.push_back(baseIndex);
                indices.push_back(baseIndex + j);
                indices.push_back(baseIndex + j + 1);
            }
        }
    }
    
    if (vertices.empty()) return;
    
    // Crea la mesh
    Mesh debugMesh = {0};
    debugMesh.vertexCount = vertices.size();
    debugMesh.triangleCount = indices.size() / 3;
    
    debugMesh.vertices = (float*)MemAlloc(vertices.size() * 3 * sizeof(float));
    debugMesh.indices = (unsigned short*)MemAlloc(indices.size() * sizeof(unsigned short));
    debugMesh.colors = (unsigned char*)MemAlloc(vertices.size() * 4 * sizeof(unsigned char));
    
    for (size_t i = 0; i < vertices.size(); i++) {
        debugMesh.vertices[i * 3 + 0] = vertices[i].x;
        debugMesh.vertices[i * 3 + 1] = vertices[i].y;
        debugMesh.vertices[i * 3 + 2] = vertices[i].z;
        
        debugMesh.colors[i * 4 + 0] = colors[i].r;
        debugMesh.colors[i * 4 + 1] = colors[i].g;
        debugMesh.colors[i * 4 + 2] = colors[i].b;
        debugMesh.colors[i * 4 + 3] = colors[i].a;
    }
    
    for (size_t i = 0; i < indices.size(); i++) {
        debugMesh.indices[i] = indices[i];
    }
    
    UploadMesh(&debugMesh, false);
    
    // Cleanup old model
    if (m_debugModel.meshCount > 0) {
        UnloadModel(m_debugModel);
    }
    
    m_debugModel = LoadModelFromMesh(debugMesh);
    m_debugMeshBuilt = true;
    
    TraceLog(LOG_INFO, "NavMesh: Debug mesh built with %d vertices, %d triangles", 
             (int)vertices.size(), (int)indices.size() / 3);
}

void NavMesh::drawDebug() {
    if (!m_debugMeshBuilt) {
        buildDebugMesh();
    }
    
    if (m_debugMeshBuilt && m_debugModel.meshCount > 0) {
        // Disegna con blending per la trasparenza
        rlDisableBackfaceCulling();
        rlEnableColorBlend();
        DrawModel(m_debugModel, {0, 0, 0}, 1.0f, WHITE);
        rlEnableBackfaceCulling();
    }
}

} // namespace moiras
