#include "OneWire.h"
#include "usart.h"

/**
 * internal function to reset uart when an error happens
 * 
 * It seems this function is unneeded when 8 bits are sent separatly
 */
void OW_resetUART(OneWire_t* ow)
{
	HAL_UART_DeInit(ow->huart);

	ow->huart->Instance = USART3;
	ow->huart->Init.BaudRate = 115200;
	ow->huart->Init.WordLength = UART_WORDLENGTH_8B;
	ow->huart->Init.StopBits = UART_STOPBITS_1;
	ow->huart->Init.Parity = UART_PARITY_NONE;
	ow->huart->Init.Mode = UART_MODE_TX_RX;
	ow->huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	ow->huart->Init.OverSampling = UART_OVERSAMPLING_16;
	
	HAL_UART_Init(ow->huart);

	ow->status = HAL_OK;
}

void OW_init(OneWire_t *ow, UART_HandleTypeDef *huart)
{
    /* Save settings */
    ow->huart = huart;
	ow->status = HAL_OK;
}

uint8_t bitsToByte(uint8_t *bits) {
    uint8_t target_byte, i;
    target_byte = 0;
    for (i = 0; i < 8; i++) {
        target_byte = target_byte >> 1;
        if (*bits != WIRE_0) {
            target_byte |= 0b10000000;
        }
        bits++;
    }
    return target_byte;
}

/// Convert one byte to array of 8 bytes
uint8_t *byteToBits(uint8_t ow_byte, uint8_t *bits) {
    uint8_t i;
    for (i = 0; i < 8; i++) {
        if (ow_byte & 0x01) {
            *bits = WIRE_1;
        } else {
            *bits = WIRE_0;
        }
        bits++;
        ow_byte = ow_byte >> 1;
    }
    return bits;
}

/**
 * If set baud rate with Deinit and Init there will be an unneeded byte 0xF0
 * on the bus
 */
void OW_setBaudRate(UART_HandleTypeDef *huart, uint32_t bdr)
{
    uint32_t pclk;
    if(huart->Instance == USART1)
    {
        pclk = HAL_RCC_GetPCLK2Freq();
    }
    else
    {
        pclk = HAL_RCC_GetPCLK1Freq();
    }
    __HAL_UART_DISABLE(huart); //Not sure it is needed
    huart->Instance->BRR = UART_BRR_SAMPLING16(pclk, bdr);
	__HAL_UART_ENABLE(huart); //Not sure it is needed
}

uint8_t OW_reset(OneWire_t *ow)
{
	//Reset UART if there is an error
	if (ow->status != HAL_OK) {
		OW_resetUART(ow);
	}

    uint8_t reset = 0xF0;
    uint8_t resetBack = 0;

    OW_setBaudRate(ow->huart, OW_RESET_SPEED);
    
	HAL_UART_Transmit_IT(ow->huart, &reset, 1);
    ow->status = HAL_UART_Receive(ow->huart, &resetBack, 1, OW_TIMEOUT);

    OW_setBaudRate(ow->huart, OW_WORK_SPEED);

    return reset!=resetBack;
}

void OW_sendBit(OneWire_t *ow, uint8_t b)
{
    uint8_t r,s;
    s = b ? WIRE_1 : WIRE_0;
    HAL_UART_Transmit_IT(ow->huart, &s, 1);
    ow->status = HAL_UART_Receive(ow->huart, &r, 1, OW_TIMEOUT);
}

uint8_t OW_receiveBit(OneWire_t *ow)
{
    uint8_t s = 0xFF, r;
    HAL_UART_Transmit_IT(ow->huart, &s, 1);
    ow->status = HAL_UART_Receive(ow->huart, &r, 1, OW_TIMEOUT);

	if (r==0xFF) return 1;

	return 0;
}

void OW_sendByte(OneWire_t *ow, uint8_t b)
{
    uint8_t sendByte[8];
    //uint8_t recvByte[8];

    byteToBits(b, sendByte); //0b01101001 => 0x00 0xFF 0xFF 0x00 0xFF 0x00 0x00 0xFF

	for(uint8_t i=0;i<8;i++) {
		OW_sendBit(ow, sendByte[i]);
	}
	/* 
	On a high loaded system there will be desynchronization of transmit and receive
	buffer. It will lead to timeout errors.
	I'm not sure if deinit and init will clear that error.
	Below the example that creates such error.
	*/
    //HAL_UART_Transmit_IT(ow->huart, sendByte, 8);
    //ow->status = HAL_UART_Receive(ow->huart, recvByte, 8, OW_TIMEOUT);
}

void OW_sendBytes(OneWire_t *ow, uint8_t *bytes, uint8_t len)
{
    for(uint8_t i=0;i<len;i++) {
		OW_sendByte(ow, bytes[i]);
    }
}

uint8_t OW_receiveByte(OneWire_t *ow)
{
    uint8_t sendByte[8];
    uint8_t recvByte[8];
    byteToBits(0xFF, sendByte);

	for (uint8_t i=0;i<8;i++) {
		recvByte[i] = OW_receiveBit(ow);
	}

    return bitsToByte(recvByte);
}

void OW_receiveBytes(OneWire_t *ow, uint8_t *bytes, uint8_t len)
{
    for(uint8_t i=0;i<len;i++) {
        bytes[i] = OW_receiveByte(ow);
    }
}

uint8_t OW_CRC8(uint8_t* addr, uint8_t len)
{
    uint8_t crc = 0, inbyte, i, mix;
	
	while (len--) {
		inbyte = *addr++;
		for (i = 8; i; i--) {
			mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8C;
			}
			inbyte >>= 1;
		}
	}
	
	/* Return calculated CRC */
	return crc;
}

void OW_resetSearch(OneWire_t *ow)
{
	/* Reset the search state */
	ow->LastDiscrepancy = 0;
	ow->LastDeviceFlag = 0;
	ow->LastFamilyDiscrepancy = 0;
}


uint8_t OW_first(OneWire_t *ow)
{
	/* Reset search values */
	OW_resetSearch(ow);

	/* Start with searching */
	return OW_search(ow, OW_CMD_SEARCHROM);
}


uint8_t OW_search(OneWire_t* ow, uint8_t command)
{
	uint8_t id_bit_number;
	uint8_t last_zero, rom_byte_number, search_result;
	uint8_t id_bit, cmp_id_bit;
	uint8_t rom_byte_mask, search_direction;

	/* Initialize for search */
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;

	/* Check if any devices */
	if (!ow->LastDeviceFlag) {
		/* 1-Wire reset */
		if (!OW_reset(ow)) {
			/* Reset the search */
            OW_resetSearch(ow);
			return 0; //Reset failed
		}

		/* Issue the search command */
        OW_sendByte(ow, command);
		
		/* Loop to do the search */
		do {
			/* Read a bit and its complement */
			id_bit = OW_receiveBit(ow); //0
			cmp_id_bit = OW_receiveBit(ow); //1

			/* Check for no devices on 1-wire */
			if ((id_bit == 1) && (cmp_id_bit == 1)) {
				break;
			} else {
				/* All devices coupled have 0 or 1 */
				if (id_bit != cmp_id_bit) {
					/* Bit write value for search */
					search_direction = id_bit; //1
				} else {
					/* If this discrepancy is before the Last Discrepancy on a previous next then pick the same as last time */
					if (id_bit_number < ow->LastDiscrepancy) {
						search_direction = ((ow->ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					} else {
						/* If equal to last pick 1, if not then pick 0 */
						search_direction = (id_bit_number == ow->LastDiscrepancy);
					}
					
					/* If 0 was picked then record its position in LastZero */
					if (search_direction == 0) {
						last_zero = id_bit_number;

						/* Check for Last discrepancy in family */
						if (last_zero < 9) {
							ow->LastFamilyDiscrepancy = last_zero;
						}
					}
				}

				/* Set or clear the bit in the ROM byte rom_byte_number with mask rom_byte_mask */
				if (search_direction == 1) { // 1
					ow->ROM_NO[rom_byte_number] |= rom_byte_mask; // |= 1
				} else {
					ow->ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				
				/* Serial number search direction write bit */
                OW_sendBit(ow, search_direction);  //1

				/* Increment the byte counter id_bit_number and shift the mask rom_byte_mask */
				id_bit_number++; // 1 -> 2
				rom_byte_mask <<= 1; // 0b10

				/* If the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask */
				if (rom_byte_mask == 0) {
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		/* Loop until through all ROM bytes 0-7 */
		} while (rom_byte_number < 8);

		/* If the search was successful then */
		if (!(id_bit_number < 65)) {
			/* Search successful so set LastDiscrepancy, LastDeviceFlag, search_result */
			ow->LastDiscrepancy = last_zero;

			/* Check for last device */
			if (ow->LastDiscrepancy == 0) {
				ow->LastDeviceFlag = 1;
			}

			search_result = 1;
		}
	}

	/* If no device found then reset counters so next 'search' will be like a first */
	if (!search_result || !ow->ROM_NO[0]) {
        OW_resetSearch(ow);
		search_result = 0;
	}

	return search_result;
}

uint8_t OW_next(OneWire_t* ow)
{
   /* Leave the search state alone */
   return OW_search(ow, OW_CMD_SEARCHROM);
}

uint8_t OW_getROM(OneWire_t* ow, uint8_t index)
{
	return ow->ROM_NO[index];
}

void OW_getFullROM(OneWire_t* ow, uint8_t *firstIndex)
{
    uint8_t i;
	for (i = 0; i < 8; i++) {
		*(firstIndex + i) = ow->ROM_NO[i];
	}
}

void OW_select(OneWire_t* ow, uint8_t* addr)
{
    uint8_t i;
	OW_sendByte(ow, OW_CMD_MATCHROM);
	
	for (i = 0; i < 8; i++) {
		OW_sendByte(ow, *(addr + i));
	}
}

void OW_selectWithPointer(OneWire_t* ow, uint8_t* ROM)
{
    uint8_t i;
	OW_sendByte(ow, OW_CMD_MATCHROM);
	
	for (i = 0; i < 8; i++) {
		OW_sendByte(ow, *(ROM + i));
	}
}
