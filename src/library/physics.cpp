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
        if (x[i] < -10 && v[i] < 0) {
            v[i] = -0.90 * v[i];
        } else if (x[i] > 10 && v[i] > 0) {
            v[i] = -0.90 * v[i];
        }
    }

    // FIXME: remove hard-coded scaling/size
    geo.transform = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(x[0], x[1], x[2]));
    // std::cout << geo.transform;
}

phySphere
createPhySphere(float x1, float x2, float x3,
                float v1, float v2, float v3,
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
    s.a[0] = 0;                   // z

    s.geo = loadMesh("sphere.obj", false, color);
    // FIXME: remove hard-coded scaling/size
    s.geo.transform = glm::scale(glm::vec3(0.1f)) * glm::translate(glm::vec3(s.x[0], s.x[1], s.x[2]));
    return s;
}
