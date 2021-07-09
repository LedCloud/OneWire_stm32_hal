#ifndef _DS18B10_h
#define _DS18B10_h

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif


/**
 ******************************************************************************
 * @file           : OneWire.h
 * @brief          : HAL library to work with OneWire bus
 ******************************************************************************
 * @attention
 * Copyright 2021 Konstantin Toporov
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom 
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ********************* Description *************************
 * This library use the library OneWire to communicate with DS18B20 
 * temperature sensors.
 * 
 * Create Ds18B20_t structure and pass it to Init function.
 * @attention there is no need to create OneWire_t structure
 */
#include "stm32f1xx_hal.h"
#include "OneWire.h"

/**
 * Maxumum devices that could be used. Define more before including this file
 */
#ifndef MAX_DS18B20_SENSORS
#define MAX_DS18B20_SENSORS 3
#endif

///Use this to select all devices on the bus
#define DS18B20_MEASUREALL 0xff

/** Since the minimal temperature that the sensor can measure is -55 degrees centigade
 * (raw value -880) this constants is used to indicate the errors
 */
#define DS18B20_TEMP_NOT_READ -1000
#define DS18B20_TEMP_ERROR -1500
#define DS18B20_TEMP_CRC_ERROR -1550

/**
 *  The precision of the sensor and therefor the time for converting the temperature
 */
#define DS18B20_12BITS 0b01111111 //750ms
#define DS18B20_11BITS 0b01011111 //375ms
#define DS18B20_10BITS 0b00111111 //187.5ms
#define DS18B20_9BITS  0b00011111 //93.75ms


/* OneWire commands */
#define DS18B20_CMD_READSCRATCHPAD		0xBE
#define DS18B20_CMD_WRITESCRATCHPAD		0x4E
#define DS18B20_CMD_CPYSCRATCHPAD		0x48
#define DS18B20_CMD_RECEEPROM			0xB8
#define DS18B20_CMD_RPWRSUPPLY			0xB4
#define DS18B20_CMD_SEARCHROM			0xF0
#define DS18B20_CMD_READROM				0x33
#define DS18B20_CMD_MATCHROM			0x55
#define DS18B20_CMD_SKIPROM				0xCC

#define DS18B20_COVERTTEMP 0x44

/**
 * @brief  Ds18B20 working struct
 * @note   It is fully private and should not be touched by user
 */
typedef struct {
    uint8_t sensors_found;
    int16_t correction[MAX_DS18B20_SENSORS];
    uint32_t lastTimeMeasured[MAX_DS18B20_SENSORS];
    uint8_t ROMS[MAX_DS18B20_SENSORS][8];
    uint16_t timeNeeded;
    OneWire_t ow;
} Ds18B20_t;

/**
 * @brief Initialization of the library
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure
 * @param *huart: Handle to UART
 * @param precision: select one from defined precision. It will be set all the same for all found sensors
 */
void DS18B20_init(Ds18B20_t *ds18B20, UART_HandleTypeDef *huart, uint8_t precision);

/**
 * @brief Returns the amount of found sensors in @ref DS18B20_init
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure
 * @retval amount of found sensors
 */
uint8_t DS18B20_getSensorsAvailable(Ds18B20_t *ds18B20);

/**
 * Use this function to start convertion. The temperature will be avalable after the
 * time needed for the covertion. It depends on the precition set in @ref DS18B20_init
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure
 * @param sensor: index of the sensor, if specified DS18B20_MEASUREALL the
 * convertion will be started on all connected sensors.
 */
void DS18B20_startMeasure(Ds18B20_t *ds18B20, uint8_t sensor);

/**
 * Check whether the tempereture could be read from the sensor.
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure
 * @param sensor: index of the sensor. It specified DS18B20_MEASUREALL, the time will be checked on first sensor.
 * @retval 1 - temperature could be read, 0 - not ready, keep waiting
 */
uint8_t DS18B20_isTempReady(Ds18B20_t *ds18B20, uint8_t sensor);

/**
 * Retrieve the temperature in the raw value.
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure
 * @param sensor: index of the sensor
 * @retval the temperature in steps of 0.0625 degrees centigade.
 */
int16_t DS18B20_getTempRaw(Ds18B20_t *ds18B20, uint8_t sensor);

/**
 * Set the correction of the sensor in raw value. The temperature returned by @ref DS18B20_getTempRaw will be corrected by this value
 * @param *ds18B20: Pointer to @ref Ds18B20_t working Ds18B20 structure 
 * @param sensor: index of the sensor to apply the correction
 * @param cor: correction in steps of 0.0625 degrees centigade.
 */
void DS18B20_setCorrection(Ds18B20_t *ds18B20, uint8_t sensor, int16_t cor);

///Convertion of the raw value to degrees centigare
inline static double DS18B20_convertToDouble(int16_t t){return (double)t * 0.0625;};
///Convertion of degrees centigare to raw value
inline static int16_t DS18B20_convertToInt(double t){return (int16_t)((t/0.0625)+0.5);};


/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif