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
    // physics simulation
    glm::vec4 x; // position
    glm::vec4 v; // velocity
    glm::vec4 a; // acceleration
    // position of the center of the mesh relative to the center used
    // for simulation
    glm::vec4 offset_vec;
    float radius;
    struct phyPlane *plane;
    glm::vec4 custom_color;
    bool touched_plane_last_step = false;



    // index of the triangle over which the sphere was at the last
    // tick
    int lastTriangleIndex = -1;
    // float xMax[3];
    // float aNext[3];

    phySphere(glm::vec4 x,
              glm::vec4 v,
              float radius,
              struct phyPlane *plane,
              glm::vec4 custom_color = glm::vec4(0.f, 0.f, 0.f, 0.f));
    ~phySphere();

    void render();
    // calculates the new position, velocity, acceleration
    bool step(float deltaT);
    void setPosition(glm::vec4 pos);
    void moveToPlaneHeight();
  };


  struct phyPlane {
    unsigned int vao;
    unsigned int vbo;
    float *vbo_data;
    unsigned int n_vertices;

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

    glm::vec4 custom_color;
    glm::mat4 model_mat;
    glm::mat4 inv_model_mat;

    glm::vec3 *angular_velocity;
	glm::vec3 *vertical_velocity;

    phyPlane(float xStart, float xEnd, float zStart, float zEnd,
             float *heightMap, int xNumPoints, int zNumPoints, bool useBoundingBox,
             glm::vec3 *angular_velocity, glm::vec3 *vertical_velocity, glm::vec4 custom_color = glm::vec4(0.f, 0.f, 0.f, 0.f));
    ~phyPlane();

    // Update state, currently only roatation according to
    // angular_velocity.
    void step(float deltaT);

    void set_angular_velocity(glm::vec3 *angular_velocity);
	void set_vertical_velocity(glm::vec3 *vertical_velocity);

    void set_model_mat(glm::mat4 model_mat);
    glm::mat4 get_model_mat();

    int getTriangleAt(glm::vec4 pos);
    int getTriangleAt(glm::vec3 x);
    std::vector<int> getTrianglesFromTo(float xStart, float zStart, float xEnd, float zEnd);

    bool isAbove(glm::vec3 x);

    int getNextTriangle(int index, phyDirection direction);
    int getNextTriangle(glm::vec3 position, glm::vec3 direction);

    void reflect(phySphere *sphere);

    void render();
    void destroy();
  };

  void initShader();
  void useShader(camera *cam, glm::mat4 proj_matrix, glm::vec3 light_dir);
}
