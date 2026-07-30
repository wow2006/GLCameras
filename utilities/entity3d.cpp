//-----------------------------------------------------------------------------
// Copyright (c) 2006 dhpoware. All Rights Reserved.
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

#include "entity3d.hpp"

namespace {
// mathlib composed quaternions left to right, glm composes right to left, so
// every mathlib 'a * b' becomes 'b * a' here.
constexpr float FULL_CIRCLE_DEGREES = 360.0F;

void wrapDegrees(glm::vec3 &angles) {
  for(glm::length_t i = 0; i < 3; ++i) {
    if(angles[i] > FULL_CIRCLE_DEGREES)
      angles[i] -= FULL_CIRCLE_DEGREES;

    if(angles[i] < -FULL_CIRCLE_DEGREES)
      angles[i] += FULL_CIRCLE_DEGREES;
  }
}
}  // namespace

Entity3D::Entity3D() {
  m_worldMatrix = glm::mat4(1.0F);
  m_orientation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
  m_rotation = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);

  m_right = {1.0F, 0.0F, 0.0F};
  m_up = {0.0F, 1.0F, 0.0F};
  m_forward = {0.0F, 0.0F, -1.0F};

  m_position = {0.0F, 0.0F, 0.0F};
  m_velocity = {0.0F, 0.0F, 0.0F};
  m_eulerOrient = {0.0F, 0.0F, 0.0F};
  m_eulerRotate = {0.0F, 0.0F, 0.0F};

  m_constrainedToWorldYAxis = false;
}

Entity3D::~Entity3D() = default;

void Entity3D::constrainToWorldYAxis(bool constrain) {
  // Constraining rotations to the world Y axis means that all heading
  // changes are applied to the world Y axis rather than the entity's
  // local Y axis.

  m_constrainedToWorldYAxis = constrain;
}

void Entity3D::orient(float headingDegrees, float pitchDegrees, float rollDegrees) {
  // orient() changes the direction the entity is facing. This directly
  // affects the orientation of the entity's right, up, and forward vectors.
  // orient() is usually called in response to the user's input if the entity
  // is able to be moved by the user.

  m_eulerOrient.x += pitchDegrees;
  m_eulerOrient.y += headingDegrees;
  m_eulerOrient.z += rollDegrees;

  wrapDegrees(m_eulerOrient);
}

void Entity3D::rotate(float headingDegrees, float pitchDegrees, float rollDegrees) {
  // rotate() does not change the direction the entity is facing. This method
  // allows the entity to freely spin around without affecting its orientation
  // and its right, up, and forward vectors. For example, if this entity is
  // a planet, then rotate() is used to spin the planet on its y axis. If this
  // entity is an asteroid, then rotate() is used to tumble the asteroid as
  // it moves in space.

  m_eulerRotate.x += pitchDegrees;
  m_eulerRotate.y += headingDegrees;
  m_eulerRotate.z += rollDegrees;

  wrapDegrees(m_eulerRotate);
}

void Entity3D::setPosition(float x, float y, float z) { m_position = {x, y, z}; }

void Entity3D::setVelocity(float x, float y, float z) { m_velocity = {x, y, z}; }

void Entity3D::setWorldMatrix(const glm::mat4 &worldMatrix) {
  m_worldMatrix = worldMatrix;
  m_orientation = glm::quat_cast(worldMatrix);
  m_position = {worldMatrix[3][0], worldMatrix[3][1], worldMatrix[3][2]};
  extractAxes();
}

void Entity3D::update(float elapsedTimeSec) {
  const glm::vec3 velocityElapsed = m_velocity * elapsedTimeSec;
  const glm::vec3 eulerOrientElapsed = m_eulerOrient * elapsedTimeSec;
  const glm::vec3 eulerRotateElapsed = m_eulerRotate * elapsedTimeSec;

  // Update the entity's position.

  extractAxes();

  const glm::vec3 oldPos = m_position;

  m_position += m_right * velocityElapsed.x;
  m_position += m_up * velocityElapsed.y;
  m_position += m_forward * velocityElapsed.z;

  // Guard the normalize: a stationary entity has a zero length heading, and
  // glm::normalize() would turn that into NaNs.
  const glm::vec3 displacement = m_position - oldPos;
  const glm::vec3 heading = (glm::dot(displacement, displacement) > 0.0F) ? glm::normalize(displacement) : glm::vec3(0.0F);

  // Update the entity's orientation.

  glm::quat temp = eulerToQuaternion(glm::mat4_cast(m_orientation), eulerOrientElapsed.y, eulerOrientElapsed.x, eulerOrientElapsed.z);

  // When moving backwards invert rotations to match direction of travel.
  if(glm::dot(heading, m_forward) < 0.0F)
    temp = glm::inverse(temp);

  m_orientation = glm::normalize(temp * m_orientation);

  // Update the entity's free rotation.

  temp = eulerToQuaternion(glm::mat4_cast(m_rotation), eulerRotateElapsed.y, eulerRotateElapsed.x, eulerRotateElapsed.z);

  m_rotation = glm::normalize(temp * m_rotation);

  // Update the entity's world matrix.

  const glm::quat worldOrientation = glm::normalize(m_orientation * m_rotation);

  m_worldMatrix = glm::mat4_cast(worldOrientation);
  m_worldMatrix[3][0] = m_position.x;
  m_worldMatrix[3][1] = m_position.y;
  m_worldMatrix[3][2] = m_position.z;

  // Clear the entity's cached euler rotations and velocity for this frame.

  m_velocity = {0.0F, 0.0F, 0.0F};
  m_eulerOrient = {0.0F, 0.0F, 0.0F};
  m_eulerRotate = {0.0F, 0.0F, 0.0F};
}

glm::quat Entity3D::eulerToQuaternion(const glm::mat4 &m, float headingDegrees, float pitchDegrees, float rollDegrees) const {
  // Construct a quaternion from an euler transformation. We do this rather
  // than use a heading-pitch-roll helper to support constraining heading
  // changes to the world Y axis.

  glm::quat result = glm::quat(1.0F, 0.0F, 0.0F, 0.0F);
  const glm::vec3 localXAxis = {m[0][0], m[0][1], m[0][2]};
  const glm::vec3 localYAxis = {m[1][0], m[1][1], m[1][2]};
  const glm::vec3 localZAxis = {m[2][0], m[2][1], m[2][2]};

  if(headingDegrees != 0.0F) {
    const glm::vec3 axis = m_constrainedToWorldYAxis ? glm::vec3(0.0F, 1.0F, 0.0F) : localYAxis;
    result = glm::angleAxis(glm::radians(headingDegrees), axis) * result;
  }

  if(pitchDegrees != 0.0F) {
    result = glm::angleAxis(glm::radians(pitchDegrees), localXAxis) * result;
  }

  if(rollDegrees != 0.0F) {
    result = glm::angleAxis(glm::radians(rollDegrees), localZAxis) * result;
  }

  return result;
}

void Entity3D::extractAxes() {
  const glm::mat4 m = glm::mat4_cast(m_orientation);

  m_right = glm::normalize(glm::vec3(m[0][0], m[0][1], m[0][2]));
  m_up = glm::normalize(glm::vec3(m[1][0], m[1][1], m[1][2]));
  m_forward = glm::normalize(glm::vec3(-m[2][0], -m[2][1], -m[2][2]));
}
