/*
 * @Author: alexdr3437 <alexanderdingwall5398@gmail.com>
 * @Date: 2024-02-29 15:33:03
 * @Last Modified by: alexdr3437 <alexanderdingwall5398@gmail.com>
 * @Last Modified time: 2024-02-29 15:33:08
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_timer, LOG_LEVEL_WRN);

#include <zephyr/kernel.h>

#include <hal/nrf_radio.h>
#include <nrfx.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <nrfx_timer.h>

#include "mesomat/radio_timer.h"

#define OUTPUT_PIN 14

static nrf_ppi_channel_t ppi_channels[ 6 ];

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

	uint8_t out_channel;
	rc = nrfx_gpiote_channel_alloc(&out_channel);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate GPIOTE channel %08x", rc);
		return -EINVAL;
	}

	const nrfx_gpiote_output_config_t output_config = {
		.drive		   = NRF_GPIO_PIN_S0S1,
		.input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
		.pull		   = NRF_GPIO_PIN_NOPULL,
	};

	const nrfx_gpiote_task_config_t task_config = {
		.task_ch  = out_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = NRF_GPIOTE_INITIAL_VALUE_HIGH,
	};

	rc = nrfx_gpiote_output_configure(OUTPUT_PIN, &output_config, &task_config);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to configure GPIOTE output");
		return -EINVAL;
	}

	nrfx_gpiote_out_task_enable(OUTPUT_PIN);

	rc = nrfx_timer_init(&timer, &timer_config, NULL);
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize timer %08x", rc);
		return -EINVAL;
	}

	for (int i = 0; i < ARRAY_SIZE(ppi_channels); i++) {
		rc = nrfx_ppi_channel_alloc(&ppi_channels[ i ]);
		if (rc != NRFX_SUCCESS) {
			LOG_ERR("Failed to allocate PPI channel");
			return -EINVAL;
		}
	}

	rc = nrfx_ppi_channel_assign(ppi_channels[ 0 ], nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS), nrfx_timer_capture_task_address_get(&timer, NRF_TIMER_CC_CHANNEL0));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -EINVAL;
	}

	rc = nrfx_ppi_channel_assign(ppi_channels[ 4 ], nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_READY), nrfx_timer_capture_task_address_get(&timer, NRF_TIMER_CC_CHANNEL0));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -EINVAL;
	}

	rc = nrfx_ppi_channel_assign(ppi_channels[ 5 ], nrf_radio_event_address_get(NRF_RADIO, NRF_RADIO_EVENT_END), nrfx_timer_capture_task_address_get(&timer, NRF_TIMER_CC_CHANNEL0));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -EINVAL;
	}

	rc = nrfx_ppi_channel_assign(ppi_channels[ 1 ], nrf_timer_event_address_get(timer.p_reg, NRF_TIMER_EVENT_COMPARE1), nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_RXEN));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -EINVAL;
	}

	rc = nrfx_ppi_channel_assign(ppi_channels[ 2 ], nrf_timer_event_address_get(timer.p_reg, NRF_TIMER_EVENT_COMPARE2), nrf_radio_task_address_get(NRF_RADIO, NRF_RADIO_TASK_TXEN));
	if (rc != NRFX_SUCCESS) {
		LOG_ERR("Failed to assign PPI channel");
		return -EINVAL;
	}

	/* rc = nrfx_ppi_channel_assign(ppi_channels[ 3 ], nrf_timer_event_address_get(timer.p_reg, NRF_TIMER_EVENT_COMPARE3), nrfx_gpiote_out_task_address_get(OUTPUT_PIN)); */
	/* if (rc != NRFX_SUCCESS) { */
	/* 	LOG_ERR("Failed to assign PPI channel"); */
	/* 	return -EINVAL; */
	/* } */

	for (int i = 0; i < ARRAY_SIZE(ppi_channels); i++) {
		rc = nrfx_ppi_channel_enable(ppi_channels[ i ]);
		if (rc != NRFX_SUCCESS) {
			LOG_ERR("Failed to enable PPI channel");
			return -EINVAL;
		}
	}

	LOG_INF("Radio timer initialized");
	nrfx_timer_resume(&timer);

	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL3, 1000000, false);

	return 0;
}

uint32_t radio_timer_schedule_next_rx(uint32_t start, uint32_t datum) {
	uint32_t cmp_value;
	cmp_value = datum + start;
	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL1, cmp_value, false);
	return cmp_value;
}
uint32_t radio_timer_schedule_next_tx(uint32_t start, uint32_t datum) {
	uint32_t cmp_value;
	cmp_value = datum + start;
	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL2, cmp_value, false);
	return cmp_value;
}
uint32_t radio_timer_schedule_next_slot_start(uint32_t start, uint32_t datum) {
	uint32_t cmp_value;
	cmp_value = datum + start;
	nrfx_timer_compare(&timer, NRF_TIMER_CC_CHANNEL3, cmp_value, false);
	return cmp_value;
}
