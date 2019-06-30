#pragma once

#include <mesh.hpp>

struct phySphere {
    // holds the mesh, etc.
    geometry geo;

    // physics simulation
    glm::vec3 x; // position
    glm::vec3 v; // velocity
    glm::vec3 a; // acceleration

    float radius;

    // index of the triangle over which the sphere was at the last
    // tick
    int lastTriangleIndex;
    // float xMax[3];
    // float aNext[3];

    struct phyPlane *plane;

    phySphere(glm::vec3 x,
              glm::vec3 v,
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

    int getTriangleAt(glm::vec3 x);
    std::vector<int> getTrianglesFromTo(float xStart, float zStart, float xEnd, float zEnd);

    void bind();
    void release();
    void destroy();
};
