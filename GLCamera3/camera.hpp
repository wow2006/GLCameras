//-----------------------------------------------------------------------------
// Copyright (c) 2007-2008 dhpoware. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#pragma once

#include <glm/gtc/quaternion.hpp>

class Camera final {
public:
  enum CameraBehavior : uint8_t {
    CAMERA_BEHAVIOR_FIRST_PERSON,
    CAMERA_BEHAVIOR_SPECTATOR,
    CAMERA_BEHAVIOR_FLIGHT,
    CAMERA_BEHAVIOR_ORBIT
  };

  Camera();
  ~Camera();

  void lookAt(const glm::vec3 &target);
  void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up);
  void move(float dx, float dy, float dz);
  void move(const glm::vec3 &direction, const glm::vec3 &amount);
  void perspective(float fovx, float aspect, float znear, float zfar);
  void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);
  void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees);
  void undoRoll();
  void updatePosition(const glm::vec3 &direction, float elapsedTimeSec);
  void zoom(float zoom, float minZoom, float maxZoom);

  [[nodiscard]] const glm::vec3 &getAcceleration() const;
  [[nodiscard]] CameraBehavior getBehavior() const;
  [[nodiscard]] const glm::vec3 &getCurrentVelocity() const;
  [[nodiscard]] const glm::vec3 &getPosition() const;
  [[nodiscard]] float getOrbitMinZoom() const;
  [[nodiscard]] float getOrbitMaxZoom() const;
  [[nodiscard]] float getOrbitOffsetDistance() const;
  [[nodiscard]] const glm::quat &getOrientation() const;
  [[nodiscard]] float getRotationSpeed() const;
  [[nodiscard]] const glm::mat4 &getProjectionMatrix() const;
  [[nodiscard]] const glm::vec3 &getVelocity() const;
  [[nodiscard]] const glm::vec3 &getViewDirection() const;
  [[nodiscard]] const glm::mat4 &getViewMatrix() const;
  [[nodiscard]] const glm::vec3 &getXAxis() const;
  [[nodiscard]] const glm::vec3 &getYAxis() const;
  [[nodiscard]] const glm::vec3 &getZAxis() const;
  [[nodiscard]] bool preferTargetYAxisOrbiting() const;

  void setAcceleration(const glm::vec3 &acceleration);
  void setBehavior(CameraBehavior newBehavior);
  void setCurrentVelocity(const glm::vec3 &currentVelocity);
  void setCurrentVelocity(float x, float y, float z);
  void setOrbitMaxZoom(float orbitMaxZoom);
  void setOrbitMinZoom(float orbitMinZoom);
  void setOrbitOffsetDistance(float orbitOffsetDistance);
  void setOrientation(const glm::quat &newOrientation);
  void setPosition(const glm::vec3 &newEye);
  void setPreferTargetYAxisOrbiting(bool prefer);
  void setRotationSpeed(float rotationSpeed);
  void setVelocity(const glm::vec3 &velocity);
  void setVelocity(float x, float y, float z);

private:
  void rotateFirstPerson(float headingDegrees, float pitchDegrees);
  void rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees);
  void rotateOrbit(float headingDegrees, float pitchDegrees, float rollDegrees);
  void updateVelocity(const glm::vec3 &direction, float elapsedTimeSec);
  void updateViewMatrix();

  static constexpr float DEFAULT_ROTATION_SPEED = 0.3F;
  static constexpr float DEFAULT_FOVX = 90.0F;
  static constexpr float DEFAULT_ZNEAR = 0.1F;
  static constexpr float DEFAULT_ZFAR = 1000.0F;
  static constexpr float DEFAULT_ORBIT_MIN_ZOOM = DEFAULT_ZNEAR + 1.0F;
  static constexpr float DEFAULT_ORBIT_MAX_ZOOM = DEFAULT_ZFAR * 0.5F;
  static constexpr float DEFAULT_ORBIT_OFFSET_DISTANCE =
      DEFAULT_ORBIT_MIN_ZOOM + (DEFAULT_ORBIT_MAX_ZOOM - DEFAULT_ORBIT_MIN_ZOOM) * 0.25F;

  static constexpr glm::vec3 WORLD_XAXIS = {1.0F, 0.0F, 0.0F};
  static constexpr glm::vec3 WORLD_YAXIS = {0.0F, 1.0F, 0.0F};
  static constexpr glm::vec3 WORLD_ZAXIS = {0.0F, 0.0F, 1.0F};

  CameraBehavior m_behavior;
  bool m_preferTargetYAxisOrbiting;
  float m_accumPitchDegrees;
  float m_savedAccumPitchDegrees;
  float m_rotationSpeed;
  float m_fovx;
  float m_aspectRatio;
  float m_znear;
  float m_zfar;
  float m_orbitMinZoom;
  float m_orbitMaxZoom;
  float m_orbitOffsetDistance;
  float m_firstPersonYOffset;
  glm::vec3 m_eye;
  glm::vec3 m_savedEye;
  glm::vec3 m_target;
  glm::vec3 m_targetYAxis;
  glm::vec3 m_xAxis;
  glm::vec3 m_yAxis;
  glm::vec3 m_zAxis;
  glm::vec3 m_viewDir;
  glm::vec3 m_acceleration;
  glm::vec3 m_currentVelocity;
  glm::vec3 m_velocity;
  glm::quat m_orientation;
  glm::quat m_savedOrientation;
  glm::mat4 m_viewMatrix;
  glm::mat4 m_projMatrix;
};

//-----------------------------------------------------------------------------

inline const glm::vec3 &Camera::getAcceleration() const { return m_acceleration; }
inline Camera::CameraBehavior Camera::getBehavior() const { return m_behavior; }
inline const glm::vec3 &Camera::getCurrentVelocity() const { return m_currentVelocity; }
inline const glm::vec3 &Camera::getPosition() const { return m_eye; }
inline float Camera::getOrbitMinZoom() const { return m_orbitMinZoom; }
inline float Camera::getOrbitMaxZoom() const { return m_orbitMaxZoom; }
inline float Camera::getOrbitOffsetDistance() const { return m_orbitOffsetDistance; }
inline const glm::quat &Camera::getOrientation() const { return m_orientation; }
inline float Camera::getRotationSpeed() const { return m_rotationSpeed; }
inline const glm::mat4 &Camera::getProjectionMatrix() const { return m_projMatrix; }
inline const glm::vec3 &Camera::getVelocity() const { return m_velocity; }
inline const glm::vec3 &Camera::getViewDirection() const { return m_viewDir; }
inline const glm::mat4 &Camera::getViewMatrix() const { return m_viewMatrix; }
inline const glm::vec3 &Camera::getXAxis() const { return m_xAxis; }
inline const glm::vec3 &Camera::getYAxis() const { return m_yAxis; }
inline const glm::vec3 &Camera::getZAxis() const { return m_zAxis; }
inline bool Camera::preferTargetYAxisOrbiting() const { return m_preferTargetYAxisOrbiting; }
