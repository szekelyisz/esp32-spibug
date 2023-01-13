#include <stdio.h>
#include "spi_transmitter.h"
#include "driver/spi_master.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define SPI_BUS HSPI_HOST
#define SPI_BUS_MOSI_IO 4
#define SPI_BUS_CLK_IO 32
#define SPI_BUS_CS_IO 33
#define DMA_CHANNEL 2
#define TIMER_FREQ (APB_CLK_FREQ / 2)
#define TX_RATE 25
#define TIMER_RELOAD (TIMER_FREQ / TX_RATE)
#define BUFFER_LENGTH (4096 * 3)

static spi_transaction_t spi_transaction;
static uint8_t buffer[BUFFER_LENGTH];
static spi_device_handle_t spi_device_handle;
static gptimer_handle_t gptimer_handle;

static const char* TAG = "spi_transmitter";

bool IRAM_ATTR timer_cb(gptimer_handle_t timer,
		const gptimer_alarm_event_data_t *edata, void* arg) {
	spi_transaction.tx_buffer = buffer;

    esp_err_t error = spi_device_queue_trans(spi_device_handle,
    		&spi_transaction, 0);

	if(error != ESP_ERR_TIMEOUT)
		ESP_ERROR_CHECK(error);

	return false;
}

void spi_transmitter_init(void)
{
    // fill buffer with our pattern
    for (uint32_t c = 0; c != BUFFER_LENGTH;) {
    	buffer[c++] = 0b10010010;
    	buffer[c++] = 0b01001001;
    	buffer[c++] = 0b00100110;
    }

    // initialize SPI master driver
    const spi_bus_config_t spi_bus_config = {
			.mosi_io_num = SPI_BUS_MOSI_IO,
    		.miso_io_num = GPIO_NUM_NC,
			.sclk_io_num = SPI_BUS_CLK_IO,
			.quadwp_io_num = GPIO_NUM_NC,
			.quadhd_io_num = GPIO_NUM_NC,
			.data4_io_num = GPIO_NUM_NC,
			.data5_io_num = GPIO_NUM_NC,
			.data6_io_num = GPIO_NUM_NC,
			.data7_io_num = GPIO_NUM_NC,
			.max_transfer_sz = BUFFER_LENGTH,
			.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_SCLK |
					SPICOMMON_BUSFLAG_MOSI,
			.intr_flags = ESP_INTR_FLAG_IRAM
    };

    const spi_device_interface_config_t spi_dev_config = {
			.command_bits = 0,
			.address_bits = 0,
			.dummy_bits = 0,
			.mode = 0,
			.duty_cycle_pos = 0,
			.cs_ena_pretrans = 0,
			.cs_ena_posttrans = 0,
    		.clock_speed_hz = 2500000,
			.input_delay_ns = 0,
			.spics_io_num = SPI_BUS_CS_IO,
			.flags = 0,
			.queue_size = 8,
			.pre_cb = NULL,
			.post_cb = NULL
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_BUS, &spi_bus_config, DMA_CHANNEL));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_BUS, &spi_dev_config,
    		&spi_device_handle));

    spi_transaction.length = BUFFER_LENGTH * 8;
    spi_transaction.tx_buffer = buffer;

    // setup and start timer
    const gptimer_config_t timer_config = {
    		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
			.direction = GPTIMER_COUNT_UP,
			.resolution_hz = TIMER_FREQ,
			.flags = {
					.intr_shared = 0
			}
    };

    const gptimer_alarm_config_t alarm_config = {
    		.alarm_count = TIMER_RELOAD,
			.reload_count = 0,
			.flags = {
					.auto_reload_on_alarm = 1
			}
    };

    const gptimer_event_callbacks_t timer_callbacks = {
    		.on_alarm = timer_cb
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_handle, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_handle,
    		&timer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer_handle));
    ESP_ERROR_CHECK(gptimer_start(gptimer_handle));

    ESP_LOGI(TAG, "initialized");
}
