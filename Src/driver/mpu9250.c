/**
 * @file   MPU-9250.c
 * @brief  9-axis sensor(InvenSense MPU-9250) driver for TZ10xx.
 *
 * @author Cerevo Inc.
 */

/*
Copyright 2015 Cerevo Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/** Includes **/
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "mpu9250.h"

static uint8_t magnetometer_calib[3];
static float gyro_div;
static float accel_div;


typedef enum {
    MPU9250_STAT_NONE = 0,
    MPU9250_STAT_IDLE,
    MPU9250_STAT_MAESUREING,
} MPU9250_STAT;

static MPU9250_STAT stat = MPU9250_STAT_NONE;

/*
 * Read byte data from SPI
 */
static bool mpu9250_drv_read_byte(struct mpu9250_dev *dev, uint8_t addr, uint8_t *val)
{
    uint8_t temp = addr;
    temp |= 0x80;
    HAL_GPIO_WritePin(dev->gpio_port, dev->pin_cs, GPIO_PIN_RESET);
    HAL_SPI_Transmit(dev->hspi,(uint8_t*) &temp, 1, 100);
	HAL_SPI_Receive(dev->hspi,(uint8_t*) val, 1, 100);
	HAL_GPIO_WritePin(dev->gpio_port, dev->pin_cs, GPIO_PIN_SET);
    return true;
}

/*
 * Write byte data to SPI
 */
static uint16_t mpu9250_drv_write_byte(struct mpu9250_dev *dev, uint8_t addr, uint8_t val)
{
	HAL_GPIO_WritePin(dev->gpio_port, dev->pin_cs, GPIO_PIN_RESET);
	HAL_SPI_Transmit(dev->hspi,(uint8_t*) &addr, 1, 100);
	HAL_SPI_Transmit(dev->hspi,(uint8_t*) &val, 1, 100);
	HAL_GPIO_WritePin(dev->gpio_port, dev->pin_cs, GPIO_PIN_SET);
}

/*
 * Read request to MPU9250 internal I2C Master.
 */
static bool mpu9250_drv_i2c_slv0_read_request(uint8_t slv_addr, uint8_t slv_reg, uint8_t data_len)
{
    /* set read flag & slave address. */
    //if (//mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_ADDR, slv_addr | 0x80) == false) {
    //    return false;
    //}
    /* set register address. */
   // if (//mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_REG, slv_reg) == false) {
   //     return false;
   // }
    /* transfer */
   // if (//mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_CTRL, 0x80 | (data_len & 0x0f)) == false) {
   //     return false;
  //  }

    return true;
}

/*  API FUNCTIONS  */

/*
 * Initialize MPU9250 9axis sensor.
 * see: MPU-9250 Product Specification 7.5 SPI interface.
 */
bool MPU9250_drv_init(struct mpu9250_dev *dev)
{
    uint8_t val = 0x00;

    if (dev == NULL) {
        return false;
    }

    /* MPU9250 reset & initial */
    for (int i = 0; i < 100; i++) {
        mpu9250_drv_read_byte(dev, MPU9250_REG_WHO_AM_I, &val);
        if (val == 0x71) {
            break;
        }
        //Usleep(100000);  // 100ms
    }
    if (val != 0x71) {
        return false;
    }


	mpu9250_drv_write_byte(dev, MPU9250_REG_PWR_MGMT_1, 0x80);
	mpu9250_drv_write_byte(dev, MPU9250_REG_PWR_MGMT_1, 0x01);
	mpu9250_drv_write_byte(dev, MPU9250_REG_PWR_MGMT_2, 0x3f);
	mpu9250_drv_write_byte(dev, MPU9250_REG_INT_PIN_CFG, 0x30);

    stat = MPU9250_STAT_IDLE;   // Update STATE

    return true;
}

/*
 * Start maesure.
 */
bool MPU9250_drv_start_maesure(MPU9250_BIT_GYRO_FS_SEL gyro_fs, MPU9250_BIT_ACCEL_FS_SEL accel_fs, MPU9250_BIT_DLPF_CFG dlpf_cfg, MPU9250_BIT_A_DLPFCFG a_dlpfcfg)
{
    if (stat != MPU9250_STAT_IDLE) {
        return false;
    }

    uint8_t init_conf[][2] = {
        {MPU9250_REG_PWR_MGMT_2,    0x00},                      /* Enable Accel & Gyro */
        {MPU9250_REG_CONFIG,        (uint8_t)dlpf_cfg},         /* Gyro LPF */
        {MPU9250_REG_GYRO_CONFIG,   (uint8_t)gyro_fs},          /* Gyro configuration */
        {MPU9250_REG_ACCEL_CONFIG,  (uint8_t)accel_fs},         /* Accel configuration */
        {MPU9250_REG_ACCEL_CONFIG2, 0x08 | (uint8_t)a_dlpfcfg}, /* Accel LPF */
        {0xff,                      0xff}
    };
    for (int i = 0; init_conf[i][0] != 0xff; i++) {
        //mpu9250_drv_write_byte(init_conf[i][0], init_conf[i][1]);
        Usleep(1000);
    }

    switch (gyro_fs) {
    case MPU9250_BIT_GYRO_FS_SEL_250DPS:
        gyro_div = 131.0;
        break;
    case MPU9250_BIT_GYRO_FS_SEL_500DPS:
        gyro_div = 65.5;
        break;
    case MPU9250_BIT_GYRO_FS_SEL_1000DPS:
        gyro_div = 32.8;
        break;
    case MPU9250_BIT_GYRO_FS_SEL_2000DPS:
        gyro_div = 16.4;
        break;
    default:
        gyro_div = 131.0;
        break;
    }

    switch (accel_fs) {
    case MPU9250_BIT_ACCEL_FS_SEL_2G:
        accel_div = 16384;
        break;
    case MPU9250_BIT_ACCEL_FS_SEL_4G:
        accel_div = 8192;
        break;
    case MPU9250_BIT_ACCEL_FS_SEL_8G:
        accel_div = 4096;
        break;
    case MPU9250_BIT_ACCEL_FS_SEL_16G:
        accel_div = 2048;
        break;
    default:
        accel_div = 16384;
        break;
    }

    stat = MPU9250_STAT_MAESUREING; /* Update STATE. */
    return true;
}

/*
 * Stop maesure.
 */
bool MPU9250_drv_stop_maesure(void)
{
    if (stat != MPU9250_STAT_MAESUREING) {
        return false;
    }

    //mpu9250_drv_write_byte(MPU9250_REG_PWR_MGMT_2, 0x3f);   /* Disable Accel & Gyro */
    Usleep(1000);

    stat = MPU9250_STAT_IDLE;   /* Update STATE. */
    return true;
}

/*
 * Read Gyro.
 */
bool MPU9250_drv_read_gyro(MPU9250_gyro_val *gyro_val)
{
    uint8_t vals[6];

    if (stat != MPU9250_STAT_MAESUREING) {
        return false;
    }

    if (gyro_val == NULL) {
        return false;
    }

    for (int i = 0; i < sizeof(vals); i++) {
        //mpu9250_drv_read_byte(MPU9250_REG_GYRO_XOUT_HL + i, &vals[i]);
    }

    gyro_val->raw_x = ((uint16_t)vals[0] << 8) | vals[1];
    gyro_val->raw_y = ((uint16_t)vals[2] << 8) | vals[3];
    gyro_val->raw_z = ((uint16_t)vals[4] << 8) | vals[5];

    gyro_val->x = (float)(int16_t)gyro_val->raw_x / gyro_div;
    gyro_val->y = (float)(int16_t)gyro_val->raw_y / gyro_div;
    gyro_val->z = (float)(int16_t)gyro_val->raw_z / gyro_div;

    return true;
}

/*
 * Read Accel.
 */
bool MPU9250_drv_read_accel(MPU9250_accel_val *accel_val)
{
    uint8_t vals[6];

    if (stat != MPU9250_STAT_MAESUREING) {
        return false;
    }

    if (accel_val == NULL) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        //mpu9250_drv_read_byte(MPU9250_REG_ACCEL_XOUT_HL + i, &vals[i]);
    }

    accel_val->raw_x = ((uint16_t)vals[0] << 8) | vals[1];
    accel_val->raw_y = ((uint16_t)vals[2] << 8) | vals[3];
    accel_val->raw_z = ((uint16_t)vals[4] << 8) | vals[5];

    accel_val->x = (float)(int16_t)accel_val->raw_x / accel_div;
    accel_val->y = (float)(int16_t)accel_val->raw_y / accel_div;
    accel_val->z = (float)(int16_t)accel_val->raw_z / accel_div;

    return true;
}

/*
 * Read chip temperature.
 */
bool MPU9250_drv_read_temperature(MPU9250_temperature_val *temperature_val)
{
    uint8_t val[2];

    if (stat != MPU9250_STAT_MAESUREING) {
        return false;
    }

    if (temperature_val == NULL) {
        return false;
    }

    ////mpu9250_drv_read_byte(MPU9250_REG_TEMP_HL, &val[0]);
    //mpu9250_drv_read_byte(MPU9250_REG_TEMP_HL + 1, &val[1]);

    temperature_val->raw = ((uint16_t)val[0] << 8) | val[1];

    return true;
}

/*
 * Read Magnetometer.
 */
bool MPU9250_drv_read_magnetometer(MPU9250_magnetometer_val *magnetometer_val)
{
    uint8_t vals[8];

    if (stat != MPU9250_STAT_MAESUREING) {
        return false;
    }

    if (magnetometer_val == NULL) {
        return false;
    }
    /* set read flag & slave address. */
    //mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_ADDR, AK8963_I2C_ADDR | 0x80);
    /* set register address. */
    //mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_REG, AK8963_REG_ST1);
    /* transfer */
    //mpu9250_drv_write_byte(MPU9250_REG_I2C_SLV0_CTRL, 0x88);

    Usleep(1000);
    for (int i = 0; i < sizeof(vals); i++) {
        //mpu9250_drv_read_byte(MPU9250_REG_EXT_SENS_DATA_00 + i, &vals[i]);
    }

    if ((vals[7] & 0x08) != 0) {
        //detect overflow
        return false;
    }
    /* RAW data */
    magnetometer_val->raw_x = ((uint16_t)vals[2] << 8) | vals[1];
    magnetometer_val->raw_y = ((uint16_t)vals[4] << 8) | vals[3];
    magnetometer_val->raw_z = ((uint16_t)vals[6] << 8) | vals[5];
    /* Real data */
    magnetometer_val->x = (int16_t)magnetometer_val->raw_x * ((((float)(int8_t)magnetometer_calib[0] - 128) / 256) + 1);
    magnetometer_val->y = (int16_t)magnetometer_val->raw_y * ((((float)(int8_t)magnetometer_calib[1] - 128) / 256) + 1);
    magnetometer_val->z = (int16_t)magnetometer_val->raw_z * ((((float)(int8_t)magnetometer_calib[2] - 128) / 256) + 1);

    return true;
}
