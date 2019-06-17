#pragma once
#include "mesh.hpp"
#include "perlin_noise.hpp"
#include <buffer.hpp>
#include <math.h>
#include <ctime>

class terrain
{
	float * getHeights(float range);
	void calculateNormals(int frame);
public:
	terrain(float size, int resolution, int frames);
	void build(int frame);
	~terrain();
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
};

