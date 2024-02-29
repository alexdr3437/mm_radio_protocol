
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_schedule, LOG_LEVEL_DBG);

#include <stdint.h>

#include "mesomat/radio_schedule.h"

typedef struct {
	uint8_t max_n_slots;
	uint16_t slot_width_ms;
	uint8_t n_slots;
	slot_t slots[ 255 ];
} slot_schedule_t;
static slot_schedule_t slot_schedule;

static uint8_t next_slot_idx = 0;
static uint8_t current_slot	 = 0;

static int insert_slot_sorted(slot_t *slot) {
	slot_schedule.slots[ slot_schedule.n_slots++ ] = *slot;
	return 0;
}

uint32_t get_time_to_slot_start(uint8_t slot, uint8_t current_slot) {
	LOG_DBG("slot %d, current slot %d", slot, current_slot);

	uint32_t time_to_slot_start;
	if (current_slot > slot) {
		time_to_slot_start = (slot_schedule.max_n_slots - current_slot + slot) * slot_schedule.slot_width_ms * 1000;
	} else {
		time_to_slot_start = (slot - current_slot) * slot_schedule.slot_width_ms * 1000;
	}

	return time_to_slot_start;
}

slot_t *radio_schedule_get_next_slot() {
	return &slot_schedule.slots[ next_slot_idx ];
}

uint8_t radio_schedule_get_current_slot() {
	return current_slot;
}

void radio_schedule_advance_slot() {
	current_slot  = slot_schedule.slots[ next_slot_idx ].slot;
	next_slot_idx = (next_slot_idx + 1) % slot_schedule.n_slots;
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
