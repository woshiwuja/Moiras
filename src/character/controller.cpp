#include "controller.h"
#include "../input/input_manager.h"
#include "../map/map.h"
#include "../time/time_manager.h"
#include <raymath.h>
#include <limits>

namespace moiras {

CharacterController::CharacterController(Character* character, NavMesh* navMesh, const Model* groundModel)
    : m_character(character)
    , m_navMesh(navMesh)
    , m_groundModel(groundModel)
    , m_currentPathIndex(0)
    , m_isMoving(false)
    , m_movementSpeed(5.0f)
    , m_targetPoint{0, 0, 0}
    , m_hasTarget(false)
    , m_waypointThreshold(0.5f)
{
    TraceLog(LOG_INFO, "CharacterController: Created for character '%s'", character->name.c_str());

    // Snap del character sulla mesh geometrica se disponibile
    if (m_character && m_groundModel) {
        m_character->snapToGround(*m_groundModel);
        TraceLog(LOG_INFO, "CharacterController: Character snapped to ground at (%.2f,%.2f,%.2f)",
                 m_character->position.x, m_character->position.y, m_character->position.z);
    }
    // Altrimenti usa la proiezione sulla navmesh
    else if (m_character && m_navMesh) {
        Vector3 projectedPos;
        if (m_navMesh->projectPointToNavMesh(m_character->position, projectedPos)) {
            m_character->position = projectedPos;
            TraceLog(LOG_INFO, "CharacterController: Character snapped to navmesh at (%.2f,%.2f,%.2f)",
                     projectedPos.x, projectedPos.y, projectedPos.z);
        }
    }
}

CharacterController::~CharacterController() {
    TraceLog(LOG_INFO, "CharacterController: Destroyed");
}

void CharacterController::update(GameCamera* camera) {
    if (!m_character || !m_navMesh || !camera) {
        return;
    }

    // Gestisce il click del mouse per impostare il target
    handleMouseClick(camera);

    // Muove il character lungo il path
    if (m_isMoving) {
        followPath();
        // Update character animation while moving
        m_character->updateAnimation();
    }
}

void CharacterController::handleMouseClick(GameCamera* camera) {
    // Use InputManager to check for character movement action
    // InputManager automatically handles ImGui capture checking
    InputManager& input = InputManager::getInstance();
    
    // Right mouse button: hold to follow cursor
    if (input.isActionActive(InputAction::CHARACTER_MOVE)) {
        Vector3 hitPoint;

        if (raycastToNavMesh(camera, hitPoint)) {
            // Aggiorna il target solo se è cambiato significativamente
            // per evitare ricalcoli continui del path
            float distanceFromLastTarget = Vector3Distance(hitPoint, m_targetPoint);

            if (!m_hasTarget || distanceFromLastTarget > 1.0f) {
                TraceLog(LOG_INFO, "CharacterController: Target updated at (%.2f, %.2f, %.2f)",
                         hitPoint.x, hitPoint.y, hitPoint.z);

                m_targetPoint = hitPoint;
                m_hasTarget = true;

                // Calcola il path dalla posizione corrente al nuovo target
                calculatePath(hitPoint);
            }
        } else {
            TraceLog(LOG_WARNING, "CharacterController: Failed to raycast to navmesh");
        }
    }

    // Quando si rilascia il tasto, il character continua a muoversi verso l'ultimo target
}

bool CharacterController::raycastToNavMesh(GameCamera* camera, Vector3& hitPoint) {
    if (!camera) {
        return false;
    }

    // Ottiene il ray dalla camera
    Ray ray = camera->getRay();

    // Fa il raycast contro la mesh della mappa
    // Dobbiamo fare il raycast contro la geometria della mappa
    // per trovare il punto di intersezione 3D

    RayCollision closestHit = {0};
    closestHit.hit = false;
    closestHit.distance = std::numeric_limits<float>::max();

    // Ottiene il modello della mappa dal parent della camera
    auto mapObj = camera->getParent()->getChildOfType<Map>();
    if (mapObj) {
        const Model& model = mapObj->model;  // Reference to avoid VRAM copy

        // Testa il ray contro tutte le mesh del modello
        for (int m = 0; m < model.meshCount; m++) {
            RayCollision hit = GetRayCollisionMesh(ray, model.meshes[m], model.transform);

            if (hit.hit && hit.distance < closestHit.distance) {
                closestHit = hit;
            }
        }
    }

    if (closestHit.hit) {
        // Proietta il punto sulla navmesh per assicurarci che sia valido
        Vector3 projectedPoint;
        if (m_navMesh && m_navMesh->projectPointToNavMesh(closestHit.point, projectedPoint)) {
            hitPoint = projectedPoint;
            return true;
        } else {
            // Se non riesce a proiettare, usa comunque il punto del raycast
            TraceLog(LOG_WARNING, "CharacterController: Could not project hit point to navmesh, using raw point");
            hitPoint = closestHit.point;
            return true;
        }
    }

    return false;
}

void CharacterController::calculatePath(Vector3 targetPos) {
    if (!m_character || !m_navMesh) {
        return;
    }

    Vector3 startPos = m_character->position;

    // Usa NavMesh::findPath per calcolare il percorso
    m_currentPath = m_navMesh->findPath(startPos, targetPos);

    if (m_currentPath.size() > 0) {
        m_currentPathIndex = 0;
        m_isMoving = true;

        // Start the Running animation
        if (m_character->setAnimation("Running")) {
            m_character->playAnimation();
        }

        TraceLog(LOG_INFO, "CharacterController: Path calculated with %d waypoints",
                 (int)m_currentPath.size());
    } else {
        m_isMoving = false;
        m_character->stopAnimation();
        TraceLog(LOG_WARNING, "CharacterController: Failed to find path");
    }
}

void CharacterController::followPath() {
    if (!m_isMoving || m_currentPath.empty() || m_currentPathIndex >= m_currentPath.size()) {
        stop();
        return;
    }

    Vector3 currentWaypoint = m_currentPath[m_currentPathIndex];
    Vector3 currentPos = m_character->position;

    // Calcola la direzione verso il waypoint corrente
    Vector3 direction = Vector3Subtract(currentWaypoint, currentPos);

    // Ignora la componente Y per il movimento orizzontale
    direction.y = 0;
    float distance = Vector3Length(direction);

    // Se siamo vicini al waypoint corrente, passa al prossimo
    if (distance < m_waypointThreshold) {
        m_currentPathIndex++;

        // Se abbiamo raggiunto l'ultimo waypoint, ferma il movimento
        if (m_currentPathIndex >= m_currentPath.size()) {
            stop();
            TraceLog(LOG_INFO, "CharacterController: Target reached!");
            return;
        }

        return;
    }

    // Normalizza la direzione
    direction = Vector3Normalize(direction);

    // Muove il character
    float moveDistance = m_movementSpeed * TimeManager::getInstance().getGameDeltaTime();
    Vector3 movement = Vector3Scale(direction, moveDistance);

    m_character->position = Vector3Add(m_character->position, movement);

    // Snap continuo alla geometria per seguire pendenze/lati
    if (m_groundModel) {
        m_character->snapToGround(*m_groundModel);
    }

    // Calcola la rotazione del character verso la direzione di movimento
    // Usiamo atan2 per calcolare l'angolo di rotazione intorno all'asse Y
    float angle = atan2f(direction.x, direction.z) * RAD2DEG;
    m_character->eulerRot.y = angle;
}

void CharacterController::stop() {
    m_isMoving = false;
    m_currentPath.clear();
    m_currentPathIndex = 0;

    // Stop the animation when the character stops
    if (m_character) {
        m_character->stopAnimation();
    }
}

void CharacterController::drawDebug() {
    if (!m_character) {
        return;
    }

    // Disegna il path corrente
    if (!m_currentPath.empty()) {
        for (size_t i = 0; i < m_currentPath.size(); i++) {
            Vector3 point = m_currentPath[i];

            // Disegna una sfera ad ogni waypoint
            Color color = (i == m_currentPathIndex) ? GREEN : BLUE;
            DrawSphere(point, 0.2f, color);

            // Disegna una linea tra i waypoint
            if (i > 0) {
                DrawLine3D(m_currentPath[i - 1], point, YELLOW);
            }
        }

        // Disegna una linea dalla posizione corrente al prossimo waypoint
        if (m_currentPathIndex < m_currentPath.size()) {
            DrawLine3D(m_character->position, m_currentPath[m_currentPathIndex], RED);
        }
    }

    // Disegna il target point
    if (m_hasTarget) {
        DrawSphere(m_targetPoint, 0.3f, RED);
        DrawCircle3D(m_targetPoint, 0.5f, {1, 0, 0}, 90.0f, ColorAlpha(RED, 0.3f));
    }
}

} // namespace moiras
