#include "common.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "terrain.hpp"
#include <string>

/*
*
*	GLOBAL SETTINGS
*
*/

#define DEBUG false
#define x64 true

const int WINDOW_WIDTH =  800;
const int WINDOW_HEIGHT = 800;
const float FOV = 45.f;
const float NEAR_VALUE = 0.1f;
const float FAR_VALUE = 100.f;

const float TERRAIN_SIZE = 10.0;
const int FRAMES = 180;
const int RESOLUTION = (DEBUG) ? 1000 : (x64 ? 4000 : 2000);

const std::string GRASS = (DEBUG) ? "grass.jpg" : "grass_large.jpg";
const std::string STONE = (DEBUG) ? "mountain.jpg" : "mountain_large.jpg";
const std::string SNOW = (DEBUG) ? "snow.jpg" : "snow_large.jpg";


#ifndef M_PI
#define M_PI 3.14159265359
#endif

glm::mat4 proj_matrix;

void
resizeCallback(GLFWwindow* window, int width, int height);

int
main(int, char* argv[]) {
    GLFWwindow* window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    // load and compile shaders and link program
    unsigned int vertexShader = compileShader("shading_models.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("shading_models.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glUseProgram(shaderProgram);
    int model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    int view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    int proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
    int light_dir_loc = glGetUniformLocation(shaderProgram, "light_dir");
    int roughness_loc = glGetUniformLocation(shaderProgram, "roughness");
    int ref_index_loc = glGetUniformLocation(shaderProgram, "refractionIndex");
    int diffuse_loc = glGetUniformLocation(shaderProgram, "diffuse");
    int specular_loc = glGetUniformLocation(shaderProgram, "specular");
    int use_oren_nayar_loc = glGetUniformLocation(shaderProgram, "useOrenNayar");
	// Add albedo location
	int albedo_loc = glGetUniformLocation(shaderProgram, "albedo");

    proj_matrix = glm::perspective(FOV, 1.f, NEAR_VALUE, FAR_VALUE);

    glEnable(GL_DEPTH_TEST);

    float light_phi = 0.0f;
    float light_theta = 0.3f;
    float roughness = 0.6f;
    float refraction_index = 1.0f;
	float albedo = 0.5f;
    int use_oren_nayar = 1;
    const char* diffuse_models[] = { "Lambert", "Oren-Nayar" };
    glm::vec4 diffuse_color(0.7f, 0.7f, 0.7f, 1.f);
    glm::vec4 specular_color(1.0f, 1.0f, 1.0f, 1.f);

	// Prepare terrain
	terrain terr = terrain(TERRAIN_SIZE,
		RESOLUTION,
		0,
		FRAMES,
		shaderProgram,
		STONE,
		GRASS,
		SNOW);

    // rendering loop
    while (glfwWindowShouldClose(window) == false) {
        glfwPollEvents();
        glClearColor(0.25f, 0.25f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view_matrix = cam.view_matrix();
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);

        glm::vec3 light_dir(std::cos(light_phi) * std::sin(light_theta),
                            std::cos(light_theta),
                            std::sin(light_phi) * std::sin(light_theta));
        glUniform3f(light_dir_loc, light_dir.x, light_dir.y, light_dir.z);

        glUniform1i(use_oren_nayar_loc, use_oren_nayar);
        glUniform1f(roughness_loc, roughness);
		// Uniform albedo
		glUniform1f(albedo_loc, albedo);
        glUniform1f(ref_index_loc, refraction_index);
        glUniform4f(diffuse_loc, diffuse_color.x, diffuse_color.y, diffuse_color.z, diffuse_color.w);
        glUniform4f(specular_loc, specular_color.x, specular_color.y, specular_color.z, specular_color.w);

		// Render terrain
		terr.render(model_mat_loc);

		// Light motion
		light_phi = (light_phi += 0.01) > 2 * M_PI ? 0.0 : light_phi;

        // render UI
        glfwSwapBuffers(window);
    }

    glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
    // set new width and height as viewport size
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}
