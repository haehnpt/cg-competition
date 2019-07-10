#pragma once

#include "mesh.hpp"
#include "perlin_noise.hpp"
#include <buffer.hpp>
#include <camera.hpp>
#include <shader.hpp>
#include <math.h>
#include <ctime>

/*

This class encapsulates the terrain and provides necessary
functions to build and modify it

@author Patrick Hähn

 */
class terrain
{
	// The starting frame of the rise of the terrain
	int start_frame = 0;
	// Frame counter
	int current_frame = 0;
    // The maximum frame of the rise of the terrain
	int frames;
	// Shader location of the current frame
	int frame_loc;
	// Shader location of the maximum frame
	int max_frame_loc;
	// The geometry of the terrain
	geometry terra;
	// The size of the terrain in each dimension
	float size;
	// The resolution (= number of vertices) in each dimension
	int resolution;
	// The minimum height that the terrain may have
	float min_height = 0.0;
	// The maximum height that the terrain may have
	float max_height = 1.0;
	// Save the lowest height that has been generated
	float lowest_height = 0.0;
	// Save the highest height that has been generated
	float highest_height = 1.0;

	// Handle for the terrain shader program
	static int terrainShaderProgram;
	// Shader location of the stone texture
	static int stone_loc;
	// Shader location of the grass texture
	static int grass_loc;
	// Shader location of the snow texture
	static int snow_loc;
	// Shader location of the view matrix
	static int view_mat_loc;
	// Shader location of the projection matrix
	static int proj_mat_loc;
	// Shader location of the light direction
	static int light_dir_loc;
	// Shader location of the albedo value
	static int albedo_loc;
	// Shader location of the roughness value
	static int roughness_loc;
	// Shader location of the refraction index
	static int ref_index_loc;
	// Shader location of the terrain model matrix
	static int terr_model_loc;

	// Generate heights using the perlin_noise class
	float * get_heights(float range, float rigidity);
	// Build the terrain (create vertices etc.)
	void build();
	// Clamp the terrain heights to be in [min_height,max_height]
	void clamp_heights();
	// Allocate shader frame locations
	void get_frame_locations(int shader_program);
	// Set the start and maximum frame
	void set_frames(int start, int max);
	// Reset the current frame
	void reset_current_frame();
	// Increase the current frame
	void increase_current_frame(int increase = 1);

	// Allocate shader texture locations
	static void get_texture_locations(int shader_program);
	// Load the stone, grass and snow textures
	static void load_textures(std::string stone, std::string grass, std::string snow);
	// Create textures from raw data
	static unsigned create_texture_rgba32f(int width, int height, float* data);
	// Load raw texture data
	static float* load_texture_data(std::string filename, int* width, int* height);
	// Set the texture filter mode of a texture
	static void set_texture_filter_mode(unsigned int texture, GLenum mode);
	// Set the texture wrap mode of a texture
	static void set_texture_wrap_mode(unsigned int texture, GLenum mode);

public:
	// Create a new instance of terrain
	terrain(float size, int resolution, int start_frame, int max_frame, std::string stone, std::string grass, std::string snow);
	// Clean up
	~terrain();

	// Public terrain heights (=> physics)
	float * heights;

	// Get the normal of the triangle which matches position (x,z)
	glm::vec3 * get_normal_at_pos(float x, float z);
	// Render the terrain
	void render(camera * cam, glm::mat4 proj_matrix, glm::vec3 light_dir);
	// Set the model matrix of the terrain
	void set_model_mat(glm::mat4 model_mat);

	// Create the terrain shader program
	static void create_terrain_shaders();

};


// Local Variables:
// indent-tabs-mode: t
// tab-width: 4
// c-file-style: "cc-mode"
// End:
