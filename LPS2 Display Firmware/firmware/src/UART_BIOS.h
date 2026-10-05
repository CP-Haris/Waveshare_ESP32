#ifndef UART1_H
#define UART1_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "device.h"

#define UART_FrequencyGet()        (uint32_t)(10000000UL)

#define UART_BLE    2
#define UART_CTRL   1
#define UART_UNDEFINED    0

#define EOT 0x04
#define SOH 0x01
#define DLE 0x10

#define RX_MSG_SIZE        20
#define CHARS_PER_LOOP     35

#define UART_READ_BUFFER_SIZE      512//128 //400
#define UART_WRITE_BUFFER_SIZE     512//324 //400


#define BLUETOOTH_CODE

// Struct UART2_Buffer is declared with UART_BUFFER_SIZE, but checked for UART2_BUFFER_SIZE; they must remain identical
// Also, UART_ReadCountGet() and UART_WritePendingBytesGet() checks only UART_BUFFER_SIZE, hence would be incorrect if BUFFER1 and BUFFER2 are not identical
#ifdef BLUETOOTH_CODE
#define UART2_READ_BUFFER_SIZE      512//128 //400
#define UART2_WRITE_BUFFER_SIZE     512//128 //400
#endif


#define UART1_RX_INT_DISABLE()      IEC1CLR = _IEC1_U1RXIE_MASK;
#define UART1_RX_INT_ENABLE()       IEC1SET = _IEC1_U1RXIE_MASK;
#define UART1_TX_INT_DISABLE()      IEC1CLR = _IEC1_U1TXIE_MASK;
#define UART1_TX_INT_ENABLE()       IEC1SET = _IEC1_U1TXIE_MASK;

#define UART2_RX_INT_DISABLE()      IEC1CLR = _IEC1_U2RXIE_MASK;
#define UART2_RX_INT_ENABLE()       IEC1SET = _IEC1_U2RXIE_MASK;
#define UART2_TX_INT_DISABLE()      IEC1CLR = _IEC1_U2TXIE_MASK;
#define UART2_TX_INT_ENABLE()       IEC1SET = _IEC1_U2TXIE_MASK;


//Error Enums
typedef enum {
    UART_ERROR_NONE     = 0,
    UART_ERROR_OVERRUN  = 0x00000002,
    UART_ERROR_FRAMING  = 0x00000004,
    UART_ERROR_PARITY   = 0x00000008
} UART_ERROR;

//Example Callback
//typedef void (* Callback)(void);
//Callback CALLBACK_RX_Buffer_Overflow;

//Buffer Container
typedef unsigned char (*PTR_ReadFunc)(void);

typedef struct UART_Buffer {
    unsigned char ReadBuffer[UART_READ_BUFFER_SIZE];
    unsigned char WriteBuffer[UART_WRITE_BUFFER_SIZE];
    PTR_ReadFunc ReadByteFunction;
    unsigned char Msg_Buffer[20];
    unsigned char Msg_Counter;
    unsigned int Index_RXIn;
    unsigned int Index_RXOut;
    unsigned int Index_TXIn;
    unsigned int Index_TXOut;
    unsigned char Uart_Module;
    bool    SOH_Detected;
    bool    DLE_Detected;
    unsigned char CharLoopCount;
} UART_Buffer;
extern UART_Buffer UART1_Buffer;
#ifdef BLUETOOTH_CODE
extern UART_Buffer UART2_Buffer;
#endif


unsigned int UART_ReadCountGet(UART_Buffer * Buffer);
size_t UART_WritePendingBytesGet(UART_Buffer * Buffer);
void UART_BufferInit(volatile UART_Buffer* Buffer);

/****************************** UART1 API - Control Board *********************/
void UART1_Init(unsigned int baud, bool ResetBuffer);
uint16_t UART1_Read(uint8_t* ArgBuffer_Array, const uint16_t size);
unsigned char UART1_ReadByte(void);
size_t UART1_Write(uint8_t* WriteBuffer, size_t Size );
void UART1_Enable();
void UART1_Disable();

/****************************** UART2 API - BLE *******************************/
#ifdef BLUETOOTH_CODE
void UART2_Init(unsigned int baud, bool ResetBuffer);
uint16_t UART2_Read(uint8_t* ArgBuffer_Array, const uint16_t size);
unsigned char UART2_ReadByte(void);
size_t UART2_Write(uint8_t* WriteBuffer, size_t Size );
void UART2_Enable();
void UART2_Disable();
bool UART2_IfEnable();
#endif
#endif // PLIB_UART1_H
