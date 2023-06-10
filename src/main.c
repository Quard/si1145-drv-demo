/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>


void main(void) {
	printk("Hello World! %s\n", CONFIG_BOARD);

	// const struct device *const dev = DEVICE_DT_GET_ANY(silabs_si1145);
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(uvsens));
	// const struct device *const dev = DEVICE_DT_GET_ANY(i2c1);
	if (dev == NULL) {
		printk("Failed to get device binding");
		return;
	} else if (!device_is_ready(dev)) {
		printk("Device %s is not ready", dev->name);
		return;
	}

	int err;
	struct sensor_value uv;
	while (1) {
		err = sensor_sample_fetch(dev);
		if (err) {
			printk("Failed to read data from Si1145: %d\n", err);
			return;
		}
		err = sensor_channel_get(dev, SENSOR_CHAN_BLUE, &uv);
		if (err) {
			printk("Unable to get UV data %d\n", err);
			return;
		}

		printk("UV: %d\n", uv.val1);

		k_sleep(K_MSEC(5000));
	}
}
