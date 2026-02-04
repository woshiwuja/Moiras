#include "character.h"
#include <limits>
#include <raylib.h>
#include "../gui/inventory.hpp"
#include "../time/time_manager.h"
#include <raymath.h>

namespace moiras
{

    // Definizione dello shader statico
    Shader Character::sharedShader = {0};

    void Character::setSharedShader(Shader shader)
    {
        sharedShader = shader;
        TraceLog(LOG_INFO, "Character shared shader set, ID: %d", shader.id);
    }

    void Character::applyShader(Shader shader)
    {
        if (modelInstance.isValid() && shader.id > 0)
        {
            modelInstance.applyShader(shader);
            TraceLog(LOG_INFO, "Applied shader ID %d to character '%s' (%d materials)",
                     shader.id, name.c_str(), modelInstance.materialCount());
        }
    }

    Character::Character()
        : health(100), scale(1.0f), name("Character"), isVisible(true),
          quat_rotation{1.0, 0.0, 0.0, 0.0}, eulerRot{0, 0, 0}
    {
        quat_rotation = QuaternionFromEuler(eulerRot.x, eulerRot.y, eulerRot.z);
        auto inventory = std::make_unique<Inventory>(10, 6);
        inventory.get()->setName("Inventory");
        addChild(std::move(inventory));
    }

    Character::~Character()
    {
        TraceLog(LOG_INFO, "Destroying character: %s", name.c_str());
        // ModelInstance destructor handles cleanup automatically
        unloadAnimations();
    }

    Character::Character(Character &&other) noexcept
        : GameObject(std::move(other)), health(other.health),
          name(std::move(other.name)), eulerRot(other.eulerRot),
          isVisible(other.isVisible), scale(other.scale),
          modelInstance(std::move(other.modelInstance)),
          quat_rotation(other.quat_rotation),
          m_animations(other.m_animations),
          m_animationCount(other.m_animationCount),
          m_currentAnimIndex(other.m_currentAnimIndex),
          m_currentFrame(other.m_currentFrame),
          m_lastUpdatedFrame(other.m_lastUpdatedFrame),
          m_animationTimer(other.m_animationTimer),
          m_isAnimating(other.m_isAnimating)
    {
        // Clear other's animation pointers to prevent double-free
        other.m_animations = nullptr;
        other.m_animationCount = 0;
        TraceLog(LOG_INFO, "Character moved: %s", name.c_str());
    }

    Character &Character::operator=(Character &&other) noexcept
    {
        if (this != &other)
        {
            GameObject::operator=(std::move(other));

            // Unload existing animations before moving
            unloadAnimations();

            health = other.health;
            name = std::move(other.name);
            eulerRot = other.eulerRot;
            isVisible = other.isVisible;
            scale = other.scale;
            modelInstance = std::move(other.modelInstance);
            quat_rotation = other.quat_rotation;

            // Move animation data
            m_animations = other.m_animations;
            m_animationCount = other.m_animationCount;
            m_currentAnimIndex = other.m_currentAnimIndex;
            m_currentFrame = other.m_currentFrame;
            m_lastUpdatedFrame = other.m_lastUpdatedFrame;
            m_animationTimer = other.m_animationTimer;
            m_isAnimating = other.m_isAnimating;

            // Clear other's animation pointers
            other.m_animations = nullptr;
            other.m_animationCount = 0;

            TraceLog(LOG_INFO, "Character move-assigned: %s", name.c_str());
        }
        return *this;
    }

    void Character::loadModel(ModelManager &manager, const std::string &path)
    {
        TraceLog(LOG_INFO, "Loading model: %s", path.c_str());

        // Release old model instance if any
        modelInstance = ModelInstance();

        // Acquire new model instance from manager
        modelInstance = manager.acquire(path);

        if (modelInstance.isValid())
        {
            TraceLog(LOG_INFO, "Model loaded: %d meshes, %d materials",
                     modelInstance.meshCount(), modelInstance.materialCount());

            // Debug: stampa info materiali
            for (int i = 0; i < modelInstance.materialCount(); i++)
            {
                Material &mat = modelInstance.materials()[i];
                TraceLog(LOG_INFO, "Material %d:", i);
                TraceLog(LOG_INFO, "  - Albedo texture ID: %d", mat.maps[MATERIAL_MAP_ALBEDO].texture.id);
                TraceLog(LOG_INFO, "  - Normal texture ID: %d", mat.maps[MATERIAL_MAP_NORMAL].texture.id);
                TraceLog(LOG_INFO, "  - Metalness texture ID: %d", mat.maps[MATERIAL_MAP_METALNESS].texture.id);
                TraceLog(LOG_INFO, "  - Albedo color: %d,%d,%d,%d",
                         mat.maps[MATERIAL_MAP_ALBEDO].color.r,
                         mat.maps[MATERIAL_MAP_ALBEDO].color.g,
                         mat.maps[MATERIAL_MAP_ALBEDO].color.b,
                         mat.maps[MATERIAL_MAP_ALBEDO].color.a);
                TraceLog(LOG_INFO, "  - Metalness value: %.2f", mat.maps[MATERIAL_MAP_METALNESS].value);
                TraceLog(LOG_INFO, "  - Roughness value: %.2f", mat.maps[MATERIAL_MAP_ROUGHNESS].value);
            }

            // Applica lo shader
            if (sharedShader.id > 0)
            {
                applyShader(sharedShader);
            }

            // Load animations from the same file
            loadAnimations(path);
        }
        else
        {
            TraceLog(LOG_ERROR, "Failed to load model");
        }
    }

    void Character::unloadModel()
    {
        modelInstance = ModelInstance(); // Release via move assignment
    }

    void Character::handleDroppedModel()
    {
        // Note: This functionality is disabled without access to ModelManager
        // The game should handle dropped models at a higher level with access to modelManager
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();
            // Just log for now - proper handling requires ModelManager access
            if (droppedFiles.count == 1)
            {
                TraceLog(LOG_WARNING, "Dropped model handling requires ModelManager - ignoring: %s",
                         droppedFiles.paths[0]);
            }
            UnloadDroppedFiles(droppedFiles);
        }
    }

    void Character::update()
    {
        Vector3 axis = {0, 1, 0};
        float angularSpeed = 0.0f;
        // Use scaled time so rotation stops when paused
        Quaternion deltaQuat = QuaternionFromAxisAngle(axis, angularSpeed * TimeManager::getInstance().getGameDeltaTime());
        quat_rotation = QuaternionMultiply(deltaQuat, quat_rotation);
        quat_rotation = QuaternionNormalize(quat_rotation);
        handleDroppedModel();
    }

    void Character::draw()
    {
        if (!isVisible)
            return;

        if (modelInstance.isValid())
        {
            Quaternion q = QuaternionFromEuler(
                eulerRot.x * DEG2RAD,
                eulerRot.y * DEG2RAD,
                eulerRot.z * DEG2RAD);

            Vector3 axis;
            float angle;
            QuaternionToAxisAngle(q, &axis, &angle);

            // Calcola la matrice di trasformazione
            Matrix matScale = MatrixScale(scale, scale, scale);
            Matrix matRotation = MatrixRotate(axis, angle); // angle is already in radians from QuaternionToAxisAngle
            Matrix matTranslation = MatrixTranslate(position.x, position.y, position.z);

            Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

            // Disegna ogni mesh con il suo materiale
            // Note: bindAnimationData/unbindAnimationData are no-ops with per-instance meshes
            for (int i = 0; i < modelInstance.meshCount(); i++)
            {
                int materialIndex = modelInstance.meshMaterial()[i];
                Material material = modelInstance.materials()[materialIndex];

                // DrawMesh applica automaticamente lo shader del materiale
                DrawMesh(modelInstance.meshes()[i], material, transform);
            }
        }
        else
        {
            DrawCube(position, scale, scale, scale, GREEN);
            DrawCubeWires(Vector3{position.x, position.y + (scale / 2), position.z},
                          scale, scale, scale, BLACK);
        }
    }

    void Character::snapToGround(const Model &ground)
    {
        Ray downRay;
        downRay.position = {position.x, position.y + 100.0f, position.z};
        downRay.direction = {0.0f, -1.0f, 0.0f};

        RayCollision closest = {0};
        closest.hit = false;
        closest.distance = std::numeric_limits<float>::max();

        for (int m = 0; m < ground.meshCount; m++)
        {
            RayCollision hit = GetRayCollisionMesh(downRay, ground.meshes[m], ground.transform);
            if (hit.hit && hit.distance < closest.distance)
            {
                closest = hit;
            }
        }

        if (closest.hit)
        {
            position.y = closest.point.y;
        }
    }

    void Character::gui() {
        GameObject::gui();
    }

    void Character::loadAnimations(const std::string &modelPath)
    {
        // Unload existing animations first
        unloadAnimations();

        // Load animations from the model file
        m_animations = LoadModelAnimations(modelPath.c_str(), &m_animationCount);

        if (m_animationCount > 0 && m_animations != nullptr)
        {
            TraceLog(LOG_INFO, "Loaded %d animations from '%s':", m_animationCount, modelPath.c_str());
            for (int i = 0; i < m_animationCount; i++)
            {
                TraceLog(LOG_INFO, "  [%d] '%s' (%d frames)", i, m_animations[i].name, m_animations[i].frameCount);
            }

            // Prepare per-instance animation data so this character can animate independently
            modelInstance.prepareForAnimation();
            
            // Cache Model struct to avoid reconstruction every frame
            m_cachedModel.meshCount = modelInstance.meshCount();
            m_cachedModel.meshes = modelInstance.meshes();
            m_cachedModel.materialCount = modelInstance.materialCount();
            m_cachedModel.materials = modelInstance.materials();
            m_cachedModel.meshMaterial = modelInstance.meshMaterial();
            m_cachedModel.boneCount = modelInstance.boneCount();
            m_cachedModel.bones = modelInstance.bones();
            m_cachedModel.bindPose = modelInstance.bindPose();
        }
        else
        {
            TraceLog(LOG_WARNING, "No animations found in '%s'", modelPath.c_str());
        }
    }

    void Character::unloadAnimations()
    {
        if (m_animations != nullptr && m_animationCount > 0)
        {
            UnloadModelAnimations(m_animations, m_animationCount);
            m_animations = nullptr;
            m_animationCount = 0;
        }
        m_currentAnimIndex = -1;
        m_currentFrame = 0;
        m_animationTimer = 0.0f;
        m_isAnimating = false;
    }

    int Character::getAnimationIndex(const std::string &name) const
    {
        if (m_animations == nullptr || m_animationCount == 0)
        {
            return -1;
        }

        for (int i = 0; i < m_animationCount; i++)
        {
            if (name == m_animations[i].name)
            {
                return i;
            }
        }
        return -1;
    }

    bool Character::setAnimation(const std::string &animName)
    {
        int index = getAnimationIndex(animName);
        if (index >= 0)
        {
            if (m_currentAnimIndex != index)
            {
                m_currentAnimIndex = index;
                m_currentFrame = 0;
                m_lastUpdatedFrame = -1;
                m_animationTimer = 0.0f;
                TraceLog(LOG_INFO, "Character '%s': Set animation to '%s' (index %d)",
                         name.c_str(), animName.c_str(), index);
            }
            return true;
        }
        TraceLog(LOG_WARNING, "Character '%s': Animation '%s' not found", name.c_str(), animName.c_str());
        return false;
    }

    void Character::playAnimation()
    {
        if (m_currentAnimIndex >= 0 && m_currentAnimIndex < m_animationCount)
        {
            m_isAnimating = true;
        }
    }

    void Character::stopAnimation()
    {
        m_isAnimating = false;
        m_currentFrame = 0;
        m_lastUpdatedFrame = -1;
        m_animationTimer = 0.0f;
    }

    void Character::updateAnimation()
    {
        if (!m_isAnimating || m_currentAnimIndex < 0 || m_currentAnimIndex >= m_animationCount)
        {
            return;
        }

        if (!modelInstance.isValid())
        {
            return;
        }

        ModelAnimation &anim = m_animations[m_currentAnimIndex];

        // Advance animation timer with scaled time (affected by game speed)
        m_animationTimer += TimeManager::getInstance().getGameDeltaTime();

        // Calculate FPS (typically 30 or 60 for most animations)
        float animFPS = 30.0f; // Standard animation framerate
        float frameDuration = 1.0f / animFPS;

        // Advance frame based on timer
        while (m_animationTimer >= frameDuration)
        {
            m_animationTimer -= frameDuration;
            m_currentFrame++;

            // Loop the animation
            if (m_currentFrame >= anim.frameCount)
            {
                m_currentFrame = 0;
            }
        }

        // Only update bone matrices when the animation frame actually changes.
        // We use UpdateModelAnimationBones (not UpdateModelAnimation) because our
        // local meshes have animVertices/animNormals set to null for GPU skinning.
        // UpdateModelAnimation writes directly to animVertices without a null check,
        // which would segfault. UpdateModelAnimationBones only computes bone matrices.
        if (m_currentFrame == m_lastUpdatedFrame)
        {
            return;
        }
        m_lastUpdatedFrame = m_currentFrame;

        // Use cached Model struct to avoid reconstruction overhead every frame
        // Only update bone matrices — GPU shader handles vertex transformation
        UpdateModelAnimationBones(m_cachedModel, anim, m_currentFrame);
    }

} // namespace moiras
