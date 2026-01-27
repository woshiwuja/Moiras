#pragma once

#include "character.h"
#include "../navigation/navmesh.h"
#include "../camera/camera.h"
#include <raylib.h>
#include <vector>

namespace moiras {

/**
 * CharacterController gestisce il movimento del character sulla navmesh
 * tramite raycast dalla camera.
 */
class CharacterController {
public:
    CharacterController(Character* character, NavMesh* navMesh, const Model* groundModel = nullptr);
    ~CharacterController();

    /**
     * Aggiorna il controller: gestisce input e movimento
     * @param camera La camera per fare il raycast
     */
    void update(GameCamera* camera);

    /**
     * Disegna debug info (path, target point, etc.)
     */
    void drawDebug();

    /**
     * Imposta la velocità di movimento del character
     */
    void setMovementSpeed(float speed) { m_movementSpeed = speed; }

    /**
     * Ottiene la velocità di movimento corrente
     */
    float getMovementSpeed() const { return m_movementSpeed; }

    /**
     * Verifica se il character sta muovendosi
     */
    bool isMoving() const { return m_isMoving; }

    /**
     * Ferma il movimento del character
     */
    void stop();

private:
    Character* m_character;
    NavMesh* m_navMesh;

    // Path corrente
    std::vector<Vector3> m_currentPath;
    int m_currentPathIndex;
    bool m_isMoving;

    // Velocità di movimento
    float m_movementSpeed;

    // Target point sulla navmesh
    Vector3 m_targetPoint;
    bool m_hasTarget;

    // Distanza minima per considerare raggiunto un waypoint
    float m_waypointThreshold;

    /**
     * Gestisce il click del mouse per impostare il target
     * @param camera La camera per fare il raycast
     */
    void handleMouseClick(GameCamera* camera);

    /**
     * Fa il raycast dalla camera alla navmesh
     * @param camera La camera da cui parte il ray
     * @param hitPoint Punto di intersezione con la navmesh (output)
     * @return true se ha colpito la navmesh
     */
    bool raycastToNavMesh(GameCamera* camera, Vector3& hitPoint);

    /**
     * Muove il character lungo il path corrente
     */
    void followPath();

    /**
     * Calcola il path dalla posizione corrente al target
     */
    void calculatePath(Vector3 targetPos);
};

} // namespace moiras
