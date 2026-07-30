//-----------------------------------------------------------------------------
// Copyright (c) 2008 dhpoware. All Rights Reserved.
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

//-----------------------------------------------------------------------------
// A quaternion based third person camera class. This camera model incorporates
// a spring system to smooth out camera movement. It is enabled by default but
// can be disabled.
//
// Call lookAt(eye, target, up) to establish the camera's initial settings.
// This will define the camera's initial position in relation to the target
// that the camera will be looking. When the target moves call the camera's
// lookAt(target) method to set the target's new world position. When the
// target rotates call the camera's rotate() method.
//
// The camera's update() method must be called once per frame. The update()
// method performs any pending camera rotations and recalculates the camera's
// view matrix.
//-----------------------------------------------------------------------------

class ThirdPersonCamera final {
public:
  ThirdPersonCamera();
  ~ThirdPersonCamera();

  void lookAt(const glm::vec3 &target);
  void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up);
  void perspective(float fovx, float aspect, float znear, float zfar);
  void rotate(float headingDegrees, float pitchDegrees);
  void update(float elapsedTimeSec);

  [[nodiscard]] float getDampingConstant() const;
  [[nodiscard]] float getOffsetDistance() const;
  [[nodiscard]] const glm::quat &getOrientation() const;
  [[nodiscard]] const glm::vec3 &getPosition() const;
  [[nodiscard]] const glm::mat4 &getProjectionMatrix() const;
  [[nodiscard]] float getSpringConstant() const;
  [[nodiscard]] const glm::vec3 &getTargetYAxis() const;
  [[nodiscard]] const glm::vec3 &getViewDirection() const;
  [[nodiscard]] const glm::mat4 &getViewMatrix() const;
  [[nodiscard]] const glm::vec3 &getXAxis() const;
  [[nodiscard]] const glm::vec3 &getYAxis() const;
  [[nodiscard]] const glm::vec3 &getZAxis() const;
  [[nodiscard]] bool springSystemIsEnabled() const;

  void enableSpringSystem(bool enableSpringSystem);
  void setOffsetDistance(float offsetDistance);
  void setSpringConstant(float springConstant);

private:
  void updateOrientation(float elapsedTimeSec);
  void updateViewMatrix();
  void updateViewMatrix(float elapsedTimeSec);

  static constexpr float DEFAULT_SPRING_CONSTANT = 16.0F;
  static constexpr float DEFAULT_DAMPING_CONSTANT = 8.0F;

  static constexpr float DEFAULT_FOVX = 80.0F;
  static constexpr float DEFAULT_ZFAR = 1000.0F;
  static constexpr float DEFAULT_ZNEAR = 1.0F;

  static constexpr glm::vec3 WORLD_XAXIS = {1.0F, 0.0F, 0.0F};
  static constexpr glm::vec3 WORLD_YAXIS = {0.0F, 1.0F, 0.0F};
  static constexpr glm::vec3 WORLD_ZAXIS = {0.0F, 0.0F, 1.0F};

  bool m_enableSpringSystem;
  float m_springConstant;
  float m_dampingConstant;
  float m_offsetDistance;
  float m_headingDegrees;
  float m_pitchDegrees;
  float m_fovx;
  float m_znear;
  float m_zfar;
  glm::vec3 m_eye;
  glm::vec3 m_target;
  glm::vec3 m_targetYAxis;
  glm::vec3 m_xAxis;
  glm::vec3 m_yAxis;
  glm::vec3 m_zAxis;
  glm::vec3 m_viewDir;
  glm::vec3 m_velocity;
  glm::mat4 m_viewMatrix;
  glm::mat4 m_projMatrix;
  glm::quat m_orientation;
};

//-----------------------------------------------------------------------------

inline float ThirdPersonCamera::getDampingConstant() const { return m_dampingConstant; }
inline float ThirdPersonCamera::getOffsetDistance() const { return m_offsetDistance; }
inline const glm::quat &ThirdPersonCamera::getOrientation() const { return m_orientation; }
inline const glm::vec3 &ThirdPersonCamera::getPosition() const { return m_eye; }
inline const glm::mat4 &ThirdPersonCamera::getProjectionMatrix() const { return m_projMatrix; }
inline float ThirdPersonCamera::getSpringConstant() const { return m_springConstant; }
inline const glm::vec3 &ThirdPersonCamera::getTargetYAxis() const { return m_targetYAxis; }
inline const glm::vec3 &ThirdPersonCamera::getViewDirection() const { return m_viewDir; }
inline const glm::mat4 &ThirdPersonCamera::getViewMatrix() const { return m_viewMatrix; }
inline const glm::vec3 &ThirdPersonCamera::getXAxis() const { return m_xAxis; }
inline const glm::vec3 &ThirdPersonCamera::getYAxis() const { return m_yAxis; }
inline const glm::vec3 &ThirdPersonCamera::getZAxis() const { return m_zAxis; }
inline bool ThirdPersonCamera::springSystemIsEnabled() const { return m_enableSpringSystem; }
