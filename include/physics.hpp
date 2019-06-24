#pragma once

#include <mesh.hpp>

void testPhysicsLibraryLinking();


struct phySphere {
    // holds the mesh, etc.
    geometry geo;
    float radius;

    // physics simulation
    float x[3]; // position
    float v[3]; // velocity
    float a[3]; // acceleration

    // calculates the new position, velocity, acceleration
    void step(float deltaT);
};

// x1..x3: Position, v1..v3: velocity
phySphere createPhySphere(float x1, float x2, float x3,
                          float v1, float v2, float v3,
                          glm::vec4 color);

phySphere createPhySphere(float x1, float x2, float x3,
                          float v1, float v2, float v3,
                          float radius, glm::vec4 color);
