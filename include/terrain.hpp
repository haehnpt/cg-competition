#pragma once

/*
*
* This terrain class was inspired by
* - https://www.redblobgames.com/maps/terrain-from-noise/
* 
* - author : Patrick H�hn
*
*/

#include "mesh.hpp"
#include "perlin_noise.hpp"
#include <buffer.hpp>
#include <camera.hpp>
#include <shader.hpp>
#include <math.h>
#include <ctime>

class terrain
{
	// Private member properties
	int start_frame = 0;
	int current_frame = 0;
	int frame_loc;
	int max_frame_loc;
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
	float min_height = 0.0;
	float max_height = 1.0;
	float lowest_height = 0.0;
	float highest_height = 1.0;

	// Static private class properties
	static int terrainShaderProgram;
	static int stone_loc;
	static int grass_loc;
	static int snow_loc;
	static int view_mat_loc;
	static int proj_mat_loc;
	static int light_dir_loc;
	static int albedo_loc;
	static int roughness_loc;
	static int ref_index_loc;
	static int terr_model_loc;

	// Private member functions
	float * get_heights(float range, float rigidity);
	void build();
	void clamp_heights();
	void get_frame_locations(int shader_program);
	void set_frames(int start, int max);
	void reset_current_frame();
	void increase_current_frame(int increase = 1);

	// Static private class functions
	static void get_texture_locations(int shader_program);
	static void load_textures(std::string stone, std::string grass, std::string snow);
	static unsigned create_texture_rgba32f(int width, int height, float* data);
	static float* load_texture_data(std::string filename, int* width, int* height);
	static void set_texture_filter_mode(unsigned int texture, GLenum mode);
	static void set_texture_wrap_mode(unsigned int texture, GLenum mode);

public:
	// Public constructor & destructor
	terrain(float size, int resolution, int start_frame, int max_frame, std::string stone, std::string grass, std::string snow);
	~terrain();

	// Public member functions
	glm::vec3 * get_normal_at_pos(float x, float z);
	void render(camera * cam, glm::mat4 proj_matrix, glm::vec3 light_dir);

	// Public static functions
	static void create_terrain_shaders();
};
