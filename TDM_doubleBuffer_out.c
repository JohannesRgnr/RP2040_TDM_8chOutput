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


oscillator_t sine_osc1, sine_osc2, sine_osc3, sine_osc4;



static void process_audio(int32_t* output, size_t samples) {
    for (size_t i = 0; i < samples; i++) {

        float sample1, sample2, sample3, sample4;
       
        /*************** Generate sine waves *****************/
        sample1 = osc_Sine(&sine_osc1);
        sample2 = osc_Sine(&sine_osc2);
        sample3 = osc_Sine(&sine_osc3);
        sample4 = osc_Sine(&sine_osc4);
        
        /*************** Output to 8 channels *****************/
        output[i<<3]     = float_to_24bits(sample1);
        output[(i<<3)+1] = float_to_24bits(sample2);
        output[(i<<3)+2] = float_to_24bits(sample3);
        output[(i<<3)+3] = float_to_24bits(sample4);
        output[(i<<3)+4] = float_to_24bits(sample1);
        output[(i<<3)+5] = float_to_24bits(sample2);
        output[(i<<3)+6] = float_to_24bits(sample3);
        output[(i<<3)+7] = float_to_24bits(sample4);

        // values for testing
        // output[i<<3]     = -8388607 << 8;
        // output[(i<<3)+1] = 222222 << 8;
        // output[(i<<3)+2] = 333333 << 8;
        // output[(i<<3)+3] = 444444 << 8;
        // output[(i<<3)+4] = 555555 << 8;
        // output[(i<<3)+5] = 666666 << 8;
        // output[(i<<3)+6] = 777777 << 8;
        // output[(i<<3)+7] = 888888 << 8;
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

    // initialize oscillators
    osc_init(&sine_osc1, 1.0f, 220);
    osc_init(&sine_osc2, 0.5f, 440);
    osc_init(&sine_osc3, 1.0f, 660);
    osc_init(&sine_osc4, 0.5f, 880);

    tdm_program_start_synched(pio0, &tdm_config_default, dma_tdm_in_handler, &tdm);

    while (true) {
        //printf("sample %f\n", sample);
        //printf("value %u\n", value);
        //sleep_ms(10);
    }

    return 0;
}
