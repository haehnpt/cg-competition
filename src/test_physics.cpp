#include <chrono>
#include <glm/gtx/transform.hpp>
#include "common.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "mesh.hpp"
#include "physics.hpp"

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
    unsigned int vertexShader = compileShader("mesh_render.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("mesh_render.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    phySphere sphere1 = createPhySphere(-3.f, 5.f, -10.f,
                                        0.f, 0.f, 0.f,
                                        0.2f, glm::vec4(0.f, 0.f, 1.f, 1.f));
    phySphere sphere2 = createPhySphere(0.f, 5.f, -10.f,
                                        0.f, 0.f, 0.f,
                                        0.4f, glm::vec4(1.f, 0.f, 0.f, 1.f));
    phySphere sphere3 = createPhySphere(3.f, 5.f, -10.f,
                                        0.f, 0.f, 0.f,
                                        0.8f, glm::vec4(0.f, 1.f, 0.f, 1.f));

    phyPlane plane = createPhyPlane(1.1f, 1.1f);
    std::cout << "vdo_size: " << plane.mVertices << "\n";

    glUseProgram(shaderProgram);
    int model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    int view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    int proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
    proj_matrix = glm::perspective(FOV, 1.f, NEAR, FAR);
    int light_dir_loc = glGetUniformLocation(shaderProgram, "light_dir");
    glm::vec3 light_dir = glm::normalize(glm::vec3(1.0, 1.0, 1.0));
    glUniform3fv(light_dir_loc, 1, &light_dir[0]);

    glEnable(GL_DEPTH_TEST);

    // TODO: Use this to calculate delta t instead of using a fixed
    // time per frame?
    start_time = std::chrono::system_clock::now();

    testPhysicsLibraryLinking();

    // rendering loop
    while (glfwWindowShouldClose(window) == false) {
        // set background color...
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        // and fill screen with it (therefore clearing the window)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 view_matrix = cam.view_matrix();
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);

        // render sphere
        sphere1.step(0.05);
        sphere2.step(0.05);
        sphere3.step(0.05);

        // glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere1.geo.transform[0][0]);
        // sphere1.geo.bind();
        // glDrawElements(GL_TRIANGLES, sphere1.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);

        // glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere2.geo.transform[0][0]);
        // sphere2.geo.bind();
        // glDrawElements(GL_TRIANGLES, sphere1.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);

        // glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &sphere3.geo.transform[0][0]);
        // sphere3.geo.bind();
        // glDrawElements(GL_TRIANGLES, sphere1.geo.vertex_count, GL_UNSIGNED_INT, (void*) 0);

        // reset the model matrix before rendering the plane
        glm::mat4 m = glm::mat4(1.f);
        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &m[0][0]);

        plane.bind();
        glDrawArrays(GL_TRIANGLES, 0, plane.mVertices);

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
