#include "Ds18B20.h"
#include "OneWire.h"

void DS18B20_init(Ds18B20_t *ds18B20, UART_HandleTypeDef *huart, uint8_t precision)
{   
    uint8_t i=0;
    //fill with zeros
    uint8_t *p = (uint8_t*)ds18B20;
    for(;i<sizeof(Ds18B20_t);i++) {
        p[i] = 0;
    }
    //OneWire initialization
    OW_init(&ds18B20->ow, huart);

    uint8_t status = OW_first(&ds18B20->ow);

    //Looking for the sensors while there are avalable and their amount could be stored
    while (status && ds18B20->sensors_found<MAX_DS18B20_SENSORS) {
        //Save all ROMs
        OW_getFullROM(&ds18B20->ow, ds18B20->ROMS[ds18B20->sensors_found]);
        //Check the CRC
        if (OW_CRC8(ds18B20->ROMS[ds18B20->sensors_found], 7) == ds18B20->ROMS[ds18B20->sensors_found][7]) {
            ds18B20->sensors_found++;
        }
        //Looking for the next
        status = OW_next(&ds18B20->ow);
    }

    //Set the precition for all sensors at once
    uint8_t data[] = {  OW_CMD_SKIPROM, 
                        OW_CMD_WSCRATCHPAD, 
                        0x7F, //0b0111 1111 //temp high
                        0xFF, //0b1111 1111 //temp low 
                        precision };

    if (OW_reset(&ds18B20->ow)) {
        //Select all sensors. It's faster
        OW_sendBytes(&ds18B20->ow, data, sizeof(data));
    }

    switch (precision) {
        case DS18B20_11BITS : ds18B20->timeNeeded = 380; break;
        case DS18B20_10BITS : ds18B20->timeNeeded = 195; break;
        case DS18B20_9BITS  : ds18B20->timeNeeded = 100; break;
        case DS18B20_12BITS : 
        default:
                              ds18B20->timeNeeded = 760; break;
    }
}

uint8_t DS18B20_getSensorsAvailable(Ds18B20_t *ds18B20)
{
    return ds18B20->sensors_found;
}

void DS18B20_startMeasure(Ds18B20_t *ds18B20, uint8_t sensor)
{
    if (sensor<ds18B20->sensors_found || sensor==DS18B20_MEASUREALL) {
        uint32_t now = HAL_GetTick();
        if (sensor == DS18B20_MEASUREALL) {
            if (OW_reset(&ds18B20->ow)) {
                //Select all sensors. It's faster
                OW_sendByte(&ds18B20->ow, OW_CMD_SKIPROM);
                OW_sendByte(&ds18B20->ow, DS18B20_COVERTTEMP);
                for(uint8_t i=0;i<ds18B20->sensors_found;i++) {
                    ds18B20->lastTimeMeasured[i] = now;
                }
            }
        } else {
            if (OW_reset(&ds18B20->ow)) {
                OW_select(&ds18B20->ow, ds18B20->ROMS[sensor]);
                OW_sendByte(&ds18B20->ow, DS18B20_COVERTTEMP);
                ds18B20->lastTimeMeasured[sensor] = now;
            }
        }
    }
}

uint8_t DS18B20_isTempReady(Ds18B20_t *ds18B20, uint8_t sensor)
{
    if (sensor != DS18B20_MEASUREALL && sensor>=ds18B20->sensors_found) return 0;

    if (sensor == DS18B20_MEASUREALL) sensor = 0;
    return ((HAL_GetTick() - ds18B20->lastTimeMeasured[sensor])>=ds18B20->timeNeeded);
}

int16_t DS18B20_getTempRaw(Ds18B20_t *ds18B20, uint8_t sensor)
{
    if (sensor != DS18B20_MEASUREALL && sensor>=ds18B20->sensors_found) 
        return DS18B20_TEMP_NOT_READ;
    
    if (!OW_reset(&ds18B20->ow)){
        return DS18B20_TEMP_NOT_READ;
    }
    
    if (sensor == DS18B20_MEASUREALL) {
        OW_sendByte(&ds18B20->ow, DS18B20_CMD_SKIPROM);
    } else {
        OW_select(&ds18B20->ow, ds18B20->ROMS[sensor]);
    }
    OW_sendByte(&ds18B20->ow, OW_CMD_RSCRATCHPAD);
    
    uint8_t data[12];
    uint16_t s = 0;
    
    for (uint8_t j = 0; j < 9; j++) {           // we need 9 bytes
        data[j] = OW_receiveByte(&ds18B20->ow);
        s += data[j];
    }
    //The CRC algorithm has an error. If all bytes are zeros the CRC will be ok
    //So this check is agains it
    if (s==0) {
        return DS18B20_TEMP_CRC_ERROR;
    }

    if (OW_CRC8(data, 8) != data[8]) {
        return DS18B20_TEMP_ERROR;
    }
    
    //temp calculation
    int16_t raw = (data[1] << 8) | data[0];
    uint8_t cfg = (data[4] & 0x60);

    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
    return raw + ds18B20->correction[sensor==DS18B20_MEASUREALL?0:sensor];
}

void DS18B20_setCorrection(Ds18B20_t *ds18B20, uint8_t sensor, int16_t cor)
{
    if (sensor>=ds18B20->sensors_found) return;
    ds18B20->correction[sensor] = cor;
}