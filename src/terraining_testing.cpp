#include "common.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "terrain.hpp"
#include "ffmpeg_wrapper.hpp"
#include "physics.hpp"

#include <string>

// Global settings
//#define DEBUG
#define x64
// #define RENDER_VIDEO
// #define DO_FULLSCREEN

// Render size
#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080
#define RENDER_FRAMES 1800

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
	#define TERRAIN_RESOLUTION 1000
#else
#define TERRAIN_RESOLUTION 1000
#endif

// Sphere/Physics settings
#define SPHERE_RADIUS 0.08f
#define X_N_SPHERES 25
#define Z_N_SPHERES 25
#define USE_PHY_PLANE
#define SPHERES_APPEARANCE_FRAME 0
#define SPHERES_RELASE_FRAME 100


// Camera settings
#define CAMERA_PHI 0.25f
#define CAMERA_THETA -0.15f
#define CAMERA_DISTANCE 10.0f

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

	///////////////////////// Physics /////////////////////////
	phy::initShader();

	// Prepare terrain
	terrain terr = terrain(TERRAIN_SIZE,
						   TERRAIN_RESOLUTION,
						   0,
						   TERRAIN_FRAMES,
						   STONE,
						   GRASS,
						   SNOW);

	// Prepare physics plane
	phy::phyPlane phyplane(-TERRAIN_SIZE / 2.f,
						   TERRAIN_SIZE / 2.f,
						   -TERRAIN_SIZE / 2.f,
						   TERRAIN_SIZE / 2.f,
						   terr.heights,
						   TERRAIN_RESOLUTION,
						   TERRAIN_RESOLUTION,
						   false,
						   glm::vec4(1.f, 0.f, 1.f, 1.f));

	// Prepare spheres
	phy::phySphere * spheres[X_N_SPHERES * Z_N_SPHERES];

	float dx = (phyplane.xEnd - phyplane.xStart) / X_N_SPHERES;
	float dz = (phyplane.zEnd - phyplane.zStart) / Z_N_SPHERES;

	for (int x = 0; x < X_N_SPHERES; x++ ) {
		for (int z = 0; z < Z_N_SPHERES; z++ ) {
			float col = (float)x * (float)z / X_N_SPHERES / X_N_SPHERES;

			spheres[x * Z_N_SPHERES + z]
				= new phy::phySphere(glm::vec3(phyplane.xStart + x * dx,
											   1.f,
											   phyplane.zStart + z * dz),
									 glm::vec3(0.f, 0.f, 0.f),
									 SPHERE_RADIUS,
									 &phyplane,
									 glm::vec4(col, 1.f - col, 1.0f, 1.f));
		}
	}

	// "ffmpeg" command and preparation
#ifdef RENDER_VIDEO
	ffmpeg_wrapper fw(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FRAMES);
#endif

	int frame = 0;
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
#ifdef USE_PHY_PLANE
			phy::useShader(&cam, proj_matrix, light_dir);
			phyplane.render();
#else
			terr.render(&cam, proj_matrix, light_dir);
#endif
			// Render spheres
			if (frame >= SPHERES_APPEARANCE_FRAME) {
				if (frame >= SPHERES_RELASE_FRAME) {
					for (int i = 0; i < X_N_SPHERES * Z_N_SPHERES; i++) {
						spheres[i]->step(0.015);
					}
				}
				// render all spheres
				phy::useShader(&cam, proj_matrix, light_dir);
				for (int i = 0; i < X_N_SPHERES * Z_N_SPHERES; i++) {
					spheres[i]->render();
				}
			}

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
			frame++;
		}

	glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
	// set new width and height as viewport size
	glViewport(0, 0, width, height);
	proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}

// Local Variables:
// indent-tabs-mode: t
// tab-width: 4
// c-file-style: "cc-mode"
// End:
