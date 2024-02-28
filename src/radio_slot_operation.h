
#ifndef RADIO_TIMESLOTTING_H__
#define RADIO_TIMESLOTTING_H__

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	SLOT_TYPE_TX,
	SLOT_TYPE_RX,
	SLOT_TYPE_BROADCAST,
} slot_type_t;

void radio_schedule_init(uint8_t max_n_slots, uint16_t slot_width_ms);
int radio_schedule_timeslot_add(slot_type_t slot_type, uint8_t channel, uint8_t slot);
void radio_timeslot_print_schedule();

void radio_timeslot_operation_start(bool initiator);
void radio_timeslot_end_event(uint32_t capture);
void radio_timeslot_address_event(uint32_t capture);

#endif
