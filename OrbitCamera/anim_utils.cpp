///////////////////////////////////////////////////////////////////////////////
// anim_utils.cpp
// ==============
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2009-04-12
// UPDATED: 2016-06-01
//
// Copyright 2009 Song Ho Ahn. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "anim_utils.hpp"

namespace anim {

float accelerate(bool isMoving, float speed, float maxSpeed, float accel, float deltaTime) {
  // determine direction
  const float sign = (maxSpeed > 0.0F) ? 1.0F : -1.0F;

  if(isMoving) {
    // accelerating
    speed += sign * accel * deltaTime;
    if((sign * speed) > (sign * maxSpeed))
      speed = maxSpeed;
  } else {
    // deaccelerating
    speed -= sign * accel * deltaTime;
    if((sign * speed) < 0.0F)
      speed = 0.0F;
  }

  return speed;
}

}  // namespace anim
