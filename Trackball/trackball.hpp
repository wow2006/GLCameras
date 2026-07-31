///////////////////////////////////////////////////////////////////////////////
// trackball.hpp
// =============
// Trackball class
// This class takes the current mouse cursor position (x,y), and map it to the
// point (x,y,z) on the trackball(sphere) surface. Since the cursor point is in
// screen space, this class depends on the current screen width and height in
// order to compute the vector on the sphere.
//
// There are 2 modes (Arc and Project) to compute the vector on the sphere: Arc
// mode is that the length of the mouse position from the centre of screen
// becomes arc length moving on the sphere, and Project mode is that directly
// projects the mouse position to the sphere.
//
// The default mode is Arc because it allows negative z-value (a point on the
// back of the sphere). On the other hand, Project mode is limited to
// front hemisphere rotation (z-value is always positive).
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2011-12-09
// UPDATED: 2016-03-31
//
// Copyright (C) 2011. Song Ho Ahn
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Trackball final {
public:
  enum Mode { ARC = 0, PROJECT };

  Trackball();
  Trackball(float radius, int screenWidth, int screenHeight);

  void set(float r, int w, int h);
  void setScreenSize(int w, int h);
  void setRadius(float r) { m_radius = r; }
  void setMode(Trackball::Mode mode) { m_mode = mode; }

  [[nodiscard]] int getScreenWidth() const { return m_screenWidth; }
  [[nodiscard]] int getScreenHeight() const { return m_screenHeight; }
  [[nodiscard]] float getRadius() const { return m_radius; }
  [[nodiscard]] Trackball::Mode getMode() const { return m_mode; }

  // return a point on the sphere for the given mouse (x,y)
  [[nodiscard]] glm::vec3 getVector(int x, int y) const;
  // return the same point normalized
  [[nodiscard]] glm::vec3 getUnitVector(int x, int y) const;

  // rotation taking v1 onto v2, the trackball's delta rotation
  [[nodiscard]] static glm::quat getQuaternion(const glm::vec3 &v1, const glm::vec3 &v2);

private:
  [[nodiscard]] glm::vec3 getVectorWithArc(float x, float y) const;
  [[nodiscard]] glm::vec3 getVectorWithProject(float x, float y) const;

  float m_radius;
  int m_screenWidth;
  int m_screenHeight;
  float m_halfScreenWidth;
  float m_halfScreenHeight;
  Trackball::Mode m_mode;
};
