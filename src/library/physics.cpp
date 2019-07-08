#include <physics.hpp>

#include <common.hpp>
#include <mesh.hpp>
#include <camera.hpp>
#include <shader.hpp>
#include <glm/gtx/transform.hpp>

// For glm::to_string
#include "glm/gtx/string_cast.hpp"

// Taking into account (affine) plane transformations requires
// additional matrix multiplications in each phySphere::step() but is
// REQUIRED if any affine transformations (other than the identity
// matrix) will be applied the original plane data. If you never apply
// any transformations to your planes you can comment this out.
#define WITH_AFFINE_PLANE_TRANSFORMATIONS


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
    int custom_color_loc;
    geometry geo;
  }

  phySphere::phySphere(glm::vec4 x,
                       glm::vec4 v,
                       float radius,
                       phyPlane *plane,
                       glm::vec4 color) :
    x{x},
    v{v},
    radius{radius},
    plane{plane},
    custom_color{color},
    a{glm::vec4(0.f, -4.f, 0.f, 0.f)}
  {
    // std::cout << "phySphere with pos = (" << x.x << ", " << x.y << ", " << x.z << ")\n";

    // Since the collision code does not yet take the radius into
    // account, we choose the lowest point of the sphere (in
    // y-direction) as the reference point for the simulation and
    // simply place the center of the mesh @x.y + @radius.
    offset_vec = glm::vec4(0.f, radius, 0.f, 0);
  }

  phySphere::~phySphere() {}

  bool
  phySphere::step(float deltaT) {
    // next position if there were no obstacles
    glm::vec3 targetPos = x + v * deltaT + 0.5f * a * deltaT * deltaT;

#ifdef WITH_AFFINE_PLANE_TRANSFORMATIONS
    // transfer all vectors to the plane's coordinate system, that is
    // before application of the plane's model_mat
    targetPos = glm::inverse(plane->model_mat) * glm::vec4(targetPos, 1.f);
    x = glm::inverse(plane->model_mat) * x;
    v = glm::inverse(plane->model_mat) * v;
    a = glm::inverse(plane->model_mat) * a;
#endif //  WITH_AFFINE_PLANE_TRANSFORMATIONS

    bool touched_plane = false;

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
      } else {
        // TODO: x is not the position of the first contact!
        // FIXME: this crashes the program
        plane->reflect(this);
        touched_plane = true;
      }
    } else {
      // ignoring the bounding box
      if(targetPos.x < plane->xStart || targetPos.x > plane->xEnd
         || targetPos.z < plane->zStart || targetPos.z > plane->zEnd) {
        // EMPTY
      } else {
        // inside bounding box, reflect
        if (plane->isAbove(targetPos)) {
          // EMPTY
        } else {
          // TODO: x is not the position of the first contact!
          // FIXME: this crashes the program
          plane->reflect(this);
          touched_plane = true;
        }
      }

#ifdef WITH_AFFINE_PLANE_TRANSFORMATIONS
      // transform back from the position relative to the plane to
      // world
      x = plane->model_mat * x;
      v = plane->model_mat * v;
      a = plane->model_mat * a;
#endif // WITH_AFFINE_PLANE_TRANSFORMATIONS

      x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
      // update velocity
      v = v + a * deltaT;

      // drag a = glm::vec3(0, -10.f, 0.f); a = a - 0.01f *
      // (float)pow(glm::length(v), 2.f) * glm::normalize(v);

      // plane->getTriangleIndex(this);

      // Once a sphere is on the ground, it will be reflected each
      // step, since it will get below the plane each time. In this
      // case, reducing the velocity would look like friction, which
      // we don't want to simulate. Therefore only reduce the speed
      // when the sphere touches the plane for the first time.
      if (touched_plane && !touched_plane_last_step) {
        v = v * 0.8f;
      }

    touched_plane_last_step = touched_plane;

    }

    return true;
  }

  void
  phySphere::setPosition(glm::vec4 pos) {
    x = pos;
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
  }

  void
  phySphere::render() {
    geo.transform = glm::translate(glm::vec3(x + offset_vec))
      * glm::scale(glm::vec3(radius));
    geo.bind();

    // Set model matrix for this sphere
    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &geo.transform[0][0]);
    // Set custom color for this sphere
    glUniform4fv(custom_color_loc, 1, &custom_color[0]);
    glDrawElements(GL_TRIANGLES, geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);
  }

  phyPlane::phyPlane(float xStart,
                     float xEnd,
                     float zStart,
                     float zEnd,
                     float *heightMap,
                     int xNumPoints,
                     int zNumPoints,
                     bool useBoundingBox,
                     glm::vec3 *angular_velocity,
                     glm::vec4 custom_color) :
    xStart{xStart},
    xEnd{xEnd},
    zStart{zStart},
    zEnd{zEnd},
    xNumPoints{xNumPoints},
    zNumPoints{zNumPoints},
    useBoundingBox{useBoundingBox},
    custom_color{custom_color},
    model_mat{glm::mat4(1.f)},
    angular_velocity{angular_velocity}
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
    n_vertices = 6 * (xNumPoints * zNumPoints - xNumPoints - zNumPoints + 1);
    vbo_data = new float[n_vertices * 10];
    float deltaX = (xEnd - xStart) / (xNumPoints - 1);
    float deltaZ = (zEnd - zStart) / (zNumPoints - 1);

    // DEBUG:
    std::cout << "heightMap dimension: " << zNumPoints << "x" << xNumPoints << "\n";
    std::cout << "deltaX = " << deltaX << ", deltaZ  = " << deltaZ << "\n";
    std::cout << "n_vertices = " << n_vertices << "\n";
    std::cout << "vbo_data size: " << n_vertices * 10 * sizeof(float) / 1000.f << "K\n";

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
    // for (int i = 0; i < n_vertices; i++) {
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
    glBufferData(GL_ARRAY_BUFFER, n_vertices * 10 * sizeof(float), vbo_data, GL_STATIC_DRAW);

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
  phyPlane::destroy() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }

  // TODO: Use pointer
  void
  phyPlane::set_model_mat(glm::mat4 model_mat)
  {
    this->model_mat = model_mat;
    this->inv_model_mat = glm::inverse(model_mat);
  }

  void
  phyPlane::set_angular_velocity(glm::vec3 *angular_velocity) {
    this->angular_velocity = angular_velocity;
  }

  void
  phyPlane::step(float deltaT) {
    if (angular_velocity) {
      model_mat = glm::rotate(model_mat, deltaT * glm::length(*angular_velocity),
                              glm::normalize(*angular_velocity));
    }
  }

  int
  phyPlane::getTriangleAt(glm::vec4 pos) {
    return getTriangleAt(glm::vec3(pos.x, pos.y, pos.z));
  }

  // @pos must coordinates realtive to the plane before any
  // transformations.
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


  // x is the position relative to the plane before any
  // transformations.
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

  // v is the velocity relative to the plane BEFORE any plane transformations
  // reflect the vector @v at the normal of the triangle at position
  // @pos
  void
  phyPlane::reflect(phySphere *s) {
    if (!useBoundingBox) {
      if(s->x.x < xStart  || s->x.x > xEnd
         || s->x.z < zStart || s->x.z > zEnd) {
        // the bounding box is diabled, and the sphere has left
        // the bounding box, don't reflect at all, just 'fall of
        // the world'
        return;
      }
    }

    int index = getTriangleAt(s->x);

    // normal of the triangle's plane
    glm::vec4 norm(vbo_data[index * 3 * 10 + 3 + 0],
                   vbo_data[index * 3 * 10 + 3 + 1],
                   vbo_data[index * 3 * 10 + 3 + 2],
                   0);

    // std::cout << "pos: " << glm::to_string(pos) << "\n";
    // std::cout << "vec3(pos): " << glm::to_string(glm::vec3(pos)) << "\n";

    if (angular_velocity) {
      // std::cout << "*angular_velocity: " << glm::to_string(*angular_velocity) << "\n";

      // velocity of the plane in direction of its normal. The plane is
      // considered to have infinity weight, so we can substract this
      // speed from @v.
      glm::vec4 v_plane = glm::dot(glm::cross(*angular_velocity, glm::vec3(s->x)),
                                   glm::vec3(norm)) * norm;
      s->v -= v_plane;

      // std::cout << "v: " << glm::to_string(s->v)
      //           << ", v += v_plane: " << glm::to_string(s->v += v_plane) << "\n";
    }

    // More accurate version, but this is also only correct if the
    // triangle above which the sphere entered the plane is identical
    // to the triangle above which the sphere is now.
    // s->moveToPlaneHeight();

    // Move the sphere APPROXIMATELY to the point of first
    // contact. This is a very quick but also very poor (if the
    // incoming angle was not steep) approximation of the point of
    // first contact of the sphere with the plane.
    s->x.y = vbo_data[index * 3 * 10 + 0 + 1];

    s->v = s->v - 2*glm::dot(norm, s->v) * norm;
  }

  void
  phyPlane::render() {
    glBindVertexArray(vao);
    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &model_mat[0][0]);
    glUniform4fv(custom_color_loc, 1, &custom_color[0]);
    glDrawArrays(GL_TRIANGLES, 0, n_vertices);
  }

  void
  initShader() {
    geo = loadMesh("sphere.obj", false);

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
    custom_color_loc = glGetUniformLocation(phyShaderProgram, "custom_color");
  }

  // Load the phyShaderProgram and set all values that are identical
  // for all phy objcets such as planes and spheres. The @model_mat
  // will be set individually by each phy object in their render()
  // method.
  void
  useShader(camera *cam, glm::mat4 proj_matrix, glm::vec3 light_dir) {
    glUseProgram(phyShaderProgram);
    glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);
    glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &cam->view_matrix()[0][0]);
    glUniform3fv(light_dir_loc, 1, &light_dir[0]);
  }
}
