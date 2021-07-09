#ifndef ONE_WIRE_UART_LIB_h
#define ONE_WIRE_UART_LIB_h

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
 * This library is based on the labrary taken from 
 * https://stm32f4-discovery.net/2015/07/hal-library-05-onewire-for-stm32fxxx/
 * Unlike that library this one doesn't use delays.
 * It use system UART to communicate with OneWire device(s).
 * 
 * UART must be set before calling this function
 * Settings of UART:
 * Asynchronous mode, global interrupt enabled.
 * Baud Rate - any, it will be reset in the reset function.
 * 8-n-1
 * Connection of RX & TX should be done as specified here 
 * https://www.maximintegrated.com/en/design/technical-documents/tutorials/2/214.html
 * or use a diod to connect RX and TX to 1Wire bus
 * +3.3V----+
 *          |
 *          П 4.7k Resistor
 *          U
 *          |
 * RX ------+-------- to 1Wire bus
 *          |
 * TX ---K|-+ Diod Cathode to TX pin
 * 
 *  -K|- any fast switching diod for exmaple FR103
 * 
 */

#include "stm32f1xx_hal.h"

#define WIRE_1 0xFF
#define WIRE_0 0x00

#define OW_RESET_SPEED 9600
#define OW_WORK_SPEED 115200

#define OW_TIMEOUT 5 //1 ms is enouph

/* OneWire commands */
#define OW_CMD_RSCRATCHPAD			0xBE
#define OW_CMD_WSCRATCHPAD			0x4E
#define OW_CMD_CPYSCRATCHPAD		0x48
#define OW_CMD_RECEEPROM			0xB8
#define OW_CMD_RPWRSUPPLY			0xB4
#define OW_CMD_SEARCHROM			0xF0
#define OW_CMD_READROM				0x33
#define OW_CMD_MATCHROM			    0x55
#define OW_CMD_SKIPROM				0xCC

/**
 * @brief  OneWire working struct
 * @note   Except ROM_NO member, everything is fully private and should not be touched by user
 */
typedef struct {
	uint8_t LastDiscrepancy;       /*!< Search private */
	uint8_t LastFamilyDiscrepancy; /*!< Search private */
	uint8_t LastDeviceFlag;        /*!< Search private */
	uint8_t ROM_NO[8];             /*!< 8-bytes address of last search device */
	UART_HandleTypeDef *huart;
	HAL_StatusTypeDef status;
} OneWire_t;

/**
 * @brief Initialization of library. Do it before using
 * @par ow - pointer to OneWire_t structure
 * @par huart pointer to UART handle
 */
void OW_init(OneWire_t *ow, UART_HandleTypeDef *huart);

/**
 * @brief Each communication with OneWire bus must start with
 * this function.
 * @par	   ow - pointer to OneWire_t structure
 * @return 1 - some devices discovered, 0 - no devices on the bus
 */
uint8_t OW_reset(OneWire_t *ow);

/**
 * @brief   Send one byte through OneWire bus
 * @par	    ow - pointer to OneWire_t structure
 * @par     b - bytes to send
 */
void OW_sendByte(OneWire_t *ow, uint8_t b);

/**
 * @brief   Send some bytes through OneWire bus
 * @par	    ow - pointer to OneWire_t structure
 * @par     *bytes - array of bytes to send
 * @par     len - length of array
 */
void OW_sendBytes(OneWire_t *ow, uint8_t *bytes, uint8_t len);

/**
 * @brief   Receive one byte through OneWire bus
 * @par	    ow - pointer to OneWire_t structure
 * @return  received byte
 */
uint8_t OW_receiveByte(OneWire_t *ow);

/**
 * @brief   Receive some bytes from OneWire bus
 * @par	    ow - pointer to OneWire_t structure
 * @par     *bytes - array to fill
 * @par     len - length of bytes
 */
void OW_receiveBytes(OneWire_t *ow, uint8_t *bytes, uint8_t len);

/**
 * @brief  Calculates 8-bit CRC for 1-wire devices
 * @par    *addr: Pointer to 8-bit array of data to calculate CRC
 * @par    len: Number of bytes to check
 *
 * @return Calculated CRC from input data
 */
uint8_t OW_CRC8(uint8_t* addr, uint8_t len);

/**
 * @brief  Starts search, reset states first
 * @note   When you want to search for ALL devices on one onewire port, you should first use this function.
\code
//...Initialization before
status = OW_first(&OneWireStruct);
while (status) {
	//Save ROM number from device
	OW_getFullROM(&OneWireStruct, ROM_Array_Pointer);
	//Check for new device
	status = OW_next(&OneWireStruct);
}
\endcode
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire where to reset search values
 * @retval  Device status:
 *            - 0: No devices detected
 *            - > 0: Device detected
 */
uint8_t OW_first(OneWire_t *ow);

/**
 * @brief  Resets search states
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire where to reset search values
 */
void OW_resetSearch(OneWire_t *ow);

/**
 * @brief  Searches for OneWire devices on specific Onewire port
 * @note   Not meant for public use. Use @ref OW_first and @ref OW_next for this.
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire structure where to search
 * @param command - command to send to OneWire devices
 * @retval Device status:
 *            - 0: No devices detected
 *            - > 0: Device detected
 */
uint8_t OW_search(OneWire_t* ow, uint8_t command);

/**
 * @brief  Reads next device
 * @note   Use @ref OW_first to start searching
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire
 * @retval  Device status:
 *            - 0: No devices detected any more
 *            - > 0: New device detected
 */
uint8_t OW_next(OneWire_t* ow);

/**
 * @brief  Gets ROM number from device from search
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire
 * @param  index: Because each device has 8-bytes long ROM address, you have to call this 8 times, to get ROM bytes from 0 to 7
 * @retval ROM byte for index (0 to 7) at current found device
 */
uint8_t OW_getROM(OneWire_t* ow, uint8_t index);

/**
 * @brief  Gets all 8 bytes ROM value from device from search
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire
 * @param  *firstIndex: Pointer to first location for first byte, other bytes are automatically incremented
 */
void OW_getFullROM(OneWire_t* ow, uint8_t *firstIndex);

/**
 * @brief  Selects specific slave on bus
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire
 * @param  *addr: Pointer to first location of 8-bytes long ROM address
 */
void OW_select(OneWire_t* ow, uint8_t* addr);

/**
 * @brief  Selects specific slave on bus with pointer address
 * @param  *OneWireStruct: Pointer to @ref OneWire_t working onewire
 * @param  *ROM: Pointer to first byte of ROM address
 */
void OW_selectWithPointer(OneWire_t* ow, uint8_t* ROM);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* of ONE_WIRE_UART_LIB_h */