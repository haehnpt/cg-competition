#include <chrono>
#include <glm/gtx/transform.hpp>
#include "common.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "physics.hpp"
#include "after_effects.hpp"

#define TRACKER

const int WINDOW_WIDTH =  800;
const int WINDOW_HEIGHT = 800;
const float FOV = 45.f;
const float NEAR = 0.1f;
const float FAR = 100.f;

glm::mat4 proj_matrix;
std::chrono::time_point<std::chrono::system_clock> start_time;

float getTimeDelta();

// called whenever the window gets resized
void
resizeCallback(GLFWwindow* window, int width, int height);















unsigned int *genTextures(int width, int height, int size) {
  unsigned int *textures = (unsigned int *)malloc(sizeof(unsigned int) * size);

  for (int i = 0; i < size; i++) {
    glGenTextures(1, textures + i);

    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // Set the texture quality
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  }

  return textures;
}

unsigned int makeTextureVAO() {
  float vertices[] = {
      -1.0f, -1.0f, 0.f,
       1.0f, -1.0f, 0.f,
       1.0f,  1.0f, 0.f,
      -1.0f,  1.0f, 0.f
  };

  unsigned int indices[] = {
      0, 1, 2, 2, 3, 0
  };

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  unsigned int VBO = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(vertices), vertices);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  unsigned int IBO = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(indices), indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

  return VAO;
}

void motion_blur(unsigned int shader, unsigned int *textures, int size, unsigned int tex_loc, unsigned int texture_vao) {
  glDeleteTextures(1, textures + size - 1);

  for (int i = size - 1; i > 0; i--) {
    textures[i] = textures[i - 1];
  }

  unsigned int *new_tex = genTextures(WINDOW_WIDTH, WINDOW_HEIGHT, 1);
  textures[0] = new_tex[0];
  free(new_tex);

  glReadBuffer(GL_BACK);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, textures[0]);
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glDisable(GL_DEPTH_TEST);

  glUseProgram(shader);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  for (int i = 0; i < size; i++) {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glUniform1i(tex_loc, 0);
    glBindVertexArray(texture_vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*) 0);
  }

  glDisable(GL_BLEND);
}















int
main(int, char* argv[]) {
    GLFWwindow* window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    // load and compile shaders and link program
    unsigned int vertexShader = compileShader("test_physics_mesh_render.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("mesh_render.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glUseProgram(shaderProgram);

    phyPlane plane(-10.f, 10.f, -10.f, 10.f);

    phySphere sphere1(glm::vec3(-10.f, 0.f, -10.f),
                      glm::vec3(1.8f, 1.f, 0.2f),
                      0.3f, glm::vec4(1.0f, 0.2f, 0.2f, 1.f), &plane);
#ifdef TRACKER
    phySphere sphereTracker(glm::vec3(0.f, 0.f, 0.f),
                            glm::vec3(0.f, 0.f, 0.f),
                            0.3f, glm::vec4(0.2f, 0.2f, 0.8f, 1.f), &plane);
#endif

    int model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    int view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    int proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");

    int use_special_color_loc = glGetUniformLocation(shaderProgram, "use_special_color");
    glUniform1i(use_special_color_loc, true);

    int special_color_loc = glGetUniformLocation(shaderProgram, "special_color");
    glm::vec4 color_above = glm::vec4(1.f, 1.f, 0.0f, 1.f);
    glm::vec4 color_below = glm::vec4(0.f, 1.f, 1.0f, 1.f);

    // to indicate the position of the sphere on the plane
    int sphere_pos_loc = glGetUniformLocation(shaderProgram, "sphere_pos");


    // these hold the vertices of the triangle over which the sphere is
    int active_vert_1_loc = glGetUniformLocation(shaderProgram, "active_vert_1");
    int active_vert_2_loc = glGetUniformLocation(shaderProgram, "active_vert_2");
    int active_vert_3_loc = glGetUniformLocation(shaderProgram, "active_vert_3");

    proj_matrix = glm::perspective(FOV, 1.f, NEAR, FAR);
    int light_dir_loc = glGetUniformLocation(shaderProgram, "light_dir");
    glm::vec3 light_dir = glm::normalize(glm::vec3(1.0, 1.0, 1.0));
    glUniform3fv(light_dir_loc, 1, &light_dir[0]);

    glm::vec3 active_vert_1;
    glm::vec3 active_vert_2;
    glm::vec3 active_vert_3;






    // int size = 5;
    //
    // unsigned int motionShader = getShader("motion_blur.vert", "motion_blur.frag");
    // glUseProgram(motionShader);
    // int tex_loc = glGetUniformLocation(motionShader, "tex");
    // int alpha_loc = glGetUniformLocation(motionShader, "alpha");
    // glUniform1f(alpha_loc, 1.f / size);
    //
    // unsigned int *textures = genTextures(WINDOW_WIDTH, WINDOW_HEIGHT, size);
    // unsigned int texture_vao = makeTextureVAO();



    MotionBlur motion_blur_effect = MotionBlur(5, WINDOW_WIDTH, WINDOW_HEIGHT);






    glEnable(GL_DEPTH_TEST);

    // TODO: Use this to calculate delta t instead of using a fixed
    // time per frame?
    start_time = std::chrono::system_clock::now();

    // rendering loop
    while (glfwWindowShouldClose(window) == false) {
        // set background color...
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);

        // and fill screen with it (therefore clearing the window)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // glUseProgram(shaderProgram);

        glm::mat4 view_matrix = cam.view_matrix();

        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);

        bool above = sphere1.step(0.01);
        if (above) {
          glUniform4fv(special_color_loc, 1, &color_above[0]);
        } else {
          glUniform4fv(special_color_loc, 1, &color_below[0]);
        }
        glUniform3fv(sphere_pos_loc, 1, &sphere1.x[0]);

        // if (!above) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

#ifdef TRACKER
        // set the tracker to the same position
        sphereTracker.setPosition(sphere1.x);
        sphereTracker.moveToPlaneHeight();
#endif

        // This is for testing, will be moved inside the sphere code later
        int i = plane.getTriangleAt(sphere1.x);
        active_vert_1 = glm::vec3(plane.vbo_data[i * 30 + 0],
                                  plane.vbo_data[i * 30 + 1],
                                  plane.vbo_data[i * 30 + 2]);
        active_vert_2 = glm::vec3(plane.vbo_data[i * 30 + 10],
                                  plane.vbo_data[i * 30 + 11],
                                  plane.vbo_data[i * 30 + 12]);
        active_vert_3 = glm::vec3(plane.vbo_data[i * 30 + 20],
                                  plane.vbo_data[i * 30 + 21],
                                  plane.vbo_data[i * 30 + 22]);

        glUniform3fv(active_vert_1_loc, 1, &active_vert_1[0]);
        glUniform3fv(active_vert_2_loc, 1, &active_vert_2[0]);
        glUniform3fv(active_vert_3_loc, 1, &active_vert_3[0]);

        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere1.geo.transform[0][0]);
        sphere1.geo.bind();
        glDrawElements(GL_TRIANGLES, sphere1.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);

#ifdef TRACKER
        // tracker on the plane
        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphereTracker.geo.transform[0][0]);
        sphereTracker.geo.bind();
        glDrawElements(GL_TRIANGLES, sphereTracker.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);
#endif

        // reset the model matrix before rendering the plane
        glm::mat4 m = glm::mat4(1.f);
        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &m[0][0]);

        plane.bind();
        // glUniform1i(use_special_color_loc, true);
        glDrawArrays(GL_TRIANGLES, 0, plane.mVertices);
        // if (!above) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);





        // motion_blur(motionShader, textures, size, tex_loc, texture_vao);
        // glEnable(GL_DEPTH_TEST);
        // glUseProgram(shaderProgram);
        motion_blur_effect.render();




        // swap buffers == show rendered content
        glfwSwapBuffers(window);
        // process window events
        glfwPollEvents();
    }


    glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
    // set new width and height as viewport size
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR, FAR);
}

float getTimeDelta() {
    auto now = std::chrono::system_clock::now();
    return static_cast<float>((std::chrono::duration_cast<std::chrono::milliseconds>(now-start_time).count() % 5000) / 5000.f);
}
