#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 color;

uniform mat4 model_mat;

uniform mat4 view_mat;
uniform mat4 proj_mat;
uniform vec3 light_dir;
uniform float frame;
uniform float max_frame;
uniform int index;

uniform vec4 hill_color;
uniform vec4 mountain_color;
uniform vec4 snow_color;
uniform vec4 ground_color;

vec4 actual_col;

out vec4 interp_color;
out vec3 interp_normal;
out vec3 interp_light_dir;
out vec2 uv;
out vec2 tex_height;

float rand(vec3 position){
	vec3 n = normalize(position);
	return (n.x + n.y + n.z) / 1.0;
}

void main()
{
	// POSITION
	float delta = min(1.0, frame / max_frame);
    gl_Position = proj_mat * view_mat * model_mat * vec4(position.x, delta * position.y, position.z, 1.0);

	// COLOR
	//float color_byte = (delta * position.y) / 2.0 + 0.5;
    //interp_color = vec4(color_byte, 0.0, 0.0, 0.5);
	float actual_height = delta * position.y;
	actual_col = snow_color;
	if (actual_height < 0.9) actual_col = mountain_color;
	if (actual_height < 0.5) actual_col = hill_color;
	if (actual_height < 0.1) actual_col = ground_color;
	interp_color = vec4(1.0,1.0,1.0,1.0);//actual_col;

	// LIGHT DIRECTION
    // compute directions towards light as well as surface normal in eye-space
	vec3 temp_normal = normalize((1.0 - delta) * vec3(0.0,1.0,0.0) + delta * normal);
    interp_normal = normalize((transpose(inverse(view_mat * model_mat)) * vec4(temp_normal, 0.0)).xyz);
    interp_light_dir = normalize((view_mat * vec4(light_dir, 0.0)).xyz);

	// get uv
	uv = vec2(color.x, color.y);

	// decide texture
	actual_height += rand(position) * 0.1;
	tex_height = vec2(actual_height,0.0);// > 0.3 ? vec2(1.0,0.0) : vec2(0.0,0.0);
}
