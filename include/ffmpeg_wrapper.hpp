#pragma once

#include <string>
#include "config.hpp"
#include "common.hpp"

class ffmpeg_wrapper
{
	int width;
	int height;
	int frames;
	int frame_counter;
	FILE * ffmpeg;
public:
	ffmpeg_wrapper(int width, int height, int frames);
	~ffmpeg_wrapper();
	void save_frame();
	bool is_finished();
};