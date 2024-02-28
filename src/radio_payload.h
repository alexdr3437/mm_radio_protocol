
#ifndef RADIO_PAYLOAD_H__
#define RADIO_PAYLOAD_H__

#include <stdint.h>

#define RADIO_MAX_PAYLOAD_LEN 255

typedef struct {
	uint8_t data[ RADIO_MAX_PAYLOAD_LEN ];
	uint8_t length;
} radio_buffer_t;

#endif
