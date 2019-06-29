#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color;

uniform mat4 model_mat;
uniform mat4 view_mat;
uniform mat4 proj_mat;

out vec4 interp_color;
out vec3 interp_normal;

uniform vec4 special_color;
uniform bool use_special_color;
uniform vec3 active_vert_1;
uniform vec3 active_vert_2;
uniform vec3 active_vert_3;

void main()
{
    gl_Position = proj_mat * view_mat * model_mat * vec4(position.x, position.y, position.z, 1.0);

    if (use_special_color
        && (position == active_vert_1
            || position == active_vert_2
            || position == active_vert_3)) {
      interp_color = special_color;
    } else {
      interp_color = color;
    }

    interp_normal = normalize((transpose(inverse(model_mat)) * vec4(normal, 0.0)).xyz);
}
