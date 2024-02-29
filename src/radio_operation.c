
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>

#define CONFIG_RADIO_OPERATION_STACK_SIZE 4096

K_THREAD_STACK_DEFINE(radio_operation_stack_area, CONFIG_RADIO_OPERATION_STACK_SIZE);
struct k_thread radio_operation_thread_data;

volatile bool terminate;

void radio_operation_start_new_thread(k_thread_entry_t entry) {
	k_thread_create(&radio_operation_thread_data, radio_operation_stack_area, K_THREAD_STACK_SIZEOF(radio_operation_stack_area), entry, &terminate, NULL, NULL, -4, 0, K_NO_WAIT);
}
