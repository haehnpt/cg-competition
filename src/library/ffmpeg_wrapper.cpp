#include "ffmpeg_wrapper.hpp"

// Create a new ffmpeg_wrapper instance
ffmpeg_wrapper::ffmpeg_wrapper(int width, int height, int frames, char * render_filename)
{
	// Set properties
	this->width = width;
	this->height = height;
	this->frames = frames;
	this->frame_counter = 0;

	// Distinguish between linux and windows
	// and call ffmpeg accordingly
    #ifdef __linux__
	std::string cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s "
	  + std::to_string(width) + "x" + std::to_string(height) + " -i - "
	  + "-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip "
	  + FFMPEG_ROOT + std::string(render_filename);
        ffmpeg = popen(cmd.c_str(), "w");
    #else
	std::string cmd = std::string("\"") + FFMPEG_ROOT + std::string("ffmpeg.exe\" -r 60 -f rawvideo -pix_fmt rgba -s " + std::to_string(width) + "x" + std::to_string(height) + " -i - "
		"-threads 0 -preset fast -y -pix_fmt yuv420p -crf 15 -vf vflip ") + FFMPEG_ROOT + std::string(render_filename);
	ffmpeg = _popen(cmd.c_str(), "wb");
    #endif

    // Check if the handle has been created
	if (!ffmpeg) {
	  std::cerr << "Error starting ffmpeg process!\n";
	}
}

// Clear up
ffmpeg_wrapper::~ffmpeg_wrapper()
{
	// Distinguish between linux and windows
	// and close the file handle accordingly
    #ifdef __linux__
	pclose(ffmpeg);
    #else
	_pclose(ffmpeg);
    #endif
}

// Save a frame, e.g. send it to FFMPEG
void ffmpeg_wrapper::save_frame()
{
    // Create a local buffer of the necessary size
	int * buffer = new int[width * height];

    // Read the frame buffer and write it to the ffmpeg handle
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	fwrite(buffer, sizeof(int)*width*height, 1, ffmpeg);

    // Delete the local buffer and increase the frame counter 
	delete[] buffer;
	++frame_counter;
}

// Retrieve whether the rendering has finished
bool ffmpeg_wrapper::is_finished()
{
    // The rendering has finished when the frame counter
	// is greater than the maximum number of frames
	return frame_counter > frames;
}
