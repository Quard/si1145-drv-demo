#define DT_DRV_COMPAT silabs_si1145

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "si1145.h"


LOG_MODULE_REGISTER(SI1145, CONFIG_SENSOR_LOG_LEVEL);

static int si1145_reg_read(const struct si1145_config *conf, uint8_t reg, uint8_t *val, size_t len) {
	if (i2c_burst_read_dt(&conf->i2c, reg, val, len) < 0) {
		return -EIO;
	}

	return 0;
}

static int si1145_reg_write(const struct si1145_config *conf, uint8_t reg, uint8_t val) {
	uint8_t buf[2] = {reg, val};

	return i2c_write_dt(&conf->i2c, buf, 2);
}

static int si1145_write_param(const struct si1145_config *conf, uint8_t param, uint8_t value) {
	si1145_reg_write(conf, SI1145_REG_PARAM_WR, value);
	si1145_reg_write(conf, SI1145_REG_COMMAND, param | SI1145_REG_COMMAND);
	return si1145_reg_read(conf, SI1145_REG_PARAM_RD, &value, 1);
}

static int si1145_reset(const struct si1145_config *conf) {
	si1145_reg_write(conf, SI1145_REG_MEAS_RATE, 0);
	si1145_reg_write(conf, SI1145_REG_IRQ_ENABLE, 0);
	si1145_reg_write(conf, SI1145_REG_IRQ_MODE1, 0);
	si1145_reg_write(conf, SI1145_REG_IRQ_MODE2, 0);
	si1145_reg_write(conf, SI1145_REG_INT_CFG, 0);
	si1145_reg_write(conf, SI1145_REG_IRQ_STATUS, 0xFF);
	si1145_reg_write(conf, SI1145_REG_COMMAND, SI1145_CMD_RESET);
	k_sleep(K_MSEC(10));
	si1145_reg_write(conf, SI1145_REG_HW_KEY, SI1145_HWKEY);
	k_sleep(K_MSEC(10));

	return 0;
}

static int si1145_config_uv(const struct si1145_config *conf) {
	si1145_reg_write(conf, SI1145_REG_UCOEF0, 0x29);
	si1145_reg_write(conf, SI1145_REG_UCOEF1, 0x89);
	si1145_reg_write(conf, SI1145_REG_UCOEF2, 0x02);
	si1145_reg_write(conf, SI1145_REG_UCOEF3, 0x00);

	si1145_write_param(conf, SI1145_PARAM_CHLIST, SI1145_PARAM_CHLIST__EN_EV);
	si1145_write_param(conf, SI1145_PARAM_ALS_VIS_ADC_MISC, SI1145_PARAM_ALS_VIS_ADC_MISC__VIS_RANGE);
	si1145_write_param(conf, SI1145_PARAM_ALS_IR_ADC_MISC, SI1145_PARAM_ALS_IR_ADC_MISC__IR_RANGE);

	return 0;
}

static int si1145_start(const struct si1145_config *conf) {
	si1145_write_param(conf, SI1145_REG_MEAS_RATE, 0xFF);
  	si1145_write_param(conf, SI1145_REG_COMMAND, SI1145_CMD_PSALS_AUTO);

	return 0;
}

static int si1145_sample_fetch(const struct device *dev, enum sensor_channel chan) {
	struct si1145_data *drv_data = dev->data;
	const struct si1145_config *cfg = dev->config;

	return si1145_reg_read(cfg, SI1145_REG_AUX_DATA0, (uint8_t *)&drv_data->uv, 2);
}

static int si1145_channel_get(const struct device *dev,
			      enum sensor_channel ch,
			      struct sensor_value *val) {
	struct si1145_data *drv_data = dev->data;

	if (ch == SENSOR_CHAN_BLUE) {

	} else if (ch) {
		return -ENOTSUP;
	}

	val->val1 = drv_data->uv;

	return 0;
}

static const struct sensor_driver_api si1145_driver_api = {
	.sample_fetch = si1145_sample_fetch,
	.channel_get = si1145_channel_get,
};

int si1145_init(const struct device *dev) {
	const struct si1145_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->i2c.bus->name);
		return -EINVAL;
	}

	uint8_t part_id;
	si1145_reg_read(cfg, SI1145_REG_PART_ID, &part_id, 1);
	if (part_id != SI1145_PART_ID) {
		printk("Si1145 PART ID is wrong");
		return -EIO;
	}
	si1145_reset(cfg);
	si1145_config_uv(cfg);
	si1145_start(cfg);

	return 0;
}

#define SI1145_INST(inst)						    \
	static struct si1145_data si1145_data_##inst;			    \
	static const struct si1145_config si1145_config_##inst = {	    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			    \
	};								    \
									    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, si1145_init, NULL, &si1145_data_##inst, \
			      &si1145_config_##inst, POST_KERNEL,	    \
			      CONFIG_SENSOR_INIT_PRIORITY, &si1145_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SI1145_INST)