/**
 * @file tdm.c
 * @author Johannes Regnier
 * @brief 
 * @version 0.1
 * @date 2024-05-06
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "tdm.h"
#include <math.h>
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "tdm.pio.h"

const tdm_config tdm_config_default = {48000, 256, 32, 10, 6, 7, 8};

static float pio_div(float freq, uint16_t* div, uint8_t* frac) {
    float clk   = (float)clock_get_hz(clk_sys);
    float ratio = clk / freq;
    float d;
    float f = modff(ratio, &d);
    *div    = (uint16_t)d;
    *frac   = (uint8_t)(f * 256.0f);

    // Use post-converted values to get actual freq after any rounding
    float result = clk / ((float)*div + ((float)*frac / 256.0f));

    return result;
}

static void calc_clocks(const tdm_config* config, pio_tdm_clocks* clocks) {
    // Try to get a precise ratio between SCK and BCK regardless of how
    // perfect the system_clock divides. First, see what sck we can actually get:
    float sck_desired   = (float)config->fs * (float)config->sck_mult * (float)tdm_sck_program_pio_mult;
    float sck_attained  = pio_div(sck_desired, &clocks->sck_d, &clocks->sck_f);
    clocks->fs_attained = sck_attained / (float)config->sck_mult / (float)tdm_sck_program_pio_mult;

    // Now that we have the closest fs our dividers will give us, we can
    // re-calculate SCK and BCK as correct ratios of this adjusted fs:
    float sck_hz       = clocks->fs_attained * (float)config->sck_mult;
    clocks->sck_pio_hz = pio_div(sck_hz * (float)tdm_sck_program_pio_mult, &clocks->sck_d, &clocks->sck_f);
    float bck_hz       = clocks->fs_attained * (float)config->bit_depth * 8.0f;
    clocks->bck_pio_hz = pio_div(bck_hz * (float)tdm_out_master_program_pio_mult, &clocks->bck_d, &clocks->bck_f);
}

static bool validate_sck_bck_sync(pio_tdm_clocks* clocks) {
    float ratio      = clocks->sck_pio_hz / clocks->bck_pio_hz;
    float actual_sck = clocks->sck_pio_hz / (float)tdm_sck_program_pio_mult;
    float actual_bck = clocks->bck_pio_hz / (float)tdm_out_master_program_pio_mult;
    printf("Clock speed for SCK: %f (PIO %f Hz with divider %d.%d)\n", actual_sck, clocks->sck_pio_hz, clocks->sck_d, clocks->sck_f);
    printf("Clock speed for BCK: %f (PIO %f Hz with divider %d.%d)\n", actual_bck, clocks->bck_pio_hz, clocks->bck_d, clocks->bck_f);
    printf("Clock Ratio: %f\n", ratio);
    float whole_ratio;
    float fractional_ratio = modff(ratio, &whole_ratio);
    return (fractional_ratio == 0.0f);
}

static void dma_double_buffer_init(pio_tdm* tdm, void (*dma_handler)(void)) {
    // Set up DMA for PIO tdm - two channels, out only
    tdm->dma_ch_out_ctrl = dma_claim_unused_channel(true);
    tdm->dma_ch_out_data = dma_claim_unused_channel(true);

    // Control blocks support double-buffering with interrupts on buffer change
    tdm->out_ctrl_blocks[0] = tdm->output_buffer;
    tdm->out_ctrl_blocks[1] = &tdm->output_buffer[TDM_BUFFER_SIZE];

    // DMA tdm OUT control channel - wrap read address every 8 bytes (2 words)
    // Transfer 1 word at a time, to the out channel read address and trigger.
    dma_channel_config c = dma_channel_get_default_config(tdm->dma_ch_out_ctrl);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_ring(&c, false, 3);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    dma_channel_configure(tdm->dma_ch_out_ctrl,
                          &c, 
                          &dma_hw->ch[tdm->dma_ch_out_data].al3_read_addr_trig,
                          tdm->out_ctrl_blocks,
                          1,
                          false);

    c = dma_channel_get_default_config(tdm->dma_ch_out_data);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_chain_to(&c, tdm->dma_ch_out_ctrl);
    channel_config_set_dreq(&c, pio_get_dreq(tdm->pio, tdm->sm_dout, true));

    dma_channel_configure(tdm->dma_ch_out_data,
                          &c,
                          &tdm->pio->txf[tdm->sm_dout],  // Destination pointer
                          NULL,                          // Source pointer, will be set by ctrl channel
                          TDM_BUFFER_SIZE,               // Number of transfers
                          false                          // Start immediately
                          );




    // output channel triggers the DMA interrupt handler
    dma_channel_set_irq0_enabled(tdm->dma_ch_out_data, true); /// <<<<<<<<------------------- HERE -------------------- <<<<<<<<<
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Enable all the dma channels
    dma_channel_start(tdm->dma_ch_out_ctrl);  // This will trigger-start the out chan
}



/* Initializes an tdm block (of 3 state machines) on the designated PIO.
 * NOTE! This does NOT START the PIO units. You must call tdm_program_start
 *       with the resulting tdm object!
 */
static void tdm_sync_program_init(PIO pio, const tdm_config* config, pio_tdm* tdm) {
    uint offset  = 0;
    tdm->pio     = pio;
    tdm->sm_mask = 0;

    pio_tdm_clocks clocks;
    calc_clocks(config, &clocks);

    // Out block, clocked with BCK
    tdm->sm_dout = pio_claim_unused_sm(pio, true);
    tdm->sm_mask |= (1u << tdm->sm_dout);
    offset = pio_add_program(pio, &tdm_out_master_program);
    tdm_out_master_program_init(pio, tdm->sm_dout, offset, config->bit_depth, config->dout_pin, config->clock_pin_base);
    pio_sm_set_clkdiv_int_frac(pio, tdm->sm_dout, clocks.bck_d, clocks.bck_f);
}



void tdm_program_start_synched(PIO pio, const tdm_config* config, void (*dma_handler)(void), pio_tdm* tdm) {
    if (((uint32_t)tdm & 0x7) != 0) {
        panic("pio_tdm argument must be 8-byte aligned!");
    }
    tdm_sync_program_init(pio, config, tdm);
    dma_double_buffer_init(tdm, dma_handler);
    pio_enable_sm_mask_in_sync(tdm->pio, tdm->sm_mask);
}
