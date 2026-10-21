#include "device.h"
#include "UART_BIOS.h"
#include "LED.h"
#include "UART_App.h"
#include "Buzzer.h"
#include <string.h>

//////////////////// Buffer Handling //////////////////////////////////////////
UART_Buffer UART1_Buffer = {.Uart_Module = UART_CTRL, .ReadByteFunction = &UART1_ReadByte};

#ifdef BLUETOOTH_CODE
UART_Buffer UART2_Buffer = {.Uart_Module = UART_BLE, .ReadByteFunction = &UART2_ReadByte};
#endif

void UART_BufferInit(volatile UART_Buffer* Buffer)
{
    Buffer->Index_RXIn = 0;
    Buffer->Index_RXOut = 0;
    Buffer->Index_TXIn = 0;
    Buffer->Index_TXOut = 0;
    //memset(&Buffer->Msg_Buffer, 0, sizeof Buffer->Msg_Buffer);
}

//Returns number of bytes in RX buffer that is suposed to be read
unsigned int UART_ReadCountGet(UART_Buffer * Buffer)
{
    unsigned int UnreadBytesAvailable = 0;
    //Take a snapshot of indices to avoid processing in critical section 
    unsigned int InIndex = Buffer->Index_RXIn;
    unsigned int OutIndex = Buffer->Index_RXOut;
    
    if(OutIndex != InIndex)
    {
        if ( InIndex >=  OutIndex)
            //Get UnreadBytes when the buffer is not looped
            UnreadBytesAvailable =  InIndex - OutIndex;
        else
            //Get UnreadBytes when the buffer is looped
            UnreadBytesAvailable =  (UART_READ_BUFFER_SIZE - OutIndex) + InIndex;
    }
    return UnreadBytesAvailable;
}

//Returns number of bytes in TX Buffer to transmit - Called from ISR TX and Application
size_t UART_WritePendingBytesGet(UART_Buffer * Buffer)
{
    size_t PendingTxBytes;

    // Take a snapshot of indices to avoid processing in critical section
    unsigned int OutIndex = Buffer->Index_TXOut;
    unsigned int InIndex = Buffer->Index_TXIn;
    
    if ( InIndex >=  OutIndex)
        //Get UnreadBytes when the buffer is not looped
        PendingTxBytes =  InIndex - OutIndex;

    else     
        //Get UnreadBytes when the buffer is looped
        PendingTxBytes =  (UART_WRITE_BUFFER_SIZE -  OutIndex) + InIndex;

    return PendingTxBytes;
}

//////////////////// Initialization ///////////////////////////////////////////
void UART1_Init(unsigned int baud, bool ResetBuffer)
{
    U1MODE = 0x0;
    
    if(ResetBuffer)
        UART_BufferInit(&UART1_Buffer);
         
    // Enable UART1 Receiver and Transmitter 
    U1STASET = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
    
    //Disable Error Interrupt
    IEC1CLR = _IEC1_U1EIE_MASK;

    //Disable RX Interrupt
    IEC1CLR = _IEC1_U1RXIE_MASK;

    //Disable TX Interrupt
    IEC1CLR = _IEC1_U1TXIE_MASK;
    
    //Clear Interrupts:
    U1STACLR = _U1STA_OERR_MASK;    //Clear Overflow    
        
    //Configure UART1 Baud Rate for highspeed uart
    unsigned int Freq = UART_FrequencyGet(); 
    
    uint32_t brgValHigh = 0;
    uint32_t brgValLow = 0;
    uint32_t brgVal = 0;
    
    /* Calculate BRG value */
    brgValLow = ((Freq / baud) >> 4) - 1;
    brgValHigh = ((Freq / baud) >> 2) - 1;

    /* Check if the baud value can be set with low baud settings */
    if((brgValHigh >= 0) && (brgValHigh <= UINT16_MAX))
    {
        brgVal =  (((Freq >> 2) + (baud >> 1)) / baud ) - 1;
        U1MODESET = _U1MODE_BRGH_MASK;
    }
    else if ((brgValLow >= 0) && (brgValLow <= UINT16_MAX))
    {
        brgVal = ( ((Freq >> 4) + (baud >> 1)) / baud ) - 1;
        U1MODECLR = _U1MODE_BRGH_MASK;
    }
    
    U1BRG = brgVal;
    
    
    //Turn ON UART1 and Set BRGH to 1 to change Uart devider to 4
    U1MODESET = _U1MODE_ON_MASK;
    
    
    //Clear Interrupts
    IFS1CLR = _IFS1_U1RXIF_MASK; //Clear receive interrupt flag
    IFS1CLR = _IFS1_U1TXIF_MASK; //Clear Transmit interrupt flag
    IFS1CLR = _IFS1_U1EIF_MASK;     // Clear error interrupt flag
    
    // Enable UART1_FAULT Interrupt --> Kan hænge længe i rutinen. (800ms)
    IEC1SET = _IEC1_U1EIE_MASK;

    // Enable UART1_RX Interrupt
    IEC1SET = _IEC1_U1RXIE_MASK;
    
}

void UART1_Enable()
{
    // Enable UART1 Receiver and Transmitter
    U1STASET = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
    U1MODEbits.ON = true;
}

void UART1_Disable()
{
    // Disable UART1 Receiver and Transmitter 
    U1STACLR = (_U1STA_UTXEN_MASK | _U1STA_URXEN_MASK);
    U1MODEbits.ON = false;
}


#ifdef BLUETOOTH_CODE

void UART2_Enable()
{
    // Enable UART2 Receiver and Transmitter 
    U2MODEbits.ON = true;
}

void UART2_Disable()
{
    // Disable UART2 Receiver and Transmitter 
    U2MODEbits.ON = false;
}

bool UART2_IfEnable()
{
    //Return if TX is enabled;
    return U2MODEbits.ON;
    
}



//////////////////// Initialization ///////////////////////////////////////////
void UART2_Init(unsigned int baud, bool ResetBuffer)
{
    
    if(ResetBuffer)
        UART_BufferInit(&UART2_Buffer);
          
    
    // Enable UART2 Receiver and Transmitter 
    U2STASET = (_U2STA_UTXEN_MASK | _U2STA_URXEN_MASK);
    
    //Disable Error Interrupt
    IEC1CLR = _IEC1_U2EIE_MASK;

    //Disable RX Interrupt
    IEC1CLR = _IEC1_U2RXIE_MASK;

    //Disable TX Interrupt
    IEC1CLR = _IEC1_U2TXIE_MASK;
    
    //Clear Interrupts:
    U2STACLR = _U2STA_OERR_MASK;    //Clear Overflow    
    
    //Configure UART2 Baud Rate for highspeed uart
    unsigned int Freq = UART_FrequencyGet(); 
    U2BRG = (((Freq >> 2) + (baud >> 1)) / baud ) - 1;
    
    
    //Turn ON UART2 and Set BRGH to 1 to change Uart devider to 4
    U2MODESET = (_U2MODE_ON_MASK | _U2MODE_BRGH_MASK);
    U2MODEbits.UEN = 0b10;
    U2MODEbits.RTSMD = 0;

    
    //Clear Interrupts
    IFS1CLR = _IFS1_U2RXIF_MASK; //Clear receive interrupt flag
    IFS1CLR = _IFS1_U2TXIF_MASK; //Clear Transmit interrupt flag
    IFS1CLR = _IFS1_U2EIF_MASK;     // Clear error interrupt flag
    
    // Enable UART2_FAULT Interrupt --> Kan hænge i længe i rutinen. (800ms)
    IEC1SET = _IEC1_U2EIE_MASK;

    // Enable UART2_RX Interrupt
    IEC1SET = _IEC1_U2RXIE_MASK;
    
}
#endif

// Put data into RX buffer - Called from ISR RX
static inline bool UART1_PushByteInRxBuff(unsigned char ByteToReadBuff)
{
    bool isSuccess = false;
    unsigned int tempInIndex;
    tempInIndex = UART1_Buffer.Index_RXIn + 1;
    
    
    //Loop round if end of buffer is reached
    if (tempInIndex >= UART_READ_BUFFER_SIZE)
        tempInIndex = 0;
    
    //Check if RX buffer is full - Overflow
    if (tempInIndex == UART1_Buffer.Index_RXOut)
    {
        //Force to empty
        //BUZ_SetMelody(BUZ_BIP);
        UART1_Buffer.Index_RXIn = 0;
        UART1_Buffer.Index_RXOut = 0;
    }
    else
    {
        //Put data to buffer
        UART1_Buffer.ReadBuffer[UART1_Buffer.Index_RXIn] = ByteToReadBuff;
        UART1_Buffer.Index_RXIn = tempInIndex;
        isSuccess = true;
    }
    return isSuccess;
}

#ifdef BLUETOOTH_CODE
// Put data into RX buffer - Called from ISR RX
static inline bool UART2_PushByteInRxBuff(unsigned char ByteToReadBuff)
{
    bool isSuccess = false;
    unsigned int tempInIndex;
    tempInIndex = UART2_Buffer.Index_RXIn + 1;
    
    //Loop round if end of buffer is reached
    if (tempInIndex >= UART2_READ_BUFFER_SIZE)
        tempInIndex = 0;
    
    //Check if RX buffer is full - Overflow
    if (tempInIndex == UART2_Buffer.Index_RXOut)
    {
        //COM_GetRXData(&UART2_Buffer);
        //BUZ_SetMelody(BUZ_BIP);
        UART2_Buffer.Index_RXIn = 0;
        UART2_Buffer.Index_RXOut = 0;
    }
    else
    {
        //Put data to buffer
        UART2_Buffer.ReadBuffer[UART2_Buffer.Index_RXIn] = ByteToReadBuff;
        UART2_Buffer.Index_RXIn = tempInIndex;
        isSuccess = true;
    }
    return isSuccess;
}
#endif

unsigned char UART1_ReadByte()
{
    unsigned char Byte = 0;
    UART1_RX_INT_DISABLE();
    
    unsigned int OutIndex = UART1_Buffer.Index_RXOut;
    unsigned int InIndex = UART1_Buffer.Index_RXIn;
    
    //If there is new data in buffer
    if (OutIndex != InIndex)
    {
        Byte = UART1_Buffer.ReadBuffer[OutIndex];
        
        OutIndex++;
        if (OutIndex >= UART_READ_BUFFER_SIZE)
            OutIndex = 0;
        
        UART1_Buffer.Index_RXOut = OutIndex;
        
        /*//Store byte in argument buffer (pointer)
        Byte = UART1_Buffer.ReadBuffer[UART1_Buffer.Index_RXOut++];

        //Loop round if end is reached
        if (UART1_Buffer.Index_RXOut >= UART_READ_BUFFER_SIZE)
            UART1_Buffer.Index_RXOut = 0;*/
    }
    
    UART1_RX_INT_ENABLE();

    //Return number of read bytes
    return Byte;
}

#ifdef BLUETOOTH_CODE
unsigned char UART2_ReadByte()
{
    unsigned char Byte = 0;
    UART2_RX_INT_DISABLE();
    
    unsigned int OutIndex = UART2_Buffer.Index_RXOut;
    unsigned int InIndex = UART2_Buffer.Index_RXIn;
    
    //If there is new data in buffer
    if (OutIndex != InIndex)
    {
        Byte = UART2_Buffer.ReadBuffer[OutIndex];
        
        OutIndex++;
        if (OutIndex >= UART_READ_BUFFER_SIZE)
            OutIndex = 0;
        
        UART2_Buffer.Index_RXOut = OutIndex;
        
        /*//Store byte in argument buffer (pointer)
        Byte = UART2_Buffer.ReadBuffer[UART2_Buffer.Index_RXOut++];

        //Loop round if end is reached
        if (UART2_Buffer.Index_RXOut >= UART_READ_BUFFER_SIZE)
            UART2_Buffer.Index_RXOut = 0;*/
    }
    
    UART2_RX_INT_ENABLE();

    //Return number of read bytes
    return Byte;
}
#endif

// This routine is only called from ISR
static bool UART1_TxPullByte(unsigned char* ArgBuffer_Byte)
{
    bool isSuccess = false;

    //Check if there is data in buffer
    if (UART1_Buffer.Index_TXOut != UART1_Buffer.Index_TXIn)
    {
        //Get byte to write
        *ArgBuffer_Byte = UART1_Buffer.WriteBuffer[UART1_Buffer.Index_TXOut];
        
        //Loop round if end is reached
        UART1_Buffer.Index_TXOut++;
        if (UART1_Buffer.Index_TXOut >= UART_WRITE_BUFFER_SIZE)
            UART1_Buffer.Index_TXOut = 0;
        
        isSuccess = true;
    }

    return isSuccess;
}

#ifdef BLUETOOTH_CODE
// This routine is only called from ISR
static bool UART2_TxPullByte(unsigned char* ArgBuffer_Byte)
{
    bool isSuccess = false;

    //Check if there is data in buffer
    if (UART2_Buffer.Index_TXOut != UART2_Buffer.Index_TXIn)
    {
        //Get byte to write
        *ArgBuffer_Byte = UART2_Buffer.WriteBuffer[UART2_Buffer.Index_TXOut];
        
        //Loop round if end is reached
        UART2_Buffer.Index_TXOut++;
        if (UART2_Buffer.Index_TXOut >= UART2_WRITE_BUFFER_SIZE)
            UART2_Buffer.Index_TXOut = 0;
        
        isSuccess = true;
    }

    return isSuccess;
}
#endif

//Used by application
static inline bool UART1_TxPushByte(unsigned char ByteToWrite)
{
    unsigned int tempInIndex;
    bool isSuccess = false;

    tempInIndex = UART1_Buffer.Index_TXIn + 1;

    if (tempInIndex >= UART_WRITE_BUFFER_SIZE)
    {
        tempInIndex = 0;
    }
    //If not full place in buffer
    if (tempInIndex != UART1_Buffer.Index_TXOut)
    {
        UART1_Buffer.WriteBuffer[UART1_Buffer.Index_TXIn] = ByteToWrite;
        UART1_Buffer.Index_TXIn = tempInIndex;
        isSuccess = true;
    }
    else
    {
        //BUZ_SetMelody(BUZ_BIP);
    }

    return isSuccess;
}

#ifdef BLUETOOTH_CODE
//Used by application
static inline bool UART2_TxPushByte(unsigned char ByteToWrite)
{
    unsigned int tempInIndex;
    bool isSuccess = false;

    tempInIndex = UART2_Buffer.Index_TXIn + 1;

    if (tempInIndex >= UART2_WRITE_BUFFER_SIZE)
    {
        tempInIndex = 0;
    }
    //If not full place in buffer
    if (tempInIndex != UART2_Buffer.Index_TXOut)
    {
        UART2_Buffer.WriteBuffer[UART2_Buffer.Index_TXIn] = ByteToWrite;
        UART2_Buffer.Index_TXIn = tempInIndex;
        isSuccess = true;
    }

    return isSuccess;
}
#endif


#ifdef BLUETOOTH_CODE
//Used by application to send bytes
size_t UART2_Write(uint8_t* WriteBuffer, size_t Size )
{
    size_t BytesWritten = 0;
    UART2_TX_INT_DISABLE();

    //Write all bytes from WriteBuffer to Uart TX buffer
    while (BytesWritten < Size)
    {
        if (UART2_TxPushByte(WriteBuffer[BytesWritten]) == true)
        {
            //Write ok. Increment counter for written bytes.
            BytesWritten++;
            UART2_Buffer.CharLoopCount++;
        }
        else
        {       
            //Buffer is full, exit the loop
            break;
        }
    }

    if(UART2_Buffer.CharLoopCount < CHARS_PER_LOOP)
    {
        // Check if any data is pending for transmission 
        if (UART_WritePendingBytesGet(&UART2_Buffer) > 0)
        {
            /* Enable TX interrupt as data is pending for transmission */
            UART2_TX_INT_ENABLE();
        }
    }
    else
        UART2_Buffer.CharLoopCount = CHARS_PER_LOOP;
    return BytesWritten;
}
#endif

//Used by application to send bytes
size_t UART1_Write(uint8_t* WriteBuffer, size_t Size )
{
    size_t BytesWritten = 0;
    UART1_TX_INT_DISABLE();

    //Write all bytes from WriteBuffer to Uart TX buffer
    while (BytesWritten < Size)
    {
        if (UART1_TxPushByte(WriteBuffer[BytesWritten]) == true)
        {
            //Write ok. Increment counter for written bytes.
            BytesWritten++;
            
        }
        else
        {       
            //Buffer is full, exit the loop
            break;
        }
    }

    // Check if any data is pending for transmission 
    if (UART_WritePendingBytesGet(&UART1_Buffer) > 0)
    {
        /* Enable TX interrupt as data is pending for transmission */
        UART1_TX_INT_ENABLE();
    }

    return BytesWritten;
}

//INTERUPTS 
void __ISR(_UART_1_VECTOR, ipl3AUTO) UART_1_Handler (void)
{
    //Call RX handler if RX interrupt flag is set
    if ((IFS1 & _IFS1_U1RXIF_MASK) && (IEC1 & _IEC1_U1RXIE_MASK))
    {
        // Clear UART1 RX Interrupt flag
        IFS1CLR = _IFS1_U1RXIF_MASK;

        // Keep reading until there is a character availabe in the RX FIFO
        while((U1STA & _U1STA_URXDA_MASK) == _U1STA_URXDA_MASK)
            UART1_PushByteInRxBuff((unsigned char)(U1RXREG));
    }
    
    //Call TX handler if TX interrupt flag is set
    else if ((IFS1 & _IFS1_U1TXIF_MASK) && (IEC1 & _IEC1_U1TXIE_MASK))
    {
        unsigned char ByteToWrite;

        if(UART1_Buffer.CharLoopCount > CHARS_PER_LOOP)
        {
            Timer_CLK10ms_Communication.Active = true;
            UART1_TX_INT_DISABLE();
        }
        // Check if any data is pending for transmission
        else if (UART_WritePendingBytesGet(&UART1_Buffer) > 0)
        {
            // Clear UART1TX Interrupt flag 
            IFS1CLR = _IFS1_U1TXIF_MASK;
            
            // Keep writing to the TX FIFO as long as there is space 
            while(!(U1STA & _U1STA_UTXBF_MASK))
            {
                if (UART1_TxPullByte(&ByteToWrite) == true)
                {
                    //Send Byte
                    U1TXREG = ByteToWrite;
                    UART1_Buffer.CharLoopCount++;
                }
                else
                {
                    // Nothing to transmit. Disable the data register empty interrupt.
                    UART1_TX_INT_DISABLE();
                    break;
                }
            }
        }
        else
        {
            // Nothing to transmit. Disable the data register empty interrupt.
            UART1_TX_INT_DISABLE();
        }
    }
    
    
    //Call Error handler if Error interrupt flag is set --> Kan hænge i længe i rutinen. (800ms)
    else if ((IFS1 & _IFS1_U1EIF_MASK) && (IEC1 & _IEC1_U1EIE_MASK))
    {      
        // Disable the interrupt
        IEC1CLR = _IEC1_U1EIE_MASK;
        IEC1CLR = _IEC1_U1RXIE_MASK;
        uint8_t DummyData;

        //Clear Error
        if(U1STAbits.OERR == 1)
            U1STAbits.OERR = 0;
        if(U1STAbits.FERR == 1)
            U1STAbits.FERR = 0;
        if(U1STAbits.PERR == 1)
            U1STAbits.PERR = 0;
        
        //Clear buffer
        while(U1STAbits.URXDA == 1)
            DummyData = (uint8_t)(U1RXREG);
        
        //Ignore unused variable
        (void)DummyData;
        
        //Clear error interrupt flag
        IFS1CLR = _IFS1_U1EIF_MASK;
        IFS1CLR = _IFS1_U1RXIF_MASK;
        
        //Enable Interupt
        IEC1SET = _IEC1_U1EIE_MASK;
        IEC1SET = _IEC1_U1RXIE_MASK;
        
        
        /*
        
        // Disable the fault interrupt
        IEC1CLR = _IEC1_U1EIE_MASK;
        // Disable the receive interrupt
        IEC1CLR = _IEC1_U1RXIE_MASK;

        //Clear Error
        while(1){
            UART_ERROR errors = UART_ERROR_NONE;
            unsigned int status = U1STA;

            //Gets error
            errors = (UART_ERROR)(status & (_U1STA_OERR_MASK | _U1STA_FERR_MASK | _U1STA_PERR_MASK));

            //If error then clear
            if(errors != UART_ERROR_NONE) {
                // Clear overrun
                U1STACLR = _U1STA_OERR_MASK;

                // Clear error interrupt flag
                IFS1CLR = _IFS1_U1EIF_MASK;

                // Clear up the receive interrupt flag so that RX interrupt is not triggered for error bytes
                 
                IFS1CLR = _IFS1_U1RXIF_MASK;
            }
            else
                break;
        
        };

        //Enable Interupt
        IEC1SET = _IEC1_U1EIE_MASK;
        IEC1SET = _IEC1_U1RXIE_MASK;
        */
    }
}

#ifdef BLUETOOTH_CODE
void __ISR(_UART_2_VECTOR, ipl2AUTO) UART_2_Handler (void)
{
    //Call RX handler if RX interrupt flag is set
    if ((IFS1 & _IFS1_U2RXIF_MASK) && (IEC1 & _IEC1_U2RXIE_MASK))
    {
        // Clear UART2 RX Interrupt flag
        IFS1CLR = _IFS1_U2RXIF_MASK;

        // Keep reading until there is a character availabe in the RX FIFO
        while((U2STA & _U2STA_URXDA_MASK) == _U2STA_URXDA_MASK)
            UART2_PushByteInRxBuff((unsigned char)(U2RXREG));
    }
    
    //Call TX handler if TX interrupt flag is set
    else if ((IFS1 & _IFS1_U2TXIF_MASK) && (IEC1 & _IEC1_U2TXIE_MASK))
    {
        unsigned char ByteToWrite;

        // Check if any data is pending for transmission
        if (UART_WritePendingBytesGet(&UART2_Buffer) > 0)
        {
            // Clear UART2TX Interrupt flag 
            IFS1CLR = _IFS1_U2TXIF_MASK;

            // Keep writing to the TX FIFO as long as there is space 
            while(!(U2STA & _U2STA_UTXBF_MASK))
            {
                if (UART2_TxPullByte(&ByteToWrite) == true)
                    //Send Byte
                    U2TXREG = ByteToWrite;
                else
                {
                    // Nothing to transmit. Disable the data register empty interrupt.
                    UART2_TX_INT_DISABLE();
                    break;
                }
            }
        }
        else
        {
            // Nothing to transmit. Disable the data register empty interrupt.
            UART2_TX_INT_DISABLE();
        }
    }
    
    
    //Call Error handler if Error interrupt flag is set --> Kan hænge i længe i rutinen. (800ms)
    else if ((IFS1 & _IFS1_U2EIF_MASK) && (IEC1 & _IEC1_U2EIE_MASK))
    {
        // Disable the interrupt
        IEC1CLR = _IEC1_U2EIE_MASK;
        IEC1CLR = _IEC1_U2RXIE_MASK;
        uint8_t DummyData;

        //Clear Error
        if(U2STAbits.OERR == 1)
            U2STAbits.OERR = 0;
        if(U2STAbits.FERR == 1)
            U2STAbits.FERR = 0;
        if(U2STAbits.PERR == 1)
            U2STAbits.PERR = 0;
        
        //Clear buffer
        while(U2STAbits.URXDA == 1)
            DummyData = (uint8_t)(U2RXREG);
        
        //Ignore unused variable
        (void)DummyData;
        
        //Clear error interrupt flag
        IFS1CLR = _IFS1_U2EIF_MASK;
        IFS1CLR = _IFS1_U2RXIF_MASK;
        
        //Enable Interupt
        IEC1SET = _IEC1_U2EIE_MASK;
        IEC1SET = _IEC1_U2RXIE_MASK;
    }   
}
#endif