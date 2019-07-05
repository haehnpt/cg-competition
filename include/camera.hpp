#pragma once

#include "common.hpp"

struct camera {
    camera(GLFWwindow* window);

    virtual ~camera();

    glm::mat4
    view_matrix() const;

    glm::vec3
    position() const;

	// These four functions were added
	// by me
	void set_phi(float);
	void set_theta(float);
	void set_distance(float);
	void rotate(float angle = 0.002);
};
