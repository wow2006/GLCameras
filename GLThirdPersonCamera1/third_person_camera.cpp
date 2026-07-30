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

#include <cmath>
#include "third_person_camera.hpp"

namespace {
// mathlib's Quaternion(head, pitch, roll) built a NASA heading-pitch-roll
// (RzRxRy) rotation. mathlib composed quaternions left to right, so the glm
// equivalent is the reverse product.
glm::quat fromHeadPitchRoll(float headDegrees, float pitchDegrees, float rollDegrees) {
  glm::quat qHeading = glm::angleAxis(glm::radians(headDegrees), glm::vec3(0.0F, 1.0F, 0.0F));
  glm::quat qPitch = glm::angleAxis(glm::radians(pitchDegrees), glm::vec3(1.0F, 0.0F, 0.0F));
  glm::quat qRoll = glm::angleAxis(glm::radians(rollDegrees), glm::vec3(0.0F, 0.0F, 1.0F));
  return qRoll * qPitch * qHeading;
}
}  // namespace

ThirdPersonCamera::ThirdPersonCamera() {
  m_longitudeDegrees = 0.0F;
  m_latitudeDegrees = 0.0F;

  m_fovx = DEFAULT_FOVX;
  m_znear = DEFAULT_ZNEAR;
  m_zfar = DEFAULT_ZFAR;

  m_eye = {0.0F, 0.0F, 0.0F};
  m_target = {0.0F, 0.0F, 0.0F};
  m_offset = {0.0F, 0.0F, 0.0F};

  m_xAxis = {1.0F, 0.0F, 0.0F};
  m_yAxis = {0.0F, 1.0F, 0.0F};
  m_zAxis = {0.0F, 0.0F, 1.0F};
  m_viewDir = {0.0F, 0.0F, -1.0F};

  m_viewMatrix = glm::mat4(1.0F);
  m_projMatrix = glm::mat4(1.0F);
  m_orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
}

ThirdPersonCamera::~ThirdPersonCamera() = default;

void ThirdPersonCamera::lookAt(const glm::vec3 &target) { m_target = target; }

void ThirdPersonCamera::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {
  m_eye = eye;
  m_target = target;

  // The offset vector is the vector from the target position to the camera
  // position. This happens to also be the local z axis of the camera. Notice
  // that the offset vector is always relative to the 'target' position.

  m_offset = eye - target;
  m_zAxis = glm::normalize(m_offset);

  m_viewDir = -m_zAxis;

  m_xAxis = glm::normalize(glm::cross(up, m_zAxis));
  m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));

  m_viewMatrix[0][0] = m_xAxis.x;
  m_viewMatrix[1][0] = m_xAxis.y;
  m_viewMatrix[2][0] = m_xAxis.z;
  m_viewMatrix[3][0] = -glm::dot(m_xAxis, eye);

  m_viewMatrix[0][1] = m_yAxis.x;
  m_viewMatrix[1][1] = m_yAxis.y;
  m_viewMatrix[2][1] = m_yAxis.z;
  m_viewMatrix[3][1] = -glm::dot(m_yAxis, eye);

  m_viewMatrix[0][2] = m_zAxis.x;
  m_viewMatrix[1][2] = m_zAxis.y;
  m_viewMatrix[2][2] = m_zAxis.z;
  m_viewMatrix[3][2] = -glm::dot(m_zAxis, eye);

  m_viewMatrix[0][3] = 0.0F;
  m_viewMatrix[1][3] = 0.0F;
  m_viewMatrix[2][3] = 0.0F;
  m_viewMatrix[3][3] = 1.0F;

  m_orientation = glm::quat_cast(m_viewMatrix);
}

void ThirdPersonCamera::perspective(float fovx, float aspect, float znear, float zfar) {
  // We construct a projection matrix based on the horizontal field of view
  // 'fovx' rather than the more traditional 'fovy' used in gluPerspective().

  float e = 1.0F / tanf(glm::radians(fovx) / 2.0F);
  float aspectInv = 1.0F / aspect;
  float fovy = 2.0F * atanf(aspectInv / e);
  float xScale = 1.0F / tanf(0.5F * fovy);
  float yScale = xScale / aspectInv;

  m_projMatrix[0][0] = xScale;
  m_projMatrix[0][1] = 0.0F;
  m_projMatrix[0][2] = 0.0F;
  m_projMatrix[0][3] = 0.0F;

  m_projMatrix[1][0] = 0.0F;
  m_projMatrix[1][1] = yScale;
  m_projMatrix[1][2] = 0.0F;
  m_projMatrix[1][3] = 0.0F;

  m_projMatrix[2][0] = 0.0F;
  m_projMatrix[2][1] = 0.0F;
  m_projMatrix[2][2] = (zfar + znear) / (znear - zfar);
  m_projMatrix[2][3] = -1.0F;

  m_projMatrix[3][0] = 0.0F;
  m_projMatrix[3][1] = 0.0F;
  m_projMatrix[3][2] = (2.0F * zfar * znear) / (znear - zfar);
  m_projMatrix[3][3] = 0.0F;

  m_fovx = fovx;
  m_znear = znear;
  m_zfar = zfar;
}

void ThirdPersonCamera::rotate(float longitudeDegrees, float latitudeDegrees) {
  // Both 'longitudeDegrees' and 'latitudeDegrees' represents the maximum
  // number of degrees of rotation per second.

  m_latitudeDegrees = latitudeDegrees;
  m_longitudeDegrees = longitudeDegrees;
}

void ThirdPersonCamera::update(float elapsedTimeSec) {
  // This method must be called once per frame to rebuild the view matrix.
  // The most important part of this update() method is the camera's offset
  // vector. Everything depends on it. The offset vector describes the
  // camera's position relative to the camera's current look at position.
  // The offset vector is always relative to the current look at position.
  // Adding the offset vector to the current look at position will give us
  // the correct camera eye position. So applying rotations to the camera
  // really means rotating the offset vector. What we are basically doing
  // is orbiting the eye position about the look at position.

  // Determine how many degrees of rotation to apply based on current time.

  float latitudeElapsed = m_latitudeDegrees * elapsedTimeSec;
  float longitudeElapsed = m_longitudeDegrees * elapsedTimeSec;

  // Rotate the offset vector based on the current camera rotation.
  // The original used the quaternion triple product conj(q) * v * q, which is
  // the left-to-right mathlib spelling of glm's q * v * conj(q) - i.e. exactly
  // what glm's quat-vector product already does.

  const glm::quat rotation = fromHeadPitchRoll(longitudeElapsed, latitudeElapsed, 0.0F);
  const glm::vec3 transformedOffsetVector = rotation * m_offset;

  // Once the offset vector has been rotated into its new orientation we
  // use the transformed offset vector to calculate the new camera 'eye'
  // position based on the camera's current target 'at' position. We do this
  // to ensure that the camera is always at the required distance from the
  // target 'at' position.

  const glm::vec3 newCameraPosition = transformedOffsetVector + m_target;

  // Rebuild the view matrix.

  lookAt(newCameraPosition, m_target, WORLD_YAXIS);
}
