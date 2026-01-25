#include "../game/game_object.h"
#include <raylib.h>
namespace moiras {
class Light : public GameObject {
public:
  bool enabled;
  Vector3 target;
  Vector4 color;
  float intensity;
  int typeLoc;
  int enabledLoc;
  int positionLoc;
  int targetLoc;
  int colorLoc;
  int intensityLoc;
};

class PointLight : public Light {};
class SpotLight : public Light {};
class DirectionalLight : public Light {};
} // namespace moiras
