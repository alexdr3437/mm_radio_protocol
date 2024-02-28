/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>

#include "radio.h"

K_SEM_DEFINE(test_sem, 0, 1);

static void timer_handler(struct k_timer *timer_id) {
	k_sem_give(&test_sem);
}
K_TIMER_DEFINE(test_timer, timer_handler, NULL);
