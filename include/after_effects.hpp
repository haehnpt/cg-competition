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
  MotionBlur(int width, int height, unsigned int blur_size);
  ~MotionBlur();
  void render();
};

class DepthBlur
{
private:
  unsigned int shader;
  unsigned int texture_loc;
  unsigned int texture_vao;
  unsigned int texture;
  unsigned int depth_loc;
  unsigned int depth;
  unsigned int blur_loc;
  unsigned int focus_loc;
  int width;
  int height;
public:
  DepthBlur(int width, int height, float near, float far, float blur, float focus);
  ~DepthBlur();
  void blur(float blur);
  void focus(float focus);
  void render();
};

class Anaglyph3D
{
private:
  unsigned int shader;
  unsigned int texture_loc;
  unsigned int texture_vao;
  unsigned int texture_left;
  unsigned int texture_right;
  int width;
  int height;
public:
  Anaglyph3D(int width, int height, float near, float far);
  ~Anaglyph3D();
  void render();
};
