#pragma once

// TODO: what's really needed?
#include <common.hpp>

void testPhysicsLibraryLinking();


struct object {
  float x[3]; // position
  float v[3]; // velocity
  float a[3]; // acceleration

  // calculates the new position, velocity, acceleration
  void step(float deltaT);
};

// x1..x3: Position, v1..v3: velocity
object createObject(float x1, float x2, float x3, float v1, float v2, float v3);
