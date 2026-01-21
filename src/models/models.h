#include <cstring>
#include <raylib.h>
#include <vector>
namespace moiras {
void smoothMesh(Mesh *mesh, int iterations, float lambda);
Mesh subdivideMeshNonIndexed(const Mesh &original);
void UpdateModelAnimationBonesLerp(Model model, ModelAnimation animA,
                                   int frameA, ModelAnimation animB, int frameB,
                                   float value);
void UpdateModelVertsToCurrentBones(Model model);
void UpdateModelAnimationWithBones(Model model, ModelAnimation anim, int frame);

} // namespace moiras
//
