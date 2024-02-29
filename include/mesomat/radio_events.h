
#ifndef RADIO_EVENTS_H__
#define RADIO_EVENTS_H__

#include <stdint.h>

/**
 * @enum radio_event_t
 * @brief Enumerates the different events that the radio can handle.
 */
typedef enum {
	RADIO_EVENT_READY,
	RADIO_EVENT_END,
	RADIO_EVENT_ADDRESS,
	RADIO_EVENT_PAYLOAD,
	RADIO_EVENT_DISABLED,
	RADIO_EVENT_TXREADY,
	RADIO_EVENT_RXREADY,
	RADIO_EVENT_CRCERROR,
	RADIO_EVENT_CRCOK,
	RADIO_EVENT_PHYEND,
	RADIO_EVENT_RSSIEND,

	N_RADIO_EVENTS
} radio_event_t;

typedef void (*radio_handler_t)(uint32_t capture);
void radio_event_handle(radio_event_t event);
void radio_event_register(radio_event_t event, radio_handler_t handler);

#endif
