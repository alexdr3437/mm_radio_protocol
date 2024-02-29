

#ifndef __RADIO_H
#define __RADIO_H

#include <stdint.h>

#include "radio_payload.h"

typedef void (*radio_rx_callback_t)(uint8_t *data, uint8_t len, int8_t rssi, radio_identity_t identity, radio_mode_t mode);
void radio_register_rx_callback(radio_rx_callback_t callback);

void radio_set_identity(radio_mode_t mode);
void radio_set_mode(radio_mode_t mode);
void radio_set_advertising_data(const uint8_t *data, uint8_t len);
void radio_pause_advertising(void);
void radio_resume_advertising(void);

#endif
