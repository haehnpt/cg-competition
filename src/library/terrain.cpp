#include "terrain.hpp"

/*
Get heights using "Perlin Noise"
- range : size of the terrain
- rigidity : how "rough" shall the terrain look, e.g. very spiky = 2.0
*/
float * terrain::getHeights(float range, float rigidity)
{
	float * heights = new float[resolution * resolution];
	perlin_noise noise = perlin_noise(100, 1.0, 0.3, 1.3);
	perlin_noise noise2 = perlin_noise(300, 0.333333, 0.3, 1.3);

	for (int z = 0; z < resolution; z++)
	{
		for (int x = 0; x < resolution; x++)
		{
			float x1 = x * range / resolution;
			float x2 = z * range / resolution;
			float v = 0.6 * noise.get_noise(x1, x2) + noise2.get_noise(x1, x2) * 0.4;
			v = glm::max<float>(min_height, (v < 0.0 ? -1.0 : 1.0) * pow(abs(v), rigidity));
			heights[(z * resolution) + x] = v;
			if (v < lowest_height) lowest_height = v;
			if (v > highest_height) highest_height = v;
		}
	}

	return heights;
}

/*
Clamp the calculated heights to an specified range
*/
void terrain::clampHeights()
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

	float* vbo_data = new float[nVertices * 10];
	unsigned int* ibo_data = new unsigned int[nFaces * 3];
	int step = nVertices / 100;
	for (uint32_t i = 0; i < nVertices; ++i) {
		// Log to console
		if ((i + 1) % step == 0)
		{
			std::cout << (i + 1) / step << "%" << std::endl;
		}

		glm::vec3 pos(-size/2 + (i % resolution) * deltaX, heights[i], -size/2 + (i / resolution) * deltaZ);
		
		// Calculate final normal after every vertex has reached its height
		glm::vec3 nrm;
		float hl = i == 0 ? 0.0 : heights[i - 1];
		float hr = ((i + 1) % resolution) == 0 ? 0.0 : heights[i + 1];
		float hu = (i + resolution) / resolution < resolution ? heights[i + resolution] : 0.0;
		float hd = i >= resolution ? heights[i - resolution] : 0.0;
		nrm = glm::normalize(glm::vec3(hl - hr, 2.0, hd - hu));
		
		glm::vec4 col = m.colors[0];

		m.positions[i] = pos;
		m.normals[i] = nrm;
		m.colors[i] = col;

		// Texture coordinates
		float one = 1.0;
		float scaling = resolution / 8.0;
		col[0] = modf(i / scaling, &one);//1.0 / resolution * (i % resolution);
		col[1] = modf(i / resolution / scaling, &one);//1.0 / resolution * (i / resolution);

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

	for (uint32_t i = 0; i < nFaces; ++i) {
		int pos = i / 2 + i / ((resolution - 1) * 2);
			glm::uvec3 face(pos,
				pos + resolution + (i % 2),
				pos + resolution * ((i+1) % 2) + 1);

			m.faces[i] = face;
			ibo_data[i * 3 + 0] = face[0];
			ibo_data[i * 3 + 1] = face[1];
			ibo_data[i * 3 + 2] = face[2];
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

/*
Get an new terrain instance
- size : size of the terrain
- resolution : resolution of the underlying gradient grid
- frames : number of frames in which the hills rise
*/
terrain::terrain(float size, int resolution, int frames)
{
	this->frames = frames;
	this->size = size;
	this->resolution = resolution;
	heights = getHeights(size, 1.0);
	clampHeights();
	build();
}


terrain::~terrain()
{
}
