#include "common.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "terrain.hpp"
#include "ffmpeg_wrapper.hpp"
#include "physics.hpp"
#include "after_effects.hpp"

#include <string>

// Global settings
#define DEBUG
#define x64
//#define RENDER_VIDEO
#define DO_FULLSCREEN

// Render size
#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080
#define RENDER_FRAMES 1920
#define RENDER_FILENAME "vorschau.mp4"

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
#define TERRAIN_RESOLUTION 100
#endif

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

// Physics settings
#define SECONDS_PER_FRAME (1.f / 60.f)
#define SPHERE_RADIUS 0.04f
#define X_N_SPHERES 50
#define Z_N_SPHERES 50
// #define RENDER_PHY_PLANE
#define SPHERES_DROP_HEIGHT 1.f
#define SPHERES_APPEARANCE_FRAME 400
#define SPHERES_RELASE_FRAME 460

// Tilting and Dropping
#define ENABLE_PLANE_TILT_AND_DROP

// First Round of Tilts
#define PLANE_TILT_ANGULAR_VELOCITY 0.4f, 0.f, 0.f
#define PLANE_TILT_START_FRAME 620
#define PLANE_TILT_INTERVAL 80
#define PLANE_TILT_END_FRAME (620 + 4 * 80)

// Second Round of Tilts
#define PLANE_TILT_VERTICALLY_START_FRAME 1300
#define PLANE_TILT_VERTICALLY_INTERVAL 100
#define PLANE_TILT_VERTICALLY_PAUSE_UNTIL 1500
#define PLANE_TILT_VERTICALLY_END_FRAME 1600

// Drop
#define PLANE_DROP_START_FRAME 1700
#define PLANE_DROP_INITIAL_VELOCITY 0.f, -0.001f, 0.f
#define PLANE_DROP_FACTOR 1.05f



// Whether to render with effects
#define ENABLE_EFFECTS

// Miscellaneous
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
    #endif // DO_FULLSCREEN
	glfwSetFramebufferSizeCallback(window, resizeCallback);

	// Instantiate camera and modify it
	camera cam(window);
	cam.set_phi(CAMERA_PHI);
	cam.set_theta(CAMERA_THETA);
	cam.set_distance(CAMERA_DISTANCE);

    #ifdef ENABLE_EFFECTS
	// Instantiate after effects
	DepthBlur depth_blur = DepthBlur(WINDOW_WIDTH, WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE, 0.01, 0.2);
	MotionBlur motion_blur = MotionBlur(WINDOW_WIDTH, WINDOW_HEIGHT, 3);
    #endif // ENABLE_EFFECTS

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

	glm::vec3 ang_vel(PLANE_TILT_ANGULAR_VELOCITY);
	glm::vec3 vertical_velocity(PLANE_DROP_INITIAL_VELOCITY);

	// Prepare physics plane
	phy::phyPlane phyplane(-TERRAIN_SIZE / 2.f,
						   TERRAIN_SIZE / 2.f,
						   -TERRAIN_SIZE / 2.f,
						   TERRAIN_SIZE / 2.f,
						   terr.heights,
						   TERRAIN_RESOLUTION,
						   TERRAIN_RESOLUTION,
						   false,
						   nullptr,
						   nullptr,
						   glm::vec4(0.9f, 0.9f, 0.9f, 1.f));

	glm::mat4 plane_model_mat(1.f);
	float plane_angle = 0.f;

	// Test affine transformation for the plane here!
	phyplane.set_model_mat(glm::rotate(plane_model_mat, plane_angle, glm::vec3(1.f, 0.f, 0.f)));
	terr.set_model_mat(phyplane.get_model_mat());


	// Prepare spheres
	phy::phySphere * spheres[X_N_SPHERES * Z_N_SPHERES];

	float dx = (phyplane.xEnd - phyplane.xStart) / X_N_SPHERES;
	float dz = (phyplane.zEnd - phyplane.zStart) / Z_N_SPHERES;

	for (int x = 0; x < X_N_SPHERES; x++ ) {
		for (int z = 0; z < Z_N_SPHERES; z++ ) {
			//float col = (float)x * (float)z / X_N_SPHERES / X_N_SPHERES;

			spheres[x * Z_N_SPHERES + z]
				= new phy::phySphere(glm::vec4(phyplane.xStart + x * dx,
											   SPHERES_DROP_HEIGHT,
											   phyplane.zStart + z * dz,
											   1.f),
									 glm::vec4(0.f, 0.f, 0.f, 0.f),
									 SPHERE_RADIUS,
									 &phyplane,
									 glm::vec4(sin(x * M_PI / X_N_SPHERES), cos(z * M_PI / X_N_SPHERES) / 2.f + 0.5f, exp(x * z / X_N_SPHERES / Z_N_SPHERES) / 2.718282f, 1.f),
									 SPHERES_APPEARANCE_FRAME + (phy::gauss_rand(60, 60) - 60));
		}
	}

	// "ffmpeg" command and preparation
    #ifdef RENDER_VIDEO
	ffmpeg_wrapper fw(RENDER_WIDTH, RENDER_HEIGHT, RENDER_FRAMES, RENDER_FILENAME);
    #endif // RENDER_VIDEO

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

        #ifdef ENABLE_PLANE_TILT_AND_DROP

		// First Round of Tilting
		if (frame == PLANE_TILT_START_FRAME) {
			phyplane.set_angular_velocity(&ang_vel);
		} else if (frame == PLANE_TILT_END_FRAME) {
			phyplane.set_angular_velocity(nullptr);
		} else if ((frame - PLANE_TILT_START_FRAME) % PLANE_TILT_INTERVAL == 0 && frame < PLANE_TILT_END_FRAME) {
			// Switch tilt direction
			ang_vel *= -1;
		}
		
		// Second Round of Tilting
		if (frame == PLANE_TILT_VERTICALLY_START_FRAME) {
			ang_vel *= -2;
			phyplane.set_angular_velocity(&ang_vel);
		}
		else if (frame == PLANE_TILT_VERTICALLY_END_FRAME) {
			phyplane.set_angular_velocity(nullptr);
		}
		else if (frame == PLANE_TILT_VERTICALLY_PAUSE_UNTIL) {
			// Switch tilt direction
			phyplane.set_angular_velocity(&ang_vel);
		}
		else if ((frame - PLANE_TILT_VERTICALLY_START_FRAME) % PLANE_TILT_VERTICALLY_INTERVAL == 0 && 
			frame > PLANE_TILT_VERTICALLY_START_FRAME && frame < PLANE_TILT_VERTICALLY_END_FRAME) {
			// Switch tilt direction
			phyplane.set_angular_velocity(nullptr);
			ang_vel *= -1;
		}

		// Dropping
		if (frame == PLANE_DROP_START_FRAME) {
			phyplane.set_vertical_velocity(&vertical_velocity);
		}
		else if (frame > PLANE_DROP_START_FRAME) {
			vertical_velocity *= PLANE_DROP_FACTOR;
		}

		// Apply plane transformations
		phyplane.step(SECONDS_PER_FRAME);
		// Copy transformations to the terrain
		terr.set_model_mat(phyplane.get_model_mat());

        #endif // ENABLE_PLANE_TILT_AND_DROP

		// Render terrain
        #ifdef RENDER_PHY_PLANE
		phy::useShader(&cam, proj_matrix, light_dir);
		phyplane.render();
        #else
		terr.render(&cam, proj_matrix, light_dir);
        #endif // RENDER_PHY_PLANE

		phy::useShader(&cam, proj_matrix, light_dir);
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
				if (frame > spheres[i]->visibility_frame) {
					spheres[i]->render();
				}
			}
		}

        #ifdef ENABLE_EFFECTS
		depth_blur.render();
		motion_blur.render();
        #endif // ENABLE_EFFECTS

		// Rotate camera
		cam.rotate();

		// Before swapping, read the pixels and feed them to "ffmpeg"
        #ifdef RENDER_VIDEO
		fw.save_frame();
        #endif // RENDER_VIDEO

		// render UI
		glfwSwapBuffers(window);

		// Check for stop
        #ifdef RENDER_VIDEO
		if (fw.is_finished())
			{
				break;
			}
        #endif // RENDER_VIDEO
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
