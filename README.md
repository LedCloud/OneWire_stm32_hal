## Library for STM32 HAL to use DS18B20 sensors connected to UART

This library could be used on stm32 microcontroller with HAL. 
The sensors connects to UART. The library doesn't use delays.

The library consits of two header files.
  * One for OneWire (OneWire.h) 
  * and the other for DS18B20 sensors (Ds18B20.h)

First of all set the desired UART to this mode: Asynchronous mode, 
global interrupt enabled, Baud Rate - any, it will be reset in the reset function,
8-n-1
```
//Like that
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
```

Connection of the sensor is tested with the following schematic:

![uart_ds18b20](https://user-images.githubusercontent.com/37952504/125068635-bdf49000-e0c6-11eb-8522-4c40bce92f84.png)

Example of using:

```
//Include the header
#include "Ds18B20.h"

//Create variable
Ds18B20_t ds18b20;


int main(void) {
  //Initialize the library
  //           struct of Ds18B20   handle of UART  Precicion of the sensor
  DS18B20_init(&ds18b20,           &huart3,        DS18B20_12BITS);

  //before the loop
  if (DS18B20_getSensorsAvailable(&ds18b20)) {
    DS18B20_startMeasure(&ds18b20, DS18B20_MEASUREALL);
    //Temperature convertion started
  } else {
    //No sensors found
  }
  //in the loop
  while (1)
  {
    uint8_t sensors = DS18B20_getSensorsAvailable(&ds18b20);

    if (sensors) {
      if (DS18B20_isTempReady(&ds18b20, 0)) {
        for (uint8_t s=0;s<sensors;s++) {
            int16_t tempRaw = DS18B20_getTempRaw(&ds18b20, s);
            //tempRaw - temperature in steps of 0.0625 degrees centigrade
        }
        //And start convertion again
        DS18B20_startMeasure(&ds18b20, DS18B20_MEASUREALL); 
      }
    } else {
      //There is no sensors discovered
    }
    HAL_Delay(250);
  } //end of while
} //end of main
```
