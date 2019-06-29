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

    // index of the triangle over which the sphere was at the last
    // tick
    int lastTriangleIndex;
    float xMax[3];
    // float aNext[3];

    struct phyPlane *plane;

    // calculates the new position, velocity, acceleration
    void step(float deltaT);
};

// x1..x3: Position, v1..v3: velocity
phySphere createPhySphere(float x1, float x2, float x3,
                          float v1, float v2, float v3,
                          float radius, glm::vec4 color,
                          struct phyPlane *plane);

struct phyPlane {
    unsigned int vao;
    unsigned int vbo;
    float *vbo_data;
    unsigned int mVertices;

    float xStart;
    float xEnd;
    float zStart;
    float zEnd;

    bool isCutting(phySphere *sphere);


    void bind();
    void release();
    void destroy();
};

phyPlane createPhyPlane(float xStart, float xEnd, float zStart, float zEnd);
