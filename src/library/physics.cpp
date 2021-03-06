#include <physics.hpp>

// Author: Volker Sobek <vsobek@uni-bonn.de>

// physics.cpp and physics.hpp provide a tiny 'just-get-it-working'
// and thus VERY ERROR-PRONE and VERY INCOMPLETE collection of
// functions and structs tailored to the very narrow needs of this
// project.
//
// It is used to simulate friction-less physical interaction between
// planes defined by a 2D height map, and spheres, simulated as
// infinitesimal small mass points but rendered as spheres.

// WARNING ABOUT NON-DETERMINISM
//
// Notice that a floating point operation in a program can yield
// different results each time the program runs. This may become
// obvious when running simulation using the structs/classes defined
// in this file. When running two instances of the same program side
// by side, they start out identical but can differ more and more over
// time. This is *not* caused by using operations involving the system
// time to measure the passed time; the functions herein simply
// advance the time value used for calculations by a fixed amount each
// frame.
//
// See https://isocpp.org/wiki/faq/newbie#floating-point-arith2 for
// more information.

// This is our gravity (in negative y-direction)
#define PHY_DEFAULT_ACCELERATION 0.f, -4.f, 0.f, 0.f


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
                       glm::vec4 color,
					   int visibility_frame) :
    x{x},
    v{v},
    radius{radius},
    plane{plane},
    custom_color{color},
    visibility_frame{visibility_frame},
    a{glm::vec4(PHY_DEFAULT_ACCELERATION)}
  {
    // Since the collision code does not yet take the radius into
    // account, we choose the lowest point of the sphere (in
    // y-direction) as the reference point for the simulation and
    // simply place the center of the mesh @x.y + @radius.
    offset_vec = glm::vec4(0.f, radius, 0.f, 0);
  }

  phySphere::~phySphere() {}

  bool
  phySphere::step(float deltaT) {
    if (plane) {
      // Next position if there were no obstacles.
      glm::vec3 targetPos = x + v * deltaT + 0.5f * a * deltaT * deltaT;

      // Transfer all vectors to the plane's coordinate system, that
      // is before application of the plane's model_mat.
      targetPos = glm::inverse(plane->model_mat) * glm::vec4(targetPos, 1.f);
      x = glm::inverse(plane->model_mat) * x;
      v = glm::inverse(plane->model_mat) * v;
      a = glm::inverse(plane->model_mat) * a;

      // In order to distinguish between bouncing and sliding, we have
      // to keep track of this.
      bool touched_plane = false;

      // FIXME: useBoundingBox is obsolete, with the introduction of
      // affine plane transformations it no longer makes sense!
      //
      // The bounding box is only applied to the x and z direction.
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
          plane->reflect(this);
          touched_plane = true;
        }
      } else {
        // ignoring the bounding box.
        if(targetPos.x < plane->xStart || targetPos.x > plane->xEnd
           || targetPos.z < plane->zStart || targetPos.z > plane->zEnd) {
          // EMPTY
        } else {
          // inside bounding box, reflect
          if (plane->isAbove(targetPos)) {
            // EMPTY
          } else {
            // TODO: x is not the position of the first contact!
            plane->reflect(this);
            touched_plane = true;
          }
        }

        // transform back from the position relative to the plane to
        // world
        x = plane->model_mat * x;
        v = plane->model_mat * v;
        a = plane->model_mat * a;

        x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
        // update velocity
        v = v + a * deltaT;


        // Not used for now
        // drag a = glm::vec3(0, -10.f, 0.f); a = a - 0.01f *
        // (float)pow(glm::length(v), 2.f) * glm::normalize(v);

        // Once a sphere is on the ground, it will be reflected each
        // step, since it will get below the plane each time. In this
        // case, reducing the velocity would look like friction, which
        // we don't want to simulate. Therefore only reduce the speed
        // when the sphere touches the plane for the first time.
        if (touched_plane && !touched_plane_last_step) {
          // TODO: What's the correct physics here? Depending on the
          // model it might be correct to only reduce the velocity in
          // direction of the plane's normal, since we don't use
          // friction. I haven't looked further into that yet.
          v = v * 0.8f;
        }

        touched_plane_last_step = touched_plane;

        // FIXME: This code is rudimentary as it can get. I.e. this
        // check does not take the plane's model matrix into account.
        // It only checks what's needed and easy to check for our very
        // simple use case.
        //
        // The lowest point the plane can reach is determined by its
        // size in z direction. Once the sphere is below that, we can
        // safely disable interaction with the plane. Also, since for
        // now we only tilt the plane, we know for sure that once the
        // sphere is outside the original bounding box in the
        // (x,z)-plane it won't return. Even though in our program the
        // plane only rotates around the x and y axis, this is still
        // nothing more than a rough approximation.
        if (x.y < -plane->zEnd || x.x < plane->xStart || x.x > plane->xEnd
            || x.z < plane->zStart || x.z > plane->zEnd) {
          plane = nullptr;
        }

      }
    } else {
      // no more interaction with a plane, free fall
      x = x + v * deltaT + (0.5f * deltaT * deltaT) * a;
      v = v + a * deltaT;
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
					 glm::vec3 *vertical_velocity,
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
    angular_velocity{angular_velocity},
    vertical_velocity{ vertical_velocity }
  {
    this->xTileWidth = (xEnd - xStart) / (float)(xNumPoints - 1);
    this->zTileWidth = (zEnd - zStart) / (float)(zNumPoints - 1);

    std::cout << "phy:: xTileWidth: " << xTileWidth << ", zTileWidth: " << zTileWidth << "\n";
    std::cout << "phy:: xNumPoints: " << xNumPoints << ", zNumPoints: " << zNumPoints << "\n";

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
    std::cout << "phy:: heightMap dimension: " << zNumPoints << "x" << xNumPoints << "\n";
    std::cout << "phy:: deltaX = " << deltaX << ", deltaZ  = " << deltaZ << "\n";
    std::cout << "phy:: n_vertices = " << n_vertices << "\n";
    std::cout << "phy:: vbo_data size: " << n_vertices * 10 * sizeof(float) / 1000.f << "K\n";

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

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Notice that the vbo_data is not deleted, as it will be used by
    // the collision detection methods.
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, n_vertices * 10 * sizeof(float), vbo_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3*sizeof(float)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6*sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
  }

  phyPlane::~phyPlane() {
    destroy();
    delete[] vbo_data;
  }

  void
  phyPlane::destroy() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
  }

  // TODO: use pointer
  void
  phyPlane::set_model_mat(glm::mat4 model_mat)
  {
    this->model_mat = model_mat;
    this->inv_model_mat = glm::inverse(model_mat);
  }

  glm::mat4
  phyPlane::get_model_mat() {
    return model_mat;
  }

  void
  phyPlane::set_angular_velocity(glm::vec3 *angular_velocity) {
    this->angular_velocity = angular_velocity;
  }

  void
  phyPlane::set_vertical_velocity(glm::vec3 *vertical_velocity) {
	this->vertical_velocity = vertical_velocity;
  }

  void
  phyPlane::step(float deltaT) {
    if (angular_velocity) {
      model_mat = glm::rotate(model_mat, deltaT * glm::length(*angular_velocity),
                              glm::normalize(*angular_velocity));
    }
	if (vertical_velocity) {
		glm::mat4 T = glm::identity<glm::mat4>();
		T[3] = glm::vec4(vertical_velocity->x,
                                 vertical_velocity->y,
                                 vertical_velocity->z, 1.0);
		model_mat = T * model_mat;
	}
  }

  // @pos must coordinates realtive to the plane before any
  // transformations.
  int
  phyPlane::getTriangleAt(glm::vec4 pos) {
    return getTriangleAt(glm::vec3(pos.x, pos.y, pos.z));
  }

  // @pos must coordinates realtive to the plane before any
  // transformations.
  int
  phyPlane::getTriangleAt(glm::vec3 pos) {
    // coordinates in the plane:
    //  +----→ x
    //  |
    //  |
    //  ↓
    //  z

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

    // two triangles per rectangle
    triangleIndex = 2 * (xIndexRect * (zNumPoints - 1) + zIndexRect);
    // if the sphere is in the 'upper'-left triangle it's one less
    if (zInRect > zTileWidth - xInRect * zTileWidth / xTileWidth) {
      triangleIndex++;
    }

    return triangleIndex;
  }


  // UNUSED
  //
  // Return the index of the next triangle in the phyDirection @d
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

  // UNUSED
  //
  // This would/will be needed for proper collision detection. Returns
  // the index of the next triangle that a sphere at position @x and
  // velocity @v will enter.
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

      // TODO: Exit through a corner (would probably require some
      // epsilon values).
      if (glm::cross(v0, direction).y < 0
          && glm::cross(direction, v2).y < 0) {
        // next exit: between top-left and top-right edge
        dir = up;
      } else if (glm::cross(v2, direction).y < 0
                 && glm::cross(direction, v1).y < 0) {
        // next exit: between top-right and bottom-left edge
        dir = right;
      } else if (glm::cross(v1, direction).y < 0
                 && glm::cross(direction, v0).y < 0) {
        // next exit: between bottom-left and top-left edge
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
        // next exit: between bottom-left and bottom-right edge
      } else if (glm::cross(v2, direction).y > 0
                 && glm::cross(direction, v1).y > 0) {
        // next exit: between top right and bottom left edge
        dir = right;
      } else if (glm::cross(v1, direction).y > 0
                 && glm::cross(direction, v0).y > 0) {
        // next exit: between bottom left and top left edge
        dir = left;
      }
    }

    return getNextTriangle(i, dir);
  }


  // NOT IMPLEMENTED YET
  std::vector<int>
  phyPlane::getTrianglesFromTo(float xStart, float zStart, float xEnd, float zEnd) {
    // TODO
    return std::vector<int>{(int)(xStart + zStart + xEnd + zEnd)};
  }


  // Returns true if @x is above the plane. @x must be given in the
  // initial plane coordinates (before application of the model
  // matrix).
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

  // Reflects the sphere @s at this plane. @v must be given in the
  // initial plane coordinates (before application of the model
  // matrix).
  //
  // This code calculates a VERY rough approximation of what would
  // actually happen. A couple of things to notice:
  //
  // 1. The triangle over which the sphere is now, is NOT guaranteed
  //    to be the one where the sphere hit plane. So this only works
  //    with reasonable ratios of speed and triangle sizes.
  //
  // 2. Even if the triangle is the one where the sphere first hit the
  //    plane, the code still does NOT take the real point of first
  //    contact into account. To be accurate, this point would have to
  //    be calculated to base the reflection calculations on. Again,
  //    this code works for us since speed of our spheres and our
  //    triangle sizes behave well [Oh yeah, how scientific! ;)].
  //
  // 3. A correct implementation would have to check the complete path
  //    of the sphere since the last frame for collisions.
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

    if (angular_velocity) {
      // Velocity of the plane in direction of its normal. The plane
      // is considered to have infinity weight, so we can subtract
      // this velocity from @v and apply Newton's third law.
      glm::vec4 v_plane = glm::dot(glm::cross(*angular_velocity, glm::vec3(s->x)),
                                   glm::vec3(norm)) * norm;
      s->v -= v_plane;
    }

    // Move the sphere APPROXIMATELY to the point of first
    // contact. This is a very quick but also very poor (if the
    // incoming angle was not steep, the sphere could have entered
    // many triangles away...) approximation of the point of first
    // contact of the sphere with the plane.
    s->x.y = vbo_data[index * 3 * 10 + 0 + 1];

    // More accurate but slower version of the above. This is also
    // only a good approximation if the triangle above which the
    // sphere entered the plane is identical to the triangle above
    // which the sphere is now.
    //
    // s->moveToPlaneHeight();

    // Reflect v using the plane's normal
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

  // Box-Muller Transform for gaussian random values
  float
  gauss_rand(float mean, float dev) {
	  float x = 1.0 - rand() / (float)RAND_MAX;
	  float y = 1.0 - rand() / (float)RAND_MAX;
	  float rsn = sqrt(-2.f * log(x)) * sin(2.f * 3.14159f * y);
	  return mean + dev * rsn;
  }
}
