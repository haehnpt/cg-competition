#include "terrain.hpp"

/*
Get heights using "Perlin Noise"
- range : size of the terrain
- rigidity : how "rough" shall the terrain look, e.g. very spiky = 2.0
*/
float * terrain::get_heights(float range, float rigidity)
{
	float * heights = new float[resolution * resolution];
	perlin_noise noise = perlin_noise(resolution, 1.0, 0.0, 1.3);
	perlin_noise noise2 = perlin_noise(resolution * 3, 0.333333, 0.0, 1.3);

	for (int z = 0; z < resolution; z++)
	{
		for (int x = 0; x < resolution; x++)
		{
			float x1 = x * range / resolution;
			float x2 = z * range / resolution;
			float v = 0.6 * noise.get_noise(x1, x2) + noise2.get_noise(x1, x2) * 0.4;
			v = (v < 0.0 ? -1.0 : 1.0) * pow(fabs(v), rigidity);
			heights[(z * resolution) + x] = v;
			if (v < lowest_height) lowest_height = v;
			if (v > highest_height) highest_height = v;
		}
	}

	noise.clear_gradients();
	noise2.clear_gradients();

	return heights;
}

/*
Clamp the calculated heights to an specified range
*/
void terrain::clamp_heights()
{
	for (int i = 0; i < resolution * resolution; i++)
	{
		heights[i] = min_height + (heights[i] - lowest_height) / (highest_height - lowest_height) * (max_height - min_height);
	}
}

/*
Build the terrain instance
- calculate vertices
- calculate normals
- calculate texture coordinates
- set VBO
*/
void terrain::build()
{
	geometry m;
	int nVertices = resolution * resolution;
	int nFaces = (resolution - 1) * (resolution - 1) * 2;
	float deltaX = size / (resolution - 1);
	float deltaZ = size / (resolution - 1);

	m.positions.resize(nVertices);
	m.normals.resize(nVertices);
	m.colors.resize(nVertices, glm::vec4(1.0,1.0,1.0,1.0));
	m.faces.resize(nFaces);
	m.faces_normals.resize(nFaces);

	float* vbo_data = new float[nVertices * 10];
	unsigned int* ibo_data = new unsigned int[nFaces * 3];
	int step = nVertices / 100;
	// Calculate vertices
	for (uint32_t i = 0; i < nVertices; ++i) {
		// Log to console
		if ((i + 1) % step == 0)
		{
			std::cout << (i + 1) / step << "%" << std::endl;
		}

		glm::vec3 pos(-size/2.0 + (i % resolution) * deltaX, heights[i], -size/2.0 + (i / resolution) * deltaZ);
		
		// Calculate final normal after every vertex has reached its height
		glm::vec3 nrm;
		float hl = i == 0 ? heights[i] : heights[i - 1];
		float hr = ((i + 1) % resolution) == 0 ? heights[i] : heights[i + 1];
		float hu = (i + resolution) / resolution < resolution ? heights[i + resolution] : heights[i];
		float hd = i >= resolution ? heights[i - resolution] : heights[i];
		nrm = glm::normalize(glm::vec3(hl - hr, deltaX, hd - hu));
		glm::vec4 col = m.colors[0];

		// Set geometry properties
		m.positions[i] = pos;
		m.normals[i] = nrm;
		m.colors[i] = col;

		// Texture coordinates
		float one = 1.0;
		float scaling = resolution / fmax(size,8.0);
		col[0] = (i % resolution) / (float) resolution;// (i % resolution) / scaling;
		col[1] = (i / resolution) / (float)resolution;//(i / resolution) / scaling;

		// Fill VBO
		vbo_data[10 * i + 0] = pos[0];
		vbo_data[10 * i + 1] = pos[1];
		vbo_data[10 * i + 2] = pos[2];
		vbo_data[10 * i + 3] = nrm[0];
		vbo_data[10 * i + 4] = nrm[1];
		vbo_data[10 * i + 5] = nrm[2];
		vbo_data[10 * i + 6] = col[0];
		vbo_data[10 * i + 7] = col[1];
		vbo_data[10 * i + 8] = col[2];
		vbo_data[10 * i + 9] = col[3];
	}

	// Calculate faces
	for (uint32_t i = 0; i < nFaces; ++i) {
		int pos = i / 2 + i / ((resolution - 1) * 2);
			glm::uvec3 face(pos,
				pos + resolution + (i % 2),
				pos + resolution * ((i+1) % 2) + 1);

			// Save faces
			m.faces[i] = face;
			ibo_data[i * 3 + 0] = face[0];
			ibo_data[i * 3 + 1] = face[1];
			ibo_data[i * 3 + 2] = face[2];

			// Save normals of faces
			glm::vec3 v = m.positions[face[1]] - m.positions[face[0]];
			glm::vec3 w = m.positions[face[2]] - m.positions[face[0]];
			m.faces_normals[i] = glm::normalize(glm::cross(v, w));
	}

	glGenVertexArrays(1, &m.vao);
	glBindVertexArray(m.vao);

	m.vbo = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, nVertices * 10 * sizeof(float), vbo_data);
	m.ibo = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, nFaces * 3 * sizeof(unsigned int), ibo_data);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), (void*)(6 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);

	m.transform = glm::identity<glm::mat4>();
	m.vertex_count = 3 * nFaces;

	terra = m;

	delete[] vbo_data;
	delete[] ibo_data;
}

void terrain::get_frame_locations(int shader_program)
{
	this->frame_loc = glGetUniformLocation(shader_program, "frame");
	this->max_frame_loc = glGetUniformLocation(shader_program, "max_frame");
}

void terrain::set_frames(int start, int max)
{
	this->start_frame = start;
	this->frames = max;
}

void terrain::reset_current_frame()
{
	this->current_frame = this->start_frame;
}

void terrain::increase_current_frame(int increase)
{
	this->current_frame += (this->current_frame < this->frames ? increase : 0);
}

void terrain::render(int model_loc)
{
	glUniform1f(this->frame_loc, this->current_frame);
	glUniform1f(this->max_frame_loc, this->frames);

	glUniform1i(stone_loc, 0);
	glUniform1i(grass_loc, 1);
	glUniform1i(snow_loc, 2);

	glUniformMatrix4fv(model_loc, 1, GL_FALSE, &this->terra.transform[0][0]);
	this->terra.bind();
	glDrawElements(GL_TRIANGLES, this->terra.vertex_count, GL_UNSIGNED_INT, (void*)0);

	increase_current_frame();
}

/*
Get an new terrain instance
- size : size of the terrain
- resolution : resolution of the underlying gradient grid
- start_frame : starting frame
- max_frame : maximum frame
- shader_program :
- stone : stone texture file name
- grass : grass - - -
- snow : snow - - -
*/
terrain::terrain(float size, int resolution, int start_frame, int max_frame, int shader_program, std::string stone, std::string grass, std::string snow)
{
	this->size = size;
	this->resolution = resolution;
	heights = get_heights(size, 1.0);
	clamp_heights();
	build();
	get_texture_locations(shader_program);
	load_textures(stone, grass, snow);
	get_frame_locations(shader_program);
	set_frames(start_frame, max_frame);
}


terrain::~terrain()
{
}

int terrain::stone_loc;
int terrain::grass_loc;
int terrain::snow_loc;
void terrain::get_texture_locations(int shader_program)
{
	stone_loc = glGetUniformLocation(shader_program, "stone_tex");
	grass_loc = glGetUniformLocation(shader_program, "grass_tex");
	snow_loc = glGetUniformLocation(shader_program, "snow_tex");
}
