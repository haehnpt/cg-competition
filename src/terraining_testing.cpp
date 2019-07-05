#include "common.hpp"
#include "shader.hpp"
#include "mesh.hpp"
#include "camera.hpp"
#include "terrain.hpp"
#include "ffmpeg_wrapper.hpp"
#include "physics.hpp"

#include <string>

// Global settings
#define DEBUG
#define x64
#define RENDER_VIDEO
#define DO_FULLSCREEN

// Render size
#define RENDER_WIDTH 1920
#define RENDER_HEIGHT 1080
#define RENDER_FRAMES 1000

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

// Sphere/Physics settings
#define X_N_BALLS 25
#define Z_N_BALLS 25
// #define USE_PHY_PLANE
#define BALLS_APPEARANCE_FRAME 300
#define BALLS_RELASE_FRAME 400


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

	///////////////////////// Physics /////////////////////////
	// TODO: Move this code inside the physics class
	unsigned int vertexShader = compileShader("shading_models.vert", GL_VERTEX_SHADER);
	unsigned int fragmentShader = compileShader("shading_models.frag", GL_FRAGMENT_SHADER);
	int phyShaderProgram = linkProgram(vertexShader, fragmentShader);
	// after linking the program the shader objects are no longer needed
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glUseProgram(phyShaderProgram);
	int light_dir_loc = glGetUniformLocation(phyShaderProgram, "light_dir");
	int model_mat_loc = glGetUniformLocation(phyShaderProgram, "model_mat");
	int proj_mat_loc = glGetUniformLocation(phyShaderProgram, "proj_mat");
	int view_mat_loc = glGetUniformLocation(phyShaderProgram, "view_mat");
	int roughness_loc = glGetUniformLocation(phyShaderProgram, "roughness");
	int ref_index_loc = glGetUniformLocation(phyShaderProgram, "refractionIndex");
	int diffuse_loc = glGetUniformLocation(phyShaderProgram, "diffuse");
	int specular_loc = glGetUniformLocation(phyShaderProgram, "specular");
	int use_oren_nayar_loc = glGetUniformLocation(phyShaderProgram, "useOrenNayar");
	// Add albedo location
	int albedo_loc = glGetUniformLocation(phyShaderProgram, "albedo");

	// Shader variables
	float roughness = 0.4f;
	float refraction_index = 0.4f;
	// Add variable for albedo
	float albedo = 1.0f;
	int use_oren_nayar = 1;
	glm::vec4 diffuse_color(0.7f, 0.7f, 0.7f, 1.f);
	glm::vec4 specular_color(1.0f, 1.0f, 1.0f, 1.f);

	// Prepare terrain
	terrain terr = terrain(TERRAIN_SIZE,
		TERRAIN_RESOLUTION,
		0,
		TERRAIN_FRAMES,
		STONE,
		GRASS,
		SNOW);

	// Prepare physics plane
	phyPlane phyplane(-TERRAIN_SIZE / 2.f,
					  TERRAIN_SIZE / 2.f,
					  -TERRAIN_SIZE / 2.f,
					  TERRAIN_SIZE / 2.f,
					  terr.heights,
					  TERRAIN_RESOLUTION,
					  TERRAIN_RESOLUTION,
					  false);
	// Prepare spheres
	phySphere * spheres[X_N_BALLS * Z_N_BALLS];

	float dx = (phyplane.xEnd - phyplane.xStart) / X_N_BALLS;
	float dz = (phyplane.zEnd - phyplane.zStart) / Z_N_BALLS;

	for (int x = 0; x < X_N_BALLS; x++ ) {
		for (int z = 0; z < Z_N_BALLS; z++ ) {
			float col = (float)x * (float)z / X_N_BALLS / X_N_BALLS;

			spheres[x * Z_N_BALLS + z]
			    = new phySphere(glm::vec3(phyplane.xStart + x * dx,
						      1.f,
						      phyplane.zStart + z * dz),
					    glm::vec3(0.f, 0.f, 0.f),
					    0.08f,
					    glm::vec4(col, 1.f - col, 1.0f, 1.f),
					    &phyplane,
					    model_mat_loc);
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
#ifndef USE_PHY_PLANE
		terr.render(&cam, proj_matrix, light_dir);
#else
		glUseProgram(phyShaderProgram);
		// reset the model matrix before rendering the plane
		glm::mat4 m = glm::mat4(1.f);
		glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &m[0][0]);
		phyplane.bind();
		glUniform1i(use_oren_nayar_loc, use_oren_nayar);
		glUniform1f(roughness_loc, roughness);
		// Uniform albedo
		glUniform1f(albedo_loc, albedo);
		glUniform1f(ref_index_loc, refraction_index);
		glUniform4f(diffuse_loc, diffuse_color.x, diffuse_color.y, diffuse_color.z, diffuse_color.w);
		glUniform4f(specular_loc, specular_color.x, specular_color.y, specular_color.z, specular_color.w)
		glDrawArrays(GL_TRIANGLES, 0, phyplane.mVertices);
#endif // USE_PHY_PLANE
		glUseProgram(phyShaderProgram);
		glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);
		glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &cam.view_matrix()[0][0]);
		glUniform3f(light_dir_loc, light_dir.x, light_dir.y, light_dir.z);
		glUniform1i(use_oren_nayar_loc, use_oren_nayar);
		glUniform1f(roughness_loc, roughness);
		// Uniform albedo
		glUniform1f(albedo_loc, albedo);
		glUniform1f(ref_index_loc, refraction_index);
		glUniform4f(diffuse_loc, diffuse_color.x, diffuse_color.y, diffuse_color.z, diffuse_color.w);
		glUniform4f(specular_loc, specular_color.x, specular_color.y, specular_color.z, specular_color.w);
		if (frame >= BALLS_APPEARANCE_FRAME) {
			if (frame >= BALLS_RELASE_FRAME) {
				for (int i = 0; i < X_N_BALLS * Z_N_BALLS; i++) {
					spheres[i]->step(0.015);
				}
			}

			for (int i = 0; i < X_N_BALLS * Z_N_BALLS; i++) {
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
