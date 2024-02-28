
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio_events, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/init.h>

#include "radio_events.h"
#include "radio_payload.h"
#include "radio_timer.h"
#include "radio_slot_operation.h"

#define RADIO_EVENT_WORK_Q_SIZE 10

K_THREAD_STACK_DEFINE(radio_event_thread_stack, 1024);

K_SEM_DEFINE(radio_end_sem, 0, 1);

struct k_work_q radio_event_work_q;

typedef struct {
	radio_event_t event;
	uint32_t timer_capture;
	struct k_work work;
} event_work_t;
static event_work_t radio_event_work[ RADIO_EVENT_WORK_Q_SIZE ];
int event_work_idx = 0;

void radio_event_wait_for_rx(radio_buffer_t **buffer) {
}

void radio_event_wait_for_tx() {
}

static void radio_event_payload(uint32_t capture) {
}

static void radio_event_address(uint32_t capture) {
	static uint32_t prev_capture = 0;
	if (prev_capture != 0) {
		LOG_DBG("capture: %d", capture - prev_capture);
	}
	prev_capture = capture;
}

static void radio_event_ready(uint32_t capture) {
	LOG_DBG("radio ready %d", radio_timer_capture_get(3));
}

// clang-format off
typedef void (*radio_handler_t)(uint32_t capture);
radio_handler_t radio_handlers[ ] = {
	[RADIO_EVENT_END] = radio_timeslot_end_event,
	[RADIO_EVENT_DISABLED] = NULL, 
	[RADIO_EVENT_READY] = radio_event_ready,
	[RADIO_EVENT_ADDRESS] = radio_timeslot_address_event,
	[RADIO_EVENT_PAYLOAD] = radio_event_payload,
	[RADIO_EVENT_CRCOK] = NULL,


	[RADIO_EVENT_CRCERROR] = NULL,
};
// clang-format on

static void radio_event(struct k_work *work) {
	event_work_t *event_work = CONTAINER_OF(work, event_work_t, work);
	radio_event_t event		 = event_work->event;
	uint32_t capture		 = event_work->timer_capture;
	LOG_DBG("radio_event: %d, capture %u", event, capture);
	if (radio_handlers[ event ] != NULL) {
		radio_handlers[ event ](capture);
	}
}

void radio_handle_event(radio_event_t event) {
	radio_event_work[ event_work_idx ].timer_capture = radio_timer_capture_get(0);
	radio_event_work[ event_work_idx ].event		 = event;
	k_work_submit_to_queue(&radio_event_work_q, &radio_event_work[ event_work_idx ].work);
	event_work_idx = (event_work_idx + 1) % RADIO_EVENT_WORK_Q_SIZE;
}

int radio_event_handler_init(void) {
	const struct k_work_queue_config radio_event_work_q_config = {
		.name	  = "radio_event_work_q",
		.no_yield = true,
	};
	k_work_queue_init(&radio_event_work_q);
	k_work_queue_start(&radio_event_work_q, radio_event_thread_stack, K_THREAD_STACK_SIZEOF(radio_event_thread_stack), -7, &radio_event_work_q_config);

	for (int i = 0; i < RADIO_EVENT_WORK_Q_SIZE; i++) {
		k_work_init(&radio_event_work[ i ].work, radio_event);
	}
	return 0;
}
SYS_INIT(radio_event_handler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
