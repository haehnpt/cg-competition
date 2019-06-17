#include "terrain.hpp"

float * terrain::getHeights(float range)
{
	float * heights = new float[resolution * resolution];
	perlin_noise noise = perlin_noise(22, 1.0, 0.0, 1.0);
	perlin_noise noise2 = perlin_noise(32, 0.8, 0.0, 1.0);

	for (int z = 0; z < resolution; z++)
	{
		for (int x = 0; x < resolution; x++)
		{
			float x1 = x * range / resolution;
			float x2 = z * range / resolution;
			heights[(z * resolution) + x] = 0.6 * noise.get_noise(x1,x2) + noise2.get_noise(x1,x2) * 0.4;
		}
	}

	return heights;
}

// done TODO: Höhenberechnung in GPU
// TODO: Texturen
// TODO: Korrekte normalen auf GPU
void terrain::build(int frame)
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
	for (uint32_t i = 0; i < nVertices; ++i) {

		glm::vec3 pos(-size/2 + (i % resolution) * deltaX, heights[i], -size/2 + (i / resolution) * deltaZ);
		
		glm::vec3 nrm;
		if (frame)
		{
			float delta = frame / (float)frames;
			float hl = i-1 < 0 ? 0.0 : heights[i - 1];
			float hr = ((i + 1) % resolution) == 0 ? 0.0 : heights[i + 1];
			float hu = (i + resolution) / resolution < resolution ? heights[i + resolution] : 0.0;
			float hd = i >= resolution ? heights[i - resolution] : 0.0;
			nrm = glm::normalize(glm::vec3(delta * (hl - hr), 2.0, delta * (hd - hu)));
		}
		else
		{
			nrm = glm::vec3(0.0, 1.0, 0.0);
		}
		
		glm::vec4 col = m.colors[0];

		/*
		if (mesh->HasVertexColors(0)) {
			col[0] = mesh->mColors[0][i].r;
			col[1] = mesh->mColors[0][i].g;
			col[2] = mesh->mColors[0][i].b;
			col[3] = mesh->mColors[0][i].a;
		}
		*/

		m.positions[i] = pos;
		m.normals[i] = nrm;
		m.colors[i] = col;

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

	//delete[] heights;
	delete[] vbo_data;
	delete[] ibo_data;
}

void terrain::calculateNormals(int frame)
{
	float delta = frame / (float)this->frames;
	
}

terrain::terrain(float size, int resolution, int frames)
{
	this->frames = frames;
	this->size = size;
	this->resolution = resolution;
	heights = getHeights(size);
	build(0);
}


terrain::~terrain()
{
}
