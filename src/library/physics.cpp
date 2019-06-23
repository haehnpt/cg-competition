#include <physics.hpp>

void testPhysicsLibraryLinking() {
  std::cout << "physics.cpp linked successfully!\n";
}

void
object::step(float deltaT) {
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
  }
}

object
createObject(float x1, float x2, float x3,
               float v1, float v2, float v3) {
  object o{};

  // position
  o.x[0] = x1;                  // x
  o.x[1] = x2;                  // y
  o.x[2] = x3;                  // z

  // velocity
  o.v[0] = v1;                  // x
  o.v[1] = v2;                  // y
  o.v[2] = v3;                  // z

  // acceleration (hard-coded for now!)
  o.a[0] = 0;                   // x
  o.a[1] = -1; // -9.81;        // y
  o.a[0] = 0;                   // z

  return o;
}
