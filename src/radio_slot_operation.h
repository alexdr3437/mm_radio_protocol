
#ifndef RADIO_TIMESLOTTING_H__
#define RADIO_TIMESLOTTING_H__

#include <stdbool.h>
#include <stdint.h>

#include "radio_payload.h"

typedef enum {
	SLOT_TYPE_TX,
	SLOT_TYPE_RX,
	SLOT_TYPE_BROADCAST,
} slot_type_t;

typedef void (*radio_rx_callback_t)(uint8_t *data, uint8_t len, int8_t rssi, radio_identity_t identity, radio_mode_t mode);
void radio_register_rx_callback(radio_rx_callback_t callback);

void radio_schedule_init(uint8_t max_n_slots, uint16_t slot_width_ms);
int radio_schedule_timeslot_add(slot_type_t slot_type, uint8_t channel, uint8_t slot);
void radio_timeslot_print_schedule();

void radio_timeslot_operation_start();
void radio_timeslot_end_event(uint32_t capture);
void radio_timeslot_disabled_event(uint32_t capture);
void radio_timeslot_address_event(uint32_t capture);
void radio_timeslot_crcok_event(uint32_t capture);
void radio_timeslot_rssiend_event(uint32_t capture);

#endif
