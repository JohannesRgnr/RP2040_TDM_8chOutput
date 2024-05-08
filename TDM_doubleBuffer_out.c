/**
 * @file TDM_doubleBuffer_out.c
 * @author Johannes Regnier
 * @brief 
 * @version 0.1
 * @date 2024-05-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "tdm.h"
#include "pico/stdlib.h"
#include "sine_oscillator.h"


static __attribute__((aligned(8))) pio_tdm tdm;


oscillator_t sine_osc;
float sample;
int32_t value;

static void process_audio(int32_t* output, size_t samples) {
    for (size_t i = 0; i < samples; i++) {

        
        sample = osc_Sine(&sine_osc);
        // sample = -1.0f; // testing
        /*************** Convert to 16 bits int *****************/
	    //value = (uint32_t) ((int32_t) ((8388607.0f) * sample));
        value = ((int32_t) (8388607.0f * sample)) << 8; // 24-bit data, left-aligned: sample value is converted from float to integer by multiplying by the maximum 24 bit value of 8388607.0, then shift by 8 bits to get a 32-bits data slot value

        output[i<<3]     = value;
        output[(i<<3)+1] = value;
        output[(i<<3)+2] = value;
        output[(i<<3)+3] = value;
        output[(i<<3)+4] = value;
        output[(i<<3)+5] = value;
        output[(i<<3)+6] = value;
        output[(i<<3)+7] = value;

        // output[i<<3]     = 11111100;
        // output[(i<<3)+1] = 22222200;
        // output[(i<<3)+2] = 33333300;
        // output[(i<<3)+3] = 44444400;
        // output[(i<<3)+4] = 55555500;
        // output[(i<<3)+5] = 66666600;
        // output[(i<<3)+6] = 77777700;
        // output[(i<<3)+7] = 88888800;
    }
}

static void dma_tdm_in_handler(void) {
    /* We're double buffering using chained TCBs. By checking which buffer the
     * DMA is currently reading from, we can identify which buffer it has just
     * finished reading (the completion of which has triggered this interrupt).
     */
    if (*(int32_t**)dma_hw->ch[tdm.dma_ch_out_ctrl].read_addr == tdm.output_buffer) {
        // It is inputting to the second buffer so we can overwrite the first
        process_audio(tdm.output_buffer, AUDIO_BUFFER_SIZE);
    } else {
        // It is currently inputting the first buffer, so we write to the second
        process_audio(&tdm.output_buffer[TDM_BUFFER_SIZE], AUDIO_BUFFER_SIZE);
    }
    dma_hw->ints0 = 1u << tdm.dma_ch_out_data;  // clear the IRQ
}

int main() {
    // Set a 132.000 MHz system clock to more evenly divide the audio frequencies
    set_sys_clock_khz(132000, true);
    stdio_init_all();

    printf("System Clock: %lu\n", clock_get_hz(clk_sys));

    osc_init(&sine_osc, 1.0f, 220);

    tdm_program_start_synched(pio0, &tdm_config_default, dma_tdm_in_handler, &tdm);

    puts("TDM_doubleBuffer_out started");
    
    while (true) {
        //printf("sample %f\n", sample);
        //printf("value %u\n", value);
        //sleep_ms(10);
    }

    return 0;
}
