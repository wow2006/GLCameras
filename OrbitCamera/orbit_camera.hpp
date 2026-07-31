///////////////////////////////////////////////////////////////////////////////
// orbit_camera.hpp
// ================
// Orbital camera class for OpenGL
// Use lookAt() for initial positioning the camera, then call rotateTo() for
// orbital rotation, moveTo()/moveForward() to move camera position only and
// shiftTo() to move position and target together (panning)
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2011-12-02
// UPDATED: 2016-10-24
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
//
#include "anim_utils.hpp"

class OrbitCamera final {
public:
  OrbitCamera();
  OrbitCamera(const glm::vec3 &position, const glm::vec3 &target);

  void update(float frameTime = 0.0F);  // update position, target and matrix during given sec

  // set position, target and transform matrix so camera looks at the target
  void lookAt(const glm::vec3 &pos, const glm::vec3 &target);
  void lookAt(const glm::vec3 &pos, const glm::vec3 &target, const glm::vec3 &up);

  // move the camera position to the destination
  // if duration(sec) is greater than 0, it will animate for the given duration
  // otherwise, it will set the position immediately
  // use moveForward() to move the camera forward/backward
  // NOTE: you must call update() before getting the delta movement per frame
  void moveTo(const glm::vec3 &to, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void moveForward(float delta, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void startForward(float maxSpeed = 1.0F, float accel = 1.0F);
  void stopForward();

  // pan the camera, shift both position and target point in same direction
  // use this function to offset the camera's rotation pivot
  void shiftTo(const glm::vec3 &to, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void shift(const glm::vec2 &delta, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void startShift(const glm::vec2 &shiftVector, float accel = 1.0F);
  void stopShift();

  // rotate the camera around the target point
  // You can use either quaternion or Euler angles
  void rotateTo(const glm::vec3 &angle, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void rotateTo(const glm::quat &quat, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);
  void rotate(const glm::vec3 &deltaAngle, float duration = 0.0F, anim::AnimationMode mode = anim::EASE_OUT);

  void setPosition(const glm::vec3 &v);
  void setTarget(const glm::vec3 &v);
  void setDistance(float distance);
  void setRotation(const glm::vec3 &angle);  // angles in degree
  void setRotation(const glm::quat &q);

  [[nodiscard]] const glm::vec3 &getPosition() const { return m_position; }
  [[nodiscard]] const glm::vec3 &getTarget() const { return m_target; }
  [[nodiscard]] const glm::vec3 &getAngle() const { return m_angle; }
  [[nodiscard]] const glm::mat4 &getMatrix() const { return m_matrix; }
  [[nodiscard]] float getDistance() const { return m_distance; }
  [[nodiscard]] const glm::quat &getQuaternion() const { return m_quaternion; }

  // return camera's 3 axis vectors
  [[nodiscard]] glm::vec3 getLeftAxis() const;
  [[nodiscard]] glm::vec3 getUpAxis() const;
  [[nodiscard]] glm::vec3 getForwardAxis() const;

  // convert rotation angles (degree) to a rotation matrix and back
  [[nodiscard]] static glm::mat4 angleToMatrix(const glm::vec3 &angles);
  [[nodiscard]] static glm::vec3 matrixToAngle(const glm::mat4 &matrix);

private:
  void updateMove(float frameTime);
  void updateShift(float frameTime);
  void updateForward(float frameTime);
  void updateTurn(float frameTime);
  void computeMatrix();

  glm::vec3 m_position = {0.0F, 0.0F, 0.0F};          // camera position at world space
  glm::vec3 m_target = {0.0F, 0.0F, 0.0F};            // camera focal(lookat) position at world space
  float m_distance = 0.0F;                            // distance between position and target
  glm::vec3 m_angle = {0.0F, 0.0F, 0.0F};             // angle in degree around the target (pitch, heading, roll)
  glm::mat4 m_matrix = glm::mat4(1.0F);               // 4x4 matrix combined rotation and translation
  glm::mat4 m_matrixRotation = glm::mat4(1.0F);       // rotation only
  glm::quat m_quaternion = {1.0F, 0.0F, 0.0F, 0.0F};  // quaternion for rotations

  // for position movement
  glm::vec3 m_movingFrom = {0.0F, 0.0F, 0.0F};    // camera starting position
  glm::vec3 m_movingTo = {0.0F, 0.0F, 0.0F};      // camera destination position
  glm::vec3 m_movingVector = {0.0F, 0.0F, 0.0F};  // normalized direction vector
  float m_movingTime = 0.0F;                      // animation elapsed time (sec)
  float m_movingDuration = 0.0F;                  // animation duration (sec)
  bool m_moving = false;                          // flag to start/stop animation
  anim::AnimationMode m_movingMode = anim::EASE_OUT;

  // for target movement (shift)
  glm::vec3 m_shiftingFrom = {0.0F, 0.0F, 0.0F};
  glm::vec3 m_shiftingTo = {0.0F, 0.0F, 0.0F};
  glm::vec3 m_shiftingVector = {0.0F, 0.0F, 0.0F};
  float m_shiftingTime = 0.0F;
  float m_shiftingDuration = 0.0F;
  float m_shiftingSpeed = 0.0F;     // current velocity of shift vector
  float m_shiftingAccel = 0.0F;     // acceleration, units per second squared
  float m_shiftingMaxSpeed = 0.0F;  // max velocity of shift vector
  bool m_shifting = false;
  anim::AnimationMode m_shiftingMode = anim::EASE_OUT;

  // for forwarding using distance between position and target
  float m_forwardingFrom = 0.0F;  // starting distance
  float m_forwardingTo = 0.0F;    // ending distance
  float m_forwardingTime = 0.0F;
  float m_forwardingDuration = 0.0F;
  float m_forwardingSpeed = 0.0F;  // current velocity of moving forward
  float m_forwardingAccel = 0.0F;
  float m_forwardingMaxSpeed = 0.0F;
  bool m_forwarding = false;
  anim::AnimationMode m_forwardingMode = anim::EASE_OUT;

  // for rotation
  glm::vec3 m_turningAngleFrom = {0.0F, 0.0F, 0.0F};
  glm::vec3 m_turningAngleTo = {0.0F, 0.0F, 0.0F};
  glm::quat m_turningQuaternionFrom = {1.0F, 0.0F, 0.0F, 0.0F};
  glm::quat m_turningQuaternionTo = {1.0F, 0.0F, 0.0F, 0.0F};
  float m_turningTime = 0.0F;
  float m_turningDuration = 0.0F;
  bool m_turning = false;
  bool m_quaternionUsed = false;  // flag to use quaternion
  anim::AnimationMode m_turningMode = anim::EASE_OUT;
};
