#include <physics.hpp>

#include <common.hpp>
#include <mesh.hpp>
#include <camera.hpp>
#include <shader.hpp>
#include <glm/gtx/transform.hpp>

// #include <buffer.hpp>
// #define PHYSICS_DEBUG

// WARNING ABOUT NON-DETERMINISM
//
// Notice that a floating point operation in a program can yield
// different results each time the program runs. This may become
// obvious when running the test_physics program, where the simulation
// can take different paths due to different results of the floating
// point operations which accumulate over time. As a result, when
// running two instances of the same program side by side, they start
// out identical might differ more and more over time. This is *not*
// caused by using operations involving the system time to measure the
// passed time; the test_physics program simply advances the time
// value used for calculations by a fixed value each frame.
//
// See https://isocpp.org/wiki/faq/newbie#floating-point-arith2 for
// more information.

namespace phy {
  namespace {
    int phyShaderProgram;
    int light_dir_loc;
    int model_mat_loc;
    int proj_mat_loc;
    int view_mat_loc;
  }

  phySphere::phySphere(glm::vec3 x,
                       glm::vec3 v,
                       float radius,
                       glm::vec4 color,
                       phyPlane *plane) :
    x{x},
    v{v},
    radius{radius},
    plane{plane}
  {
    // std::cout << "phySphere with pos = (" << x.x << ", " << x.y << ", " << x.z << ")\n";
    a.x = 0;
    a.y = -4;
    a.z = 0;

    geo = loadMesh("sphere.obj", false, color);

    // Since the collision code does not yet take the radius into
    // account, we choose the lowest point of the sphere (in
    // y-direction) as the reference point for the simulation and
    // simply place the center of the mesh @x.y + @radius.
    offset_vec = glm::vec3(0.f, radius, 0.f);

    //
    geo.transform = glm::translate(x + offset_vec)
      * glm::scale(glm::vec3(radius));
  }

  phySphere::~phySphere() {}

  bool
  phySphere::step(float deltaT) {
    // next position if there were no obstacles
    glm::vec3 targetPos = x + v * deltaT + 0.5f * a * deltaT * deltaT;

    // the bounding box is only applied to the x and z direction
    if(plane->useBoundingBox) {
      // maybe reflect in x direction
      if (targetPos.x  < plane->xStart + radius) {
        x.x = plane->xStart + radius + 0.0001f;
        v.x = -v.x;
      } else if (targetPos.x > plane->xEnd - radius) {
        x.x = plane->xEnd - radius -0.0001f;
        v.x = -v.x;
      }

      // maybe reflect in z direction
      if (targetPos.z < plane->zStart + radius) {
        x.z = plane->zStart + radius + 0.0001f;
        v.z = -v.z;
      } else if (targetPos.z > plane->zEnd - radius) {
        x.z = plane->zEnd - radius - 0.0001f;
        v.z = -v.z;
      }

      // the sphere is now back in the  plane area
      if (plane->isAbove(x)) {
        x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
        // update velocity
        v = v + a * deltaT;
      } else {
        // TODO: x is not the position of the first contact!
        v = plane->reflectAt(x, v);
        x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
        // update velocity
        v = v + a * deltaT;
        // lost energy when bouncing
        v *= 0.80;
      }
    } else {
      // ignoring the bounding box
      if(targetPos.x < plane->xStart || targetPos.x > plane->xEnd
         || targetPos.z < plane->zStart || targetPos.z > plane->zEnd) {
        // outside of the plane
        x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
        // update velocity
        v = v + a * deltaT;
      } else {
        // inside bounding box, reflect
        if (plane->isAbove(targetPos)) {
          x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
          // update velocity
          v = v + a * deltaT;
        } else {
          // TODO: x is not the position of the first contact!
          v = plane->reflectAt(x, v);
          x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
          // update velocity
          v = v + a * deltaT;
          // lost energy when bouncing
          v *= 0.90;
        }
      }

      // drag
      // a = glm::vec3(0, -10.f, 0.f);
      // a = a - 0.01f * (float)pow(glm::length(v), 2.f) * glm::normalize(v);

      // plane->getTriangleIndex(this);
    }

    geo.transform = glm::translate(x + offset_vec)
      * glm::scale(glm::vec3(radius));

    return true;
  }

  void
  phySphere::setPosition(glm::vec3 pos) {
    x = pos;

    geo.transform = glm::translate(x + offset_vec)
      * glm::scale(glm::vec3(radius));
  }

  void
  phySphere::moveToPlaneHeight() {
    int index = plane->getTriangleAt(x);

    // first vertex of the triangle
    glm::vec3 v1(plane->vbo_data[index * 3 * 10 + 0],
                 plane->vbo_data[index * 3 * 10 + 1],
                 plane->vbo_data[index * 3 * 10 + 2]);

    // normal of the triangle's plane
    glm::vec3 norm(plane->vbo_data[index * 3 * 10 + 3 + 0],
                   plane->vbo_data[index * 3 * 10 + 3 + 1],
                   plane->vbo_data[index * 3 * 10 + 3 + 2]);

    float d = glm::dot(v1, norm);

    // x.y = 1.f;
    x.y = (d - x.x * norm.x - x.z * norm.z) / norm.y;

    geo.transform = glm::translate(x + offset_vec)
      * glm::scale(glm::vec3(radius));
  }

  phyPlane::phyPlane(float xStart,
                     float xEnd,
                     float zStart,
                     float zEnd,
                     float *heightMap,
                     int xNumPoints,
                     int zNumPoints,
                     bool useBoundingBox) :
    xStart{xStart},
    xEnd{xEnd},
    zStart{zStart},
    zEnd{zEnd},
    xNumPoints{xNumPoints},
    zNumPoints{zNumPoints},
    useBoundingBox{useBoundingBox}
  {
    this->xTileWidth = (xEnd - xStart) / (float)(xNumPoints - 1);
    this->zTileWidth = (zEnd - zStart) / (float)(zNumPoints - 1);

    std::cout << "xTileWidth: " << xTileWidth << ", zTileWidth: " << zTileWidth << "\n";
    std::cout << "xNumPoints: " << xNumPoints << ", zNumPoints: " << zNumPoints << "\n";
    // Set vertex coordinates using the heightMap for the y-value.
    //
    // Each square of the (m-1)*(n-1) squares is separated into two
    // triangles: An upper-left and an bottom-right one. From this
    // subdivision follows:
    //
    // - A single vertex not on the edge of the map will be present in 6
    //   triangles.
    //
    // - The top-left and the bottom-right vertices of the map are each
    //   part of only one triangle.
    //
    // - The vertices of the top-right and the bottom-left edge of the
    //   map are each part of 2 triangles.
    //
    // - All other vertices on the edge of the map have are each present
    //   in 3 triangles.
    //
    // Example: Numbers = number of triangles the vertex is part of
    //
    // 1---3---3---3---2
    // | / | / | / | / |
    // 3---6---6---6---3
    // | / | / | / | / |
    // 3---6---6---6---3
    // | / | / | / | / |
    // 2---3---3---3---1
    //
    // Thus the total number of vertices needed is:
    // (m-2)*(n-2)*6 + 2*(n-2)*3 + 2*(m-2)*3 + 1 + 1 + 2 + 2
    //  = 6*(n*m - n - m + 1)
    mVertices = 6 * (xNumPoints * zNumPoints - xNumPoints - zNumPoints + 1);
    vbo_data = new float[mVertices * 10];
    float deltaX = (xEnd - xStart) / (xNumPoints - 1);
    float deltaZ = (zEnd - zStart) / (zNumPoints - 1);

    // DEBUG:
    std::cout << "heightMap dimension: " << zNumPoints << "x" << xNumPoints << "\n";
    std::cout << "deltaX = " << deltaX << ", deltaZ  = " << deltaZ << "\n";
    std::cout << "mVertices = " << mVertices << "\n";
    std::cout << "vbo_data size: " << mVertices * 10 * sizeof(float) / 1000.f << "K\n";

    // This for-loop loops over the squares between the data
    // points. (x,z) represents the upper-left vertex of the current
    // square. For each square the two contained triangles (called
    // top-left and bottom-right triangle) are added to the VBO data.
    int indexRectTopLeft = 0;
    for (int x = 0; x < xNumPoints - 1; x++) {
      for (int z = 0; z < zNumPoints - 1; z++) {
        glm::vec3 a, b, nrm;
        // coordinates in the plane:
        //  +----→ x
        //  |
        //  |
        //  ↓
        //  z

        //// TOP-LEFT TRIANGLE ////
        // top-left vertex of the square
        vbo_data[indexRectTopLeft + 0] = xStart + x * deltaX;
        vbo_data[indexRectTopLeft + 1] = heightMap[x * zNumPoints + z];
        vbo_data[indexRectTopLeft + 2] = zStart + z * deltaZ;
        // bottom-left vertex of the square
        vbo_data[indexRectTopLeft + 10 + 0] = xStart + x * deltaX;
        vbo_data[indexRectTopLeft + 10 + 1] = heightMap[x * zNumPoints + (z + 1)];
        vbo_data[indexRectTopLeft + 10 + 2] = zStart + (z + 1) * deltaZ;
        // top-right vertex of the square
        vbo_data[indexRectTopLeft + 20 + 0] = xStart + (x + 1) * deltaX;
        vbo_data[indexRectTopLeft + 20 + 1] = heightMap[(x + 1) * zNumPoints + z];
        vbo_data[indexRectTopLeft + 20 + 2] = zStart + z * deltaZ;
        // add the same normal to all three vertices:
        // top-left --> bottom-left
        a = glm::vec3(vbo_data[indexRectTopLeft + 10 + 0] - vbo_data[indexRectTopLeft + 0],
                      vbo_data[indexRectTopLeft + 10 + 1] - vbo_data[indexRectTopLeft + 1],
                      vbo_data[indexRectTopLeft + 10 + 2] - vbo_data[indexRectTopLeft + 2]);
        // top-left --> top-right
        b = glm::vec3(vbo_data[indexRectTopLeft + 20 + 0] - vbo_data[indexRectTopLeft + 0],
                      vbo_data[indexRectTopLeft + 20 + 1] - vbo_data[indexRectTopLeft + 1],
                      vbo_data[indexRectTopLeft + 20 + 2] - vbo_data[indexRectTopLeft + 2]);
        nrm = glm::normalize(glm::cross(a, b));
        for (int vert = 0; vert < 3; vert++) {
          for (int coord = 0; coord < 3; coord++) {
            // normal's x,y,z values start at index 3
            vbo_data[indexRectTopLeft + (vert * 10) + 3 + coord] = nrm[coord];
          }
          // default color
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 0] = 0.8f; // r
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 1] = 0.8f; // g
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 2] = 0.8f; // b
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 3] = 1.0f; // a
        }

        //// BOTTOM-RIGHT TRIANGLE ////
        // bottom-left vertex of the square
        vbo_data[indexRectTopLeft + 30 + 0] = xStart + x * deltaX;
        vbo_data[indexRectTopLeft + 30 + 1] = heightMap[x * zNumPoints + (z + 1)];
        vbo_data[indexRectTopLeft + 30 + 2] = zStart + (z + 1) * deltaZ;
        // top-right vertex of the square
        vbo_data[indexRectTopLeft + 40 + 0] = xStart + (x + 1) * deltaX;
        vbo_data[indexRectTopLeft + 40 + 1] = heightMap[(x + 1) * zNumPoints + z];
        vbo_data[indexRectTopLeft + 40 + 2] = zStart + z * deltaZ;
        // bottom-right vertex of the square
        vbo_data[indexRectTopLeft + 50 + 0] = xStart + (x + 1) * deltaX;
        vbo_data[indexRectTopLeft + 50 + 1] = heightMap[(x + 1) * zNumPoints + (z + 1)];
        vbo_data[indexRectTopLeft + 50 + 2] = zStart + (z + 1) * deltaZ;
        // add the same normal to all three vertices:
        // bottom-right --> top-right
        a = glm::vec3(vbo_data[indexRectTopLeft + 40 + 0] - vbo_data[indexRectTopLeft + 50 + 0],
                      vbo_data[indexRectTopLeft + 40 + 1] - vbo_data[indexRectTopLeft + 50 + 1],
                      vbo_data[indexRectTopLeft + 40 + 2] - vbo_data[indexRectTopLeft + 50 + 2]);
        // bottom-right --> bottom-left
        b = glm::vec3(vbo_data[indexRectTopLeft + 30 + 0] - vbo_data[indexRectTopLeft + 50 + 0],
                      vbo_data[indexRectTopLeft + 30 + 1] - vbo_data[indexRectTopLeft + 50 + 1],
                      vbo_data[indexRectTopLeft + 30 + 2] - vbo_data[indexRectTopLeft + 50 + 2]);
        nrm = glm::normalize(glm::cross(a, b));

        for (int vert = 3; vert < 6; vert++) {
          for (int coord = 0; coord < 3; coord++) {
            // normal's x,y,z values start at index 3
            vbo_data[indexRectTopLeft + (vert * 10) + 3 + coord] = nrm[coord];
          }
          // default color
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 0] = 0.8f; // r
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 1] = 0.8f; // g
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 2] = 0.8f; // b
          vbo_data[indexRectTopLeft + (vert * 10) + 6 + 3] = 1.0f; // a
        }

        // Per square (= 2 triangles) 6 vertices are added, update index
        indexRectTopLeft += 6 * 10;
      }
    }


    // DEBUG print
    // for (int i = 0; i < mVertices; i++) {
    //     std::cout << "====== Vertix " << i << ":\n";

    //     for (int j = 0; j < 10; j++) {
    //         std::cout << vbo_data[i * 10 + j] << " ";
    //         std::cout << ((j == 2 || j == 5) ? "\n" : " ");
    //     }

    //     std::cout << "\n\n";
    // }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, mVertices * 10 * sizeof(float), vbo_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3*sizeof(float)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6*sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // We don't delete vbo_data, as we will use it for collision
    // detection!
    //
    // delete [vbo_data];
  }


  phyPlane::~phyPlane() {
    delete[] vbo_data;
  }

  void
  phyPlane::bind() {
    glBindVertexArray(vao);
  }

  void
  phyPlane::release() {
    glBindVertexArray(0);
  }

  void
  phyPlane::destroy() {
    release();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }

  // For the start we assume the sphere has a radius of zero!
  int
  phyPlane::getTriangleAt(glm::vec3 pos) {
    // printf("isCutting with s coordinates: %02.2f %02.2f %02.2f\n",
    //        s->x[0], s->x[1], s->x[2]);

    // position of the sphere's center relative to the plane
    float xInPlane = pos.x - xStart;
    float zInPlane = pos.z - zStart;

    if(pos.x < xStart || pos.x > xEnd
       || pos.z < zStart || pos.z > zEnd) {
      return -1;
    }

    // indices of the rectangle containing the sphere's center
    int xIndexRect = floor(xInPlane / xTileWidth);
    int zIndexRect = floor(zInPlane / zTileWidth);

    // x and z positions relative to the top (z-axis pointing
    // downwards!) left corner of the rectangle
    float xInRect = fmod(xInPlane, xTileWidth);
    float zInRect = fmod(zInPlane, zTileWidth);

#ifdef PHYSICS_DEBUG
    // top-right (-1) or bottom left (-0) triangle?
    int oldTriangleIndex = triangleIndex;
#endif

    // two triangles per rectangle
    triangleIndex = 2 * (xIndexRect * (zNumPoints - 1) + zIndexRect);
    // if the sphere is in the 'upper'-left triangle it's one less
    if (zInRect > zTileWidth - xInRect * zTileWidth / xTileWidth) {
      triangleIndex++;
    }

#ifdef PHYSICS_DEBUG
    if (triangleIndex != oldTriangleIndex) {
      std::cout << "zIndexRect: " << zIndexRect
                << ", xIndexRect: " << xIndexRect
                << ", triangleIndex: " << triangleIndex << "\n" << std::flush;

    }
#endif // PHYSICS_DEBUG

    return triangleIndex;
  }

  // Return (the index of) the next triangle in the phyDirection d
  int
  phyPlane::getNextTriangle(int index, phyDirection d) {
    switch(d) {
    case right: return index + 1; break;
    case left: return index - 1; break;
    case down: return index + 2 * (xNumPoints - 1); break;
    case up: return index - 2 * (xNumPoints - 1); break;
    default: return -1;
    }
  }

  int
  phyPlane::getNextTriangle(glm::vec3 x, glm::vec3 direction) {
    int i = getTriangleAt(x);
    // vectors from x to each vertices
    glm::vec3 v0(vbo_data[i * 30 + 0] - x.x,
                 vbo_data[i * 30 + 1] - x.y,
                 vbo_data[i * 30 + 2] - x.z);
    glm::vec3 v1(vbo_data[i * 30 + 10 + 0] - x.x,
                 vbo_data[i * 30 + 10 + 1] - x.y,
                 vbo_data[i * 30 + 10 + 2] - x.z);
    glm::vec3 v2(vbo_data[i * 30 + 20 + 0] - x.x,
                 vbo_data[i * 30 + 20 + 1] - x.y,
                 vbo_data[i * 30 + 20 + 2] - x.z);


    phyDirection dir;
    if (i % 2 == 0) {
      // x is in a top-left triangle, the order of the vertices in
      // the vbo is:
      //
      //   0    2
      //   +---+
      //   |x /
      //   | /
      //   |/
      //   +
      //   1

      // TODO: Exit through a corner
      if (glm::cross(v0, direction).y < 0
          && glm::cross(direction, v2).y < 0) {
        // Next exit: between top-left and top-right edge
        dir = up;
      } else if (glm::cross(v2, direction).y < 0
                 && glm::cross(direction, v1).y < 0) {
        // Next exit: between top-right and bottom-left edge
        dir = right;
      } else if (glm::cross(v1, direction).y < 0
                 && glm::cross(direction, v0).y < 0) {
        // Next exit: between bottom-left and top-left edge
        dir = left;
      }
    } else {
      // x is in a bottom-right triangle, the order of the vertices
      // in the vbo is:
      //
      //       1
      //       +
      //      /|
      //     / |
      //    / x|
      //   +---+
      //   0    2

      if (glm::cross(v0, direction).y > 0
          && glm::cross(direction, v2).y > 0) {
        // Next exit: between bottom-left and bottom-right edge
      } else if (glm::cross(v2, direction).y > 0
                 && glm::cross(direction, v1).y > 0) {
        // Next exit: between top right and bottom left edge
        dir = right;
      } else if (glm::cross(v1, direction).y > 0
                 && glm::cross(direction, v0).y > 0) {
        // Next exit: between bottom left and top left edge
        dir = left;
      }
    }

    return getNextTriangle(i, dir);
  }

  std::vector<int>
  phyPlane::getTrianglesFromTo(float xStart, float zStart, float xEnd, float zEnd) {
    // TODO
    return std::vector<int>{(int)(xStart + zStart + xEnd + zEnd)};
  }

  bool
  phyPlane::isAbove(glm::vec3 x) {
    int index = getTriangleAt(x);
    // first vertex of the triangle
    glm::vec3 v1(vbo_data[index * 3 * 10 + 0],
                 vbo_data[index * 3 * 10 + 1],
                 vbo_data[index * 3 * 10 + 2]);

    // normal of the triangle's plane
    glm::vec3 norm(vbo_data[index * 3 * 10 + 3 + 0],
                   vbo_data[index * 3 * 10 + 3 + 1],
                   vbo_data[index * 3 * 10 + 3 + 2]);

    // distance between plane and origin
    // float d = -glm::dot(v1, norm);

    return glm::dot(x, norm) > glm::dot(v1, norm);
  }

  // reflect the vector @v at the normal of the triangle at position
  // @pos
  glm::vec3
  phyPlane::reflectAt(glm::vec3 pos, glm::vec3 v) {
    if (!useBoundingBox) {
      if(pos.x < xStart  || pos.x > xEnd
         || pos.z < zStart || pos.z > zEnd) {
        // the bounding box is diabled, and the sphere has left
        // the bounding box, don't reflect at all, just 'fall of
        // the world'
        return v;
      }
    }

    int index = getTriangleAt(pos);

    // normal of the triangle's plane
    glm::vec3 norm(vbo_data[index * 3 * 10 + 3 + 0],
                   vbo_data[index * 3 * 10 + 3 + 1],
                   vbo_data[index * 3 * 10 + 3 + 2]);

    return v - 2*glm::dot(norm, v) * norm;
  }

  void
  phySphere::render() {
    geo.bind();
    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &geo.transform[0][0]);
    glDrawElements(GL_TRIANGLES, geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);
  }

  void
  initShader() {
    unsigned int vertexShader = compileShader("physics.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("physics.frag", GL_FRAGMENT_SHADER);
    phyShaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    light_dir_loc = glGetUniformLocation(phyShaderProgram, "light_dir");
    model_mat_loc = glGetUniformLocation(phyShaderProgram, "model_mat");
    proj_mat_loc = glGetUniformLocation(phyShaderProgram, "proj_mat");
    view_mat_loc = glGetUniformLocation(phyShaderProgram, "view_mat");
  }

  // Load the phyShaderProgram and set all values that are identical
  // for all spheres. The @model_mat will be set individually by each
  // sphere in phySphere:render().
  void
  useShader(camera *cam, glm::mat4 proj_matrix, glm::vec3 light_dir) {
    glUseProgram(phyShaderProgram);
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &cam->view_matrix()[0][0]);
    glUniform3fv(light_dir_loc, 1, &light_dir[0]);
  }
}
