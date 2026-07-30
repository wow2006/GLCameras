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

#include <algorithm>
#include <cmath>
#include "camera.hpp"

namespace {
glm::quat fromHeadPitchRoll(float headDegrees, float pitchDegrees, float rollDegrees) {
  glm::quat qHeading = glm::angleAxis(glm::radians(headDegrees), glm::vec3(0.0F, 1.0F, 0.0F));
  glm::quat qPitch = glm::angleAxis(glm::radians(pitchDegrees), glm::vec3(1.0F, 0.0F, 0.0F));
  glm::quat qRoll = glm::angleAxis(glm::radians(rollDegrees), glm::vec3(0.0F, 0.0F, 1.0F));
  return qRoll * qPitch * qHeading;
}
}  // namespace

Camera::Camera() {
  m_behavior = CAMERA_BEHAVIOR_FLIGHT;
  m_preferTargetYAxisOrbiting = true;

  m_accumPitchDegrees = 0.0F;
  m_savedAccumPitchDegrees = 0.0F;

  m_rotationSpeed = DEFAULT_ROTATION_SPEED;
  m_fovx = DEFAULT_FOVX;
  m_aspectRatio = 0.0F;
  m_znear = DEFAULT_ZNEAR;
  m_zfar = DEFAULT_ZFAR;

  m_orbitMinZoom = DEFAULT_ORBIT_MIN_ZOOM;
  m_orbitMaxZoom = DEFAULT_ORBIT_MAX_ZOOM;
  m_orbitOffsetDistance = DEFAULT_ORBIT_OFFSET_DISTANCE;
  m_firstPersonYOffset = 0.0F;

  m_eye = {0.0F, 0.0F, 0.0F};
  m_savedEye = {0.0F, 0.0F, 0.0F};
  m_target = {0.0F, 0.0F, 0.0F};
  m_targetYAxis = {0.0F, 1.0F, 0.0F};
  m_xAxis = {1.0F, 0.0F, 0.0F};
  m_yAxis = {0.0F, 1.0F, 0.0F};
  m_zAxis = {0.0F, 0.0F, 1.0F};
  m_viewDir = {0.0F, 0.0F, -1.0F};

  m_acceleration = {0.0F, 0.0F, 0.0F};
  m_currentVelocity = {0.0F, 0.0F, 0.0F};
  m_velocity = {0.0F, 0.0F, 0.0F};

  m_orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
  m_savedOrientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);

  m_viewMatrix = glm::mat4(1.0F);
  m_projMatrix = glm::mat4(1.0F);
}

Camera::~Camera() = default;

void Camera::lookAt(const glm::vec3 &target) { lookAt(m_eye, target, m_yAxis); }

void Camera::lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) {
  m_eye = eye;
  m_target = target;

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

  m_accumPitchDegrees = glm::degrees(asinf(m_viewMatrix[1][2]));
  m_orientation = glm::quat_cast(m_viewMatrix);
  updateViewMatrix();
}

void Camera::move(float dx, float dy, float dz) {
  if(m_behavior == CAMERA_BEHAVIOR_ORBIT)
    return;

  glm::vec3 eye = m_eye;
  glm::vec3 forwards;

  if(m_behavior == CAMERA_BEHAVIOR_FIRST_PERSON) {
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
  if(m_behavior == CAMERA_BEHAVIOR_ORBIT)
    return;

  m_eye.x += direction.x * amount.x;
  m_eye.y += direction.y * amount.y;
  m_eye.z += direction.z * amount.z;

  updateViewMatrix();
}

void Camera::perspective(float fovx, float aspect, float znear, float zfar) {
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
  m_aspectRatio = aspect;
  m_znear = znear;
  m_zfar = zfar;
}

void Camera::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
  pitchDegrees = -pitchDegrees;
  headingDegrees = -headingDegrees;
  rollDegrees = -rollDegrees;

  switch(m_behavior) {
  case CAMERA_BEHAVIOR_FIRST_PERSON:
  case CAMERA_BEHAVIOR_SPECTATOR: rotateFirstPerson(headingDegrees, pitchDegrees); break;

  case CAMERA_BEHAVIOR_FLIGHT: rotateFlight(headingDegrees, pitchDegrees, rollDegrees); break;

  case CAMERA_BEHAVIOR_ORBIT: rotateOrbit(headingDegrees, pitchDegrees, rollDegrees); break;
  }

  updateViewMatrix();
}

void Camera::rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) {
  headingDegrees *= m_rotationSpeed;
  pitchDegrees *= m_rotationSpeed;
  rollDegrees *= m_rotationSpeed;

  rotate(headingDegrees, pitchDegrees, rollDegrees);
}

void Camera::undoRoll() {
  if(m_behavior == CAMERA_BEHAVIOR_ORBIT)
    lookAt(m_eye, m_target, m_targetYAxis);
  else
    lookAt(m_eye, m_eye + m_viewDir, WORLD_YAXIS);
}

void Camera::updatePosition(const glm::vec3 &direction, float elapsedTimeSec) {
  if(glm::dot(m_currentVelocity, m_currentVelocity) != 0.0F) {
    glm::vec3 displacement =
        (m_currentVelocity * elapsedTimeSec) + (0.5F * m_acceleration * elapsedTimeSec * elapsedTimeSec);

    if(direction.x == 0.0F && glm::abs(m_currentVelocity.x) <= 0.0F)
      displacement.x = 0.0F;

    if(direction.y == 0.0F && glm::abs(m_currentVelocity.y) <= 0.0F)
      displacement.y = 0.0F;

    if(direction.z == 0.0F && glm::abs(m_currentVelocity.z) <= 0.0F)
      displacement.z = 0.0F;

    move(displacement.x, displacement.y, displacement.z);
  }

  updateVelocity(direction, elapsedTimeSec);
}

void Camera::zoom(float zoom, float minZoom, float maxZoom) {
  if(m_behavior == CAMERA_BEHAVIOR_ORBIT) {
    glm::vec3 offset = m_eye - m_target;

    m_orbitMaxZoom = maxZoom;
    m_orbitMinZoom = minZoom;

    m_orbitOffsetDistance = glm::length(offset);
    offset = glm::normalize(offset);
    m_orbitOffsetDistance += zoom;
    m_orbitOffsetDistance = std::min(std::max(m_orbitOffsetDistance, minZoom), maxZoom);

    offset *= m_orbitOffsetDistance;
    m_eye = offset + m_target;

    updateViewMatrix();
  } else {
    zoom = std::min(std::max(zoom, minZoom), maxZoom);
    perspective(zoom, m_aspectRatio, m_znear, m_zfar);
  }
}

void Camera::setAcceleration(const glm::vec3 &acceleration) { m_acceleration = acceleration; }

void Camera::setBehavior(CameraBehavior newBehavior) {
  CameraBehavior prevBehavior = m_behavior;

  if(prevBehavior == newBehavior)
    return;

  m_behavior = newBehavior;

  switch(newBehavior) {
  case CAMERA_BEHAVIOR_FIRST_PERSON:
    switch(prevBehavior) {
    default: break;

    case CAMERA_BEHAVIOR_FLIGHT:
      m_eye.y = m_firstPersonYOffset;
      updateViewMatrix();
      break;

    case CAMERA_BEHAVIOR_SPECTATOR:
      m_eye.y = m_firstPersonYOffset;
      updateViewMatrix();
      break;

    case CAMERA_BEHAVIOR_ORBIT:
      m_eye.x = m_savedEye.x;
      m_eye.z = m_savedEye.z;
      m_eye.y = m_firstPersonYOffset;
      m_orientation = m_savedOrientation;
      m_accumPitchDegrees = m_savedAccumPitchDegrees;
      updateViewMatrix();
      break;
    }

    undoRoll();
    break;

  case CAMERA_BEHAVIOR_SPECTATOR:
    switch(prevBehavior) {
    default: break;

    case CAMERA_BEHAVIOR_FLIGHT: updateViewMatrix(); break;

    case CAMERA_BEHAVIOR_ORBIT:
      m_eye = m_savedEye;
      m_orientation = m_savedOrientation;
      m_accumPitchDegrees = m_savedAccumPitchDegrees;
      updateViewMatrix();
      break;
    }

    undoRoll();
    break;

  case CAMERA_BEHAVIOR_FLIGHT:
    if(prevBehavior == CAMERA_BEHAVIOR_ORBIT) {
      m_eye = m_savedEye;
      m_orientation = m_savedOrientation;
      m_accumPitchDegrees = m_savedAccumPitchDegrees;
      updateViewMatrix();
    } else {
      m_savedEye = m_eye;
      updateViewMatrix();
    }
    break;

  case CAMERA_BEHAVIOR_ORBIT:
    if(prevBehavior == CAMERA_BEHAVIOR_FIRST_PERSON)
      m_firstPersonYOffset = m_eye.y;

    m_savedEye = m_eye;
    m_savedOrientation = m_orientation;
    m_savedAccumPitchDegrees = m_accumPitchDegrees;

    m_targetYAxis = m_yAxis;

    glm::vec3 newEye = m_eye + m_zAxis * m_orbitOffsetDistance;
    glm::vec3 newTarget = m_eye;

    lookAt(newEye, newTarget, m_targetYAxis);
    break;
  }
}

void Camera::setCurrentVelocity(const glm::vec3 &currentVelocity) { m_currentVelocity = currentVelocity; }

void Camera::setCurrentVelocity(float x, float y, float z) { m_currentVelocity = {x, y, z}; }

void Camera::setOrbitMaxZoom(float orbitMaxZoom) { m_orbitMaxZoom = orbitMaxZoom; }

void Camera::setOrbitMinZoom(float orbitMinZoom) { m_orbitMinZoom = orbitMinZoom; }

void Camera::setOrbitOffsetDistance(float orbitOffsetDistance) { m_orbitOffsetDistance = orbitOffsetDistance; }

void Camera::setOrientation(const glm::quat &newOrientation) {
  glm::mat4 m = glm::mat4_cast(newOrientation);

  m_accumPitchDegrees = glm::degrees(asinf(m[1][2]));
  m_orientation = newOrientation;

  if(m_behavior == CAMERA_BEHAVIOR_FIRST_PERSON || m_behavior == CAMERA_BEHAVIOR_SPECTATOR)
    lookAt(m_eye, m_eye + m_viewDir, WORLD_YAXIS);

  updateViewMatrix();
}

void Camera::setPosition(const glm::vec3 &newEye) {
  m_eye = newEye;
  updateViewMatrix();
}

void Camera::setPreferTargetYAxisOrbiting(bool prefer) {
  m_preferTargetYAxisOrbiting = prefer;

  if(m_preferTargetYAxisOrbiting)
    undoRoll();
}

void Camera::setRotationSpeed(float rotationSpeed) { m_rotationSpeed = rotationSpeed; }

void Camera::setVelocity(const glm::vec3 &velocity) { m_velocity = velocity; }

void Camera::setVelocity(float x, float y, float z) { m_velocity = {x, y, z}; }

void Camera::rotateFirstPerson(float headingDegrees, float pitchDegrees) {
  m_accumPitchDegrees += pitchDegrees;

  if(m_accumPitchDegrees > 90.0F) {
    pitchDegrees = 90.0F - (m_accumPitchDegrees - pitchDegrees);
    m_accumPitchDegrees = 90.0F;
  }

  if(m_accumPitchDegrees < -90.0F) {
    pitchDegrees = -90.0F - (m_accumPitchDegrees - pitchDegrees);
    m_accumPitchDegrees = -90.0F;
  }

  glm::quat rot;

  if(headingDegrees != 0.0F) {
    rot = glm::angleAxis(glm::radians(headingDegrees), WORLD_YAXIS);
    m_orientation = m_orientation * rot;
  }

  if(pitchDegrees != 0.0F) {
    rot = glm::angleAxis(glm::radians(pitchDegrees), WORLD_XAXIS);
    m_orientation = rot * m_orientation;
  }
}

void Camera::rotateFlight(float headingDegrees, float pitchDegrees, float rollDegrees) {
  m_accumPitchDegrees += pitchDegrees;

  if(m_accumPitchDegrees > 360.0F)
    m_accumPitchDegrees -= 360.0F;

  if(m_accumPitchDegrees < -360.0F)
    m_accumPitchDegrees += 360.0F;

  glm::quat rot = fromHeadPitchRoll(headingDegrees, pitchDegrees, rollDegrees);
  m_orientation = rot * m_orientation;
}

void Camera::rotateOrbit(float headingDegrees, float pitchDegrees, float rollDegrees) {
  glm::quat rot;

  if(m_preferTargetYAxisOrbiting) {
    if(headingDegrees != 0.0F) {
      rot = glm::angleAxis(glm::radians(headingDegrees), m_targetYAxis);
      m_orientation = m_orientation * rot;
    }

    if(pitchDegrees != 0.0F) {
      rot = glm::angleAxis(glm::radians(pitchDegrees), WORLD_XAXIS);
      m_orientation = rot * m_orientation;
    }
  } else {
    rot = fromHeadPitchRoll(headingDegrees, pitchDegrees, rollDegrees);
    m_orientation = rot * m_orientation;
  }
}

void Camera::updateVelocity(const glm::vec3 &direction, float elapsedTimeSec) {
  if(direction.x != 0.0F) {
    m_currentVelocity.x += direction.x * m_acceleration.x * elapsedTimeSec;

    if(m_currentVelocity.x > m_velocity.x)
      m_currentVelocity.x = m_velocity.x;
    else if(m_currentVelocity.x < -m_velocity.x)
      m_currentVelocity.x = -m_velocity.x;
  } else {
    if(m_currentVelocity.x > 0.0F) {
      if((m_currentVelocity.x -= m_acceleration.x * elapsedTimeSec) < 0.0F)
        m_currentVelocity.x = 0.0F;
    } else {
      if((m_currentVelocity.x += m_acceleration.x * elapsedTimeSec) > 0.0F)
        m_currentVelocity.x = 0.0F;
    }
  }

  if(direction.y != 0.0F) {
    m_currentVelocity.y += direction.y * m_acceleration.y * elapsedTimeSec;

    if(m_currentVelocity.y > m_velocity.y)
      m_currentVelocity.y = m_velocity.y;
    else if(m_currentVelocity.y < -m_velocity.y)
      m_currentVelocity.y = -m_velocity.y;
  } else {
    if(m_currentVelocity.y > 0.0F) {
      if((m_currentVelocity.y -= m_acceleration.y * elapsedTimeSec) < 0.0F)
        m_currentVelocity.y = 0.0F;
    } else {
      if((m_currentVelocity.y += m_acceleration.y * elapsedTimeSec) > 0.0F)
        m_currentVelocity.y = 0.0F;
    }
  }

  if(direction.z != 0.0F) {
    m_currentVelocity.z += direction.z * m_acceleration.z * elapsedTimeSec;

    if(m_currentVelocity.z > m_velocity.z)
      m_currentVelocity.z = m_velocity.z;
    else if(m_currentVelocity.z < -m_velocity.z)
      m_currentVelocity.z = -m_velocity.z;
  } else {
    if(m_currentVelocity.z > 0.0F) {
      if((m_currentVelocity.z -= m_acceleration.z * elapsedTimeSec) < 0.0F)
        m_currentVelocity.z = 0.0F;
    } else {
      if((m_currentVelocity.z += m_acceleration.z * elapsedTimeSec) > 0.0F)
        m_currentVelocity.z = 0.0F;
    }
  }
}

void Camera::updateViewMatrix() {
  m_viewMatrix = glm::mat4_cast(m_orientation);

  m_xAxis = {m_viewMatrix[0][0], m_viewMatrix[1][0], m_viewMatrix[2][0]};
  m_yAxis = {m_viewMatrix[0][1], m_viewMatrix[1][1], m_viewMatrix[2][1]};
  m_zAxis = {m_viewMatrix[0][2], m_viewMatrix[1][2], m_viewMatrix[2][2]};
  m_viewDir = -m_zAxis;

  if(m_behavior == CAMERA_BEHAVIOR_ORBIT) {
    m_eye = m_target + m_zAxis * m_orbitOffsetDistance;
  }

  m_viewMatrix[3][0] = -glm::dot(m_xAxis, m_eye);
  m_viewMatrix[3][1] = -glm::dot(m_yAxis, m_eye);
  m_viewMatrix[3][2] = -glm::dot(m_zAxis, m_eye);
}
