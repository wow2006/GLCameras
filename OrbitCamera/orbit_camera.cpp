///////////////////////////////////////////////////////////////////////////////
// orbit_camera.cpp
// ================
// Orbital camera class for OpenGL
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2011-12-02
// UPDATED: 2016-10-24
///////////////////////////////////////////////////////////////////////////////
//
// NOTE on the port: the original mathlib stored a Matrix4 as a flat array in
// OpenGL's column-major order, so its m[i] is glm's matrix[i / 4][i % 4] and
// every index expression carries over unchanged. Its quaternion product is the
// standard Hamilton product, so quaternion multiplications keep their order
// too; only Quaternion(axis, angle) differs, because it took a half angle
// where glm::angleAxis() takes the full one.
//-----------------------------------------------------------------------------

#include <glm/gtc/matrix_transform.hpp>
//
#include "orbit_camera.hpp"

namespace {
constexpr float EPSILON = 0.00001F;

///////////////////////////////////////////////////////////////////////////////
// Euler angles (radian) to a quaternion, rotating in the order z->y->x
///////////////////////////////////////////////////////////////////////////////
glm::quat eulerToQuaternion(const glm::vec3 &radians) {
  const glm::quat qx = glm::angleAxis(radians.x, glm::vec3{1.0F, 0.0F, 0.0F});
  const glm::quat qy = glm::angleAxis(radians.y, glm::vec3{0.0F, 1.0F, 0.0F});
  const glm::quat qz = glm::angleAxis(radians.z, glm::vec3{0.0F, 0.0F, 1.0F});
  return qx * qy * qz;
}
}  // namespace

OrbitCamera::OrbitCamera() = default;

OrbitCamera::OrbitCamera(const glm::vec3 &position, const glm::vec3 &target) { lookAt(position, target); }

///////////////////////////////////////////////////////////////////////////////
// update each frame, frame time is sec
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::update(float frameTime) {
  if(m_moving)
    updateMove(frameTime);
  if(m_shifting || m_shiftingSpeed != 0.0F)
    updateShift(frameTime);
  if(m_forwarding || m_forwardingSpeed != 0.0F)
    updateForward(frameTime);
  if(m_turning)
    updateTurn(frameTime);
}

///////////////////////////////////////////////////////////////////////////////
// update position movement only
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::updateMove(float frameTime) {
  m_movingTime += frameTime;
  if(m_movingTime >= m_movingDuration) {
    setPosition(m_movingTo);
    m_moving = false;
  } else {
    setPosition(anim::interpolate(m_movingFrom, m_movingTo, m_movingTime / m_movingDuration, m_movingMode));
  }
}

///////////////////////////////////////////////////////////////////////////////
// update target movement only
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::updateShift(float frameTime) {
  m_shiftingTime += frameTime;

  // shift with duration
  if(m_shiftingDuration > 0.0F) {
    if(m_shiftingTime >= m_shiftingDuration) {
      setTarget(m_shiftingTo);
      m_shifting = false;
    } else {
      setTarget(anim::interpolate(m_shiftingFrom, m_shiftingTo, m_shiftingTime / m_shiftingDuration, m_shiftingMode));
    }
  }
  // shift with acceleration
  else {
    m_shiftingSpeed = anim::accelerate(m_shifting, m_shiftingSpeed, m_shiftingMaxSpeed, m_shiftingAccel, frameTime);
    setTarget(m_target + (m_shiftingVector * m_shiftingSpeed * frameTime));
  }
}

///////////////////////////////////////////////////////////////////////////////
// update forward movement only
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::updateForward(float frameTime) {
  m_forwardingTime += frameTime;

  // move forward for duration
  if(m_forwardingDuration > 0.0F) {
    if(m_forwardingTime >= m_forwardingDuration) {
      setDistance(m_forwardingTo);
      m_forwarding = false;
    } else {
      setDistance(anim::interpolate(m_forwardingFrom, m_forwardingTo, m_forwardingTime / m_forwardingDuration, m_forwardingMode));
    }
  }
  // move forward with acceleration
  else {
    m_forwardingSpeed = anim::accelerate(m_forwarding, m_forwardingSpeed, m_forwardingMaxSpeed, m_forwardingAccel, frameTime);
    setDistance(m_distance - m_forwardingSpeed * frameTime);
  }
}

///////////////////////////////////////////////////////////////////////////////
// update rotation only
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::updateTurn(float frameTime) {
  m_turningTime += frameTime;
  if(m_turningTime >= m_turningDuration) {
    if(m_quaternionUsed)
      setRotation(m_turningQuaternionTo);
    else
      setRotation(m_turningAngleTo);
    m_turning = false;
  } else {
    if(m_quaternionUsed) {
      setRotation(anim::slerp(m_turningQuaternionFrom, m_turningQuaternionTo, m_turningTime / m_turningDuration, m_turningMode));
    } else {
      setRotation(anim::interpolate(m_turningAngleFrom, m_turningAngleTo, m_turningTime / m_turningDuration, m_turningMode));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// set position of camera, set transform matrix as well
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::setPosition(const glm::vec3 &v) { lookAt(v, m_target); }

///////////////////////////////////////////////////////////////////////////////
// set target of camera, then rebuild matrix
// rotation parts are not changed, but translation part must be recalculated
// And, position is also shifted
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::setTarget(const glm::vec3 &v) {
  m_target = v;

  m_position = m_target - (m_distance * getForwardAxis());
  computeMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// set distance of camera, then recompute camera position
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::setDistance(float d) {
  m_distance = d;
  computeMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// set transform matrix equivalent to gluLookAt()
// 1. Mt: Translate scene to camera position inversely, (-x, -y, -z)
// 2. Mr: Rotate scene inversly so camera looks at the scene
// 3. Find matrix = Mr * Mt
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::lookAt(const glm::vec3 &position, const glm::vec3 &target) {
  // remember the camera position & target position
  m_position = position;
  m_target = target;

  // if pos and target are same, only translate camera to position without rotation
  if(position == target) {
    m_matrix = glm::mat4(1.0F);
    m_matrix[3] = glm::vec4(-position, 1.0F);
    // rotation stuff
    m_matrixRotation = glm::mat4(1.0F);
    m_angle = {0.0F, 0.0F, 0.0F};
    m_quaternion = {1.0F, 0.0F, 0.0F, 0.0F};
    return;
  }

  // first, compute the forward vector of rotation matrix
  // NOTE: the direction is reversed (target to camera pos) because of camera transform
  glm::vec3 forward = position - target;
  m_distance = glm::length(forward);  // remember the distance
  forward /= m_distance;

  // compute temporal up vector based on the forward vector
  // watch out when look up/down at 90 degree, for example, forward vector is
  // on the Y axis
  glm::vec3 up = {0.0F, 1.0F, 0.0F};
  if(std::abs(forward.x) < EPSILON && std::abs(forward.z) < EPSILON)
    up = (forward.y > 0.0F) ? glm::vec3{0.0F, 0.0F, -1.0F} : glm::vec3{0.0F, 0.0F, 1.0F};

  // compute the left vector of rotation matrix
  glm::vec3 left = glm::normalize(glm::cross(up, forward));

  // re-calculate the orthonormal up vector
  up = glm::cross(forward, left);

  // set inverse rotation matrix: M^-1 = M^T for Euclidean transform, so the
  // three axes go into the rows
  m_matrixRotation = glm::mat4(1.0F);
  m_matrixRotation[0] = {left.x, up.x, forward.x, 0.0F};
  m_matrixRotation[1] = {left.y, up.y, forward.y, 0.0F};
  m_matrixRotation[2] = {left.z, up.z, forward.z, 0.0F};

  // copy it to matrix and set the translation part
  m_matrix = m_matrixRotation;
  m_matrix[3] = {-glm::dot(left, position), -glm::dot(up, position), -glm::dot(forward, position), 1.0F};

  // set Euler angles
  m_angle = matrixToAngle(m_matrixRotation);

  // set quaternion from angle
  // NOTE: yaw must be negated again for quaternion
  m_quaternion = eulerToQuaternion(glm::radians(glm::vec3{m_angle.x, -m_angle.y, m_angle.z}));
}

///////////////////////////////////////////////////////////////////////////////
// set transform matrix with target and camera's up vectors
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::lookAt(const glm::vec3 &position, const glm::vec3 &target, const glm::vec3 &upDir) {
  m_position = position;
  m_target = target;

  if(position == target) {
    m_matrix = glm::mat4(1.0F);
    m_matrix[3] = glm::vec4(-position, 1.0F);
    m_matrixRotation = glm::mat4(1.0F);
    m_angle = {0.0F, 0.0F, 0.0F};
    m_quaternion = {1.0F, 0.0F, 0.0F, 0.0F};
    return;
  }

  glm::vec3 forward = position - target;
  m_distance = glm::length(forward);
  forward /= m_distance;

  const glm::vec3 left = glm::normalize(glm::cross(upDir, forward));
  const glm::vec3 up = glm::cross(forward, left);

  m_matrixRotation = glm::mat4(1.0F);
  m_matrixRotation[0] = {left.x, up.x, forward.x, 0.0F};
  m_matrixRotation[1] = {left.y, up.y, forward.y, 0.0F};
  m_matrixRotation[2] = {left.z, up.z, forward.z, 0.0F};

  m_matrix = m_matrixRotation;
  m_matrix[3] = {-glm::dot(left, position), -glm::dot(up, position), -glm::dot(forward, position), 1.0F};

  m_angle = matrixToAngle(m_matrixRotation);
  m_quaternion = eulerToQuaternion(glm::radians(glm::vec3{m_angle.x, -m_angle.y, m_angle.z}));
}

///////////////////////////////////////////////////////////////////////////////
// set transform matrix with rotation angles (degree)
// NOTE: the angle is for camera, so yaw value must be negated for computation.
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::setRotation(const glm::vec3 &angle) {
  // remember angles
  // NOTE: assume all angles are already reversed for camera
  m_angle = angle;

  // remember quaternion value
  // NOTE: yaw must be negated again for quaternion
  //
  // The original handed the raw degrees to Quaternion::getQuaternion() here,
  // which expects half angles in radians, so the quaternion it reported after
  // a slider move was meaningless. The conversion lookAt() used is applied
  // instead. The quaternion is read-only for the demo, so nothing else moves.
  m_quaternion = eulerToQuaternion(glm::radians(glm::vec3{angle.x, -angle.y, angle.z}));

  // compute rotation matrix from angle
  m_matrixRotation = angleToMatrix(angle);

  // construct camera matrix
  computeMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// set rotation with new quaternion
// NOTE: quaternion value is for matrix, so matrixToAngle() will reverse yaw.
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::setRotation(const glm::quat &q) {
  m_quaternion = q;

  // quaternion to matrix
  // NOTE: mathlib's Quaternion::getMatrix() built the transpose of the
  // rotation, which is what the camera matrix wants; glm::mat4_cast() does not.
  m_matrixRotation = glm::transpose(glm::mat4_cast(q));

  computeMatrix();

  m_angle = matrixToAngle(m_matrixRotation);
}

///////////////////////////////////////////////////////////////////////////////
// construct camera matrix: M = Mt2 * Mr * Mt1
// where Mt1: move scene to target (-x,-y,-z)
//       Mr : rotate scene at the target point
//       Mt2: move scene away from target with distance -d
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::computeMatrix() {
  // the three rows of the rotation are the camera's left/up/forward axes
  const glm::vec3 left = {m_matrixRotation[0][0], m_matrixRotation[1][0], m_matrixRotation[2][0]};
  const glm::vec3 up = {m_matrixRotation[0][1], m_matrixRotation[1][1], m_matrixRotation[2][1]};
  const glm::vec3 forward = {m_matrixRotation[0][2], m_matrixRotation[1][2], m_matrixRotation[2][2]};

  m_matrix = m_matrixRotation;
  m_matrix[3] = {-glm::dot(left, m_target), -glm::dot(up, m_target), -glm::dot(forward, m_target) - m_distance, 1.0F};

  // re-compute camera position
  // NOTE: camera's forward vector is the forward vector of the inverse matrix
  m_position = m_target - (m_distance * getForwardAxis());
}

///////////////////////////////////////////////////////////////////////////////
// return left, up, forward axis
///////////////////////////////////////////////////////////////////////////////
glm::vec3 OrbitCamera::getLeftAxis() const { return {-m_matrix[0][0], -m_matrix[1][0], -m_matrix[2][0]}; }

glm::vec3 OrbitCamera::getUpAxis() const { return {m_matrix[0][1], m_matrix[1][1], m_matrix[2][1]}; }

glm::vec3 OrbitCamera::getForwardAxis() const { return {-m_matrix[0][2], -m_matrix[1][2], -m_matrix[2][2]}; }

///////////////////////////////////////////////////////////////////////////////
// move the camera position with the given duration
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::moveTo(const glm::vec3 &to, float duration, anim::AnimationMode mode) {
  if(duration <= 0.0F) {
    setPosition(to);
    return;
  }

  m_movingFrom = m_position;
  m_movingTo = to;
  m_movingVector = m_movingTo - m_movingFrom;
  if(glm::dot(m_movingVector, m_movingVector) != 0.0F)
    m_movingVector = glm::normalize(m_movingVector);
  m_movingTime = 0.0F;
  m_movingDuration = duration;
  m_movingMode = mode;
  m_moving = true;
}

///////////////////////////////////////////////////////////////////////////////
// pan the camera target left/right/up/down with the given duration
// the camera position will be shifted after transform
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::shiftTo(const glm::vec3 &to, float duration, anim::AnimationMode mode) {
  if(duration <= 0.0F) {
    setTarget(to);
    return;
  }

  m_shiftingFrom = m_target;
  m_shiftingTo = to;
  m_shiftingVector = m_shiftingTo - m_shiftingFrom;
  if(glm::dot(m_shiftingVector, m_shiftingVector) != 0.0F)
    m_shiftingVector = glm::normalize(m_shiftingVector);
  m_shiftingTime = 0.0F;
  m_shiftingDuration = duration;
  m_shiftingMode = mode;
  m_shifting = true;
}

///////////////////////////////////////////////////////////////////////////////
// shift the camera position and target left/right/up/down
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::shift(const glm::vec2 &delta, float duration, anim::AnimationMode mode) {
  // get left & up vectors of camera
  const glm::vec3 cameraLeft = {-m_matrix[0][0], -m_matrix[1][0], -m_matrix[2][0]};
  const glm::vec3 cameraUp = {-m_matrix[0][1], -m_matrix[1][1], -m_matrix[2][1]};

  // compute delta movement
  glm::vec3 deltaMovement = delta.x * cameraLeft;
  deltaMovement += -delta.y * cameraUp;  // reverse up direction

  shiftTo(m_target + deltaMovement, duration, mode);
}

///////////////////////////////////////////////////////////////////////////////
// start accelerating to shift camera
// It takes shift direction vector and acceleration per squared sec.
// acceleration should be always positive.
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::startShift(const glm::vec2 &shiftVector, float accel) {
  const glm::vec3 cameraLeft = {-m_matrix[0][0], -m_matrix[1][0], -m_matrix[2][0]};
  const glm::vec3 cameraUp = {-m_matrix[0][1], -m_matrix[1][1], -m_matrix[2][1]};

  glm::vec3 vector = shiftVector.x * cameraLeft;
  vector += -shiftVector.y * cameraUp;  // reverse up direction

  m_shiftingMaxSpeed = glm::length(shiftVector);
  m_shiftingVector = vector;
  if(glm::dot(m_shiftingVector, m_shiftingVector) != 0.0F)
    m_shiftingVector = glm::normalize(m_shiftingVector);
  m_shiftingSpeed = 0.0F;
  m_shiftingAccel = accel;
  m_shiftingTime = 0.0F;
  m_shiftingDuration = 0.0F;
  m_shifting = true;
}

void OrbitCamera::stopShift() { m_shifting = false; }

///////////////////////////////////////////////////////////////////////////////
// zoom in/out the camera position with the given delta movement and duration
// it actually moves the camera forward or backward.
// positive delta means moving forward (decreasing distance)
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::moveForward(float delta, float duration, anim::AnimationMode mode) {
  if(duration <= 0.0F) {
    setDistance(m_distance - delta);
    return;
  }

  m_forwardingFrom = m_distance;
  m_forwardingTo = m_distance - delta;
  m_forwardingTime = 0.0F;
  m_forwardingDuration = duration;
  m_forwardingMode = mode;
  m_forwarding = true;
}

///////////////////////////////////////////////////////////////////////////////
// start accelerating to move forward
// It takes maximum speed per sec and acceleration per squared sec.
// positive speed means moving forward (decreasing distance).
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::startForward(float maxSpeed, float accel) {
  m_forwardingSpeed = 0.0F;
  m_forwardingMaxSpeed = maxSpeed;
  m_forwardingAccel = accel;
  m_forwardingTime = 0.0F;
  m_forwardingDuration = 0.0F;
  m_forwarding = true;
}

void OrbitCamera::stopForward() { m_forwarding = false; }

///////////////////////////////////////////////////////////////////////////////
// rotate camera to the given angle with duration
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::rotateTo(const glm::vec3 &angle, float duration, anim::AnimationMode mode) {
  m_quaternionUsed = false;
  if(duration <= 0.0F) {
    setRotation(angle);
    return;
  }

  m_turningAngleFrom = m_angle;
  m_turningAngleTo = angle;
  m_turningTime = 0.0F;
  m_turningDuration = duration;
  m_turningMode = mode;
  m_turning = true;
}

///////////////////////////////////////////////////////////////////////////////
// rotate camera to the given quaternion with duration
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::rotateTo(const glm::quat &q, float duration, anim::AnimationMode mode) {
  m_quaternionUsed = true;
  if(duration <= 0.0F) {
    setRotation(q);
    return;
  }

  m_turningQuaternionFrom = m_quaternion;
  m_turningQuaternionTo = q;
  m_turningTime = 0.0F;
  m_turningDuration = duration;
  m_turningMode = mode;
  m_turning = true;
}

///////////////////////////////////////////////////////////////////////////////
// rotate camera with delta angle
// NOTE: delta angle must be negated already
///////////////////////////////////////////////////////////////////////////////
void OrbitCamera::rotate(const glm::vec3 &delta, float duration, anim::AnimationMode mode) {
  rotateTo(m_angle + delta, duration, mode);
}

///////////////////////////////////////////////////////////////////////////////
// convert rotation angles (degree) to 4x4 matrix
// NOTE: the angle is for orbit camera, so yaw angle must be reversed before
// matrix computation.
//
// The order of rotation is Roll->Yaw->Pitch (Rx*Ry*Rz)
// Rx: rotation about X-axis, pitch
// Ry: rotation about Y-axis, yaw(heading)
// Rz: rotation about Z-axis, roll
//    Rx           Ry          Rz
// |1  0   0| | Cy  0 Sy| |Cz -Sz 0|   | CyCz        -CySz         Sy  |
// |0 Cx -Sx|*|  0  1  0|*|Sz  Cz 0| = | SxSyCz+CxSz -SxSySz+CxCz -SxCy|
// |0 Sx  Cx| |-Sy  0 Cy| | 0   0 1|   |-CxSyCz+SxSz  CxSySz+SxCz  CxCy|
///////////////////////////////////////////////////////////////////////////////
glm::mat4 OrbitCamera::angleToMatrix(const glm::vec3 &angle) {
  // rotation angle about X-axis (pitch)
  const float sx = std::sin(glm::radians(angle.x));
  const float cx = std::cos(glm::radians(angle.x));

  // rotation angle about Y-axis (yaw)
  const float sy = std::sin(glm::radians(-angle.y));
  const float cy = std::cos(glm::radians(-angle.y));

  // rotation angle about Z-axis (roll)
  const float sz = std::sin(glm::radians(angle.z));
  const float cz = std::cos(glm::radians(angle.z));

  glm::mat4 matrix = glm::mat4(1.0F);

  // determine left axis
  matrix[0] = {cy * cz, sx * sy * cz + cx * sz, -cx * sy * cz + sx * sz, 0.0F};

  // determine up axis
  matrix[1] = {-cy * sz, -sx * sy * sz + cx * cz, cx * sy * sz + sx * cz, 0.0F};

  // determine forward axis
  matrix[2] = {sy, -sx * cy, cx * cy, 0.0F};

  return matrix;
}

///////////////////////////////////////////////////////////////////////////////
// retrieve angles in degree from rotation matrix, M = Rx*Ry*Rz
//
// Pitch: atan(-m[9] / m[10])
// Yaw  : asin(m[8])
// Roll : atan(-m[4] / m[0])
//
// The camera's yaw is reversed on the way out.
///////////////////////////////////////////////////////////////////////////////
glm::vec3 OrbitCamera::matrixToAngle(const glm::mat4 &matrix) {
  float pitch = 0.0F;
  float roll = 0.0F;

  // find yaw (around y-axis) first
  // NOTE: asin() returns -90~+90, so correct the angle range -180~+180
  // using z value of forward vector
  float yaw = glm::degrees(std::asin(glm::clamp(matrix[2][0], -1.0F, 1.0F)));
  if(matrix[2][2] < 0.0F) {
    if(yaw >= 0.0F)
      yaw = 180.0F - yaw;
    else
      yaw = -180.0F - yaw;
  }

  // find roll (around z-axis) and pitch (around x-axis)
  // if forward vector is (1,0,0) or (-1,0,0), then m[0]=m[4]=m[9]=m[10]=0
  if(matrix[0][0] > -EPSILON && matrix[0][0] < EPSILON) {
    roll = 0.0F;  //@@ assume roll=0
    pitch = glm::degrees(std::atan2(matrix[0][1], matrix[1][1]));
  } else {
    roll = glm::degrees(std::atan2(-matrix[1][0], matrix[0][0]));
    pitch = glm::degrees(std::atan2(-matrix[2][1], matrix[2][2]));
  }

  // atan2() and the yaw negation below both hand back a negative zero for an
  // unrotated camera, which the control panel would then print as "-0"
  const auto unsign = [](float value) { return (value == 0.0F) ? 0.0F : value; };

  // reverse yaw for the camera
  return {unsign(pitch), unsign(-yaw), unsign(roll)};
}
