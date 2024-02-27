

#ifndef RADIO_TIMER_H__
#define RADIO_TIMER_H__

#include <nrfx_timer.h>

extern const nrfx_timer_t timer;

int radio_timer_init(void);
int radio_timer_schedule_next_start(uint32_t start_time, uint32_t datum);

inline uint32_t radio_timer_capture_get() {
	return nrfx_timer_capture_get(&timer, NRF_TIMER_CC_CHANNEL0);
}

#endif
