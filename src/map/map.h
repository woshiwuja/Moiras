#include "../game/game_object.h"
#include "rlgl.h"
#include <raylib.h>
#include <string>
namespace moiras {
class Map : public GameObject {
public:
  Shader seaShaderLoaded;
  float hiddenTimeCounter = 0;
  Image perlinNoiseImage;
  Texture perlinNoiseMap;
  float width;
  float height;
  float length;
  Vector3 position = {0., 0., 0.};
  Model model;
  Mesh mesh;
  Texture texture;
  std::string seaShaderVertex;
  std::string seaShaderFragment;
  Mesh seaMesh;
  Model seaModel;
  Model skyboxModel;
  Shader skyboxShader;
  Texture skyboxTexture;
  std::string skyboxShaderVertex;
  std::string skyboxShaderFragment;
  Map();
  Map(float width_, float height_, float length_, Model model_, Mesh mesh_,
      Texture texture_);
  Map(Model model_);
  Map(const Map &) = delete;
  Map &operator=(const Map &) = delete;
  Map(Map &&other) noexcept;
  Map &operator=(Map &&other) noexcept;
  ~Map();
  void draw() override;
  void loadSeaShader();
  void addSea() {
    seaMesh = GenMeshPlane(5000, 5000, 50, 50);
    seaModel = LoadModelFromMesh(seaMesh);
    seaModel.materials[0].shader = seaShaderLoaded;
  }
  void loadSkybox(const std::string& texturePath);
  void drawSkybox(Vector3 cameraPosition);
  void update() override;
};
std::unique_ptr<Map> mapFromHeightmap(const std::string &filename, float width,
                                      float height, float lenght);
std::unique_ptr<Map> mapFromModel(const std::string &filename);

} // namespace moiras
