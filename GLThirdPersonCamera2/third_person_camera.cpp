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

#include <cmath>
#include "third_person_camera.hpp"

ThirdPersonCamera::ThirdPersonCamera() {
  m_enableSpringSystem = true;
  m_springConstant = DEFAULT_SPRING_CONSTANT;
  m_dampingConstant = DEFAULT_DAMPING_CONSTANT;

  m_offsetDistance = 0.0F;
  m_headingDegrees = 0.0F;
  m_pitchDegrees = 0.0F;

  m_fovx = DEFAULT_FOVX;
  m_znear = DEFAULT_ZNEAR;
  m_zfar = DEFAULT_ZFAR;

  m_eye = {0.0F, 0.0F, 0.0F};
  m_target = {0.0F, 0.0F, 0.0F};
  m_targetYAxis = {0.0F, 1.0F, 0.0F};

  m_xAxis = {1.0F, 0.0F, 0.0F};
  m_yAxis = {0.0F, 1.0F, 0.0F};
  m_zAxis = {0.0F, 0.0F, 1.0F};
  m_viewDir = {0.0F, 0.0F, -1.0F};

  m_velocity = {0.0F, 0.0F, 0.0F};

  m_viewMatrix = glm::mat4(1.0F);
  m_projMatrix = glm::mat4(1.0F);
  m_orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
}

ThirdPersonCamera::~ThirdPersonCamera() = default;

void ThirdPersonCamera::enableSpringSystem(bool enableSpringSystem) { m_enableSpringSystem = enableSpringSystem; }

void ThirdPersonCamera::lookAt(const glm::vec3 &target) { m_target = target; }

void ThirdPersonCamera::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {
  m_eye = eye;
  m_target = target;
  m_targetYAxis = up;

  m_zAxis = glm::normalize(eye - target);

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

  m_offsetDistance = glm::length(m_target - m_eye);
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

void ThirdPersonCamera::rotate(float headingDegrees, float pitchDegrees) {
  m_headingDegrees = -headingDegrees;
  m_pitchDegrees = -pitchDegrees;
}

void ThirdPersonCamera::setOffsetDistance(float offsetDistance) { m_offsetDistance = offsetDistance; }

void ThirdPersonCamera::setSpringConstant(float springConstant) {
  // We're using a critically damped spring system where the damping ratio
  // is equal to one.
  //
  // damping ratio = m_dampingConstant / (2.0f * sqrtf(m_springConstant))

  m_springConstant = springConstant;
  m_dampingConstant = 2.0F * sqrtf(springConstant);
}

void ThirdPersonCamera::update(float elapsedTimeSec) {
  updateOrientation(elapsedTimeSec);

  if(m_enableSpringSystem)
    updateViewMatrix(elapsedTimeSec);
  else
    updateViewMatrix();
}

void ThirdPersonCamera::updateOrientation(float elapsedTimeSec) {
  m_pitchDegrees *= elapsedTimeSec;
  m_headingDegrees *= elapsedTimeSec;

  // mathlib composed quaternions left to right where glm composes right to
  // left, so both products below are reversed. Heading still turns about the
  // target's y axis in world space and pitch about the camera's local x axis.

  if(m_headingDegrees != 0.0F) {
    const glm::quat rot = glm::angleAxis(glm::radians(m_headingDegrees), m_targetYAxis);
    m_orientation = m_orientation * rot;
  }

  if(m_pitchDegrees != 0.0F) {
    const glm::quat rot = glm::angleAxis(glm::radians(m_pitchDegrees), WORLD_XAXIS);
    m_orientation = rot * m_orientation;
  }
}

void ThirdPersonCamera::updateViewMatrix() {
  m_viewMatrix = glm::mat4_cast(m_orientation);

  m_xAxis = {m_viewMatrix[0][0], m_viewMatrix[1][0], m_viewMatrix[2][0]};
  m_yAxis = {m_viewMatrix[0][1], m_viewMatrix[1][1], m_viewMatrix[2][1]};
  m_zAxis = {m_viewMatrix[0][2], m_viewMatrix[1][2], m_viewMatrix[2][2]};
  m_viewDir = -m_zAxis;

  m_eye = m_target + m_zAxis * m_offsetDistance;

  m_viewMatrix[3][0] = -glm::dot(m_xAxis, m_eye);
  m_viewMatrix[3][1] = -glm::dot(m_yAxis, m_eye);
  m_viewMatrix[3][2] = -glm::dot(m_zAxis, m_eye);
}

void ThirdPersonCamera::updateViewMatrix(float elapsedTimeSec) {
  m_viewMatrix = glm::mat4_cast(m_orientation);

  m_xAxis = {m_viewMatrix[0][0], m_viewMatrix[1][0], m_viewMatrix[2][0]};
  m_yAxis = {m_viewMatrix[0][1], m_viewMatrix[1][1], m_viewMatrix[2][1]};
  m_zAxis = {m_viewMatrix[0][2], m_viewMatrix[1][2], m_viewMatrix[2][2]};

  // Calculate the new camera position. The 'idealPosition' is where the
  // camera should be position. The camera should be positioned directly
  // behind the target at the required offset distance. What we're doing here
  // is rather than have the camera immediately snap to the 'idealPosition'
  // we slowly move the camera towards the 'idealPosition' using a spring
  // system.
  //
  // References:
  //   Stone, Jonathan, "Third-Person Camera Navigation," Game Programming
  //     Gems 4, Andrew Kirmse, Editor, Charles River Media, Inc., 2004.

  const glm::vec3 idealPosition = m_target + m_zAxis * m_offsetDistance;
  const glm::vec3 displacement = m_eye - idealPosition;
  const glm::vec3 springAcceleration = (-m_springConstant * displacement) - (m_dampingConstant * m_velocity);

  m_velocity += springAcceleration * elapsedTimeSec;
  m_eye += m_velocity * elapsedTimeSec;

  // The view matrix is always relative to the camera's current position
  // 'm_eye'. Since a spring system is being used here 'm_eye' will be
  // relative to 'idealPosition'. When the camera is no longer being
  // moved 'm_eye' will become the same as 'idealPosition'. The local
  // x, y, and z axes that were extracted from the camera's orientation
  // 'm_orienation' is correct for the 'idealPosition' only. We need
  // to recompute these axes so that they're relative to 'm_eye'. Once
  // that's done we can use those axes to reconstruct the view matrix.

  m_zAxis = glm::normalize(m_eye - m_target);

  m_xAxis = glm::normalize(glm::cross(m_targetYAxis, m_zAxis));
  m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));

  m_viewMatrix = glm::mat4(1.0F);

  m_viewMatrix[0][0] = m_xAxis.x;
  m_viewMatrix[1][0] = m_xAxis.y;
  m_viewMatrix[2][0] = m_xAxis.z;
  m_viewMatrix[3][0] = -glm::dot(m_xAxis, m_eye);

  m_viewMatrix[0][1] = m_yAxis.x;
  m_viewMatrix[1][1] = m_yAxis.y;
  m_viewMatrix[2][1] = m_yAxis.z;
  m_viewMatrix[3][1] = -glm::dot(m_yAxis, m_eye);

  m_viewMatrix[0][2] = m_zAxis.x;
  m_viewMatrix[1][2] = m_zAxis.y;
  m_viewMatrix[2][2] = m_zAxis.z;
  m_viewMatrix[3][2] = -glm::dot(m_zAxis, m_eye);

  m_viewDir = -m_zAxis;
}
