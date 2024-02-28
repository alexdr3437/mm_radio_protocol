
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio_timeslotting, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <stdbool.h>
#include <stdint.h>

#include <hal/nrf_radio.h>

#include "radio.h"
#include "radio_slot_operation.h"
#include "radio_timer.h"

#define NOMINAL_TX_OFFSET_us 55 // measured empirically
#define GUARD_TIME_us		 1000
#define MAX_RX_TIME_us		 10000

typedef struct {
	slot_type_t slot_type;
	uint8_t channel;
	uint8_t slot;
} slot_t;

typedef struct {
	uint8_t max_n_slots;
	uint16_t slot_width_ms;
	uint8_t n_slots;
	slot_t slots[ 255 ];
} slot_schedule_t;
static slot_schedule_t slot_schedule;

static uint32_t last_timeslot_start = 0;

static bool is_initiator = false;

static uint8_t mode = 1;

K_SEM_DEFINE(timeslot_operation_start_sem, 0, 1);
K_SEM_DEFINE(got_rx_message_sem, 0, 1);

K_MUTEX_DEFINE(slot_end_mut);
K_CONDVAR_DEFINE(slot_end_var);

static void wait_for_initiation() {
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_RXEN);
	while (k_sem_take(&got_rx_message_sem, K_MSEC(500)) != 0)
		;
}

static void slot_end_timeout_handler(struct k_timer *timer) {
	LOG_DBG("ended at %u", radio_timer_now());
	k_condvar_broadcast(&slot_end_var);
}
K_TIMER_DEFINE(slot_end_timer, slot_end_timeout_handler, slot_end_timeout_handler);

void radio_timeslot_end_event(uint32_t capture) {
	LOG_DBG("timeslot end event at %u", capture);
	k_timer_stop(&slot_end_timer);
}

static inline void wait_for_slot_end() {
	LOG_DBG("Waiting for slot end");
	k_condvar_wait(&slot_end_var, &slot_end_mut, K_FOREVER);
}

static inline uint32_t get_time_to_slot_start(uint8_t slot, uint8_t current_slot) {
	LOG_DBG("slot %d, current slot %d, n_slots %u", slot, current_slot, slot_schedule.n_slots);

	uint32_t time_to_slot_start;
	if (current_slot > slot) {
		time_to_slot_start = (slot_schedule.max_n_slots - current_slot + slot - 1) * slot_schedule.slot_width_ms * 1000;
	} else {
		time_to_slot_start = (slot + 1 - current_slot) * slot_schedule.slot_width_ms * 1000;
	}
	return time_to_slot_start;
}

static void radio_timeslot_rx(slot_t *slot, uint8_t current_slot) {
	LOG_DBG("RX slot %d, current slot %d", slot->slot, current_slot);
	uint32_t time_until_slot_start = get_time_to_slot_start(slot->slot, current_slot) - GUARD_TIME_us;
	radio_timer_schedule_next_slot_start(last_timeslot_start + GUARD_TIME_us, time_until_slot_start);
	last_timeslot_start = radio_timer_schedule_next_rx(last_timeslot_start, time_until_slot_start);
	k_timer_start(&slot_end_timer, K_USEC(time_until_slot_start + MAX_RX_TIME_us), K_NO_WAIT);
}

static void radio_timeslot_tx(slot_t *slot, uint8_t current_slot) {
	LOG_DBG("TX slot %d, channel %d, current slot %d", slot->slot, slot->channel, current_slot);
	uint32_t time_until_slot_start = get_time_to_slot_start(slot->slot, current_slot);
	radio_timer_schedule_next_slot_start(last_timeslot_start, time_until_slot_start);
	last_timeslot_start = radio_timer_schedule_next_tx(last_timeslot_start, time_until_slot_start);
	k_timer_start(&slot_end_timer, K_USEC(time_until_slot_start + MAX_RX_TIME_us), K_NO_WAIT);
}

void radio_timeslot_address_event(uint32_t capture) {
	if (mode) {
		LOG_DBG("assigning datum %u", capture);
		last_timeslot_start = capture - NOMINAL_TX_OFFSET_us;
		mode				= 0;
	}
	k_sem_give(&got_rx_message_sem);
}

static void radio_timeslot_operation_thread(void *p1, void *p2, void *p3) {
	static uint8_t current_slot_idx = 0;
	static uint8_t current_slot		= 0;

	k_sem_take(&timeslot_operation_start_sem, K_FOREVER);

	if (!is_initiator) {
		LOG_DBG("Waiting for initiation");
		wait_for_initiation();
	} else {
		last_timeslot_start = radio_timer_now();
		LOG_INF("Initiator, last_timeslot_start: %u", last_timeslot_start);
	}

	LOG_DBG("Starting timeslot operation");

	for (;;) {
		LOG_DBG("current_slot_idx: %d, time: %u", current_slot_idx, radio_timer_now());
		if (slot_schedule.slots[ current_slot_idx ].slot_type == SLOT_TYPE_RX) {
			radio_timeslot_rx(&slot_schedule.slots[ current_slot_idx ], current_slot);
		} else {
			radio_timeslot_tx(&slot_schedule.slots[ current_slot_idx ], current_slot);
		}
		wait_for_slot_end();
		current_slot	 = slot_schedule.slots[ current_slot_idx ].slot;
		current_slot_idx = (current_slot_idx + 1) % slot_schedule.n_slots;
	}
}
K_THREAD_DEFINE(radio_timeslot_operation, 1024, radio_timeslot_operation_thread, NULL, NULL, NULL, 8, 0, 0);

static int insert_slot_sorted(slot_t *slot) {
	slot_schedule.slots[ slot_schedule.n_slots++ ] = *slot;
	return 0;
}

void radio_schedule_init(uint8_t max_n_slots, uint16_t slot_width_ms) {
	slot_schedule = (slot_schedule_t){
		.max_n_slots   = max_n_slots,
		.slot_width_ms = slot_width_ms,
		.n_slots	   = 0,
	};
}

int radio_schedule_timeslot_add(slot_type_t slot_type, uint8_t channel, uint8_t slot) {
	if (slot_schedule.n_slots >= slot_schedule.max_n_slots) {
		return -1;
	}
	slot_t s = {
		.slot_type = slot_type,
		.channel   = channel,
		.slot	   = slot,
	};
	insert_slot_sorted(&s);

	return 0;
}

void radio_timeslot_print_schedule() {
	for (int i = 0; i < slot_schedule.n_slots; i++) {
		LOG_INF("slot %d: type %s, channel %d, slot %d", i, slot_schedule.slots[ i ].slot_type == SLOT_TYPE_RX ? "Rx" : "Tx", slot_schedule.slots[ i ].channel, slot_schedule.slots[ i ].slot);
	}
}

void radio_timeslot_operation_start(bool initiator) {
	is_initiator = initiator;
	k_sem_give(&timeslot_operation_start_sem);
}
