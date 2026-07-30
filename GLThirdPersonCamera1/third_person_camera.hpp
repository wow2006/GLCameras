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

#include <glm/gtc/quaternion.hpp>

//-----------------------------------------------------------------------------
// A quaternion based third person camera class.
//
// Call the lookAt() method to establish the distance and position of the
// camera relative to the target look at position. The lookAt() method stores
// this relationship to the target look at position in the offset vector. The
// look at position can be changed using the lookAt(target) method.
//
// Aside from the lookAt() method there is no way to move the camera's eye
// position. The camera's eye position is automatically calculated and updated
// in the update() method using the offset vector.
//
// Rotating the camera using the rotate() method allows the camera to be
// orbited around the the look at position.
//-----------------------------------------------------------------------------

class ThirdPersonCamera final {
public:
  ThirdPersonCamera();
  ~ThirdPersonCamera();

  void lookAt(const glm::vec3 &target);
  void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up);
  void perspective(float fovx, float aspect, float znear, float zfar);
  void rotate(float longitudeDegrees, float latitudeDegrees);
  void update(float elapsedTimeSec);

  [[nodiscard]] const glm::vec3 &getPosition() const;
  [[nodiscard]] const glm::quat &getOrientation() const;
  [[nodiscard]] const glm::mat4 &getViewMatrix() const;
  [[nodiscard]] const glm::vec3 &getViewDirection() const;
  [[nodiscard]] const glm::mat4 &getProjectionMatrix() const;
  [[nodiscard]] const glm::vec3 &getXAxis() const;
  [[nodiscard]] const glm::vec3 &getYAxis() const;
  [[nodiscard]] const glm::vec3 &getZAxis() const;

private:
  static constexpr float DEFAULT_FOVX = 80.0F;
  static constexpr float DEFAULT_ZFAR = 1000.0F;
  static constexpr float DEFAULT_ZNEAR = 1.0F;

  static constexpr glm::vec3 WORLD_XAXIS = {1.0F, 0.0F, 0.0F};
  static constexpr glm::vec3 WORLD_YAXIS = {0.0F, 1.0F, 0.0F};
  static constexpr glm::vec3 WORLD_ZAXIS = {0.0F, 0.0F, 1.0F};

  float m_longitudeDegrees;
  float m_latitudeDegrees;
  float m_fovx;
  float m_znear;
  float m_zfar;
  glm::vec3 m_eye;
  glm::vec3 m_target;
  glm::vec3 m_offset;
  glm::vec3 m_xAxis;
  glm::vec3 m_yAxis;
  glm::vec3 m_zAxis;
  glm::vec3 m_viewDir;
  glm::mat4 m_viewMatrix;
  glm::mat4 m_projMatrix;
  glm::quat m_orientation;
};

//-----------------------------------------------------------------------------

inline const glm::vec3 &ThirdPersonCamera::getPosition() const { return m_eye; }
inline const glm::quat &ThirdPersonCamera::getOrientation() const { return m_orientation; }
inline const glm::mat4 &ThirdPersonCamera::getViewMatrix() const { return m_viewMatrix; }
inline const glm::vec3 &ThirdPersonCamera::getViewDirection() const { return m_viewDir; }
inline const glm::mat4 &ThirdPersonCamera::getProjectionMatrix() const { return m_projMatrix; }
inline const glm::vec3 &ThirdPersonCamera::getXAxis() const { return m_xAxis; }
inline const glm::vec3 &ThirdPersonCamera::getYAxis() const { return m_yAxis; }
inline const glm::vec3 &ThirdPersonCamera::getZAxis() const { return m_zAxis; }
