#include "after_effects.hpp"

MotionBlur::MotionBlur(int width, int height, unsigned int blur_size)
{
  this->width  = width;
  this->height = height;
  this->blur_size = blur_size;

  this->shader = getShader("after_effects/tex2d.vert", "after_effects/motion_blur.frag");

  int current_shader;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_shader);
  {
    glUseProgram(this->shader);
    this->texture_loc = glGetUniformLocation(this->shader, "tex");
    this->alpha_loc   = glGetUniformLocation(this->shader, "alpha");
    glUniform1f(this->alpha_loc, 1.f / blur_size);

    this->textures = (unsigned int *) malloc(sizeof(unsigned int) * blur_size);
    for (int i = 0; i < blur_size; i++)
    {
      this->textures[i] = genTexture(width, height);
    }
    this->texture_vao = genTextureVAO();
  }
  glUseProgram(current_shader);
}

MotionBlur::~MotionBlur()
{
}

void MotionBlur::render()
{
  glDeleteTextures(1, this->textures + this->blur_size - 1);

  for (int i = this->blur_size - 1; i > 0; i--) {
    this->textures[i] = this->textures[i - 1];
  }

  this->textures[0] = genTexture(this->width, this->height);

  int current_texture;
  float current_clear_color[4];
  int current_shader;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &current_texture);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, &(current_clear_color[0]));
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_shader);
  {

    glReadBuffer(GL_BACK);//needs reset
    glActiveTexture(GL_TEXTURE0);//needs reset
    glBindTexture(GL_TEXTURE_2D, this->textures[0]);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, this->width, this->height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);//needs reset

    glUseProgram(this->shader);

    glEnable(GL_BLEND);//needs reset
    glBlendFunc(GL_ONE, GL_ONE);//needs reset

    for (int i = 0; i < this->blur_size; i++) {
      glBindTexture(GL_TEXTURE_2D, this->textures[i]);
      glUniform1i(this->texture_loc, 0);
      glBindVertexArray(this->texture_vao);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*) 0);
    }

    glDisable(GL_BLEND);//todo reset
    glEnable(GL_DEPTH_TEST);//todo reset
  }
  glUseProgram(current_shader);
  glClearColor(current_clear_color[0], current_clear_color[1], current_clear_color[2], current_clear_color[3]);
  glBindTexture(GL_TEXTURE_2D, current_texture);
}


DepthBlur::DepthBlur(int width, int height, float near, float far)
{
  this->width  = width;
  this->height = height;

  this->shader = getShader("after_effects/tex2d.vert", "after_effects/depth_blur.frag");

  int current_shader;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_shader);
  {
    glUseProgram(this->shader);
    this->texture_loc = glGetUniformLocation(this->shader, "tex");
    this->depth_loc   = glGetUniformLocation(this->shader, "depth");
    glUniform1f(glGetUniformLocation(this->shader, "near"), near);
    glUniform1f(glGetUniformLocation(this->shader, "far"), far);

    this->texture = genTexture(width, height);
    this->depth   = genTexture(width, height);
    this->texture_vao = genTextureVAO();
  }
  glUseProgram(current_shader);
}

DepthBlur::~DepthBlur()
{
}

void DepthBlur::render()
{
  int current_texture;
  int current_active_texture;
  float current_clear_color[4];
  int current_shader;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &current_texture);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &current_active_texture);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, &(current_clear_color[0]));
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_shader);
  {
    glBindTexture(GL_TEXTURE_2D, this->depth);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 0, 0, this->width, this->height, 0);

    glBindTexture(GL_TEXTURE_2D, this->texture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, this->width, this->height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(this->shader);
    glUniform1i(this->depth_loc, 0);
    glUniform1i(this->texture_loc, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->depth);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    glBindVertexArray(this->texture_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*) 0);

    glEnable(GL_DEPTH_TEST);
  }
  glUseProgram(current_shader);
  glClearColor(current_clear_color[0], current_clear_color[1], current_clear_color[2], current_clear_color[3]);
  glActiveTexture(current_active_texture);
  glBindTexture(GL_TEXTURE_2D, current_texture);
}


unsigned int genTexture(int width, int height)
{
  unsigned int texture;

  int current_texture;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &current_texture);
  {
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // Set the texture quality
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  }
  glBindTexture(GL_TEXTURE_2D, current_texture);

  return texture;
}

unsigned int genTextureVAO()
{
  float vertices[] = {
      -1.0f, -1.0f, 0.f,
       1.0f, -1.0f, 0.f,
       1.0f,  1.0f, 0.f,
      -1.0f,  1.0f, 0.f
  };

  unsigned int indices[] = {
      0, 1, 2, 2, 3, 0
  };

  int current_vao;
  int current_vbo;
  int current_ibo;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_vbo);
  glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &current_ibo);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  unsigned int VBO = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  unsigned int IBO = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(indices), indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

  glBindVertexArray(current_vao);
  glBindBuffer(GL_ARRAY_BUFFER, current_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_ibo);

  return VAO;
}
