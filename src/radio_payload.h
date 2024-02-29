
#ifndef RADIO_PAYLOAD_H__
#define RADIO_PAYLOAD_H__

#include <stdint.h>

#define RADIO_MAX_PACKET_LEN 255

typedef struct {
	uint8_t val[ 6 ];
} radio_address_t;

typedef enum {
	RADIO_IDENTITY_SENSOR  = 0,
	RADIO_IDENTITY_GATEWAY = 1,
} radio_identity_t;

typedef enum {
	RADIO_MODE_ADVERTISING	 = 0,
	RADIO_MODE_MESH			 = 1,
	RADIO_MODE_FILE_TRANSFER = 2,
} radio_mode_t;

typedef struct __PACKED {
	uint8_t mode : 2;
	uint8_t reserved : 5;
	uint8_t identity : 1;
} descriptor_t;

typedef struct {
	uint8_t length;
	radio_address_t address;
	descriptor_t descriptor;
	uint8_t payload[ RADIO_MAX_PACKET_LEN - sizeof(radio_address_t) - sizeof(uint8_t) - sizeof(descriptor_t) ];
} radio_buffer_t;

void radio_set_address(const radio_address_t *address);
void radio_descriptor_set_identity(radio_identity_t identity);
void radio_descriptor_set_mode(radio_mode_t mode);
radio_identity_t radio_descriptor_get_identity();

void radio_payload_set(const uint8_t *data, uint8_t len);
radio_buffer_t *radio_payload_get_outgoing();
radio_buffer_t *radio_payload_rx_buffer_get_next();
radio_buffer_t *radio_payload_rx_buffer_get_current();

#endif
