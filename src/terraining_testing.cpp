#include "common.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "terrain.hpp"
#include "ffmpeg_wrapper.hpp"
#include <string>

// Global settings
//#define DEBUG
#define x64
//#define RENDER_VIDEO
#define DO_FULLSCREEN

// Render size
#define RENDER_WIDTH 1920;
#define RENDER_HEIGHT 1080;
#define RENDER_FRAMES 1200;

// Window size
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

// Visual settings
#define FOV 45.0f
#define NEAR_VALUE 0.1f
#define FAR_VALUE 100.0f
#define BACKGROUND_COLOR 0.2f, 0.2f, 0.2f, 1.0f

// Terrain settings
#define TERRAIN_SIZE 8.0f
#define TERRAIN_FRAMES 360
#if defined(x64) && !defined(DEBUG)
	#define TERRAIN_RESOLUTION 2000
#else
	#define TERRAIN_RESOLUTION 1000
#endif

// Camera settings
#define CAMERA_PHI 0.25f
#define CAMERA_THETA -0.15f
#define CAMERA_DISTANCE 5.0f

// Light settings
#define LIGHT_PHI 0.0f
#define LIGHT_THETA 0.3f

// Texture settings
#if defined(DEBUG)
	#define GRASS "grass.jpg"
	#define STONE "mountain.jpg"
	#define SNOW "snow.jpg"
#else
	#define GRASS "grass_large.jpg"
	#define STONE "mountain_large.jpg"
	#define SNOW "snow_large.jpg"
#endif


#ifndef M_PI
#define M_PI 3.14159265359
#endif

glm::mat4 proj_matrix;

void
resizeCallback(GLFWwindow* window, int width, int height);

int
main(int, char* argv[]) {
	// Create a window
#ifdef DO_FULLSCREEN
	GLFWwindow* window = initOpenGL(0, 0, argv[0]);
#else
	GLFWwindow * window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
#endif
    glfwSetFramebufferSizeCallback(window, resizeCallback);

	// Instantiate camera and modify it
    camera cam(window);
	cam.set_phi(CAMERA_PHI);
	cam.set_theta(CAMERA_THETA);
	cam.set_distance(CAMERA_DISTANCE);

	// Projection matrix
    proj_matrix = glm::perspective(FOV, 1.f, NEAR_VALUE, FAR_VALUE);

	// Enable Depth Test
    glEnable(GL_DEPTH_TEST);

	// Light position
    float light_phi = LIGHT_PHI;
    float light_theta = LIGHT_THETA;

	// Prepare terrain
	terrain terr = terrain(TERRAIN_SIZE,
		TERRAIN_RESOLUTION,
		0,
		TERRAIN_FRAMES,
		STONE,
		GRASS,
		SNOW);

	// "ffmpeg" command and preparation
	#ifdef RENDER_VIDEO
	ffmpeg_wrapper fw(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FRAMES);
	#endif

    // rendering loop
	while (glfwWindowShouldClose(window) == false)
	{
		// Poll and set background color
		glfwPollEvents();
		glClearColor(BACKGROUND_COLOR);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Light direction
		glm::vec3 light_dir(std::cos(light_phi) * std::sin(light_theta),
			std::cos(light_theta),
			std::sin(light_phi) * std::sin(light_theta));

		// Render terrain
		terr.render(&cam, proj_matrix, light_dir);

		// Rotate camera
		cam.rotate();

		// Before swapping, read the pixels and feed them to "ffmpeg"
		#ifdef RENDER_VIDEO
		fw.save_frame();
		#endif

        // render UI
        glfwSwapBuffers(window);

		// Check for stop
		#ifdef RENDER_VIDEO
		if (fw.is_finished())
		{
			break;
		}
		#endif
    }

    glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
    // set new width and height as viewport size
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}
