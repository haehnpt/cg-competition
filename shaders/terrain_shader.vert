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

uniform sampler2D stone_tex;
uniform sampler2D grass_tex;
uniform sampler2D snow_tex;

vec4 actual_col;

out vec4 interp_color;
out vec3 interp_normal;
out vec3 interp_light_dir;
out vec2 uv;
out vec2 tex_height;

float texture_height_offset(vec3 position){
	vec3 n = normalize(position);
	return 0.2 * (sin(n.x * n.y) + cos(n.y + n.x));
}

void main()
{
	// GET (U,V) TEXTURE COORDINATES
	uv = vec2(color.x, color.y);

	// GET DELTA AND FRAME HEIGHT
	float delta = min(1.0, frame / max_frame);
	float actual_height = delta * position.y;

	// DECIDE TEXTURE ( & TEXTURE HEIGHT)
	actual_height += texture_height_offset(position);
	tex_height = vec2(actual_height,0.0);// > 0.3 ? vec2(1.0,0.0) : vec2(0.0,0.0);

	// LIGHT DIRECTION
	vec3 temp_normal = normalize((1.0 - delta) * vec3(0.0,1.0,0.0) + delta * normal);
    interp_normal = normalize((transpose(inverse(view_mat * model_mat)) * vec4(temp_normal, 0.0)).xyz);
    interp_light_dir = normalize((view_mat * vec4(light_dir, 0.0)).xyz);

	// POSITION & DISPLACEMENT MAPPING
	float displacement_amplitude = 0.005;
	vec4 displacement;
	if (tex_height.x > 0.8) displacement = texture2D(snow_tex, uv);
	else if (tex_height.x > 0.5) displacement = texture2D(stone_tex, uv);
	else displacement = texture2D(grass_tex, uv);
	float displacement_scalar = ((displacement.r + displacement.g + displacement.b) / 3.0 / (0.5 / displacement_amplitude) - displacement_amplitude) * tex_height.x;

	vec4 displaced_position = vec4(position.x, delta * position.y, position.z, 1.0) + displacement_scalar * vec4(interp_normal, 0.0);
    gl_Position = proj_mat * view_mat * model_mat * displaced_position;

	// COLOR
	interp_color = vec4(1.0,1.0,1.0,1.0);
}
