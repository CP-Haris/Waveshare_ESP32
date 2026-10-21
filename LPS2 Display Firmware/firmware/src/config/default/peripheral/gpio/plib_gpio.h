/*******************************************************************************
  GPIO PLIB

  Company:
    Microchip Technology Inc.

  File Name:
    plib_gpio.h

  Summary:
    GPIO PLIB Header File

  Description:
    This library provides an interface to control and interact with Parallel
    Input/Output controller (GPIO) module.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2019 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

#ifndef PLIB_GPIO_H
#define PLIB_GPIO_H

//Select processor for configuration

//#define PROCESSOR_PIC32MX170F256D_HW0130 //Device NR1
//#define PROCESSOR_PIC32MX130F256D_HW0150_HW0160 //Device NR2
//#define PROCESSOR_PIC32MX170F256D_HW0160_HW0170 //Device NR3
//#define PROCESSOR_PIC32MX230F256D_HW0160_HW0210 //Device NR4

// Automation: Change processor in project to correctly define HW
#if defined(__32MX130F256D__)
    #define PROCESSOR_PIC32MX130F256D_HW0150_HW0160 //Device NR2
#elif defined(__32MX170F256D__)
    #define PROCESSOR_PIC32MX170F256D_HW0160_HW0170 //Device NR3
#elif defined(__32MX230F256D__)
    #define PROCESSOR_PIC32MX230F256D_HW0160_HW0210 //Device NR4
#endif

//CHANGE PROCESSOR
//APP CHANGES
//Processor in project
//Ensure it is correctly defined in plib_gpio.h (Line 47-58)
//Set correct Sofware verison in Values.h
//Include correct linker
//Set correct loadable boot hex file


//BOOTLOADER CHANGES
//define in main.c (Line 70, 71)
//Processor in project
//Include correct linker


#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility
    extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Data types and constants
// *****************************************************************************
// *****************************************************************************

#if defined(PROCESSOR_PIC32MX130F256D_HW0150_HW0160) || defined(PROCESSOR_PIC32MX170F256D_HW0160_HW0170) || defined(PROCESSOR_PIC32MX230F256D_HW0160_HW0210) 
/*** Macros for LCD_D3 pin ***/
#define LCD_D3_Set()               (LATBSET = (1<<9))
#define LCD_D3_Clear()             (LATBCLR = (1<<9))
#define LCD_D3_Toggle()            (LATBINV= (1<<9))
#define LCD_D3_Get()               ((PORTB >> 9) & 0x1)
#define LCD_D3_OutputEnable()      (TRISBCLR = (1<<9))
#define LCD_D3_InputEnable()       (TRISBSET = (1<<9))
#define LCD_D3_PIN                  GPIO_PIN_RB9
/*** Macros for PGD pin ***/
#define PGD_Set()               (LATBSET = (1<<10))
#define PGD_Clear()             (LATBCLR = (1<<10))
#define PGD_Toggle()            (LATBINV= (1<<10))
#define PGD_Get()               ((PORTB >> 10) & 0x1)
#define PGD_OutputEnable()      (TRISBCLR = (1<<10))
#define PGD_InputEnable()       (TRISBSET = (1<<10))
#define PGD_PIN                  GPIO_PIN_RB10
/*** Macros for PGC pin ***/
#define PGC_Set()               (LATBSET = (1<<11))
#define PGC_Clear()             (LATBCLR = (1<<11))
#define PGC_Toggle()            (LATBINV= (1<<11))
#define PGC_Get()               ((PORTB >> 11) & 0x1)
#define PGC_OutputEnable()      (TRISBCLR = (1<<11))
#define PGC_InputEnable()       (TRISBSET = (1<<11))
#define PGC_PIN                  GPIO_PIN_RB11
/*** Macros for BLE_ON pin ***/
#define BLE_ON_Set()               (LATASET = (1<<10))
#define BLE_ON_Clear()             (LATACLR = (1<<10))
#define BLE_ON_Toggle()            (LATAINV= (1<<10))
#define BLE_ON_Get()               ((PORTA >> 10) & 0x1)
#define BLE_ON_OutputEnable()      (TRISACLR = (1<<10))
#define BLE_ON_InputEnable()       (TRISASET = (1<<10))
#define BLE_ON_PIN                  GPIO_PIN_RA10
/*** Macros for LCD_CD pin ***/
#define LCD_CD_Set()               (LATBSET = (1<<14))
#define LCD_CD_Clear()             (LATBCLR = (1<<14))
#define LCD_CD_Toggle()            (LATBINV= (1<<14))
#define LCD_CD_Get()               ((PORTB >> 14) & 0x1)
#define LCD_CD_OutputEnable()      (TRISBCLR = (1<<14))
#define LCD_CD_InputEnable()       (TRISBSET = (1<<14))
#define LCD_CD_PIN                  GPIO_PIN_RB14
/*** Macros for LCD_D7 pin ***/
#define LCD_D7_Set()               (LATASET = (1<<0))
#define LCD_D7_Clear()             (LATACLR = (1<<0))
#define LCD_D7_Toggle()            (LATAINV= (1<<0))
#define LCD_D7_Get()               ((PORTA >> 0) & 0x1)
#define LCD_D7_OutputEnable()      (TRISACLR = (1<<0))
#define LCD_D7_InputEnable()       (TRISASET = (1<<0))
#define LCD_D7_PIN                  GPIO_PIN_RA0
/*** Macros for LCD_D6 pin ***/
#define LCD_D6_Set()               (LATASET = (1<<1))
#define LCD_D6_Clear()             (LATACLR = (1<<1))
#define LCD_D6_Toggle()            (LATAINV= (1<<1))
#define LCD_D6_Get()               ((PORTA >> 1) & 0x1)
#define LCD_D6_OutputEnable()      (TRISACLR = (1<<1))
#define LCD_D6_InputEnable()       (TRISASET = (1<<1))
#define LCD_D6_PIN                  GPIO_PIN_RA1
/*** Macros for LCD_D0 pin ***/
#define LCD_D0_Set()               (LATBSET = (1<<0))
#define LCD_D0_Clear()             (LATBCLR = (1<<0))
#define LCD_D0_Toggle()            (LATBINV= (1<<0))
#define LCD_D0_Get()               ((PORTB >> 0) & 0x1)
#define LCD_D0_OutputEnable()      (TRISBCLR = (1<<0))
#define LCD_D0_InputEnable()       (TRISBSET = (1<<0))
#define LCD_D0_PIN                  GPIO_PIN_RB0
/*** Macros for LCD_D1 pin ***/
#define LCD_D1_Set()               (LATBSET = (1<<1))
#define LCD_D1_Clear()             (LATBCLR = (1<<1))
#define LCD_D1_Toggle()            (LATBINV= (1<<1))
#define LCD_D1_Get()               ((PORTB >> 1) & 0x1)
#define LCD_D1_OutputEnable()      (TRISBCLR = (1<<1))
#define LCD_D1_InputEnable()       (TRISBSET = (1<<1))
#define LCD_D1_PIN                  GPIO_PIN_RB1
/*** Macros for LCD_D2 pin ***/
#define LCD_D2_Set()               (LATBSET = (1<<2))
#define LCD_D2_Clear()             (LATBCLR = (1<<2))
#define LCD_D2_Toggle()            (LATBINV= (1<<2))
#define LCD_D2_Get()               ((PORTB >> 2) & 0x1)
#define LCD_D2_OutputEnable()      (TRISBCLR = (1<<2))
#define LCD_D2_InputEnable()       (TRISBSET = (1<<2))
#define LCD_D2_PIN                  GPIO_PIN_RB2
/*** Macros for LCD_WR pin ***/
#define LCD_WR_Set()               (LATBSET = (1<<3))
#define LCD_WR_Clear()             (LATBCLR = (1<<3))
#define LCD_WR_Toggle()            (LATBINV= (1<<3))
#define LCD_WR_Get()               ((PORTB >> 3) & 0x1)
#define LCD_WR_OutputEnable()      (TRISBCLR = (1<<3))
#define LCD_WR_InputEnable()       (TRISBSET = (1<<3))
#define LCD_WR_PIN                  GPIO_PIN_RB3
/*** Macros for LCD_CS pin ***/
#define LCD_CS_Set()               (LATCSET = (1<<2))
#define LCD_CS_Clear()             (LATCCLR = (1<<2))
#define LCD_CS_Toggle()            (LATCINV= (1<<2))
#define LCD_CS_Get()               ((PORTC >> 2) & 0x1)
#define LCD_CS_OutputEnable()      (TRISCCLR = (1<<2))
#define LCD_CS_InputEnable()       (TRISCSET = (1<<2))
#define LCD_CS_PIN                  GPIO_PIN_RC2
/*** Macros for LED_R_230V pin ***/
#define LED_R_230V_Set()               (LATASET = (1<<2))
#define LED_R_230V_Clear()             (LATACLR = (1<<2))
#define LED_R_230V_Toggle()            (LATAINV= (1<<2))
#define LED_R_230V_Get()               ((PORTA >> 2) & 0x1)
#define LED_R_230V_OutputEnable()      (TRISACLR = (1<<2))
#define LED_R_230V_InputEnable()       (TRISASET = (1<<2))
#define LED_R_230V_PIN                  GPIO_PIN_RA2
/*** Macros for LED_G_230V pin ***/
#define LED_G_230V_Set()               (LATASET = (1<<3))
#define LED_G_230V_Clear()             (LATACLR = (1<<3))
#define LED_G_230V_Toggle()            (LATAINV= (1<<3))
#define LED_G_230V_Get()               ((PORTA >> 3) & 0x1)
#define LED_G_230V_OutputEnable()      (TRISACLR = (1<<3))
#define LED_G_230V_InputEnable()       (TRISASET = (1<<3))
#define LED_G_230V_PIN                  GPIO_PIN_RA3
/*** Macros for LED_R_12V pin ***/
#define LED_R_12V_Set()               (LATASET = (1<<8))
#define LED_R_12V_Clear()             (LATACLR = (1<<8))
#define LED_R_12V_Toggle()            (LATAINV= (1<<8))
#define LED_R_12V_Get()               ((PORTA >> 8) & 0x1)
#define LED_R_12V_OutputEnable()      (TRISACLR = (1<<8))
#define LED_R_12V_InputEnable()       (TRISASET = (1<<8))
#define LED_R_12V_PIN                  GPIO_PIN_RA8
/*** Macros for LED_G_12V pin ***/
#define LED_G_12V_Set()               (LATBSET = (1<<4))
#define LED_G_12V_Clear()             (LATBCLR = (1<<4))
#define LED_G_12V_Toggle()            (LATBINV= (1<<4))
#define LED_G_12V_Get()               ((PORTB >> 4) & 0x1)
#define LED_G_12V_OutputEnable()      (TRISBCLR = (1<<4))
#define LED_G_12V_InputEnable()       (TRISBSET = (1<<4))
#define LED_G_12V_PIN                  GPIO_PIN_RB4
/*** Macros for LCD_RST pin ***/
#define LCD_RST_Set()               (LATASET = (1<<4))
#define LCD_RST_Clear()             (LATACLR = (1<<4))
#define LCD_RST_Toggle()            (LATAINV= (1<<4))
#define LCD_RST_Get()               ((PORTA >> 4) & 0x1)
#define LCD_RST_OutputEnable()      (TRISACLR = (1<<4))
#define LCD_RST_InputEnable()       (TRISASET = (1<<4))
#define LCD_RST_PIN                  GPIO_PIN_RA4
/*** Macros for SW_BACK pin ***/
#define SW_BACK_Set()               (LATASET = (1<<9))
#define SW_BACK_Clear()             (LATACLR = (1<<9))
#define SW_BACK_Toggle()            (LATAINV= (1<<9))
#define SW_BACK_Get()               ((PORTA >> 9) & 0x1)
#define SW_BACK_OutputEnable()      (TRISACLR = (1<<9))
#define SW_BACK_InputEnable()       (TRISASET = (1<<9))
#define SW_BACK_PIN                  GPIO_PIN_RA9
/*** Macros for SW_OK pin ***/
#define SW_OK_Set()               (LATCSET = (1<<3))
#define SW_OK_Clear()             (LATCCLR = (1<<3))
#define SW_OK_Toggle()            (LATCINV= (1<<3))
#define SW_OK_Get()               ((PORTC >> 3) & 0x1)
#define SW_OK_OutputEnable()      (TRISCCLR = (1<<3))
#define SW_OK_InputEnable()       (TRISCSET = (1<<3))
#define SW_OK_PIN                  GPIO_PIN_RC3
/*** Macros for SW_UP pin ***/
#define SW_UP_Set()               (LATCSET = (1<<4))
#define SW_UP_Clear()             (LATCCLR = (1<<4))
#define SW_UP_Toggle()            (LATCINV= (1<<4))
#define SW_UP_Get()               ((PORTC >> 4) & 0x1)
#define SW_UP_OutputEnable()      (TRISCCLR = (1<<4))
#define SW_UP_InputEnable()       (TRISCSET = (1<<4))
#define SW_UP_PIN                  GPIO_PIN_RC4
/*** Macros for SW_DOWN pin ***/
#define SW_DOWN_Set()               (LATCSET = (1<<5))
#define SW_DOWN_Clear()             (LATCCLR = (1<<5))
#define SW_DOWN_Toggle()            (LATCINV= (1<<5))
#define SW_DOWN_Get()               ((PORTC >> 5) & 0x1)
#define SW_DOWN_OutputEnable()      (TRISCCLR = (1<<5))
#define SW_DOWN_InputEnable()       (TRISCSET = (1<<5))
#define SW_DOWN_PIN                  GPIO_PIN_RC5
/*** Macros for LCD_RD pin ***/
#define LCD_RD_Set()               (LATBSET = (1<<6))
#define LCD_RD_Clear()             (LATBCLR = (1<<6))
#define LCD_RD_Toggle()            (LATBINV= (1<<6))
#define LCD_RD_Get()               ((PORTB >> 6) & 0x1)
#define LCD_RD_OutputEnable()      (TRISBCLR = (1<<6))
#define LCD_RD_InputEnable()       (TRISBSET = (1<<6))
#define LCD_RD_PIN                  GPIO_PIN_RB6
/*** Macros for LCD_D5 pin ***/
#define LCD_D5_Set()               (LATBSET = (1<<7))
#define LCD_D5_Clear()             (LATBCLR = (1<<7))
#define LCD_D5_Toggle()            (LATBINV= (1<<7))
#define LCD_D5_Get()               ((PORTB >> 7) & 0x1)
#define LCD_D5_OutputEnable()      (TRISBCLR = (1<<7))
#define LCD_D5_InputEnable()       (TRISBSET = (1<<7))
#define LCD_D5_PIN                  GPIO_PIN_RB7
/*** Macros for LCD_D4 pin ***/
#define LCD_D4_Set()               (LATBSET = (1<<8))
#define LCD_D4_Clear()             (LATBCLR = (1<<8))
#define LCD_D4_Toggle()            (LATBINV= (1<<8))
#define LCD_D4_Get()               ((PORTB >> 8) & 0x1)
#define LCD_D4_OutputEnable()      (TRISBCLR = (1<<8))
#define LCD_D4_InputEnable()       (TRISBSET = (1<<8))
#define LCD_D4_PIN                  GPIO_PIN_RB8
/*** Macros for BLE_ACTIVE pin ***/
#define BLE_ACTIVE_Set()           (LATASET = (1<<7))
#define BLE_ACTIVE_Clear()         (LATACLR = (1<<7))
#define BLE_ACTIVE_Toggle()        (LATAINV= (1<<7))
#define BLE_ACTIVE_Get()           ((PORTA >> 7) & 0x1)
#define BLE_ACTIVE_OutputEnable()  (TRISACLR = (1<<7))
#define BLE_ACTIVE_InputEnable()   (TRISASET = (1<<7))
#define BLE_ACTIVE_PIN              GPIO_PIN_RA7
#endif

#ifdef PROCESSOR_PIC32MX170F256D_HW0130
/*** Macros for LCD_D3 pin ***/
#define LCD_D3_Set()               (LATBSET = (1<<9))
#define LCD_D3_Clear()             (LATBCLR = (1<<9))
#define LCD_D3_Toggle()            (LATBINV= (1<<9))
#define LCD_D3_Get()               ((PORTB >> 9) & 0x1)
#define LCD_D3_OutputEnable()      (TRISBCLR = (1<<9))
#define LCD_D3_InputEnable()       (TRISBSET = (1<<9))
#define LCD_D3_PIN                  GPIO_PIN_RB9
/*** Macros for LCD_CS pin ***/
#define LCD_CS_Set()               (LATCSET = (1<<6))
#define LCD_CS_Clear()             (LATCCLR = (1<<6))
#define LCD_CS_Toggle()            (LATCINV= (1<<6))
#define LCD_CS_Get()               ((PORTC >> 6) & 0x1)
#define LCD_CS_OutputEnable()      (TRISCCLR = (1<<6))
#define LCD_CS_InputEnable()       (TRISCSET = (1<<6))
#define LCD_CS_PIN                  GPIO_PIN_RC6
/*** Macros for LCD_CD pin ***/
#define LCD_CD_Set()               (LATCSET = (1<<7))
#define LCD_CD_Clear()             (LATCCLR = (1<<7))
#define LCD_CD_Toggle()            (LATCINV= (1<<7))
#define LCD_CD_Get()               ((PORTC >> 7) & 0x1)
#define LCD_CD_OutputEnable()      (TRISCCLR = (1<<7))
#define LCD_CD_InputEnable()       (TRISCSET = (1<<7))
#define LCD_CD_PIN                  GPIO_PIN_RC7
/*** Macros for LCD_D2 pin ***/
#define LCD_D2_Set()               (LATBSET = (1<<10))
#define LCD_D2_Clear()             (LATBCLR = (1<<10))
#define LCD_D2_Toggle()            (LATBINV= (1<<10))
#define LCD_D2_Get()               ((PORTB >> 10) & 0x1)
#define LCD_D2_OutputEnable()      (TRISBCLR = (1<<10))
#define LCD_D2_InputEnable()       (TRISBSET = (1<<10))
#define LCD_D2_PIN                  GPIO_PIN_RB10
/*** Macros for LCD_D1 pin ***/
#define LCD_D1_Set()               (LATBSET = (1<<11))
#define LCD_D1_Clear()             (LATBCLR = (1<<11))
#define LCD_D1_Toggle()            (LATBINV= (1<<11))
#define LCD_D1_Get()               ((PORTB >> 11) & 0x1)
#define LCD_D1_OutputEnable()      (TRISBCLR = (1<<11))
#define LCD_D1_InputEnable()       (TRISBSET = (1<<11))
#define LCD_D1_PIN                  GPIO_PIN_RB11
/*** Macros for LCD_D0 pin ***/
#define LCD_D0_Set()               (LATBSET = (1<<12))
#define LCD_D0_Clear()             (LATBCLR = (1<<12))
#define LCD_D0_Toggle()            (LATBINV= (1<<12))
#define LCD_D0_Get()               ((PORTB >> 12) & 0x1)
#define LCD_D0_OutputEnable()      (TRISBCLR = (1<<12))
#define LCD_D0_InputEnable()       (TRISBSET = (1<<12))
#define LCD_D0_PIN                  GPIO_PIN_RB12
/*** Macros for RTC_INT pin ***/
#define RTC_INT_Set()               (LATBSET = (1<<13))
#define RTC_INT_Clear()             (LATBCLR = (1<<13))
#define RTC_INT_Toggle()            (LATBINV= (1<<13))
#define RTC_INT_Get()               ((PORTB >> 13) & 0x1)
#define RTC_INT_OutputEnable()      (TRISBCLR = (1<<13))
#define RTC_INT_InputEnable()       (TRISBSET = (1<<13))
#define RTC_INT_InterruptEnable()   (CNENBSET = (1<<13))
#define RTC_INT_InterruptDisable()  (CNENBCLR = (1<<13))
#define RTC_INT_PIN                  GPIO_PIN_RB13
/*** Macros for CS_MEM pin ***/
#define CS_MEM_Set()               (LATASET = (1<<10))
#define CS_MEM_Clear()             (LATACLR = (1<<10))
#define CS_MEM_Toggle()            (LATAINV= (1<<10))
#define CS_MEM_Get()               ((PORTA >> 10) & 0x1)
#define CS_MEM_OutputEnable()      (TRISACLR = (1<<10))
#define CS_MEM_InputEnable()       (TRISASET = (1<<10))
#define CS_MEM_PIN                  GPIO_PIN_RA10
/*** Macros for BLE_ACTIVE pin ***/
#define BLE_ACTIVE_Set()           (LATASET = (1<<7))
#define BLE_ACTIVE_Clear()         (LATACLR = (1<<7))
#define BLE_ACTIVE_Toggle()        (LATAINV= (1<<7))
#define BLE_ACTIVE_Get()           ((PORTA >> 7) & 0x1)
#define BLE_ACTIVE_OutputEnable()  (TRISACLR = (1<<7))
#define BLE_ACTIVE_InputEnable()   (TRISASET = (1<<7))
#define BLE_ACTIVE_PIN              GPIO_PIN_RA7
/*** Macros for LCD_WR pin ***/
#define LCD_WR_Set()               (LATBSET = (1<<14))
#define LCD_WR_Clear()             (LATBCLR = (1<<14))
#define LCD_WR_Toggle()            (LATBINV= (1<<14))
#define LCD_WR_Get()               ((PORTB >> 14) & 0x1)
#define LCD_WR_OutputEnable()      (TRISBCLR = (1<<14))
#define LCD_WR_InputEnable()       (TRISBSET = (1<<14))
#define LCD_WR_PIN                  GPIO_PIN_RB14
/*** Macros for PGD pin ***/
#define PGD_Set()               (LATBSET = (1<<0))
#define PGD_Clear()             (LATBCLR = (1<<0))
#define PGD_Toggle()            (LATBINV= (1<<0))
#define PGD_Get()               ((PORTB >> 0) & 0x1)
#define PGD_OutputEnable()      (TRISBCLR = (1<<0))
#define PGD_InputEnable()       (TRISBSET = (1<<0))
#define PGD_PIN                  GPIO_PIN_RB0
/*** Macros for PGC pin ***/
#define PGC_Set()               (LATBSET = (1<<1))
#define PGC_Clear()             (LATBCLR = (1<<1))
#define PGC_Toggle()            (LATBINV= (1<<1))
#define PGC_Get()               ((PORTB >> 1) & 0x1)
#define PGC_OutputEnable()      (TRISBCLR = (1<<1))
#define PGC_InputEnable()       (TRISBSET = (1<<1))
#define PGC_PIN                  GPIO_PIN_RB1
/*** Macros for LED_R_230V pin ***/
#define LED_R_230V_Set()               (LATASET = (1<<2))
#define LED_R_230V_Clear()             (LATACLR = (1<<2))
#define LED_R_230V_Toggle()            (LATAINV= (1<<2))
#define LED_R_230V_Get()               ((PORTA >> 2) & 0x1)
#define LED_R_230V_OutputEnable()      (TRISACLR = (1<<2))
#define LED_R_230V_InputEnable()       (TRISASET = (1<<2))
#define LED_R_230V_PIN                  GPIO_PIN_RA2
/*** Macros for LED_G_230V pin ***/
#define LED_G_230V_Set()               (LATASET = (1<<3))
#define LED_G_230V_Clear()             (LATACLR = (1<<3))
#define LED_G_230V_Toggle()            (LATAINV= (1<<3))
#define LED_G_230V_Get()               ((PORTA >> 3) & 0x1)
#define LED_G_230V_OutputEnable()      (TRISACLR = (1<<3))
#define LED_G_230V_InputEnable()       (TRISASET = (1<<3))
#define LED_G_230V_PIN                  GPIO_PIN_RA3
/*** Macros for LED_R_12V pin ***/
#define LED_R_12V_Set()               (LATASET = (1<<8))
#define LED_R_12V_Clear()             (LATACLR = (1<<8))
#define LED_R_12V_Toggle()            (LATAINV= (1<<8))
#define LED_R_12V_Get()               ((PORTA >> 8) & 0x1)
#define LED_R_12V_OutputEnable()      (TRISACLR = (1<<8))
#define LED_R_12V_InputEnable()       (TRISASET = (1<<8))
#define LED_R_12V_PIN                  GPIO_PIN_RA8
/*** Macros for LED_G_12V pin ***/
#define LED_G_12V_Set()               (LATBSET = (1<<4))
#define LED_G_12V_Clear()             (LATBCLR = (1<<4))
#define LED_G_12V_Toggle()            (LATBINV= (1<<4))
#define LED_G_12V_Get()               ((PORTB >> 4) & 0x1)
#define LED_G_12V_OutputEnable()      (TRISBCLR = (1<<4))
#define LED_G_12V_InputEnable()       (TRISBSET = (1<<4))
#define LED_G_12V_PIN                  GPIO_PIN_RB4
/*** Macros for SW_BACK pin ***/
#define SW_BACK_Set()               (LATASET = (1<<9))
#define SW_BACK_Clear()             (LATACLR = (1<<9))
#define SW_BACK_Toggle()            (LATAINV= (1<<9))
#define SW_BACK_Get()               ((PORTA >> 9) & 0x1)
#define SW_BACK_OutputEnable()      (TRISACLR = (1<<9))
#define SW_BACK_InputEnable()       (TRISASET = (1<<9))
#define SW_BACK_PIN                  GPIO_PIN_RA9
/*** Macros for SW_OK pin ***/
#define SW_OK_Set()               (LATCSET = (1<<3))
#define SW_OK_Clear()             (LATCCLR = (1<<3))
#define SW_OK_Toggle()            (LATCINV= (1<<3))
#define SW_OK_Get()               ((PORTC >> 3) & 0x1)
#define SW_OK_OutputEnable()      (TRISCCLR = (1<<3))
#define SW_OK_InputEnable()       (TRISCSET = (1<<3))
#define SW_OK_PIN                  GPIO_PIN_RC3
/*** Macros for SW_UP pin ***/
#define SW_UP_Set()               (LATCSET = (1<<4))
#define SW_UP_Clear()             (LATCCLR = (1<<4))
#define SW_UP_Toggle()            (LATCINV= (1<<4))
#define SW_UP_Get()               ((PORTC >> 4) & 0x1)
#define SW_UP_OutputEnable()      (TRISCCLR = (1<<4))
#define SW_UP_InputEnable()       (TRISCSET = (1<<4))
#define SW_UP_PIN                  GPIO_PIN_RC4
/*** Macros for SW_DOWN pin ***/
#define SW_DOWN_Set()               (LATCSET = (1<<5))
#define SW_DOWN_Clear()             (LATCCLR = (1<<5))
#define SW_DOWN_Toggle()            (LATCINV= (1<<5))
#define SW_DOWN_Get()               ((PORTC >> 5) & 0x1)
#define SW_DOWN_OutputEnable()      (TRISCCLR = (1<<5))
#define SW_DOWN_InputEnable()       (TRISCSET = (1<<5))
#define SW_DOWN_PIN                  GPIO_PIN_RC5
/*** Macros for LCD_D7 pin ***/
#define LCD_D7_Set()               (LATBSET = (1<<5))
#define LCD_D7_Clear()             (LATBCLR = (1<<5))
#define LCD_D7_Toggle()            (LATBINV= (1<<5))
#define LCD_D7_Get()               ((PORTB >> 5) & 0x1)
#define LCD_D7_OutputEnable()      (TRISBCLR = (1<<5))
#define LCD_D7_InputEnable()       (TRISBSET = (1<<5))
#define LCD_D7_PIN                  GPIO_PIN_RB5
/*** Macros for LCD_D6 pin ***/
#define LCD_D6_Set()               (LATBSET = (1<<6))
#define LCD_D6_Clear()             (LATBCLR = (1<<6))
#define LCD_D6_Toggle()            (LATBINV= (1<<6))
#define LCD_D6_Get()               ((PORTB >> 6) & 0x1)
#define LCD_D6_OutputEnable()      (TRISBCLR = (1<<6))
#define LCD_D6_InputEnable()       (TRISBSET = (1<<6))
#define LCD_D6_PIN                  GPIO_PIN_RB6
/*** Macros for LCD_D5 pin ***/
#define LCD_D5_Set()               (LATBSET = (1<<7))
#define LCD_D5_Clear()             (LATBCLR = (1<<7))
#define LCD_D5_Toggle()            (LATBINV= (1<<7))
#define LCD_D5_Get()               ((PORTB >> 7) & 0x1)
#define LCD_D5_OutputEnable()      (TRISBCLR = (1<<7))
#define LCD_D5_InputEnable()       (TRISBSET = (1<<7))
#define LCD_D5_PIN                  GPIO_PIN_RB7
/*** Macros for LCD_D4 pin ***/
#define LCD_D4_Set()               (LATBSET = (1<<8))
#define LCD_D4_Clear()             (LATBCLR = (1<<8))
#define LCD_D4_Toggle()            (LATBINV= (1<<8))
#define LCD_D4_Get()               ((PORTB >> 8) & 0x1)
#define LCD_D4_OutputEnable()      (TRISBCLR = (1<<8))
#define LCD_D4_InputEnable()       (TRISBSET = (1<<8))
#define LCD_D4_PIN                  GPIO_PIN_RB8
#endif    

// *****************************************************************************
/* GPIO Port

  Summary:
    Identifies the available GPIO Ports.

  Description:
    This enumeration identifies the available GPIO Ports.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all ports are available on all devices.  Refer to the specific
    device data sheet to determine which ports are supported.
*/

typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
} GPIO_PORT;

// *****************************************************************************
/* GPIO Port Pins

  Summary:
    Identifies the available GPIO port pins.

  Description:
    This enumeration identifies the available GPIO port pins.

  Remarks:
    The caller should not rely on the specific numbers assigned to any of
    these values as they may change from one processor to the next.

    Not all pins are available on all devices.  Refer to the specific
    device data sheet to determine which pins are supported.
*/

typedef enum
{
    GPIO_PIN_RA0 = 0,
    GPIO_PIN_RA1 = 1,
    GPIO_PIN_RA2 = 2,
    GPIO_PIN_RA3 = 3,
    GPIO_PIN_RA4 = 4,
    GPIO_PIN_RA7 = 7,
    GPIO_PIN_RA8 = 8,
    GPIO_PIN_RA9 = 9,
    GPIO_PIN_RA10 = 10,
    GPIO_PIN_RB0 = 16,
    GPIO_PIN_RB1 = 17,
    GPIO_PIN_RB2 = 18,
    GPIO_PIN_RB3 = 19,
    GPIO_PIN_RB4 = 20,
    GPIO_PIN_RB5 = 21,
    GPIO_PIN_RB6 = 22,
    GPIO_PIN_RB7 = 23,
    GPIO_PIN_RB8 = 24,
    GPIO_PIN_RB9 = 25,
    GPIO_PIN_RB10 = 26,
    GPIO_PIN_RB11 = 27,
    GPIO_PIN_RB12 = 28,
    GPIO_PIN_RB13 = 29,
    GPIO_PIN_RB14 = 30,
    GPIO_PIN_RB15 = 31,
    GPIO_PIN_RC0 = 32,
    GPIO_PIN_RC1 = 33,
    GPIO_PIN_RC2 = 34,
    GPIO_PIN_RC3 = 35,
    GPIO_PIN_RC4 = 36,
    GPIO_PIN_RC5 = 37,
    GPIO_PIN_RC6 = 38,
    GPIO_PIN_RC7 = 39,
    GPIO_PIN_RC8 = 40,
    GPIO_PIN_RC9 = 41,

    /* This element should not be used in any of the GPIO APIs.
       It will be used by other modules or application to denote that none of the GPIO Pin is used */
    GPIO_PIN_NONE = -1

} GPIO_PIN;


void GPIO_Initialize(void);

void GPIO_Sleep_Deinitialize(void);
void GPIO_Off_Deinitialize(void);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on multiple pins of a port
// *****************************************************************************
// *****************************************************************************

uint32_t GPIO_PortRead(GPIO_PORT port);

void GPIO_PortWrite(GPIO_PORT port, uint32_t mask, uint32_t value);

uint32_t GPIO_PortLatchRead ( GPIO_PORT port );

void GPIO_PortSet(GPIO_PORT port, uint32_t mask);

void GPIO_PortClear(GPIO_PORT port, uint32_t mask);

void GPIO_PortToggle(GPIO_PORT port, uint32_t mask);

void GPIO_PortInputEnable(GPIO_PORT port, uint32_t mask);

void GPIO_PortOutputEnable(GPIO_PORT port, uint32_t mask);

// *****************************************************************************
// *****************************************************************************
// Section: GPIO Functions which operates on one pin at a time
// *****************************************************************************
// *****************************************************************************

static inline void GPIO_PinWrite(GPIO_PIN pin, bool value)
{
    GPIO_PortWrite(pin>>4, (uint32_t)(0x1) << (pin & 0xF), (uint32_t)(value) << (pin & 0xF));
}

static inline bool GPIO_PinRead(GPIO_PIN pin)
{
    return (bool)(((GPIO_PortRead(pin>>4)) >> (pin & 0xF)) & 0x1);
}

static inline bool GPIO_PinLatchRead(GPIO_PIN pin)
{
    return (bool)((GPIO_PortLatchRead(pin>>4) >> (pin & 0xF)) & 0x1);
}

static inline void GPIO_PinToggle(GPIO_PIN pin)
{
    GPIO_PortToggle(pin>>4, 0x1 << (pin & 0xF));
}

static inline void GPIO_PinSet(GPIO_PIN pin)
{
    GPIO_PortSet(pin>>4, 0x1 << (pin & 0xF));
}

static inline void GPIO_PinClear(GPIO_PIN pin)
{
    GPIO_PortClear(pin>>4, 0x1 << (pin & 0xF));
}

static inline void GPIO_PinInputEnable(GPIO_PIN pin)
{
    GPIO_PortInputEnable(pin>>4, 0x1 << (pin & 0xF));
}

static inline void GPIO_PinOutputEnable(GPIO_PIN pin)
{
    GPIO_PortOutputEnable(pin>>4, 0x1 << (pin & 0xF));
}


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif
// DOM-IGNORE-END
#endif // PLIB_GPIO_H
