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
// Internal
#include "camera.h"

Camera::Camera() {
  ZoneScoped;  // NOLINT
  m_behavior = CameraBehavior::CAMERA_BEHAVIOR_FLIGHT;

  m_fovx = DEFAULT_FOVX;
  m_znear = DEFAULT_ZNEAR;
  m_zfar = DEFAULT_ZFAR;
  m_aspectRatio = 0.0f;

  m_accumPitchDegrees = 0.0f;

  m_eye = {0.0f, 0.0f, 0.0f};
  m_xAxis = {1.0f, 0.0f, 0.0f};
  m_yAxis = {0.0f, 1.0f, 0.0f};
  m_zAxis = {0.0f, 0.0f, 1.0f};
  m_viewDir = {0.0f, 0.0f, -1.0f};

  m_acceleration = {0.0f, 0.0f, 0.0f};
  m_currentVelocity = {0.0f, 0.0f, 0.0f};
  m_velocity = {0.0f, 0.0f, 0.0f};

  m_viewMatrix = glm::mat4(1.F);
  m_projMatrix = glm::mat4(1.F);
}

Camera::~Camera() = default;

void Camera::lookAt(const glm::vec3 &target) {
  glm::lookAt(m_eye, target, m_yAxis);
  // Extract the pitch angle from the view matrix.
  m_accumPitchDegrees = glm::degrees(-asinf(m_viewMatrix[1][2]));
}

void Camera::move(float dx, float dy, float dz)
{
    ZoneScoped; // NOLINT
  // Moves the camera by dx world units to the left or right; dy
  // world units upwards or downwards; and dz world units forwards
  // or backwards.

  glm::vec3 eye = m_eye;
  glm::vec3 forwards;

  if(m_behavior == CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON) {
    // Calculate the forwards direction. Can't just use the camera's local
    // z axis as doing so will cause the camera to move more slowly as the
    // camera's view approaches 90 degrees straight up and down.

    forwards = glm::normalize(glm::cross(WORLD_YAXIS, m_xAxis));
  } else {
    forwards = m_viewDir;
  }

  eye += m_xAxis * dx;
  eye += WORLD_YAXIS * dy;
  eye += forwards * dz;

  setPosition(eye);
}

void Camera::move(const glm::vec3 &direction, const glm::vec3 &amount) {
    ZoneScoped; // NOLINT
  // Moves the camera by the specified amount of world units in the specified
  // direction in world space.

  m_eye.x += direction.x * amount.x;
  m_eye.y += direction.y * amount.y;
  m_eye.z += direction.z * amount.z;

  updateViewMatrix(false);
}

void Camera::perspective(float fovx, float aspect, float znear, float zfar)
{
    ZoneScoped; // NOLINT
  // Construct a projection matrix based on the horizontal field of view
  // 'fovx' rather than the more traditional vertical field of view 'fovy'.

  const float e = 1.0F / tanf(glm::radians(fovx) / 2.0F);
  const float aspectInv = 1.0F / aspect;
  const float fovy = 2.0F * atanf(aspectInv / e);
  const float xScale = 1.0F / tanf(0.5F * fovy);
  const float yScale = xScale / aspectInv;

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
  m_aspectRatio = aspect;
  m_znear = znear;
  m_zfar = zfar;
}

void Camera::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
  ZoneScoped;  // NOLINT

  // Rotates the camera based on its current behavior.
  // Note that not all behaviors support rolling.

  switch(m_behavior) {
  case CameraBehavior::CAMERA_BEHAVIOR_FIRST_PERSON: rotateFirstPerson(headingDegrees, pitchDegrees); break;

  case CameraBehavior::CAMERA_BEHAVIOR_FLIGHT: rotateFlight(headingDegrees, pitchDegrees, rollDegrees); break;
  }

  updateViewMatrix(true);
}

void Camera::rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees)
{
    ZoneScoped; // NOLINT
    glm::mat4 rotMtx;

  // Rotate camera's existing x and z axes about its existing y axis.
  if(headingDegrees != 0.0f) {
    rotMtx = glm::rotate(glm::mat4(1.F), glm::radians(headingDegrees), m_yAxis);
    m_xAxis = glm::vec4(m_xAxis, 1) * rotMtx;
    m_zAxis = glm::vec4(m_zAxis, 1) * rotMtx;
  }

  // Rotate camera's existing y and z axes about its existing x axis.
  if(pitchDegrees != 0.0f) {
    rotMtx = glm::rotate(glm::mat4(1.F), glm::radians(pitchDegrees), m_xAxis);
    m_yAxis = glm::vec4(m_yAxis, 1) * rotMtx;
    m_zAxis = glm::vec4(m_zAxis, 1) * rotMtx;
  }

  // Rotate camera's existing x and y axes about its existing z axis.
  if(rollDegrees != 0.0f) {
    rotMtx = glm::rotate(glm::mat4(1.F), glm::radians(rollDegrees), m_zAxis);
    m_xAxis = glm::vec4(m_xAxis, 1) * rotMtx;
    m_yAxis = glm::vec4(m_yAxis, 1) * rotMtx;
  }
}

void Camera::rotateFirstPerson(float headingDegrees, float pitchDegrees) {
    ZoneScoped;  // NOLINT

    m_accumPitchDegrees += pitchDegrees;

  if(m_accumPitchDegrees > 90.0F) {
      pitchDegrees = 90.0F - (m_accumPitchDegrees - pitchDegrees);
    m_accumPitchDegrees = 90.0F;
  }

  if(m_accumPitchDegrees < -90.0F) {
    pitchDegrees = -90.0F - (m_accumPitchDegrees - pitchDegrees);
    m_accumPitchDegrees = -90.0F;
  }

  glm::mat4 rotMtx;

  // Rotate camera's existing x and z axes about the world y axis.
  if(headingDegrees != 0.0f) {
    rotMtx = glm::rotate(glm::mat4(1.F), glm::radians(headingDegrees), WORLD_YAXIS);
    m_xAxis = glm::vec4(m_xAxis, 1) * rotMtx;
    m_zAxis = glm::vec4(m_zAxis, 1) * rotMtx;
  }

  // Rotate camera's existing y and z axes about its existing x axis.
  if(pitchDegrees != 0.0f) {
    rotMtx = glm::rotate(glm::mat4(1.F), glm::radians(pitchDegrees), m_xAxis);
    m_yAxis = glm::vec4(m_yAxis, 1) * rotMtx;
    m_zAxis = glm::vec4(m_zAxis, 1) * rotMtx;
  }
}

void Camera::updatePosition(const glm::vec3 &direction, float elapsedTimeSec) {
  // Moves the camera using Newton's second law of motion. Unit mass is
  // assumed here to somewhat simplify the calculations. The direction vector
  // is in the range [-1,1].

  if(glm::dot(m_currentVelocity, m_currentVelocity) != 0.0f) {
    // Only move the camera if the velocity vector is not of zero length.
    // Doing this guards against the camera slowly creeping around due to
    // floating point rounding errors.

    glm::vec3 displacement = (m_currentVelocity * elapsedTimeSec) + (0.5f * m_acceleration * elapsedTimeSec * elapsedTimeSec);

    // Floating point rounding errors will slowly accumulate and cause the
    // camera to move along each axis. To prevent any unintended movement
    // the displacement vector is clamped to zero for each direction that
    // the camera isn't moving in. Note that the updateVelocity() method
    // will slowly decelerate the camera's velocity back to a stationary
    // state when the camera is no longer moving along that direction. To
    // account for this the camera's current velocity is also checked.

    if(direction.x == 0.0f && glm::abs(m_currentVelocity.x - 0.0f) <= 0.F) {
      displacement.x = 0.0f;
    }

    if(direction.y == 0.0f && glm::abs(m_currentVelocity.y - 0.0f) <= 0.F) {
      displacement.y = 0.0f;
    }

    if(direction.z == 0.0f && glm::abs(m_currentVelocity.z - 0.0f) <= 0.F) {
      displacement.z = 0.0f;
    }

    move(displacement.x, displacement.y, displacement.z);
  }

  // Continuously update the camera's velocity vector even if the camera
  // hasn't moved during this call. When the camera is no longer being moved
  // the camera is decelerating back to its stationary state.

  updateVelocity(direction, elapsedTimeSec);
}

void Camera::updateVelocity(const glm::vec3 &direction, float elapsedTimeSec) {
  // Updates the camera's velocity based on the supplied movement direction
  // and the elapsed time (since this method was last called). The movement
  // direction is in the range [-1,1].

  if(direction.x != 0.0f) {
    // Camera is moving along the x axis.
    // Linearly accelerate up to the camera's max speed.

    m_currentVelocity.x += direction.x * m_acceleration.x * elapsedTimeSec;

    if(m_currentVelocity.x > m_velocity.x)
      m_currentVelocity.x = m_velocity.x;
    else if(m_currentVelocity.x < -m_velocity.x)
      m_currentVelocity.x = -m_velocity.x;
  } else {
    // Camera is no longer moving along the x axis.
    // Linearly decelerate back to stationary state.

    if(m_currentVelocity.x > 0.0f) {
      if((m_currentVelocity.x -= m_acceleration.x * elapsedTimeSec) < 0.0f)
        m_currentVelocity.x = 0.0f;
    } else {
      if((m_currentVelocity.x += m_acceleration.x * elapsedTimeSec) > 0.0f)
        m_currentVelocity.x = 0.0f;
    }
  }

  if(direction.y != 0.0f) {
    // Camera is moving along the y axis.
    // Linearly accelerate up to the camera's max speed.

    m_currentVelocity.y += direction.y * m_acceleration.y * elapsedTimeSec;

    if(m_currentVelocity.y > m_velocity.y)
      m_currentVelocity.y = m_velocity.y;
    else if(m_currentVelocity.y < -m_velocity.y)
      m_currentVelocity.y = -m_velocity.y;
  } else {
    // Camera is no longer moving along the y axis.
    // Linearly decelerate back to stationary state.

    if(m_currentVelocity.y > 0.0f) {
      if((m_currentVelocity.y -= m_acceleration.y * elapsedTimeSec) < 0.0f)
        m_currentVelocity.y = 0.0f;
    } else {
      if((m_currentVelocity.y += m_acceleration.y * elapsedTimeSec) > 0.0f)
        m_currentVelocity.y = 0.0f;
    }
  }

  if(direction.z != 0.0f) {
    // Camera is moving along the z axis.
    // Linearly accelerate up to the camera's max speed.

    m_currentVelocity.z += direction.z * m_acceleration.z * elapsedTimeSec;

    if(m_currentVelocity.z > m_velocity.z)
      m_currentVelocity.z = m_velocity.z;
    else if(m_currentVelocity.z < -m_velocity.z)
      m_currentVelocity.z = -m_velocity.z;
  } else {
    // Camera is no longer moving along the z axis.
    // Linearly decelerate back to stationary state.

    if(m_currentVelocity.z > 0.0f) {
      if((m_currentVelocity.z -= m_acceleration.z * elapsedTimeSec) < 0.0f)
        m_currentVelocity.z = 0.0f;
    } else {
      if((m_currentVelocity.z += m_acceleration.z * elapsedTimeSec) > 0.0f)
        m_currentVelocity.z = 0.0f;
    }
  }
}

void Camera::updateViewMatrix(bool orthogonalizeAxes) {
  if(orthogonalizeAxes) {
    // Regenerate the camera's local axes to orthogonalize them.
    m_zAxis = glm::normalize(m_zAxis);

    m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));

    m_xAxis = glm::normalize(glm::cross(m_yAxis, m_zAxis));

    m_viewDir = -m_zAxis;
  }

  // Reconstruct the view matrix.
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

  m_viewMatrix[0][3] = 0.0F;
  m_viewMatrix[1][3] = 0.0F;
  m_viewMatrix[2][3] = 0.0F;
  m_viewMatrix[3][3] = 1.0F;
}
