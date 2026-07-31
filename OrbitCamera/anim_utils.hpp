///////////////////////////////////////////////////////////////////////////////
// anim_utils.hpp
// ==============
// Interpolation helpers used by OrbitCamera's timed animations.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2009-04-12
// UPDATED: 2016-06-01
//
// Copyright 2009 Song Ho Ahn. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace anim {

enum AnimationMode { LINEAR = 0, EASE_IN, EASE_OUT, EASE_IN_OUT, BOUNCE, ELASTIC };

///////////////////////////////////////////////////////////////////////////////
// re-shape a 0~1 interpolation value according to the animation mode
///////////////////////////////////////////////////////////////////////////////
inline float shapeAlpha(float alpha, AnimationMode mode) {
  if(mode == EASE_IN) {
    // with cubic function
    return alpha * alpha * alpha;
  }

  if(mode == EASE_OUT) {
    // with cubic function
    const float beta = 1.0F - alpha;
    return 1.0F - beta * beta * beta;
  }

  if(mode == EASE_IN_OUT) {
    const float beta = 1.0F - alpha;
    constexpr float scale = 4.0F;  // 0.5 / (0.5^3)
    if(alpha < 0.5F)
      return alpha * alpha * alpha * scale;
    return 1.0F - (beta * beta * beta * scale);
  }

  // BOUNCE and ELASTIC were never implemented in the original either
  return alpha;
}

///////////////////////////////////////////////////////////////////////////////
// interpolate from one value to the other, "alpha" is 0 ~ 1
///////////////////////////////////////////////////////////////////////////////
template<class T>
T interpolate(const T &from, const T &to, float alpha, AnimationMode mode) {
  const float t = shapeAlpha(alpha, mode);
  return from + t * (to - from);
}

///////////////////////////////////////////////////////////////////////////////
// spherical linear interpolation between 2 quaternions, "alpha" is 0 ~ 1
///////////////////////////////////////////////////////////////////////////////
inline glm::quat slerp(const glm::quat &from, const glm::quat &to, float alpha, AnimationMode mode = LINEAR) {
  return glm::slerp(from, to, shapeAlpha(alpha, mode));
}

///////////////////////////////////////////////////////////////////////////////
// accelerate / deaccelerate speed
// === PARAMS ===
//  isMoving: accelerate if true, deaccelerate if false
// currSpeed: the current speed
//  maxSpeed: maximum speed (positive or negative)
//     accel: acceleration (always positive)
// deltaTime: frame time in second
///////////////////////////////////////////////////////////////////////////////
float accelerate(bool isMoving, float currSpeed, float maxSpeed, float accel, float deltaTime);

}  // namespace anim
