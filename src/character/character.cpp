#include <cmath>
#include <raylib.h>
#include <string>

struct Euler {
  float x, y, z;
};

struct XVector3 {
  float x, y, z;
  XVector3 normalize() {
    float len = sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
    if (len < 1e-8f)
      return {1, 0, 0}; // fallback asse
    float inv = 1.0 / len;
    return XVector3{this->x * inv, this->y * inv, this->z * inv};
  };
  Vector3 toVector3() { return Vector3{this->x, this->y, this->z}; };
};

struct Quat {
  float w, x, y, z;

  Quat() : w(0), x(0), y(0), z(0) {};
  Quat(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}

  Quat normalize() {
    float len = sqrt(this->x * this->x + this->y * this->y + this->z * this->z +
                     this->w * this->w);

    // sicurezza
    if (len < 1e-8f)
      return Quat(0, 0, 0, 1); // identità

    float inv = 1.0f / len;
    return Quat{this->w * inv, this->x * inv, this->y * inv, this->z * inv};
  }
  float lenght() {
    return sqrt(this->w * this->w + this->x * this->x + this->y * this->y +
                this->z * this->z);
  };
};

Quat QuaternionFromAxisAngle(XVector3 axis, float arad) {
  axis = axis.normalize();
  auto half_angle = arad * .5;
  auto s = std::sin(half_angle);
  auto quat = Quat(cos(half_angle), axis.x * s, axis.y * s, axis.z * s);

  quat.w /= quat.lenght();
  quat.x /= quat.lenght();
  quat.y /= quat.lenght();
  quat.z /= quat.lenght();

  return quat;
};

Quat QuaternionMultiply(const Quat &q1, const Quat &q2) {
  Quat q;
  q.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
  q.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
  q.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
  q.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
  return q;
}

Quat quatFromEuler(Euler rot) {
  float cx = std::cos(rot.x * .5f);
  float sx = std::sin(rot.x * .5f);
  float cy = std::cos(rot.y * .5f);
  float sy = std::sin(rot.y * .5f);
  float cz = std::cos(rot.z * .5f);
  float sz = std::sin(rot.z * .5f);
  auto qx = sx * cy * cz + cx * sy * sz;
  auto qy = sx * cy * cz + cx * sy * sz;
  auto qz = cx * sy * cz + sx * cy * sz;
  auto qw = cx * cy * cz + sx * sy * sz;
  auto quat = Quat(qx, qy, qz, qw);
  return quat.normalize();
};

class Character {
private:
  Quat quat_rotation;    // orientamento reale
  Vector3 rotation_axis; // SOLO per draw
  float rotation_angle;  // SOLO per draw
public:
  int health;
  std::string name;
  XVector3 position;
  float scale;
  Model *model;
  Euler eulerRot;
  Character()
      : health(100), position({0, 0, 0}), scale(1.0f), model(nullptr),
        quat_rotation(1.0, 0.0, 0.0, 0.0) {
    this->quat_rotation = quatFromEuler(eulerRot);
  }

  void Update() {
    XVector3 axis = {0, 1, 0};
    float angularSpeed = 0.0f;

    Quat deltaQuat =
        QuaternionFromAxisAngle(axis, angularSpeed * GetFrameTime());
    quat_rotation = (QuaternionMultiply(deltaQuat, quat_rotation));
    quat_rotation = quat_rotation.normalize();
  }

  void Draw() {
    // conversione SOLO per il rendering
    quat_rotation.QuaternionToAxisAngle(&rotation_axis, &rotation_angle);
	
  }
};
