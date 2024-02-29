/*
 * @Author: alexdr3437 <alexanderdingwall5398@gmail.com>
 * @Date: 2024-02-29 15:54:03
 * @Last Modified by: alexdr3437 <alexanderdingwall5398@gmail.com>
 * @Last Modified time: 2024-02-29 16:47:13
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_timeslotting, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stdint.h>

#include <hal/nrf_radio.h>

#include "mesomat/radio.h"
#include "mesomat/radio_callbacks.h"
#include "mesomat/radio_events.h"
#include "mesomat/radio_payload.h"
#include "mesomat/radio_schedule.h"
#include "mesomat/radio_slot_operation.h"
#include "mesomat/radio_timer.h"

#define NOMINAL_TX_OFFSET_us 55	  // measured empirically, time between Tx READY and ADDRESS events
#define MAX_TRANSFER_TIME_us 4508 // measured empirically, amount of time for a maximum length packet to be transmitted, (between Tx READY and END events)
#define GUARD_TIME_us		 1000
#define MAX_RX_TIME_us		 (2 * (GUARD_TIME_us) + MAX_TRANSFER_TIME_us)

static uint32_t last_timeslot_start = 0;
static bool is_initiator			= false;
static bool is_initiated			= false;
static bool trigger_disable_flag	= false;
static bool rx_flag					= false;

K_SEM_DEFINE(timeslot_operation_start_sem, 0, 1);
K_SEM_DEFINE(got_rx_message_sem, 0, 1);

K_MUTEX_DEFINE(slot_end_mut);
K_CONDVAR_DEFINE(slot_end_var);

static void wait_for_initiation() {

	nrf_radio_packetptr_set(NRF_RADIO, (void *)radio_payload_get_outgoing());

	for (;;) {
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
		k_sem_take(&got_rx_message_sem, K_FOREVER);

		radio_buffer_t *rx_payload = (radio_buffer_t *)nrf_radio_packetptr_get(NRF_RADIO);

		LOG_HEXDUMP_DBG(rx_payload, 2 + rx_payload->length, "payload");

		if (rx_payload->descriptor.identity == RADIO_IDENTITY_GATEWAY && rx_payload->descriptor.mode == RADIO_MODE_MESH) {
			LOG_DBG("Got initiation message");
			break;
		}
	}
}

static void slot_end_timeout_handler(struct k_timer *timer) {
	LOG_DBG("ended at %u", radio_timer_now());
	if (trigger_disable_flag) {
		nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
	}
}
K_TIMER_DEFINE(slot_end_timer, slot_end_timeout_handler, NULL);

void radio_timeslot_end_event(uint32_t capture) {
}

void radio_timeslot_disabled_event(uint32_t capture) {
	LOG_DBG("timeslot end event at %u", capture);
	trigger_disable_flag = false;
	k_timer_stop(&slot_end_timer);
	k_condvar_broadcast(&slot_end_var);
}

void radio_timeslot_address_event(uint32_t capture) {
	static uint32_t last_capture = 0;
	if (!is_initiated) {
		last_timeslot_start = capture - NOMINAL_TX_OFFSET_us;
	} else {
		LOG_DBG("address event %u", capture - last_capture);
		last_capture = capture;
	}
}

void radio_timeslot_crcok_event(uint32_t capture) {
	rx_flag = true;
	k_sem_give(&got_rx_message_sem);
}

void radio_timeslot_rssiend_event(uint32_t capture) {
}

static inline void wait_for_slot_end() {
	LOG_DBG("Waiting for slot end");
	k_condvar_wait(&slot_end_var, &slot_end_mut, K_FOREVER);
}

static void handle_rx() {
	LOG_DBG("Handling RX");

	if (rx_flag) {
		radio_buffer_t *packet = radio_payload_rx_buffer_get_current();
		LOG_ERR("Rx: got %d bytes, rssi %d", packet->length, -nrf_radio_rssi_sample_get(NRF_RADIO));

		printk("i: %s\n", packet->payload);

		if (rx_callback != NULL) {
			rx_callback(packet->payload, packet->length - (RADIO_MAX_PACKET_LEN - sizeof(packet->payload)), -nrf_radio_rssi_sample_get(NRF_RADIO), packet->descriptor.identity, packet->descriptor.mode);
		}

		packet = radio_payload_get_outgoing();
		printk("o: %s\n", packet->payload);

	} else {
		LOG_DBG("Rx: no message");
	}

	rx_flag = false;
}

static void timeslot_rx(slot_t *slot, uint8_t current_slot) {
	LOG_DBG("RX slot %d, current slot %d", slot->slot, current_slot);
	uint32_t time_until_slot_start = get_time_to_slot_start(slot->slot, current_slot);

	nrf_radio_packetptr_set(NRF_RADIO, (void *)radio_payload_rx_buffer_get_next());

	radio_timer_schedule_next_slot_start(time_until_slot_start, last_timeslot_start);
	last_timeslot_start = radio_timer_schedule_next_rx(time_until_slot_start - GUARD_TIME_us, last_timeslot_start) + GUARD_TIME_us;

	trigger_disable_flag = true;
	rx_flag				 = false;
	k_timer_start(&slot_end_timer, K_USEC(time_until_slot_start + MAX_RX_TIME_us - GUARD_TIME_us), K_NO_WAIT);
}

static void timeslot_tx(slot_t *slot, uint8_t current_slot) {
	LOG_DBG("TX slot %d, channel %d, current slot %d", slot->slot, slot->channel, current_slot);
	uint32_t time_until_slot_start = get_time_to_slot_start(slot->slot, current_slot);

	nrf_radio_packetptr_set(NRF_RADIO, (void *)radio_payload_get_outgoing());

	radio_timer_schedule_next_slot_start(time_until_slot_start, last_timeslot_start);
	last_timeslot_start = radio_timer_schedule_next_tx(time_until_slot_start, last_timeslot_start);

	LOG_DBG("time_until_slot_start: %u, last_timeslot_start: %u", time_until_slot_start, last_timeslot_start);
}

static void radio_timeslot_operation_thread(bool *terminate, void *p2, void *p3) {
	k_sem_take(&timeslot_operation_start_sem, K_FOREVER);

	if (!is_initiator) {
		LOG_DBG("Waiting for initiation");
		wait_for_initiation();
	} else {
		is_initiated		= true;
		last_timeslot_start = radio_timer_now();
		LOG_INF("Initiator, last_timeslot_start: %u", last_timeslot_start);
	}

	LOG_DBG("Starting timeslot operation");

	for (;;) {
		slot_t *slot		 = radio_schedule_get_next_slot();
		uint8_t current_slot = radio_schedule_get_current_slot();
		if (slot->slot != radio_schedule_get_current_slot()) {
			LOG_ERR("current_slot_idx: %d, time: %u", current_slot, radio_timer_now());

			nrf_radio_frequency_set(NRF_RADIO, slot->channel);

			if (slot->slot_type == SLOT_TYPE_RX) {
				timeslot_rx(slot, current_slot);
			} else {
				timeslot_tx(slot, current_slot);
			}

			wait_for_slot_end();

			if (slot->slot_type == SLOT_TYPE_RX) {
				handle_rx();
			}
		}
		radio_schedule_advance_slot();
	}
}

void radio_timeslot_operation_start() {
	radio_event_register(RADIO_EVENT_DISABLED, radio_timeslot_disabled_event);
	radio_event_register(RADIO_EVENT_ADDRESS, radio_timeslot_address_event);
	radio_event_register(RADIO_EVENT_CRCOK, radio_timeslot_crcok_event);
	radio_event_register(RADIO_EVENT_RSSIEND, radio_timeslot_rssiend_event);

	nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK | NRF_RADIO_SHORT_END_DISABLE_MASK | NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | NRF_RADIO_SHORT_DISABLED_RSSISTOP_MASK);

	LOG_DBG("Starting timeslot operation");
	is_initiator = radio_descriptor_get_identity() == RADIO_IDENTITY_GATEWAY;

	
}
