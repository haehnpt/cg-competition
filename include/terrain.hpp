#pragma once
#include "mesh.hpp"
#include "perlin_noise.hpp"
#include <buffer.hpp>
#include <math.h>
#include <ctime>

class terrain
{
	float * getHeights(float range, float rigidity);
	void build();
	void clampHeights();
public:
	terrain(float size, int resolution, int frames);
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

