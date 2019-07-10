#pragma once

#include <ctime>
#include <math.h>
#include <cstdlib>
#include <iostream>

/*

This class encapsulates the noise generation using perlin
noise

@author Patrick Hähn

 */
class perlin_noise 
{
    // The gradients required for perlin noise
    float *** gradients;
    // The number of gradients in each dimension
    int gradients_count;
    // The maximum distance from (0,0)
    float max_distance;
    // The vertical offset of the height map
    float offset;
    // The scaling factor of the generated heights
    float scaling;
    // The distance between each gradient on the grid
    float gradient_grid_distance;

    // Create pseudo-random gradients
    void create_gradients();
    // Get the dot product of the gradient and the grid position
    float dot_grid_gradient(int index_x, int index_y, float x, float y);
    // Perform linear interpolation
    float lerp(float high, float low, float weight);
    // Fade the weight values
	float fade(float x);

public:
    // Create a new instance of perlin_noise
    perlin_noise(int gradients_count, float grid_distance, float offset, float scaling);
    // Clear up
    ~perlin_noise();

    // Get noise at a position (x,y)
    float get_noise(float x, float y);
    // Clear the memory occupied by the gradients
	void clear_gradients();
};