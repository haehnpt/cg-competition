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

    phySphere(float x1, float x2, float x3,
              float v1, float v2, float v3,
              float radius, glm::vec4 color,
              struct phyPlane *plane);

    // calculates the new position, velocity, acceleration
    void step(float deltaT);
};


struct phyPlane {
    unsigned int vao;
    unsigned int vbo;
    float *vbo_data;
    unsigned int mVertices;

    int zNumPoints;
    int xNumPoints;

    float xStart;
    float xEnd;
    float zStart;
    float zEnd;

    float xTileWidth;
    float zTileWidth;

    int triangleIndex;

    phyPlane(float xStart, float xEnd, float zStart, float zEnd);
    ~phyPlane();

    int getTriangleIndex(phySphere *sphere);

    void bind();
    void release();
    void destroy();
};
