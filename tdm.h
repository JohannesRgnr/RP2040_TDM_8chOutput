/**
 * @file tdm.h
 * @author Johannes Regnier
 * @brief 
 * @version 0.1
 * @date 2024-05-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stdio.h>
#include "hardware/pio.h"

#ifndef TDM_H
#define TDM_H

#define CHANNELS 8
#define AUDIO_BUFFER_SIZE 16 // in samples
#define TDM_BUFFER_SIZE  AUDIO_BUFFER_SIZE * CHANNELS  // 16 * 8 words = 128 samples latency 

typedef struct tdm_config {
    uint32_t fs;
    uint32_t sck_mult;
    uint8_t  bit_depth;
    uint8_t  sck_pin;
    uint8_t  dout_pin;
    uint8_t  din_pin;
    uint8_t  clock_pin_base;
} tdm_config;

typedef struct pio_tdm_clocks {
    // Clock computation results
    float fs_attained;
    float sck_pio_hz;
    float bck_pio_hz;

    // PIO divider ratios to obtain the computed clocks above
    uint16_t sck_d;
    uint8_t  sck_f;
    uint16_t bck_d;
    uint8_t  bck_f;
} pio_tdm_clocks;

// NOTE: Use __attribute__ ((aligned(8))) on this struct or the DMA wrap won't work!
typedef struct pio_tdm {
    PIO        pio;
    uint8_t    sm_mask;
    uint8_t    sm_sck;
    uint8_t    sm_dout;
    uint8_t    sm_din;
    uint       dma_ch_out_ctrl;
    uint       dma_ch_out_data;
    int32_t*   out_ctrl_blocks[2];
    int32_t    output_buffer[TDM_BUFFER_SIZE * 2];
    tdm_config config;
} pio_tdm;

extern const tdm_config tdm_config_default;

void tdm_program_start_slaved(PIO pio, const tdm_config* config, void (*dma_handler)(void), pio_tdm* tdm);
void tdm_program_start_synched(PIO pio, const tdm_config* config, void (*dma_handler)(void), pio_tdm* tdm);

#endif  // TDM_H
