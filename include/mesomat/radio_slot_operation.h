
#ifndef RADIO_TIMESLOTTING_H__
#define RADIO_TIMESLOTTING_H__

#include <stdbool.h>
#include <stdint.h>

#include "radio_payload.h"
#include "radio_schedule.h"

void radio_timeslot_operation_start();
void radio_timeslot_end_event(uint32_t capture);
void radio_timeslot_disabled_event(uint32_t capture);
void radio_timeslot_address_event(uint32_t capture);
void radio_timeslot_crcok_event(uint32_t capture);
void radio_timeslot_rssiend_event(uint32_t capture);

#endif
