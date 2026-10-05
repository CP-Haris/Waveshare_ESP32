/*******************************************************************************
  SYS PORTS Static Functions for PORTS System Service

  Company:
    Microchip Technology Inc.

  File Name:
    plib_gpio.c

  Summary:
    GPIO function implementations for the GPIO PLIB.

  Description:
    The GPIO PLIB provides a simple interface to manage peripheral
    input-output controller.

*******************************************************************************/

//DOM-IGNORE-BEGIN
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
//DOM-IGNORE-END

#include "plib_gpio.h"



/******************************************************************************
  Function:
    GPIO_Initialize ( void )

  Summary:
    Initialize the GPIO library.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_Initialize ( void )
{
    #if defined(PROCESSOR_PIC32MX130F256D_HW0150_HW0160) || defined(PROCESSOR_PIC32MX170F256D_HW0160_HW0170) || defined(PROCESSOR_PIC32MX230F256D_HW0160_HW0210)
    /* PORTA Initialization */
    LATA = 0x010; /* Initial Latch Value */ // - A4 = LCD_RST
    ANSELA = 0x0000; /* Digital Mode Enable */
    TRISACLR = 0x51f; /* Direction Control */
    //CNPDA = 0x1C; // LED pull-downs

    /* PORTB Initialization */
    LATB = 0x48; /* Initial Latch Value */ // - B3 = LCD_WR, B6 = LCD_RD
    ANSELB = 0x0000; /* Digital Mode Enable */
    TRISBCLR = 0x4bdf; /* Direction Control */
    //CNPDB = 0x10; // LED pull-downs

    /* PORTC Initialization */
    LATC = 0x0; /* Initial Latch Value */
    ANSELC = 0x0000; /* Digital Mode Enable */
    TRISCCLR = 0x4; /* Direction Control */
    //CNPUCSET = 0x6; /* Pull-Up Enable */
    
    LED_R_230V_Clear();
    LED_G_230V_Clear();
    LED_R_12V_Clear();
    LED_G_12V_Clear();

    /* unlock system for PPS configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.IOLOCK = 0;


    /* PPS Input Remapping */
    U1RXR = 0;
    SDI2R = 5;
    U1RXR = 6;
    U2CTSR = 5; // 0b0101
    U2RXR = 1;

    /* PPS Output Remapping */
    RPC8R = 5;
    RPC7R = 5;
    RPC0R = 1;
    RPC9R = 2;
    RPB15R = 2;

    /* Lock back the system after PPS configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.IOLOCK = 1;
    #endif

    #ifdef PROCESSOR_PIC32MX170F256D_HW0130
    /* PORTA Initialization */
    LATA = 0x0; /* Initial Latch Value */
    TRISACLR = 0x58c; /* Direction Control */
    ANSELACLR = 0x3; /* Digital Mode Enable */

    /* PORTB Initialization */
    LATB = 0x0; /* Initial Latch Value */
    TRISBCLR = 0x5ff3; /* Direction Control */
    ANSELBCLR = 0xf00f; /* Digital Mode Enable */
    CNPUBSET = 0x4; /* Pull-Up Enable */
    /* Change Notice Enable */
    CNCONBSET = _CNCONB_ON_MASK;
    PORTB;
    IEC1SET = _IEC1_CNBIE_MASK;

    /* PORTC Initialization */
    LATC = 0x0; /* Initial Latch Value */
    TRISCCLR = 0xc0; /* Direction Control */
    CNPUCSET = 0x40; /* Pull-Up Enable */


    /* unlock system for PPS configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.IOLOCK = 0;

    /* PPS Input Remapping */
    U1RXR = 4;
    SDI2R = 2;

    /* PPS Output Remapping */
    RPC8R = 5;
    RPA0R = 5;
    RPA1R = 4;
    RPB3R = 1;

    /* Lock back the system after PPS configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.IOLOCK = 1;

    #endif
}

/******************************************************************************
  Function:
    GPIO_Sleep_Deinitialize ( void )

  Summary:
    Deinitializes the GPIO library for sleep mode

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_Sleep_Deinitialize ( void )
{
    #if defined(PROCESSOR_PIC32MX130F256D_HW0150_HW0160) || defined(PROCESSOR_PIC32MX170F256D_HW0160_HW0170) || defined(PROCESSOR_PIC32MX230F256D_HW0160_HW0210)
    /* PORTA Initialization */
    LATA = 0x0600; /* Initial Latch Value */
    TRISA = 0x079F; /* Direction Control -- all inputs */
    ANSELA = 0x0; /* Digital Mode Enable */
    CNPUA = 0x0600; /* Pull-up Enable -- for A10, A9 (3V3_BLE_ON, SW_BACK) */
    CNPDA = 0x011F; // A0, A1, A2, A3, A4, A8
    
    /* PORTB Initialization */
    LATB = 0x1C48; /* Initial Latch Value */
    TRISB = 0xFFFF; /* Direction Control -- all inputs */
    ANSELB = 0x0; /* Digital Mode Enable */
    CNPUB = 0x0048; // B3, B6 (LCD_WR, LCD_RD)
    CNPDB = 0xEFB7; // B0, B1, B2, B4, B5, B7, B8, B9, B10, B11, B13, B14, B15
    // BLE_RST pull-up/-down (B13) ideally decided after function, but this was not consistent and caused increased current

    /* PORTC Initialization */
    LATC = 0x003E; /* Initial Latch Value */
    TRISC = 0x03FF; /* Direction Control -- all inputs */
    ANSELC = 0x0; /* Digital Mode Enable */
    CNPUC = 0x003C; /* Pull-up Enable -- for C5, C4, C3, C2 (SW_DOWN, SW_UP, SW_OK, LCD_CS0) */
    CNPDC = 0x03C1; // C0, C6, C7, C8, C9
    // No pull configured for UART_RX; pulled on controlboard
    #endif

    #ifdef PROCESSOR_PIC32MX170F256D_HW0130

    #error HW Rev 1 untested; change IOs, test, and approve (current consumption, wake from UART etc.)
    
    /* PORTA Initialization */
    LATA = 0x0; /* Initial Latch Value */
    TRISA = 0x79f; /* Direction Control */
    ANSELA = 0x0; /* Digital Mode Enable */
    CNPUA = 0x200; /* Pull-Up Enable -- for A9 */
    CNPDA = (~0x200 && 0x79f); /* Pull-Down Enable -- for NOT A9 */

    /* PORTB Initialization */
    LATB = 0x0; /* Initial Latch Value */
    TRISB = 0xffff; /* Direction Control */
    ANSELB = 0x0000; /* Digital Mode Enable */
    CNPUB = 0x4; /* Pull-Up Enable -- for B2*/
    CNPDB = (~0x4 && 0xffff); /* Pull-Down Enable -- for NOT B2 */

    /* PORTC Initialization */
    LATC = 0x0; /* Initial Latch Value */
    TRISC = 0x3ff; /* Direction Control */
    ANSELC = 0x0; /* Digital Mode Enable */
    CNPUC = 0x38; /* Pull-Up Enable -- for C5, C4, C3*/
    CNPDC = (~0x38 && 0x3ff); /* Pull-Down Enable -- for NOT C5, C4, C3*/
    
    #endif
}

/******************************************************************************
  Function:
    GPIO_Off_Deinitialize ( void )

  Summary:
    Deinitializes the GPIO library for when the LPS turns off

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_Off_Deinitialize ( void )
{
    #if defined(PROCESSOR_PIC32MX130F256D_HW0150_HW0160) || defined(PROCESSOR_PIC32MX170F256D_HW0160_HW0170) || defined(PROCESSOR_PIC32MX230F256D_HW0160_HW0210)
    /* PORTA Initialization */
    LATA = 0x0200; /* Initial Latch Value */
    TRISA = 0x039F; /* Direction Control -- all inputs except 3V3_BLE_ON */
    ANSELA = 0x0; /* Digital Mode Enable */
    CNPUA = 0x0200; /* Pull-up Enable -- for A9 (SW_BACK) -- 3V3_BLE_ON decided after function */
    CNPDA = 0x11F; // A0, A1, A2, A3, A4, A8

    /* PORTB Initialization */
    LATB = 0x3C68; /* Initial Latch Value */
    TRISB = 0xFFFF; /* Direction Control -- all inputs */
    ANSELB = 0x0; /* Digital Mode Enable */
    CNPUB = 0x2068; // B3, B5, B6, B13 (LCD_WR, BLE_URX, LCD_RD, BLE_RST)
    CNPDB = 0xC397; // B0, B1, B2, B4, B7, B8, B9, B14, B15

    /* PORTC Initialization */
    LATC = 0x003E; /* Initial Latch Value */
    TRISC = 0x03FF; /* Direction Control -- all inputs */
    ANSELC = 0x0; /* Digital Mode Enable */
    CNPUC = 0x003C; /* Pull-up Enable -- for C5, C4, C3, C2 (SW_DOWN, SW_UP, SW_OK, LCD_CS0) */
    CNPDC = 0x03C1; // C0, C6, C7, C8, C9
    // No pull configured for UART_RX; pulled down on controlboard
    #endif

    #ifdef PROCESSOR_PIC32MX170F256D_HW0130

    #error Microcontroller rev 01 untested; change IOs, test, and approve (current consumption, wake from UART etc.)
    
    /* PORTA Initialization */
    LATA = 0x0; /* Initial Latch Value */
    TRISA = 0x79f; /* Direction Control */
    ANSELA = 0x0; /* Digital Mode Enable */
    CNPUA = 0x200; /* Pull-Up Enable -- for A9 */
    CNPDA = (~0x200 && 0x79f); /* Pull-Down Enable -- for NOT A9 */

    /* PORTB Initialization */
    LATB = 0x0; /* Initial Latch Value */
    TRISB = 0xffff; /* Direction Control */
    ANSELB = 0x0000; /* Digital Mode Enable */
    CNPUB = 0x4; /* Pull-Up Enable -- for B2*/
    CNPDB = (~0x4 && 0xffff); /* Pull-Down Enable -- for NOT B2 */

    /* PORTC Initialization */
    LATC = 0x0; /* Initial Latch Value */
    TRISC = 0x3ff; /* Direction Control */
    ANSELC = 0x0; /* Digital Mode Enable */
    CNPUC = 0x38; /* Pull-Up Enable -- for C5, C4, C3*/
    CNPDC = (~0x38 && 0x3ff); /* Pull-Down Enable -- for NOT C5, C4, C3*/
    
    #endif
}

// *****************************************************************************
// *****************************************************************************
// Section: GPIO APIs which operates on multiple pins of a port
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Function:
    uint32_t GPIO_PortRead ( GPIO_PORT port )

  Summary:
    Read all the I/O lines of the selected port.

  Description:
    This function reads the live data values on all the I/O lines of the
    selected port.  Bit values returned in each position indicate corresponding
    pin levels.
    1 = Pin is high.
    0 = Pin is low.

    This function reads the value regardless of pin configuration, whether it is
    set as as an input, driven by the GPIO Controller, or driven by a peripheral.

  Remarks:
    If the port has less than 32-bits, unimplemented pins will read as
    low (0).
    Implemented pins are Right aligned in the 32-bit return value.
*/
uint32_t GPIO_PortRead(GPIO_PORT port)
{
    return (*(volatile uint32_t *)(&PORTA + (port * 0x40)));
}

// *****************************************************************************
/* Function:
    void GPIO_PortWrite (GPIO_PORT port, uint32_t mask, uint32_t value);

  Summary:
    Write the value on the masked I/O lines of the selected port.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortWrite(GPIO_PORT port, uint32_t mask, uint32_t value)
{
    *(volatile uint32_t *)(&LATA + (port * 0x40)) = (*(volatile uint32_t *)(&LATA + (port * 0x40)) & (~mask)) | (mask & value);
}

// *****************************************************************************
/* Function:
    uint32_t GPIO_PortLatchRead ( GPIO_PORT port )

  Summary:
    Read the latched value on all the I/O lines of the selected port.

  Remarks:
    See plib_gpio.h for more details.
*/
uint32_t GPIO_PortLatchRead(GPIO_PORT port)
{
    return (*(volatile uint32_t *)(&LATA + (port * 0x40)));
}

// *****************************************************************************
/* Function:
    void GPIO_PortSet ( GPIO_PORT port, uint32_t mask )

  Summary:
    Set the selected IO pins of a port.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortSet(GPIO_PORT port, uint32_t mask)
{
    *(volatile uint32_t *)(&LATASET + (port * 0x40)) = mask;
}

// *****************************************************************************
/* Function:
    void GPIO_PortClear ( GPIO_PORT port, uint32_t mask )

  Summary:
    Clear the selected IO pins of a port.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortClear(GPIO_PORT port, uint32_t mask)
{
    *(volatile uint32_t *)(&LATACLR + (port * 0x40)) = mask;
}

// *****************************************************************************
/* Function:
    void GPIO_PortToggle ( GPIO_PORT port, uint32_t mask )

  Summary:
    Toggles the selected IO pins of a port.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortToggle(GPIO_PORT port, uint32_t mask)
{
    *(volatile uint32_t *)(&LATAINV + (port * 0x40))= mask;
}

// *****************************************************************************
/* Function:
    void GPIO_PortInputEnable ( GPIO_PORT port, uint32_t mask )

  Summary:
    Enables selected IO pins of a port as input.

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortInputEnable(GPIO_PORT port, uint32_t mask)
{
    *(volatile uint32_t *)(&TRISASET + (port * 0x40)) = mask;
}

// *****************************************************************************
/* Function:
    void GPIO_PortOutputEnable ( GPIO_PORT port, uint32_t mask )

  Summary:
    Enables selected IO pins of a port as output(s).

  Remarks:
    See plib_gpio.h for more details.
*/
void GPIO_PortOutputEnable(GPIO_PORT port, uint32_t mask)
{
    *(volatile uint32_t *)(&TRISACLR + (port * 0x40)) = mask;
}




/*******************************************************************************
 End of File
*/
