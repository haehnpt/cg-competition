#include <chrono>
#include <glm/gtx/transform.hpp>
#include "common.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "physics.hpp"

// #define TRACKER

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


    // some friendly terrain for testing
    float heightMap[6][5] // [x][y]
    {
        {-0.1f, -0.1f, -0.0f, -0.0f, -0.1f},
        {-0.0f, -0.2f, -0.4f, -0.3f, -0.0f},
        {-0.2f, -1.6f, -1.8f, -1.2f, -0.2f},
        {-0.1f, -1.8f, -1.7f, -1.1f, -0.0f},
        {-0.1f, -0.2f, -0.2f, -1.3f, -0.2f},
        {-0.1f, -0.0f, -0.1f, -0.2f, -0.1f}
    };

    // move the map down for the camera (not using transformations to
    // keep real vertex posistions for collision detection)
    for (int i = 0; i < sizeof(heightMap) / sizeof(heightMap[0]); i++) {
        for (int j = 0; j < sizeof(heightMap[0]) / sizeof(heightMap[0][0]); j++) {
            heightMap[i][j] -= 4.f;
        }
    }

    int zNumPoints = sizeof(heightMap) / sizeof(heightMap[0]);
    int xNumPoints = sizeof(heightMap[0]) / sizeof(heightMap[0][0]);

    phyPlane plane(-10.f, 10.f, -10.f, 10.f,
                   &heightMap[0][0],
                   xNumPoints,
                   zNumPoints,
                   true);

    phySphere sphere1(glm::vec3(-10.f, 4.f, -10.f),
                      glm::vec3(1.8f, 1.f, 0.2f),
                      0.3f, glm::vec4(1.0f, 0.2f, 0.2f, 1.f), &plane);
    phySphere sphere2(glm::vec3(-5.f, 2.f, -8.f),
                      glm::vec3(4.f, -8.f, -4.f),
                      0.3f, glm::vec4(0.2f, 1.0f, 0.2f, 1.f), &plane);
    phySphere sphere3(glm::vec3(5.f, 6.f, 3.f),
                      glm::vec3(0.3f, 0.5f, 0.2f),
                      0.3f, glm::vec4(0.2f, 0.2f, 1.0f, 1.f), &plane);

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

        sphere2.step(0.03);
        sphere3.step(0.03);
        bool above = sphere1.step(0.03);
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

        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere2.geo.transform[0][0]);
        sphere2.geo.bind();
        glDrawElements(GL_TRIANGLES, sphere1.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);

        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere3.geo.transform[0][0]);
        sphere3.geo.bind();
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
