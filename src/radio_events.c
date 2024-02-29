
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mm_radio_events, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <zephyr/init.h>

#include "hal/nrf_radio.h"

#include "mesomat/radio_events.h"
#include "mesomat/radio_payload.h"
#include "mesomat/radio_slot_operation.h"
#include "mesomat/radio_timer.h"

#define RADIO_EVENT_WORK_Q_SIZE 10

K_THREAD_STACK_DEFINE(radio_event_thread_stack, 4096);

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

static void radio_event_ready(uint32_t capture) {
	LOG_DBG("radio ready %d", radio_timer_capture_get(3));
}

radio_handler_t radio_handlers[ N_RADIO_EVENTS ] = {0};

static void radio_event(struct k_work *work) {
	event_work_t *event_work = CONTAINER_OF(work, event_work_t, work);
	radio_event_t event		 = event_work->event;
	uint32_t capture		 = event_work->timer_capture;

	LOG_DBG("radio_event %d %u", event, capture);
	if (radio_handlers[ event ] != NULL) {
		radio_handlers[ event ](capture);
	}
}

void radio_event_handle(radio_event_t event) {
	radio_event_work[ event_work_idx ].timer_capture = radio_timer_capture_get(0);
	radio_event_work[ event_work_idx ].event		 = event;

	int rc		   = k_work_submit_to_queue(&radio_event_work_q, &radio_event_work[ event_work_idx ].work);
	event_work_idx = (event_work_idx + 1) % RADIO_EVENT_WORK_Q_SIZE;
}

void radio_event_register(radio_event_t event, radio_handler_t handler) {
	radio_handlers[ event ] = handler;
}

void radio_isr(void *arg) {

	__disable_irq();

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_END)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_END);
		radio_event_handle(RADIO_EVENT_END);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_READY)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_READY);
		radio_event_handle(RADIO_EVENT_READY);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_ADDRESS);
		radio_event_handle(RADIO_EVENT_ADDRESS);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_PAYLOAD);
		radio_event_handle(RADIO_EVENT_PAYLOAD);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
		radio_event_handle(RADIO_EVENT_DISABLED);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCERROR)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCERROR);
		radio_event_handle(RADIO_EVENT_CRCERROR);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_CRCOK)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_CRCOK);
		radio_event_handle(RADIO_EVENT_CRCOK);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_TXREADY)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_TXREADY);
		radio_event_handle(RADIO_EVENT_TXREADY);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_RXREADY)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_RXREADY);
		radio_event_handle(RADIO_EVENT_RXREADY);
	}

	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_RSSIEND)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_RSSIEND);
		radio_event_handle(RADIO_EVENT_RSSIEND);
	}

	__enable_irq();
}

int radio_event_handler_init(void) {
	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_END_MASK | NRF_RADIO_INT_READY_MASK | NRF_RADIO_INT_ADDRESS_MASK | NRF_RADIO_INT_PAYLOAD_MASK | NRF_RADIO_INT_DISABLED_MASK | NRF_RADIO_INT_CRCERROR_MASK | NRF_RADIO_INT_CRCOK_MASK | NRF_RADIO_INT_TXREADY_MASK | NRF_RADIO_INT_RXREADY_MASK | NRF_RADIO_INT_RSSIEND_MASK);

	IRQ_CONNECT(RADIO_IRQn, 0, radio_isr, NULL, 0);
	irq_enable(RADIO_IRQn);

	const struct k_work_queue_config radio_event_work_q_config = {
		.name	  = "radio_event_work_q",
		.no_yield = false,
	};
	LOG_DBG("radio_event_handler_init\n");

	k_work_queue_init(&radio_event_work_q);
	k_work_queue_start(&radio_event_work_q, radio_event_thread_stack, K_THREAD_STACK_SIZEOF(radio_event_thread_stack), -7, &radio_event_work_q_config);

	for (int i = 0; i < RADIO_EVENT_WORK_Q_SIZE; i++) {
		k_work_init(&radio_event_work[ i ].work, radio_event);
	}
	return 0;
}
SYS_INIT(radio_event_handler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
