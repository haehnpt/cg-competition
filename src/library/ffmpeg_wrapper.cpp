#include "ffmpeg_wrapper.hpp"

ffmpeg_wrapper::ffmpeg_wrapper(int width, int height, int frames)
{
	this->width = width;
	this->height = height;
	this->frames = frames;
	this->frame_counter = 0;
#ifdef __linux__
	std::string cmd = "ffmpeg -r 60 -f rawvideo -pix_fmt rgba -s "
	  + std::to_string(width) + "x" + std::to_string(height) + " -i - "
	  + "-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip "
	  + FFMPEG_ROOT + "Terrain_Rising.mp4";
        ffmpeg = popen(cmd.c_str(), "w");
#else
	std::string cmd = std::string("\"") + FFMPEG_ROOT + std::string("\\ffmpeg.exe\" -r 60 -f rawvideo -pix_fmt rgba -s " + std::to_string(width) + "x" + std::to_string(height) + " -i - "
		"-threads 0 -preset fast -y -pix_fmt yuv420p -crf 21 -vf vflip ") + FFMPEG_ROOT + std::string("\\Terrain_Rising.mp4");
	ffmpeg = _popen(cmd.c_str(), "wb");
#endif	// __linux__

	if (!ffmpeg) {
	  std::cerr << "Error starting ffmpeg process!\n";
	}
}

ffmpeg_wrapper::~ffmpeg_wrapper()
{
#ifdef __linux__
	pclose(ffmpeg);
#else
	_pclose(ffmpeg);
#endif	// __linux__
}

void ffmpeg_wrapper::save_frame()
{
	int * buffer = new int[width * height];
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	fwrite(buffer, sizeof(int)*width*height, 1, ffmpeg);
	delete[] buffer;
	++frame_counter;
}

bool ffmpeg_wrapper::is_finished()
{
	return frame_counter > frames;
}
