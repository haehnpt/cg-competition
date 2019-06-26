#include <physics.hpp>

#include <common.hpp>
#include <mesh.hpp>
#include <glm/gtx/transform.hpp>

void testPhysicsLibraryLinking() {
    std::cout << "physics.cpp linked successfully!\n";
}

void
phySphere::step(float deltaT) {
    // All these calculations could be done with one big matrix and one
    // vector, but I don't know yet how to best do that (are there any
    // matrix/vector libraries already? glm only goes up to 4x4
    // matrices). Also, using a single big matrix involves many
    // superfluous multiplications with zeros.

    for (int i = 0; i < 3; i++) {
        // update position
        x[i] = x[i] + v[i] * deltaT + a[i] * 0.5 * deltaT * deltaT;
        // update velocity
        v[i] = v[i] + a[i] * deltaT;


        // for TESTING: a hard-coded bounding box
        if (x[i] - radius < -6 && v[i] < 0) {
            v[i] = -0.90 * v[i];
        } else if (x[i] + radius > 6 && v[i] > 0) {
            v[i] = -0.90 * v[i];
        }
    }

    geo.transform = glm::translate(glm::vec3(x[0], x[1], x[2]))
        * glm::scale(glm::vec3(radius));
}

phySphere
createPhySphere(float x1, float x2, float x3,
                float v1, float v2, float v3,
                float radius,
                glm::vec4 color) {

    phySphere s{};

    // position
    s.x[0] = x1;                  // x
    s.x[1] = x2;                  // y
    s.x[2] = x3;                  // z

    // velocity
    s.v[0] = v1;                  // x
    s.v[1] = v2;                  // y
    s.v[2] = v3;                  // z

    // acceleration (hard-coded for now!)
    s.a[0] = 0;                   // x
    s.a[1] = -10;                 // y
    s.a[2] = 0;                   // z

    s.geo = loadMesh("sphere.obj", false, color);
    s.radius = radius;

    s.geo.transform = glm::translate(glm::vec3(s.x[0], s.x[1], s.x[2]))
        * glm::scale(glm::vec3(s.radius));
    return s;
}

phyPlane
createPhyPlane(float xStart, float xEnd, float zStart, float zEnd) {
    // TODO: heightMap as parameter
    phyPlane p{};

    // This will later come from Patrick's generted data
    // float heightMap[5][5] {
    //   {0.05f, 0.f, 0.f, 0.05f, 0.f},
    //   {0.f, 0.f, 0.f, 0.05f, 0.f},
    //   {0.05f, 0.f, 0.f, 0.05f, 0.05f},
    //   {0.05f, 0.f, 0.f, 0.f, 0.f},
    //   {0.05f, 0.f, 0.05f, 0.05f, 0.05f}
    // };

    // some friendly terrain for testing
    float heightMap[6][5] {
        {-0.1f, -0.1f, -0.0f, -0.0f, -0.1f},
        {-0.0f, -0.2f, -0.4f, -0.3f, -0.0f},
        {-0.2f, -1.6f, -1.8f, -1.2f, -0.2f},
        {-0.1f, -1.8f, -1.7f, -1.1f, -0.0f},
        {-0.1f, -0.2f, -0.2f, -1.3f, -0.2f},
        {-0.1f, -0.0f, -0.1f, -0.2f, -0.1f}
    };

    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 5; j++) {
            heightMap[i][j] -= 4.f;
        }
    }

    // float heightMap[4][3] {
    //     {0.f, 0.f, 0.f},
    //     {0.f, 0.f, 0.f},
    //     {0.f, 0.f, 0.f},
    //     {0.f, 0.f, 0.f}
    // };

    // If you can't be bothered to learn C++ ... ;)
    int zNumPoints = sizeof(heightMap) / sizeof(heightMap[0]);
    int xNumPoints = sizeof(heightMap[0]) / sizeof(heightMap[0][0]);

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
    p.mVertices = 6 * (xNumPoints * zNumPoints - xNumPoints - zNumPoints + 1);
    p.vbo_data = new float[p.mVertices];
    float deltaX = (xEnd - xStart) / (xNumPoints - 1);
    float deltaZ = (zEnd - zStart) / (zNumPoints - 1);

    // DEBUG:
    std::cout << "heightMap dimension: " << zNumPoints << "x" << xNumPoints << "\n";
    std::cout << "deltaX = " << deltaX << ", deltaZ  = " << deltaZ << "\n";
    std::cout << "p.mVertices = " << p.mVertices << "\n";

    // This for-loop loops over the squares between the data
    // points. (x,z) represents the upper-left vertex of the current
    // square. For each square the two contained triangles (called
    // top-left and bottom-right triangle) are added to the VBO data.
    int indexRectTopLeft = 0;
    for (int z = 0; z < zNumPoints - 1; z++) {
        for (int x = 0; x < xNumPoints - 1; x++) {
            glm::vec3 a, b, nrm;
            //// TOP-LEFT TRIANGLE ////
            // top-left vertex of the square
            p.vbo_data[indexRectTopLeft + 0] = xStart + x * deltaX;
            p.vbo_data[indexRectTopLeft + 1] = heightMap[x][z];
            p.vbo_data[indexRectTopLeft + 2] = zStart + z * deltaZ;
            // bottom-left vertex of the square
            p.vbo_data[indexRectTopLeft + 10 + 0] = xStart + x * deltaX;
            p.vbo_data[indexRectTopLeft + 10 + 1] = heightMap[x][z + 1];
            p.vbo_data[indexRectTopLeft + 10 + 2] = zStart + (z + 1) * deltaZ;
            // top-right vertex of the square
            p.vbo_data[indexRectTopLeft + 20 + 0] = xStart + (x + 1) * deltaX;
            p.vbo_data[indexRectTopLeft + 20 + 1] = heightMap[x + 1][z];
            p.vbo_data[indexRectTopLeft + 20 + 2] = zStart + z * deltaZ;
            // add the same normal to all three vertices:
            a = glm::vec3(p.vbo_data[indexRectTopLeft + 10 + 0] - p.vbo_data[indexRectTopLeft + 0],
                          p.vbo_data[indexRectTopLeft + 10 + 1] - p.vbo_data[indexRectTopLeft + 1],
                          p.vbo_data[indexRectTopLeft + 10 + 2] - p.vbo_data[indexRectTopLeft + 2]);
            b = glm::vec3(p.vbo_data[indexRectTopLeft + 20 + 0] - p.vbo_data[indexRectTopLeft + 0],
                          p.vbo_data[indexRectTopLeft + 20 + 1] - p.vbo_data[indexRectTopLeft + 1],
                          p.vbo_data[indexRectTopLeft + 20 + 2] - p.vbo_data[indexRectTopLeft + 2]);
            nrm = glm::normalize(glm::cross(a, b));
            for (int vert = 0; vert < 3; vert++) {
                for (int coord = 0; coord < 3; coord++) {
                    // normal's x,y,z values start at index 3
                    p.vbo_data[indexRectTopLeft + (vert * 10) + 3 + coord] = nrm[coord];
                }
                // default color
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 0] = 0.5f; // r
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 1] = 0.5f; // g
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 2] = 1.f; // b
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 3] = 1.f; // a
            }

            //// BOTTOM-RIGHT TRIANGLE ////
            // bottom-left vertex of the square
            p.vbo_data[indexRectTopLeft + 30 + 0] = xStart + x * deltaX;
            p.vbo_data[indexRectTopLeft + 30 + 1] = heightMap[x][z + 1];
            p.vbo_data[indexRectTopLeft + 30 + 2] = zStart + (z + 1) * deltaZ;
            // top-right vertex of the square
            p.vbo_data[indexRectTopLeft + 40 + 0] = xStart + (x + 1) * deltaX;
            p.vbo_data[indexRectTopLeft + 40 + 1] = heightMap[x + 1][z];
            p.vbo_data[indexRectTopLeft + 40 + 2] = zStart + z * deltaZ;
            // bottom-right vertex of the square
            p.vbo_data[indexRectTopLeft + 50 + 0] = xStart + (x + 1) * deltaX;
            p.vbo_data[indexRectTopLeft + 50 + 1] = heightMap[x + 1][z + 1];
            p.vbo_data[indexRectTopLeft + 50 + 2] = zStart + (z + 1) * deltaZ;
            // add the same normal to all three vertices:
            a = glm::vec3(p.vbo_data[indexRectTopLeft + 40 + 0] - p.vbo_data[indexRectTopLeft + 50 + 0],
                          p.vbo_data[indexRectTopLeft + 40 + 1] - p.vbo_data[indexRectTopLeft + 50 + 1],
                          p.vbo_data[indexRectTopLeft + 40 + 2] - p.vbo_data[indexRectTopLeft + 50 + 2]);
            b = glm::vec3(p.vbo_data[indexRectTopLeft + 30 + 0] - p.vbo_data[indexRectTopLeft + 50 + 0],
                          p.vbo_data[indexRectTopLeft + 30 + 1] - p.vbo_data[indexRectTopLeft + 50 + 1],
                          p.vbo_data[indexRectTopLeft + 30 + 2] - p.vbo_data[indexRectTopLeft + 50 + 2]);
            nrm = glm::normalize(glm::cross(a, b));

            for (int vert = 3; vert < 6; vert++) {
                for (int coord = 0; coord < 3; coord++) {
                    // normal's x,y,z values start at index 3
                    p.vbo_data[indexRectTopLeft + (vert * 10) + 3 + coord] = nrm[coord];
                }
                // default color
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 0] = 1.f; // r
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 1] = 0.5f; // g
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 2] = 0.5f; // b
                p.vbo_data[indexRectTopLeft + (vert * 10) + 6 + 3] = 1.f; // a
            }

            // Per square (= 2 triangles) 6 vertices are added, update index
            indexRectTopLeft += 6 * 10;
        }
    }


    // DEBUG print
    for (int i = 0; i < p.mVertices; i++) {
        std::cout << "====== Vertix " << i << ":\n";

        for (int j = 0; j < 10; j++) {
            std::cout << p.vbo_data[i * 10 + j] << " ";
            std::cout << ((j == 2 || j == 5) ? "\n" : " ");
        }

        std::cout << "\n\n";
    }

    glGenVertexArrays(1, &p.vao);
    glBindVertexArray(p.vao);

    glGenBuffers(1, &p.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
    glBufferData(GL_ARRAY_BUFFER, p.mVertices * 10 * sizeof(float), p.vbo_data, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3*sizeof(float)));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6*sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // We don't delete p.vbo_data, as we will use it for collision
    // detection!
    //
    // delete [p.vbo_data];

    return p;
}

void
phyPlane::bind() {
    glBindVertexArray(vao);
}
