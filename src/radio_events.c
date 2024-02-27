
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio_events, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/init.h>

#include "radio_events.h"
#include "radio_timer.h"

#define RADIO_EVENT_WORK_Q_SIZE 10

K_THREAD_STACK_DEFINE(radio_event_thread_stack, 1024);

struct k_work_q radio_event_work_q;

typedef struct {
	radio_event_t event;
	uint32_t timer_capture;
	struct k_work_q work;
} event_work_t;
static event_work_t radio_event_work[ RADIO_EVENT_WORK_Q_SIZE ];
int event_work_idx = 0;

void radio_event_address(uint32_t capture) {
	static uint32_t prev_capture = 0;
	if (prev_capture != 0) {
		LOG_DBG("capture: %d", capture - prev_capture);
	}
	prev_capture = capture;
}

// clang-format off
typedef void (*radio_handler_t)(uint32_t capture);
radio_handler_t radio_handlers[ ] = {
	[RADIO_EVENT_END] = NULL,
	[RADIO_EVENT_DISABLED] = NULL,
	[RADIO_EVENT_READY] = NULL,
	[RADIO_EVENT_ADDRESS] = radio_event_address,
	[RADIO_EVENT_PAYLOAD] = NULL,
	[RADIO_EVENT_CRCOK] = NULL,
	[RADIO_EVENT_CRCERROR] = NULL,
};
// clang-format on

static void radio_event(struct k_work *work) {
	event_work_t *event_work = CONTAINER_OF(work, event_work_t, work);
	radio_event_t event		 = event_work->event;
	uint32_t capture		 = event_work->timer_capture;

	if (radio_handlers[ event ] != NULL) {
		radio_handlers[ event ](capture);
	}
}

void radio_handle_event(radio_event_t event) {
	radio_event_work[ event_work_idx ].timer_capture = radio_timer_capture_get();
	radio_event_work[ event_work_idx ].event		 = event;
	k_work_submit_to_queue(&radio_event_work_q, &radio_event_work[ event_work_idx ].work);
	event_work_idx = (event_work_idx + 1) % RADIO_EVENT_WORK_Q_SIZE;
}

void radio_event_handler_init(void) {
	const struct k_work_queue_config radio_event_work_q_config = {
		.name	  = "radio_event_work_q",
		.no_yield = true,
	};
	k_work_queue_init(&radio_event_work_q);
	k_work_queue_start(&radio_event_work_q, radio_event_thread_stack, K_THREAD_STACK_SIZEOF(radio_event_thread_stack), -4, &radio_event_work_q_config);

	for (int i = 0; i < RADIO_EVENT_WORK_Q_SIZE; i++) {
		k_work_init(&radio_event_work[ i ].work, radio_event);
	}
}
SYS_INIT(radio_event_handler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
