#pragma once

/*
*
* This terrain class was inspired by
* - https://www.redblobgames.com/maps/terrain-from-noise/
* 
* - author : Patrick Hähn
*
*/

#include "mesh.hpp"
#include "perlin_noise.hpp"
#include <buffer.hpp>
#include <math.h>
#include <ctime>

class terrain
{
	float * get_heights(float range, float rigidity);
	void build();
	void clamp_heights();
	int start_frame = 0;
	int current_frame = 0;
	int frame_loc;
	int max_frame_loc;
	static int stone_loc;
	static int grass_loc;
	static int snow_loc;
public:
	terrain(float size, int resolution, int start_frame, int max_frame, int shader_program, std::string stone, std::string grass, std::string snow);
	~terrain();
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
	const glm::vec4 hill_color = glm::vec4(0.7,0.7,0.7,1.0);
	const glm::vec4 mountain_color = glm::vec4(0.5,0.5,0.5, 1.0);
	const glm::vec4 snow_color = glm::vec4(1.0,1.0,1.0, 1.0);
	const glm::vec4 ground_color = glm::vec4(0.2,0.5,0.2, 1.0);
	float min_height = 0.0;
	float max_height = 1.0;
	float lowest_height = 0.0;
	float highest_height = 1.0;
	void get_frame_locations(int shader_program);
	void set_frames(int start, int max);
	void reset_current_frame();
	void increase_current_frame(int increase = 1);
	void render(int model_loc);
	static void get_texture_locations(int shader_program);

	static void load_textures(std::string stone, std::string grass, std::string snow){
		int image_width, image_height;

		// Stone texture
		float* image_tex_data = load_texture_data(std::string(DATA_ROOT) + stone, &image_width, &image_height);
		unsigned int image_tex1 = create_texture_rgba32f(image_width, image_height, image_tex_data);
		glBindTextureUnit(0, image_tex1);
		delete[] image_tex_data;

		// Grass texture
		image_tex_data = load_texture_data(std::string(DATA_ROOT) + grass, &image_width, &image_height);
		unsigned int image_tex2 = create_texture_rgba32f(image_width, image_height, image_tex_data);
		glBindTextureUnit(1, image_tex2);
		delete[] image_tex_data;

		// Snow texture
		image_tex_data = load_texture_data(std::string(DATA_ROOT) + snow, &image_width, &image_height);
		unsigned int image_tex3 = create_texture_rgba32f(image_width, image_height, image_tex_data);
		glBindTextureUnit(2, image_tex3);
		delete[] image_tex_data;

		// Set properties
		set_texture_filter_mode(image_tex1, GL_LINEAR_MIPMAP_LINEAR);
		set_texture_filter_mode(image_tex2, GL_LINEAR_MIPMAP_LINEAR);
		set_texture_filter_mode(image_tex3, GL_LINEAR_MIPMAP_LINEAR);

		set_texture_wrap_mode(image_tex1, GL_MIRRORED_REPEAT);
		set_texture_wrap_mode(image_tex2, GL_MIRRORED_REPEAT);
		set_texture_wrap_mode(image_tex3, GL_MIRRORED_REPEAT);
	}

	static unsigned create_texture_rgba32f(int width, int height, float* data) {
		unsigned handle;
		glCreateTextures(GL_TEXTURE_2D, 1, &handle);
		glTextureStorage2D(handle, 1, GL_RGBA32F, width, height);
		glTextureSubImage2D(handle, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);

		return handle;
	}

	static float* load_texture_data(std::string filename, int* width, int* height) {
		int channels;
		unsigned char* file_data = stbi_load(filename.c_str(), width, height, &channels, 3);

		int w = *width;
		int h = *height;

		float* data = new float[4 * w * h];
		for (int j = 0; j < h; ++j) {
			for (int i = 0; i < w; ++i) {
				data[j*w * 4 + i * 4 + 0] = static_cast<float>(file_data[j*w * 3 + i * 3 + 0]) / 255;
				data[j*w * 4 + i * 4 + 1] = static_cast<float>(file_data[j*w * 3 + i * 3 + 1]) / 255;
				data[j*w * 4 + i * 4 + 2] = static_cast<float>(file_data[j*w * 3 + i * 3 + 2]) / 255;
				data[j*w * 4 + i * 4 + 3] = 1.f;
			}
		}

		delete[] file_data;

		return data;
	}

	static void set_texture_filter_mode(unsigned int texture, GLenum mode) {
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, mode);
	}

	static void set_texture_wrap_mode(unsigned int texture, GLenum mode) {
		glTextureParameteri(texture, GL_TEXTURE_WRAP_S, mode);
		glTextureParameteri(texture, GL_TEXTURE_WRAP_T, mode);
	}
};
