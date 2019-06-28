#include "perlin_noise.hpp"

/*
Get a new instance of perlin noise with the specified parameters
- gradients_count : number of gradients on the grid in one dimension
- grid_distance : distance between nodes on the grid
- offset : height offset
- scaling : scaling of heights
*/
perlin_noise::perlin_noise(int gradients_count, float grid_distance, float offset, float scaling)
{
    this->gradients_count = gradients_count;
    this->gradient_grid_distance = grid_distance;
    this->offset = offset;
    this->scaling = scaling;
    create_gradients();
}

perlin_noise::~perlin_noise()
{
    delete[] gradients;
}

/*
Fade weight values (=> smoother)
*/
float perlin_noise::fade(float x)
{
	return (3 - 2 * x) * x * x;
}

/*
Create pseudo-random gradients which are used in
the process of generating perlin noise
*/
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

/*
Dot product of gradient and position
*/
float perlin_noise::dotGridGradient(int index_x, int index_y, float x, float y)
{
    float dx = x - (-max_distance + index_x * gradient_grid_distance);
    float dy = y - (-max_distance + index_y * gradient_grid_distance);
    return (dx * gradients[index_x][index_y][0] + dy * gradients[index_x][index_y][1]);
}

/*
Linear interpolation
*/
float perlin_noise::lerp(float high, float low, float weight)
{
    return (1.0 - weight) * high + weight * low;
}

/*
Get perlin noise at the location (x,y)
- x : x coordinate
- y : y coordinate
*/
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

    float n1 = dotGridGradient(x_index, y_index, x, y);
    float n2 = dotGridGradient(x_index_1, y_index, x, y);
    float i1 = lerp(n1,n2,sx);

    n1 = dotGridGradient(x_index, y_index_1, x, y);
    n2 = dotGridGradient(x_index_1, y_index_1, x, y);
    float i2 = lerp(n1,n2,sx);

    return offset + scaling * lerp(i1,i2,sy);
}