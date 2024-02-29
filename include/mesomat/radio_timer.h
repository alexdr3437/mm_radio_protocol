

#ifndef RADIO_TIMER_H__
#define RADIO_TIMER_H__

#include <nrfx_timer.h>

extern const nrfx_timer_t timer;

int radio_timer_init(void);
uint32_t radio_timer_schedule_next_tx(uint32_t start, uint32_t datum);
uint32_t radio_timer_schedule_next_rx(uint32_t start, uint32_t datum);
uint32_t radio_timer_schedule_next_slot_start(uint32_t start, uint32_t datum);

static inline uint32_t radio_timer_now() {
	return nrfx_timer_capture(&timer, 3);
}
static inline uint32_t radio_timer_capture_get(uint8_t channel) {
	return nrfx_timer_capture_get(&timer, channel);
}

#endif
