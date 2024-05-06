/**
 * @file sine_oscillator.c
 * @author Johannes Regnier
 * @brief 
 * @version 0.1
 * @date 2024-05-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include "sine_oscillator.h"
#include "lut_sine.h"


/**
 * @brief Linear interpolation within a given lookup table. Useful for wavetable oscillators.
 * 
 * @param index 
 * @param table_size 
 * @param table 
 * @return float 
 */
float interp_lin_lut(float index, uint16_t table_size, const float *table){
    float diff;
    uint32_t trunc = (uint32_t)index; // truncate the index but don't overwrite
    float frac = index - trunc; // get the fractional part

    while (trunc > table_size)
        trunc = trunc - table_size;

    diff = table[trunc+1] - table[trunc]; // no need to check and wrap, table size is 1025

    // get the interpolated output
    return table[trunc] + (diff * frac);
}

void osc_init(oscillator_t *osc, float amp, float freq)
{
    osc->amp = amp;
    osc->freq = freq;
    osc->phase = 0;
    osc->output = 0;
}


/**
 * @brief a simple sine oscillator, based on a lookup table with linear interpolation
 * 
 * @param osc 
 * @return float 
 */
float osc_Sine(oscillator_t *osc)
{

    osc->phase = wrap(osc->phase, 1);  // keep phase between 0 and 1
    osc->output = osc->amp * interp_lin_lut(LUT_SINE_SIZE * (osc->phase), LUT_SINE_SIZE, lut_sine); // linear-interpolated sinewave
    osc->phase += TS * osc->freq;  // increment phase (phase normalized from 0 to 1)

    return osc->output;
}