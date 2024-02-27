
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/irq.h>

#include <nrfx.h>

#include "hal/nrf_radio.h"

#include "radio_events.h"
#include "radio_timer.h"

enum {
	RECEIVER,
	TRANSMITTER,
};

static uint8_t packet[ 255 ];

int clocks_start(void) {
	int err;
	int res;
	struct onoff_manager *clk_mgr;
	struct onoff_client clk_cli;

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	if (!clk_mgr) {
		LOG_ERR("Unable to get the Clock manager");
		return -ENXIO;
	}

	sys_notify_init_spinwait(&clk_cli.notify);

	err = onoff_request(clk_mgr, &clk_cli);
	if (err < 0) {
		LOG_ERR("Clock request failed: %d", err);
		return err;
	}

	do {
		err = sys_notify_fetch_result(&clk_cli.notify, &res);
		if (!err && res) {
			LOG_ERR("Clock could not be started: %d", res);
			return res;
		}
	} while (err);

	LOG_DBG("HF clock started");
	return 0;
}

static void radio_config() {

	clocks_start();

	nrf_radio_power_set(NRF_RADIO, true);

	nrf_radio_mode_set(NRF_RADIO, RADIO_MODE_MODE_Ble_1Mbit);

	nrf_radio_packet_conf_t config = {
		.lflen		= 0,							// Length on air of LENGTH field in number of bits.
		.s0len		= 0,							// Length on air of S0 field in number of bytes.
		.s1len		= 0,							// Length on air of S1 field in number of bits.
		.s1incl		= RADIO_PCNF0_S1INCL_Automatic, // Include or exclude S1 field in RAM.
		.cilen		= 0,							// Length of code indicator - long range.
		.plen		= RADIO_PCNF0_PLEN_16bit,		// Length of preamble on air. Decision point: TASKS_START task.
		.crcinc		= false,						// Indicates if LENGTH field contains CRC or not.
		.termlen	= 0,							// Length of TERM field in Long Range operation.
		.maxlen		= 60,							// Maximum length of packet payload.
		.statlen	= 60,							// Static length in number of bytes.
		.balen		= 4,							// Base address length in number of bytes.
		.big_endian = false,						// On air endianness of packet.
		.whiteen	= true							// Enable or disable packet whitening.
	};
	nrf_radio_packet_configure(NRF_RADIO, &config);

	nrf_radio_datawhiteiv_set(NRF_RADIO, 0xAB);

	nrf_radio_base0_set(NRF_RADIO, 0xad081c1f);
	nrf_radio_prefix0_set(NRF_RADIO, 0xe7);

	nrf_radio_txaddress_set(NRF_RADIO, 0);
	nrf_radio_rxaddresses_set(NRF_RADIO, 1);

	nrf_radio_crc_configure(NRF_RADIO, 2, RADIO_CRCCNF_SKIP_ADDR_Skip, 0x000AA0AA);
	nrf_radio_crcinit_set(NRF_RADIO, 0x00005050);

	nrf_radio_fast_ramp_up_enable_set(NRF_RADIO, false);

	nrf_radio_txpower_set(NRF_RADIO, RADIO_TXPOWER_TXPOWER_0dBm);
	nrf_radio_frequency_set(NRF_RADIO, 0);

	nrf_radio_packetptr_set(NRF_RADIO, packet);
}

ISR_DIRECT_DECLARE(radio_isr) {

	__disable_irq();

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);
		radio_handle_event(RADIO_EVENT_END);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_READY)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);
		radio_handle_event(RADIO_EVENT_READY);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
		radio_handle_event(RADIO_EVENT_ADDRESS);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD);
		radio_handle_event(RADIO_EVENT_PAYLOAD);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
		radio_handle_event(RADIO_EVENT_DISABLED);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCERROR)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCERROR);
		radio_handle_event(RADIO_EVENT_CRCERROR);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCOK)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCOK);
		radio_handle_event(RADIO_EVENT_CRCOK);
	}

	__enable_irq();

	return 0;
}

int radio_init() {

	radio_config();
	radio_timer_init();

	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_END_MASK | NRF_RADIO_INT_READY_MASK | NRF_RADIO_INT_ADDRESS_MASK | NRF_RADIO_INT_PAYLOAD_MASK | NRF_RADIO_INT_DISABLED_MASK | NRF_RADIO_INT_CRCERROR_MASK | NRF_RADIO_INT_CRCOK_MASK);

	IRQ_DIRECT_CONNECT(RADIO_IRQn, 0, radio_isr, 0);
	irq_enable(RADIO_IRQn);

	LOG_DBG("radio state: %d", nrf_radio_state_get(NRF_RADIO));

	// ppi for ADDRESS event

	uint8_t mode = TRANSMITTER;

	switch (mode) {
	case TRANSMITTER:

		for (int i = 0; i < 255; i++) {
			packet[ i ] = i;
		}

		strncpy(packet, "Hello World!", 12);

		nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK);
		NRF_RADIO->TASKS_TXEN = 1;

		radio_timer_schedule_next_start(5 * 1000000, 0);
		k_msleep(5000);
		for (;;) {
			/* LOG_INF("radio state: %d", nrf_radio_state_get(NRF_RADIO)); */
			radio_timer_schedule_next_start(1 * 1000000, 0);
			k_msleep(1000);

		}
		break;
	case RECEIVER:

		for (int i = 0; i < 255; i++) {
			packet[ i ] = 0xaa;
		}

		nrf_radio_dacnf_set(NRF_RADIO, 1, 1);
		nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_START_MASK);
		NRF_RADIO->TASKS_RXEN  = 1;
		NRF_RADIO->TASKS_START = 1;
		k_msleep(100);
		for (;;) {
			k_msleep(1000);
			LOG_INF("radio state: %d", nrf_radio_state_get(NRF_RADIO));
		}
		break;
	}

	return 0;
}
