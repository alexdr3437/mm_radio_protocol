
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_scanner, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <hal/nrf_radio.h>

#include "mesomat/radio.h"
#include "mesomat/radio_callbacks.h"
#include "mesomat/radio_events.h"

K_SEM_DEFINE(radio_scanner_sem, 0, 1);

static void handle_rx() {
	radio_buffer_t *packet = radio_payload_rx_buffer_get_current();
	LOG_ERR("Rx: got %d bytes, rssi %d", packet->length, -nrf_radio_rssi_sample_get(NRF_RADIO));

	printk("i: %s\n", packet->payload);

	if (rx_callback != NULL) {
		rx_callback(packet->payload, packet->length - (RADIO_MAX_PACKET_LEN - sizeof(packet->payload)), -nrf_radio_rssi_sample_get(NRF_RADIO), packet->descriptor.identity, packet->descriptor.mode);
	}
}

static void handle_crcok_event(uint32_t capture) {
	k_sem_give(&radio_scanner_sem);
}

static void scanner_operation_thread(bool *terminate, void *p2, void *p3) {

	LOG_DBG("Scanner operation thread started");

	for (;;) {
		k_sem_take(&radio_scanner_sem, K_FOREVER);
		if (*terminate) {
			break;
		}

		handle_rx();

		if (*terminate) {
			break;
		}
	}
}

void radio_scanner_start() {
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
}

void radio_scanner_stop() {
}

void radio_scanner_init() {
	nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_START_MASK | NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | NRF_RADIO_SHORT_DISABLED_RSSISTOP_MASK);
	radio_event_register(RADIO_EVENT_CRCOK, handle_crcok_event);

	radio_operation_start_new_thread(scanner_operation_thread);
}
