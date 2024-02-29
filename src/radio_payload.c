#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_payload, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>

#include <stdint.h>
#include <string.h>

#include "mesomat/radio_payload.h"

#define N_RADIO_BUFFERS 5

static radio_buffer_t outgoing_packet;
static radio_buffer_t radio_buffer[ N_RADIO_BUFFERS ];
static uint8_t radio_buffer_idx = 0;

void radio_set_address(const radio_address_t *address) {
	memcpy(&outgoing_packet.address, address, sizeof(radio_address_t));
}

void radio_descriptor_set_identity(radio_identity_t identity) {
	outgoing_packet.descriptor.identity = identity;
}

void radio_descriptor_set_mode(radio_mode_t mode) {
	outgoing_packet.descriptor.mode = mode;
}

radio_identity_t radio_descriptor_get_identity() {
	return outgoing_packet.descriptor.identity;
}

void radio_payload_set(const uint8_t *data, uint8_t len) {
	outgoing_packet.length = len + (RADIO_MAX_PACKET_LEN - sizeof(outgoing_packet.payload));
	memcpy(outgoing_packet.payload, data, len);
	LOG_HEXDUMP_DBG(outgoing_packet.payload, outgoing_packet.length, "TX payload");
}

radio_buffer_t *radio_payload_get_outgoing() {
	return &outgoing_packet;
}

radio_buffer_t *radio_payload_rx_buffer_get_next() {
	printk("get next\n");
	radio_buffer_idx = (radio_buffer_idx + 1) % N_RADIO_BUFFERS;
	return &radio_buffer[ radio_buffer_idx ];
}

radio_buffer_t *radio_payload_rx_buffer_get_current() {
	printk("get current\n");
	return &radio_buffer[ radio_buffer_idx ];
}
