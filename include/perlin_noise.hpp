#pragma once

/*
 *
 * This perlin noise class was inspired by both
 * - https://en.wikipedia.org/wiki/Perlin_noise
 * - https://www.redblobgames.com/maps/terrain-from-noise/
 * 
 * @author Patrick Hähn
 * 
 */

#include <ctime>
#include <math.h>
#include <cstdlib>
#include <iostream>

class perlin_noise 
{
        float *** gradients;
        int gradients_count;
        float max_distance;
        float offset;
        float scaling;
        float gradient_grid_distance;
        void create_gradients();
        float dotGridGradient(int index_x, int index_y, float x, float y);
        float lerp(float high, float low, float weight);
		float fade(float x);
    public:
        perlin_noise(int gradients_count, float grid_distance, float offset, float scaling);
        ~perlin_noise();
        float get_noise(float x, float y);
};