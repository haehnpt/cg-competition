#pragma once

#include <mesh.hpp>
#include <camera.hpp>

namespace phy {
  // 'direction' in the plane
  enum phyDirection
    {
     right,
     up,
     left,
     down,
    };

  struct phySphere {
    // holds the mesh, etc.
    geometry geo;

    // physics simulation
    glm::vec3 x; // position
    glm::vec3 v; // velocity
    glm::vec3 a; // acceleration
    // position of the center of the mesh relative to the center used
    // for simulation
    glm::vec3 offset_vec;

    float radius;

    // index of the triangle over which the sphere was at the last
    // tick
    int lastTriangleIndex = -1;
    // float xMax[3];
    // float aNext[3];

    struct phyPlane *plane;
    int model_mat_loc;

    phySphere(glm::vec3 x,
              glm::vec3 v,
              float radius, glm::vec4 color,
              struct phyPlane *plane),
    ~phySphere();

    void render();
    // calculates the new position, velocity, acceleration
    bool step(float deltaT);
    void setPosition(glm::vec3 pos);
    void moveToPlaneHeight();
  };


  struct phyPlane {
    unsigned int vao;
    unsigned int vbo;
    float *vbo_data;
    unsigned int mVertices;

    float xStart;
    float xEnd;
    float zStart;
    float zEnd;

    int xNumPoints;
    int zNumPoints;

    bool useBoundingBox;

    float xTileWidth;
    float zTileWidth;

    int triangleIndex;


    phyPlane(float xStart, float xEnd, float zStart, float zEnd,
             float *heightMap, int xNumPoints, int zNumPoints, bool useBoundingBox);
    ~phyPlane();

    int getTriangleAt(glm::vec3 x);
    std::vector<int> getTrianglesFromTo(float xStart, float zStart, float xEnd, float zEnd);

    bool isAbove(glm::vec3 x);

    int getNextTriangle(int index, phyDirection direction);
    int getNextTriangle(glm::vec3 position, glm::vec3 direction);

    glm::vec3 reflectAt(glm::vec3 pos, glm::vec3 v);

    void bind();
    void release();
    void destroy();
  };

  void initShader();
  void useShader(camera *cam, glm::mat4 proj_matrix, glm::vec3 light_dir);
}
