
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/irq.h>

#include <nrfx.h>

#include "hal/nrf_ficr.h"
#include "hal/nrf_radio.h"

#include "mesomat/radio_events.h"
#include "mesomat/radio_payload.h"
#include "mesomat/radio_slot_operation.h"
#include "mesomat/radio_timer.h"

enum {
	RECEIVER,
	TRANSMITTER,
};

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
		.cilen		= 1,							// Length of code indicator - long range.
		.plen		= RADIO_PCNF0_PLEN_16bit,		// Length of preamble on air. Decision point: TASKS_START task.
		.crcinc		= false,						// Indicates if LENGTH field contains CRC or not.
		.termlen	= 1,							// Length of TERM field in Long Range operation.
		.maxlen		= 255,							// Maximum length of packet payload.
		.statlen	= 255,							// Static length in number of bytes.
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

	nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | NRF_RADIO_SHORT_DISABLED_RSSISTOP_MASK);
}

int radio_init() {

	radio_config();
	radio_timer_init();

	radio_set_address((radio_address_t *)NRF_FICR->DEVICEADDR);

	uint8_t node = 1;

	radio_descriptor_set_mode(RADIO_MODE_MESH);

	LOG_HEXDUMP_INF(NRF_FICR->DEVICEID, 8, "Device ID");
	LOG_HEXDUMP_INF(&NRF_FICR->DEVICEADDRTYPE, 4, "Device Addr Type");
	LOG_HEXDUMP_INF(NRF_FICR->DEVICEADDR, 8, "Device Addr");
	LOG_HEXDUMP_INF(NRF_FICR->IR, 16, "IR");

	radio_schedule_init(10, 100);

	switch (node) {
	case 1: {
		uint8_t data[] = "I am node 1";
		radio_payload_set(data, sizeof(data));

		radio_descriptor_set_identity(RADIO_IDENTITY_SENSOR);
		radio_schedule_timeslot_add(SLOT_TYPE_RX, 0, 0);
		radio_schedule_timeslot_add(SLOT_TYPE_TX, 0, 1);

		LOG_INF("Starting as node 1");
		radio_timeslot_print_schedule();

		radio_timeslot_operation_start();
		break;
	}
	default: {
		uint8_t data[] = "I am node 2";
		radio_payload_set(data, sizeof(data));

		radio_descriptor_set_identity(RADIO_IDENTITY_GATEWAY);
		radio_schedule_timeslot_add(SLOT_TYPE_TX, 0, 0);
		radio_schedule_timeslot_add(SLOT_TYPE_RX, 0, 1);

		LOG_INF("Starting as node 2");
		radio_timeslot_print_schedule();

		radio_timeslot_operation_start();
		break;
	}
	}

	return 0;
}
SYS_INIT(radio_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
