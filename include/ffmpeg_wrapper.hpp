#pragma once

#include <string>
#include "config.hpp"
#include "common.hpp"

/*

Wrapper class for the GNU General Public Licensed Tool FFMPEG
which is used in this project to render a 1920x1080@60FPS
video 

@author Patrick Hähn

 */
class ffmpeg_wrapper
{
    // Rendered video width
	int width;
	// Rendered video height
	int height;
	// Number of frames to be rendered
	int frames;
	// Frame counter
	int frame_counter;
	// Handle for the video file to be created
	FILE * ffmpeg;

public:
    // Create a new ffmpeg_wrapper instance
	ffmpeg_wrapper(int width, int height, int frames, char * render_filename);
	// Clear up
	~ffmpeg_wrapper();

    // Save a frame, e.g. send it to FFMPEG
	void save_frame();
	// Retrieve whether the rendering has finished
	bool is_finished();
};