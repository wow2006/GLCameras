///////////////////////////////////////////////////////////////////////////////
// trackball.cpp
// =============
// Trackball class
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2011-12-09
// UPDATED: 2016-04-01
//
// Copyright (C) 2011. Song Ho Ahn
///////////////////////////////////////////////////////////////////////////////

#include <glm/gtc/epsilon.hpp>
//
#include "trackball.hpp"

namespace {
constexpr float EPSILON = 0.001F;
constexpr float PI = 3.141592F;
}  // namespace

Trackball::Trackball() :
    m_radius(0.0F), m_screenWidth(0), m_screenHeight(0), m_halfScreenWidth(0.0F), m_halfScreenHeight(0.0F), m_mode(Trackball::ARC) {}

Trackball::Trackball(float radius, int width, int height) :
    m_radius(radius), m_screenWidth(width), m_screenHeight(height), m_halfScreenWidth(static_cast<float>(width) * 0.5F),
    m_halfScreenHeight(static_cast<float>(height) * 0.5F), m_mode(Trackball::ARC) {}

void Trackball::set(float r, int w, int h) {
  m_radius = r;
  setScreenSize(w, h);
}

void Trackball::setScreenSize(int w, int h) {
  m_screenWidth = w;
  m_screenHeight = h;
  m_halfScreenWidth = static_cast<float>(w) * 0.5F;
  m_halfScreenHeight = static_cast<float>(h) * 0.5F;
}

///////////////////////////////////////////////////////////////////////////////
// return the point coords on the sphere
///////////////////////////////////////////////////////////////////////////////
glm::vec3 Trackball::getVector(int x, int y) const {
  if(m_radius == 0.0F || m_screenWidth == 0 || m_screenHeight == 0)
    return {0.0F, 0.0F, 0.0F};

  // compute mouse position from the centre of screen (-half ~ +half)
  const float mx = static_cast<float>(x) - m_halfScreenWidth;
  const float my = m_halfScreenHeight - static_cast<float>(y);  // OpenGL uses bottom to up orientation

  if(m_mode == Trackball::PROJECT)
    return getVectorWithProject(mx, my);

  return getVectorWithArc(mx, my);  // default mode
}

///////////////////////////////////////////////////////////////////////////////
// return the point on the sphere as a unit vector
///////////////////////////////////////////////////////////////////////////////
glm::vec3 Trackball::getUnitVector(int x, int y) const {
  const glm::vec3 vec = getVector(x, y);

  // NOTE: mathlib's normalize() left a zero vector alone, glm returns NaNs.
  if(glm::dot(vec, vec) == 0.0F)
    return vec;

  return glm::normalize(vec);
}

///////////////////////////////////////////////////////////////////////////////
// use the mouse distance from the centre of screen as arc length on the sphere
// x = R * sin(a) * cos(b)
// y = R * sin(a) * sin(b)
// z = R * cos(a)
// where a = angle on x-z plane, b = angle on x-y plane
//
// NOTE: the calculation of arc length is an estimation using linear distance
// from screen center (0,0) to the cursor position
///////////////////////////////////////////////////////////////////////////////
glm::vec3 Trackball::getVectorWithArc(float x, float y) const {
  const float arc = std::sqrt(x * x + y * y);  // length between cursor and screen center
  const float a = arc / m_radius;              // arc = r * a
  const float b = std::atan2(y, x);            // angle on x-y plane
  const float x2 = m_radius * std::sin(a);     // x rotated by "a" on x-z plane

  return {x2 * std::cos(b), x2 * std::sin(b), m_radius * std::cos(a)};
}

///////////////////////////////////////////////////////////////////////////////
// project the mouse coords to the sphere to find the point coord
// return the point on the sphere using hyperbola where x^2 + y^2 > r^2/2
///////////////////////////////////////////////////////////////////////////////
glm::vec3 Trackball::getVectorWithProject(float x, float y) const {
  glm::vec3 vec = {x, y, 0.0F};
  const float d = x * x + y * y;
  const float rr = m_radius * m_radius;

  // use sphere if d<=0.5*r^2:  z = sqrt(r^2 - (x^2 + y^2))
  if(d <= (0.5F * rr)) {
    vec.z = std::sqrt(rr - d);
    return vec;
  }

  // use hyperbolic sheet if d>0.5*r^2:  z = (r^2 / 2) / sqrt(x^2 + y^2)
  // referenced from trackball.c by Gavin Bell at SGI
  vec.z = 0.5F * rr / std::sqrt(d);

  // scale x and y down, so the vector can be on the sphere
  // y = ax => x^2 + (ax)^2 + z^2 = r^2 => (1 + a^2)*x^2 = r^2 - z^2
  // => x = sqrt((r^2 - z^2) / (1 + a^2))
  float x2 = 0.0F;
  float y2 = 0.0F;
  if(x == 0.0F) {  // avoid dividing by 0
    y2 = std::sqrt(rr - vec.z * vec.z);
    if(y < 0.0F)  // correct sign
      y2 = -y2;
  } else {
    const float a = y / x;
    x2 = std::sqrt((rr - vec.z * vec.z) / (1.0F + a * a));
    if(x < 0.0F)  // correct sign
      x2 = -x2;
    y2 = a * x2;
  }

  vec.x = x2;
  vec.y = y2;

  return vec;
}

///////////////////////////////////////////////////////////////////////////////
// return the rotation quaternion taking v1 onto v2
//
// NOTE: the original built the quaternion out of a half angle by hand because
// its Quaternion(axis, angle) ctor did not halve the angle itself. glm's
// angleAxis() does, so the full angle is handed over here.
///////////////////////////////////////////////////////////////////////////////
glm::quat Trackball::getQuaternion(const glm::vec3 &v1, const glm::vec3 &v2) {
  // if two vectors are equal return the quaternion with 0 rotation
  if(glm::all(glm::epsilonEqual(v1, v2, EPSILON)))
    return glm::quat{1.0F, 0.0F, 0.0F, 0.0F};

  // if two vectors are opposite return a perpendicular vector with 180 angle
  if(glm::all(glm::epsilonEqual(v1, -v2, EPSILON))) {
    glm::vec3 axis = {0.0F, 0.0F, 1.0F};   // if z ~= 0
    if(v1.x > -EPSILON && v1.x < EPSILON)  // if x ~= 0
      axis = {1.0F, 0.0F, 0.0F};
    else if(v1.y > -EPSILON && v1.y < EPSILON)  // if y ~= 0
      axis = {0.0F, 1.0F, 0.0F};
    return glm::angleAxis(PI, axis);
  }

  const glm::vec3 u1 = glm::normalize(v1);
  const glm::vec3 u2 = glm::normalize(v2);

  const glm::vec3 axis = glm::cross(u1, u2);  // rotation axis
  // dot() can drift a hair outside [-1,1] once the vectors are normalized,
  // which would hand acos() a NaN.
  const float angle = std::acos(glm::clamp(glm::dot(u1, u2), -1.0F, 1.0F));

  return glm::angleAxis(angle, glm::normalize(axis));
}
