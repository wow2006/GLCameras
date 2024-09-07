//-----------------------------------------------------------------------------
// Copyright (c) 2006-2008 dhpoware. All Rights Reserved.
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

//-----------------------------------------------------------------------------
// A general purpose 6DoF (six degrees of freedom) vector based camera.
//
// This camera class supports 2 different behaviors:
// first person mode, and flight mode.
//
// First person mode only allows 5DOF (x axis movement, y axis movement, z axis
// movement, yaw, and pitch) and movement is always parallel to the world x-z
// (ground) plane.
//
// Flight mode supports 6DoF. This is the camera class' default behavior.
//
// This camera class allows the camera to be moved in 2 ways: using fixed
// step world units, and using a supplied velocity and acceleration. The former
// simply moves the camera by the specified amount. To move the camera in this
// way call one of the move() methods. The other way to move the camera
// calculates the camera's displacement based on the supplied velocity,
// acceleration, and elapsed time. To move the camera in this way call the
// updatePosition() method.
//-----------------------------------------------------------------------------

class Camera {
public:
  enum class CameraBehavior { CAMERA_BEHAVIOR_FIRST_PERSON, CAMERA_BEHAVIOR_FLIGHT };

  Camera();

  ~Camera();

  void lookAt(const glm::vec3 &target);

  void move(float dx, float dy, float dz);

  void move(const glm::vec3 &direction, const glm::vec3 &amount);

  void perspective(float fovx, float aspect, float znear, float zfar);

  void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);

  void updatePosition(const glm::vec3 &direction, float elapsedTimeSec);

  // Getter methods.
  const glm::vec3 &getAcceleration() const;

  CameraBehavior getBehavior() const;

  const glm::vec3 &getCurrentVelocity() const;

  const glm::vec3 &getPosition() const;

  const glm::mat4 &getProjectionMatrix() const;

  const glm::vec3 &getVelocity() const;

  const glm::vec3 &getViewDirection() const;

  const glm::mat4 &getViewMatrix() const;

  const glm::vec3 &getXAxis() const;

  const glm::vec3 &getYAxis() const;

  const glm::vec3 &getZAxis() const;

  // Setter methods.
  void setAcceleration(float x, float y, float z);

  void setAcceleration(const glm::vec3 &acceleration);

  void setBehavior(CameraBehavior newBehavior);

  void setCurrentVelocity(float x, float y, float z);

  void setCurrentVelocity(const glm::vec3 &currentVelocity);

  void setPosition(float x, float y, float z);

  void setPosition(const glm::vec3 &position);

  void setVelocity(float x, float y, float z);

  void setVelocity(const glm::vec3 &velocity);

private:
  void rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees);

  void rotateFirstPerson(float headingDegrees, float pitchDegrees);

  void updateVelocity(const glm::vec3 &direction, float elapsedTimeSec);

  void updateViewMatrix(bool orthogonalizeAxes);

  static constexpr float DEFAULT_FOVX = 90.0F;
  static constexpr float DEFAULT_ZFAR = 1000.0F;
  static constexpr float DEFAULT_ZNEAR = 0.1F;

  static constexpr glm::vec3 WORLD_XAXIS = {1.0F, 0.0F, 0.0F};
  static constexpr glm::vec3 WORLD_YAXIS = {0.0F, 1.0F, 0.0F};
  static constexpr glm::vec3 WORLD_ZAXIS = {0.0F, 0.0F, 1.0F};

  CameraBehavior m_behavior;
  float m_fovx;
  float m_znear;
  float m_zfar;
  float m_aspectRatio;
  float m_accumPitchDegrees;
  glm::vec3 m_eye;
  glm::vec3 m_xAxis;
  glm::vec3 m_yAxis;
  glm::vec3 m_zAxis;
  glm::vec3 m_viewDir;
  glm::vec3 m_acceleration;
  glm::vec3 m_currentVelocity;
  glm::vec3 m_velocity;
  glm::mat4 m_viewMatrix;
  glm::mat4 m_projMatrix;
};

//-----------------------------------------------------------------------------
inline const glm::vec3 &Camera::getAcceleration() const { return m_acceleration; }

inline Camera::CameraBehavior Camera::getBehavior() const { return m_behavior; }

inline const glm::vec3 &Camera::getCurrentVelocity() const { return m_currentVelocity; }

inline const glm::vec3 &Camera::getPosition() const { return m_eye; }

inline const glm::mat4 &Camera::getProjectionMatrix() const { return m_projMatrix; }

inline const glm::vec3 &Camera::getVelocity() const { return m_velocity; }

inline const glm::vec3 &Camera::getViewDirection() const { return m_viewDir; }

inline const glm::mat4 &Camera::getViewMatrix() const { return m_viewMatrix; }

inline const glm::vec3 &Camera::getXAxis() const { return m_xAxis; }

inline const glm::vec3 &Camera::getYAxis() const { return m_yAxis; }

inline const glm::vec3 &Camera::getZAxis() const { return m_zAxis; }
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
inline void Camera::setAcceleration(float x, float y, float z) { m_acceleration = {x, y, z}; }

inline void Camera::setAcceleration(const glm::vec3 &acceleration) { m_acceleration = acceleration; }

inline void Camera::setBehavior(CameraBehavior newBehavior) {
  if(m_behavior == CameraBehavior::CAMERA_BEHAVIOR_FLIGHT && newBehavior == CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON) {
    // Moving from flight-simulator mode to first-person.
    // Need to ignore camera roll, but retain existing pitch and heading.
    m_viewMatrix = glm::lookAt(m_eye, m_eye - m_zAxis, WORLD_YAXIS);
  }

  m_behavior = newBehavior;
}

inline void Camera::setCurrentVelocity(float x, float y, float z) { m_currentVelocity = {x, y, z}; }

inline void Camera::setCurrentVelocity(const glm::vec3 &currentVelocity) { m_currentVelocity = currentVelocity; }

inline void Camera::setPosition(float x, float y, float z) {
  m_eye = {x, y, z};
  updateViewMatrix(false);
}

inline void Camera::setPosition(const glm::vec3 &position) {
  m_eye = position;
  updateViewMatrix(false);
}

inline void Camera::setVelocity(float x, float y, float z) { m_velocity = {x, y, z}; }

inline void Camera::setVelocity(const glm::vec3 &velocity) { m_velocity = velocity; }
//-----------------------------------------------------------------------------