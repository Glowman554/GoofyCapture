#include <stdint.h>
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "pico/time.h"
#include <readline.h>
#include <string.h>
#include <stdlib.h>

uint32_t* sample_buffer = NULL;
int sample_buffer_size = 0;
int64_t samples_per_s = 0;
uint32_t trigger_pin = 0;
uint32_t trigger_polarity = 0;

void sampler_init(PIO pio, uint sm, float div) {
    uint16_t capture_prog_instr = pio_encode_in(pio_pins, 32);
    struct pio_program capture_prog = {
            .instructions = &capture_prog_instr,
            .length = 1,
            .origin = -1
    };
    uint offset = pio_add_program(pio, &capture_prog);

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, 0);
    sm_config_set_wrap(&c, offset, offset);
    sm_config_set_clkdiv(&c, div);

    sm_config_set_in_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, sm, offset, &c);
}

void sampler_run(PIO pio, uint sm, uint dma_chan, uint32_t* capture_buf, int capture_size, uint trigger_pin, bool trigger_level) {
    pio_sm_set_enabled(pio, sm, false);

    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

    dma_channel_configure(dma_chan, &c,
        capture_buf,
        &pio->rxf[sm],
        capture_size,
        true
    );

    pio_sm_exec(pio, sm, pio_encode_wait_gpio(trigger_level, trigger_pin));
    pio_sm_set_enabled(pio, sm, true);
}

int main() {
    stdio_init_all();

    for (int i = 0; i < 29; i++) {
        gpio_init(i);
    	gpio_set_dir(i, GPIO_IN);
        gpio_set_pulls(i, false, true);
        printf("[GPIO] configured %d\n", i);
    }

    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;
    printf("[DMA] enabled high priority\n");

	while (true) {
		char* buf = readLine();

		if (strcmp(buf, "PING") == 0) {
		} else if(strncmp(buf, "PREPARE ", 8) == 0) {
            sscanf(buf, "%s %ld %d", NULL, &samples_per_s, &sample_buffer_size);
            if (sample_buffer) {
                free(sample_buffer);
            }
            sample_buffer = malloc(sample_buffer_size * sizeof(uint32_t));
            if (sample_buffer == NULL) {
                goto error;
            }
		} else if(strncmp(buf, "RUN ", 4) == 0) {
            sscanf(buf, "%s %d %d", NULL, &trigger_pin, &trigger_polarity);

            float div = (float) clock_get_hz(clk_sys) / samples_per_s;

            sampler_init(pio0, 0, div);
            sampler_run(pio0, 0, 0, sample_buffer, sample_buffer_size, trigger_pin, trigger_polarity);
            dma_channel_wait_for_finish_blocking(0);
            pio_sm_set_enabled(pio0, 0, false);
		} else if(strcmp(buf, "DUMP") == 0) {
            printf("START\n");
            for (int i = 0; i < sample_buffer_size; i++) {
                printf("%d\n", sample_buffer[i]);
            }
            printf("END\n");
		} else {
			goto error;
		}

		printf("OK\n");
		continue;

	error:
		printf("ERROR\n");
	}

	while (true) {
        // printf("Hello world!\n");
    }
}
