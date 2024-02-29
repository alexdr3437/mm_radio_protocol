
#ifndef MM_RADIO_SCHEDULE_H__
#define MM_RADIO_SCHEDULE_H__

#include <stdint.h>

typedef enum {
	SCHEDULE_TYPE_TIMESLOT,
	SCHEDULE_TYPE_SCANNER,
} schedule_type_t;

typedef enum {
	SLOT_TYPE_TX,
	SLOT_TYPE_RX,
	SLOT_TYPE_BROADCAST,
} slot_type_t;

typedef struct {
	slot_type_t slot_type;
	uint8_t channel;
	uint8_t slot;
} slot_t;

slot_t *radio_schedule_get_next_slot();
uint8_t radio_schedule_get_current_slot();
void radio_schedule_advance_slot();
uint32_t get_time_to_slot_start(uint8_t slot, uint8_t current_slot);
void radio_schedule_init(uint8_t max_n_slots, uint16_t slot_width_ms);

int radio_schedule_timeslot_add(slot_type_t slot_type, uint8_t channel, uint8_t slot);
void radio_timeslot_print_schedule();

#endif
