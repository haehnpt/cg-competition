#include "perlin_noise.hpp"

// Create a new instance of perlin_noise
perlin_noise::perlin_noise(int gradients_count, float grid_distance, float offset, float scaling)
{
    this->gradients_count = gradients_count;
    this->gradient_grid_distance = grid_distance;
    this->offset = offset;
    this->scaling = scaling;
    create_gradients();
}

// Clear up
perlin_noise::~perlin_noise()
{
	
}

// Fade the weight values
float perlin_noise::fade(float x)
{
	return (3 - 2 * x) * x * x;
}

// Create pseudo-random gradients
void perlin_noise::create_gradients()
{
    srand(time(0));

    gradients = new float**[gradients_count];

    for (int x = 0; x < gradients_count; x++)
    {
        gradients[x] = new float*[gradients_count];

        for (int y = 0; y < gradients_count; y++)
        {
            gradients[x][y] = new float[2];
            float a = 2.0 * (rand() - RAND_MAX / 2.0) / RAND_MAX;
            float b = 2.0 * (rand() - RAND_MAX / 2.0) / RAND_MAX;
            float length = sqrt(a * a + b * b);
            gradients[x][y][0] = a / length;
            gradients[x][y][1] = b / length;
        }
    }
}

// Get the dot product of the gradient and the grid position
float perlin_noise::dot_grid_gradient(int index_x, int index_y, float x, float y)
{
	if (index_x >= gradients_count || index_y >= gradients_count)
		return 0.0;
    float dx = x - (-max_distance + index_x * gradient_grid_distance);
    float dy = y - (-max_distance + index_y * gradient_grid_distance);
    return (dx * gradients[index_x][index_y][0] + dy * gradients[index_x][index_y][1]);
}

// Perform linear interpolation
float perlin_noise::lerp(float high, float low, float weight)
{
    return (1.0 - weight) * high + weight * low;
}

// Get noise at a position (x,y)
float perlin_noise::get_noise(float x, float y)
{
    max_distance = (gradients_count - 1) / 2.0 * gradient_grid_distance;
    
    if (abs(x) > max_distance || abs(y) > max_distance)
    {
        return 0.0;
    }

    int x_index = (int)((x + max_distance) / gradient_grid_distance); 
    int y_index = (int)((y + max_distance) / gradient_grid_distance);
    int x_index_1 = x_index + 1;
    int y_index_1 = y_index + 1;

	float sx = fade((x - (-max_distance + x_index * gradient_grid_distance)) / gradient_grid_distance); 
	float sy = fade((y - (-max_distance + y_index * gradient_grid_distance)) / gradient_grid_distance);

    float n1 = dot_grid_gradient(x_index, y_index, x, y);
    float n2 = dot_grid_gradient(x_index_1, y_index, x, y);
    float i1 = lerp(n1,n2,sx);

    n1 = dot_grid_gradient(x_index, y_index_1, x, y);
    n2 = dot_grid_gradient(x_index_1, y_index_1, x, y);
    float i2 = lerp(n1,n2,sx);

    return offset + scaling * lerp(i1,i2,sy);
}

// Clear the memory occupied by the gradients
void perlin_noise::clear_gradients()
{
	for (int i = 0; i < this->gradients_count; i++)
	{
		for (int j = 0; j < this->gradients_count; j++)
		{
			delete[] gradients[i][j];
		}
		delete[] gradients[i];
	}
	delete[] gradients;
	gradients = nullptr;
}