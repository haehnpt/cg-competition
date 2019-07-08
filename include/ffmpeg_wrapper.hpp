#pragma once

#include <string>
#include "config.hpp"
#include "common.hpp"

#define FFMPEG_FILE_NAME "Vorschauvideo.mp4"

class ffmpeg_wrapper
{
    // Private member properties
	int width;
	int height;
	int frames;
	int frame_counter;
	FILE * ffmpeg;

public:
    // Public constructor & destructor
	ffmpeg_wrapper(int width, int height, int frames);
	~ffmpeg_wrapper();

    // Public member functions
	void save_frame();
	bool is_finished();
};