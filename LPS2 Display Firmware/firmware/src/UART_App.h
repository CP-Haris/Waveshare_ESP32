#include "Timer.h"
#include "UART_BIOS.h"

#ifndef _UART_H_
#define _UART_H_

bool COM_GetRXData(UART_Buffer * Buffer);
void SendMSG(uint8_t* SRC, uint8_t SRC_Length, uint8_t UartSelect);
#endif 