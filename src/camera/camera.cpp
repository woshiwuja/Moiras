#include "camera.h"
#include "../map/map.h"
#include <limits>
#include <raylib.h>
namespace moiras {
void handleCursor() {
  if (IsKeyPressed(KEY_P)) {
    if (IsCursorHidden())
      EnableCursor();
    else
      DisableCursor();
  }
}
void GameCamera::update() {
  handleCursor();
  cameraControl();
  ray = GetScreenToWorldRay(GetMousePosition(), rcamera);
}

void GameCamera::cameraControl() {
  if (updateMode == 0) {
    UpdateCamera(&rcamera, mode);
    return;
  }

  float dt = GetFrameTime();
  Vector2 mouseDelta = GetMouseDelta();

  // Clamp mouse delta to prevent jumps
  mouseDelta.x = Clamp(mouseDelta.x, -300.0f, 300.0f);
  mouseDelta.y = Clamp(mouseDelta.y, -300.0f, 300.0f);

  // Calculate current camera distance from target for speed scaling
  Vector3 cameraOffset = Vector3Subtract(rcamera.position, rcamera.target);
  float currentDistance = Vector3Length(cameraOffset);

  // Camera pan with WASD (move camera position)
  Vector3 forward =
      Vector3Normalize(Vector3Subtract(rcamera.target, rcamera.position));
  Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, rcamera.up));

  // Zero out vertical component for horizontal movement
  forward.y = 0;
  forward = Vector3Normalize(forward);
  right.y = 0;
  right = Vector3Normalize(right);

  // Scale pan speed based on distance from target
  float basePanSpeed = 10.0f;
  float minDistance = 5.0f;
  float maxDistance = 500.0f;
  float speedMultiplier =
      currentDistance / minDistance; // Faster when zoomed out
  float panSpeed = basePanSpeed * speedMultiplier * dt;

  Vector3 movement = {0};

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
    movement = Vector3Add(movement, Vector3Scale(forward, panSpeed));
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
    movement = Vector3Add(movement, Vector3Scale(forward, -panSpeed));
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
    movement = Vector3Add(movement, Vector3Scale(right, panSpeed));
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
    movement = Vector3Add(movement, Vector3Scale(right, -panSpeed));

  rcamera.position = Vector3Add(rcamera.position, movement);
  rcamera.target = Vector3Add(rcamera.target, movement);

  // Aggiorna anche il pivot di rotazione se stiamo ruotando
  if (isRotating)
  {
    rotationPivot = Vector3Add(rotationPivot, movement);
  }
  // Middle mouse button rotation around target
  if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
    // Al primo frame del click, trova il nuovo target al centro dello schermo
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
      Vector2 screenCenter = {GetScreenWidth() / 2.0f,
                              GetScreenHeight() / 2.0f};
      Ray centerRay = GetScreenToWorldRay(screenCenter, rcamera);

      RayCollision meshHit = {0};
      meshHit.hit = false;
      meshHit.distance = std::numeric_limits<float>::max();

      auto mapObj = getParent()->getChildOfType<Map>();
      if (mapObj) {
        Model model = mapObj->model;
        for (int m = 0; m < model.meshCount; m++) {
          auto hitInfo =
              GetRayCollisionMesh(centerRay, model.meshes[m], model.transform);
          if (hitInfo.hit && hitInfo.distance < meshHit.distance) {
            meshHit = hitInfo;
          }
        }
      }

      if (meshHit.hit) {
        rcamera.target = meshHit.point;
        rotationPivot = meshHit.point;
        isRotating = true;
      }
    }

    float rotationSpeed = 0.003f;

    // Horizontal rotation (around Y axis)
    float angleH = -mouseDelta.x * rotationSpeed;

    // Calculate camera offset from target
    Vector3 offset = Vector3Subtract(rcamera.position, rcamera.target);
    float distance = Vector3Length(offset);

    // Rotate offset around Y axis
    Matrix rotation = MatrixRotateY(angleH);
    offset = Vector3Transform(offset, rotation);

    // Vertical rotation (pitch) - limit to prevent flipping
    float angleV = mouseDelta.y * rotationSpeed;
    Vector3 axis = Vector3Normalize(Vector3CrossProduct(offset, rcamera.up));

    // Calculate current pitch angle to clamp it
    Vector3 normalizedOffset = Vector3Normalize(offset);
    float currentPitch = asinf(normalizedOffset.y);
    float newPitch = currentPitch + angleV;

    // Clamp pitch between -10 and 80 degrees
    float minPitch = -10.0f * DEG2RAD;
    float maxPitch = 80.0f * DEG2RAD;

    if (newPitch > minPitch && newPitch < maxPitch) {
      Matrix pitchRotation = MatrixRotate(axis, angleV);
      offset = Vector3Transform(offset, pitchRotation);
    }

    // Maintain distance and update position
    offset = Vector3Scale(Vector3Normalize(offset), distance);
    rcamera.position = Vector3Add(rcamera.target, offset);
  } else {
    isRotating = false;
  }

  // Mouse wheel zoom (move camera closer/farther from target)
  float wheel = GetMouseWheelMove();
  if (fabsf(wheel) > 0.01f)
  {
    Vector3 zoomTarget;

    // Usa sempre il centro dello schermo per lo zoom
    Vector2 screenCenter = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    Ray centerRay = GetScreenToWorldRay(screenCenter, rcamera);

    // Solo quando zoomiamo IN, usiamo il raycast verso la mesh
    if (wheel > 0)
    {
      RayCollision meshHit = {0};
      meshHit.hit = false;
      meshHit.distance = std::numeric_limits<float>::max();

      auto mapObj = getParent()->getChildOfType<Map>();
      if (mapObj)
      {
        Model model = mapObj->model;
        for (int m = 0; m < model.meshCount; m++)
        {
          auto hitInfo =
              GetRayCollisionMesh(centerRay, model.meshes[m], model.transform);
          if (hitInfo.hit && hitInfo.distance < meshHit.distance)
          {
            meshHit = hitInfo;
          }
        }
      }

      if (meshHit.hit)
      {
        zoomTarget = meshHit.point;
      }
      else
      {
        zoomTarget = rcamera.target;
      }
    }
    else
    {
      // Zoom OUT: usa sempre il target corrente della camera
      zoomTarget = rcamera.target;
    }

    // Calcola direzione dalla camera al punto di zoom
    Vector3 zoomDirection =
        Vector3Normalize(Vector3Subtract(zoomTarget, rcamera.position));

    // Scala la velocità in base alla distanza
    float distanceToTarget = Vector3Distance(rcamera.position, zoomTarget);
    float zoomSpeed = distanceToTarget * 0.1f;

    // Muovi la camera
    Vector3 movement = Vector3Scale(zoomDirection, wheel * zoomSpeed);
    rcamera.position = Vector3Add(rcamera.position, movement);

    // Aggiorna il target solo quando zoomiamo IN
    if (wheel > 0) {
      rcamera.target = Vector3Add(rcamera.target, Vector3Scale(movement, 0.5f));
    }

    // Clamp distanza dal target della CAMERA (non zoomTarget)
    float minDistance = 5.0f;
    float maxDistance = 500.0f;

    Vector3 offset = Vector3Subtract(rcamera.position, rcamera.target);
    float distance = Vector3Length(offset);

    if (distance < minDistance) {
      rcamera.position = Vector3Add(
          rcamera.target, Vector3Scale(Vector3Normalize(offset), minDistance));
    } else if (distance > maxDistance) {
      rcamera.position = Vector3Add(
          rcamera.target, Vector3Scale(Vector3Normalize(offset), maxDistance));
    }

    // Assicurati che la camera guardi sempre verso il basso (previene flip)
    Vector3 toTarget = Vector3Subtract(rcamera.target, rcamera.position);
    if (toTarget.y > -0.1f) {
      // Se la camera sta per guardare verso l'alto, correggi
      rcamera.target.y = rcamera.position.y - 0.1f;
    }
  }

  // Prevent camera from going through ground with raycast
  // This MUST be done AFTER all camera movements (pan, rotate, zoom)
  Ray ray;
  ray.position = (Vector3){rcamera.position.x, rcamera.position.y + 1000.0f,
                           rcamera.position.z};
  ray.direction = (Vector3){0.0f, -1.0f, 0.0f};

  RayCollision collision = {0};
  collision.hit = false;
  collision.distance = std::numeric_limits<float>::max();

  auto model = getParent()->getChildOfType<Map>()->model;
  for (int m = 0; m < model.meshCount; m++) {
    auto hitInfo = GetRayCollisionMesh(ray, model.meshes[m], model.transform);
    if (hitInfo.hit) {
      if (hitInfo.distance < collision.distance) {
        collision = hitInfo;
      }
    }
  }

  if (collision.hit) {
    float minHeightAboveGround = 1.0f;
    float groundHeight = collision.point.y;
    if (rcamera.position.y < groundHeight + minHeightAboveGround) {
      // Store the original offset between camera and target
      Vector3 originalOffset =
          Vector3Subtract(rcamera.position, rcamera.target);

      // Push camera up to be above ground
      rcamera.position.y = groundHeight + minHeightAboveGround;

      // Recalculate target to maintain the same viewing angle and distance
      rcamera.target = Vector3Subtract(rcamera.position, originalOffset);
    }
  }
}
void GameCamera::draw() {
  if (isRotating) {
    // Calcola la scala in base alla distanza dalla camera
    float distance = Vector3Distance(rcamera.position, rotationPivot);
    float scale = distance * 0.02f; // 2% della distanza

    // Posizione del cono (leggermente sopra il pivot)
    Vector3 conePos = {rotationPivot.x, rotationPivot.y + scale * 2.0f,
                       rotationPivot.z};

    // Disegna cono che punta verso il basso
    DrawCylinderEx(conePos,                 // top
                   rotationPivot,           // bottom (punta)
                   scale,                   // top radius
                   0.0f,                    // bottom radius (punta)
                   8,                       // slices
                   ColorAlpha(YELLOW, 0.8f) // colore
    );

    // Linea verticale per maggiore visibilità
    DrawLine3D(
        {rotationPivot.x, rotationPivot.y + scale * 3.0f, rotationPivot.z},
        rotationPivot, YELLOW);

    // Cerchio alla base
    DrawCircle3D(rotationPivot, scale * 0.5f, {1, 0, 0}, 90.0f,
                 ColorAlpha(YELLOW, 0.5f));
  }
};
} // namespace moiras
