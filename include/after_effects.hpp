#pragma once

#include "common.hpp"
#include "shader.hpp"
#include "buffer.hpp"

unsigned int genTexture(int width, int height);
unsigned int genTextureVAO();

class MotionBlur
{
private:
  unsigned int blur_size;
  unsigned int shader;
  unsigned int alpha_loc;
  unsigned int texture_loc;
  unsigned int texture_vao;
  unsigned int*textures;
  int width;
  int height;
public:
  MotionBlur(unsigned int blur_size, int width, int height);
  ~MotionBlur();
  void render();
};
