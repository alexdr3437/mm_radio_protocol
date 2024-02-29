
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
} radio_event_t;

void radio_handle_event(radio_event_t event);
