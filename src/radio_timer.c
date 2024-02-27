
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio_timer, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <hal/nrf_radio.h>
#include <nrfx.h>
#include <nrfx_ppi.h>
#include <nrfx_timer.h>

static nrf_ppi_channel_t timer_capture_ppi;
static nrf_ppi_channel_t timer_start_ppi;

const nrfx_timer_t timer = {
	.p_reg			  = NRF_TIMER2,
	.instance_id	  = NRFX_TIMER2_INST_IDX,
	.cc_channel_count = TIMER2_CC_NUM,
};

int radio_timer_init(void) {
	int rc;

	static nrfx_timer_config_t timer_config = {
		.frequency			= 1000000,
		.mode				= NRF_TIMER_MODE_TIMER,
		.bit_width			= NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = 7,
		.p_context			= NULL,
	};

	rc = nrfx_timer_init(&timer, &timer_config, NULL);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize timer %08x", rc);
		return -1;
	}

	rc = nrfx_ppi_channel_alloc(&timer_capture_ppi);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI channel");
		return -1;
	}

	rc = nrfx_ppi_channel_alloc(&timer_start_ppi);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate PPI channel");
		return -1;
	}

	rc = nrfx_ppi_channel_assign(timer_capture_ppi, nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS), nrfx_timer_capture_task_address_get(&timer, NRF_TIMER_CC_CHANNEL0));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -1;
	}

	rc = nrfx_ppi_channel_assign(timer_start_ppi, nrf_timer_event_address_get(timer.p_reg, NRF_TIMER_EVENT_COMPARE1), nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_START));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -1;
	}

	rc = nrfx_ppi_channel_enable(timer_capture_ppi);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to enable PPI channel");
		return -1;
	}

	rc = nrfx_ppi_channel_enable(timer_start_ppi);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to enable PPI channel");
		return -1;
	}

	nrfx_timer_resume(&timer);
	return 0;
}

int radio_timer_schedule_next_start(uint32_t start, uint32_t datum) {
	static uint32_t last_start = 0;
	uint32_t cmp_value;

	if (datum == 0) {
		datum = last_start;
	}

	cmp_value = datum + start;

	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL1, cmp_value, false);
	printk("radio_timer_schedule_next_start: %d %d\n", cmp_value, nrfx_timer_capture(&timer, NRF_TIMER_CC_CHANNEL0));

	last_start = cmp_value;
	return 0;
}

ssize_t radio_timer_capture_get() {
}
