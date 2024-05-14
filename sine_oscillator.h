/**
 * @file sine_oscillator.h
 * @author Johannes Regnier
 * @brief 
 * @version 0.1
 * @date 2024-05-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#ifndef SINE_OSCILLATOR_H
#define SINE_OSCILLATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "tdm.h"



#define TS		            (1.f/FS)        // sampling period

typedef struct
{
    float amp;
    float freq;
    float phase;
    float output;
} oscillator_t;


inline float wrap(float value, float max)   
{
    if (value < 0.f)
        value += max;
    if (value >= max)
        value -= max;

    return value;
}

float   interp_lin_lut(float index, uint16_t table_size, const float *table);
void    osc_init(oscillator_t * osc, float amp, float freq);
float   osc_Sine(oscillator_t * osc);



#endif // !SINE_OSCILLATOR_H