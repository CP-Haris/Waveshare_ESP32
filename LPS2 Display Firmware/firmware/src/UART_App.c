#include "State_Controller.h"
#include "Menu_Application.h"
#include <string.h>
#include "Buzzer.h"
#include "UART_App.h"
#include "CRC16.h"
#include "BLE_App.h"
#include "ERC240128FS.h"

bool COM_GetRXData(UART_Buffer * Buffer)
{
    bool NewData = false;
    
    while(UART_ReadCountGet(Buffer))
    {    
        unsigned char ReadByte = Buffer->ReadByteFunction();
        //SET Data Link Escape flag (0x10)
        if ((!Buffer->DLE_Detected) && (ReadByte == DLE))
            Buffer->DLE_Detected = true; 


        //SET Start of Header flag
        else if((!Buffer->DLE_Detected) && (ReadByte == SOH))
        {
            Buffer->Msg_Counter = 0;
            Buffer->SOH_Detected = true;
        }

        //End of Transmission received - Activate protocol
        else if((!Buffer->DLE_Detected) && (ReadByte == EOT))
        {
            Buffer->SOH_Detected = false;
            uint16_t CRC_Calc = CalculateCrc(&Buffer->Msg_Buffer[0], Buffer->Msg_Counter-2);
            uint16_t CRC_Read = (Buffer->Msg_Buffer[Buffer->Msg_Counter-1] << 8) + (Buffer->Msg_Buffer[Buffer->Msg_Counter-2]);
            if(CRC_Calc == CRC_Read)
            {
                //Remove CRC bytes (2 last bytes)
                Buffer->Msg_Counter = Buffer->Msg_Counter-2;
                              
                if(Buffer->Uart_Module == UART_CTRL)
                {
                    Timer_StartTimer(&Timer_CLK100ms_Communication); // Control Board Communication WDT reset
                    
                    //Exit Debug mode
                    if(GlobalState_GetActual() == MAINSTATE_DEBUG)
                        GlobaState_Initial(MAINSTATE_INIT);
                    
                    //Case is based on CMD
                    switch (Buffer->Msg_Buffer[0])
                    {
                        //Value
                        case 0x20:
                        {     
                            Value *Val = Values_GetValue(Buffer->Msg_Buffer[1],Buffer->Msg_Buffer[2]);
                            
                            //Set value into RAM
                            if(Val != NULL)
                            {
                                if(Buffer->Msg_Counter == 7){
                                    Val->NumValue = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16) | (long)(Buffer->Msg_Buffer[4] << 8) | (long)Buffer->Msg_Buffer[3];
                                    Val->Flag.NumValue_Received = true;
                                    GlobalState.Update_Graphic = true;

                                    //Forward value to BLE if BLE is using it
                                    #ifdef BLUETOOTH_CODE
                                    if(UART2_IfEnable() && Val->Flag.Access_BLE)
                                    {
                                        if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED) //Connected
                                            SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_BLE);
                                        else if((Val == &Block_0_ID_220) || (Val == &Block_0_ID_119))
                                            SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_BLE);
                                    }
                                    #endif
                                }
                                //Request
                                else
                                {
                                    //Forward request to BLE
                                    if(Val->Flag.Owned_BLE)
                                        SendMSG (&Buffer->Msg_Buffer[0], Buffer->Msg_Counter, UART_BLE);
                                    //Responde to request
                                    else if(Val->Flag.Access_CTRL)
                                        Values_Send_ByValue(Buffer->Msg_Buffer[0], Val, UART_CTRL);
                                }
                            }
                        }
                        break;

                        //Value (MIN/MAX)
                        case 0x22: 
                        {
                            Value *Val = Values_GetValue(Buffer->Msg_Buffer[1],Buffer->Msg_Buffer[2]);
                          
                            //Set value into RAM
                            if((Val != NULL) && (Buffer->Msg_Counter == 11)){
                                Val->NumMin = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16) | (long)(Buffer->Msg_Buffer[4] << 8) | (long)Buffer->Msg_Buffer[3];;
                                Val->NumMax = (long)(Buffer->Msg_Buffer[10] << 24)  | (long)(Buffer->Msg_Buffer[9] << 16) | (long)(Buffer->Msg_Buffer[8] << 8) | (long)Buffer->Msg_Buffer[7];;
                                Val->Flag.NumMinMax_Received = true;
                                
                                //Forward value to BLE if BLE is using it
                                #ifdef BLUETOOTH_CODE
                                if(UART2_IfEnable() && Val->Flag.Access_BLE)
                                    if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED) //Connected
                                        SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_BLE);
                                #endif
                            }
                        }
                        break;

                        //Value (Default)
                        case 0x24:   
                        {
                            Value *Val = Values_GetValue(Buffer->Msg_Buffer[1],Buffer->Msg_Buffer[2]);
                            
                            //Set value into RAM
                            if((Val != NULL) && (Buffer->Msg_Counter == 7)){
                                Val->NumDefault = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16) | (long)(Buffer->Msg_Buffer[4] << 8) | (long)Buffer->Msg_Buffer[3];
                                Val->Flag.NumDefault_Received = true;
                                
                                //Forward value to BLE if BLE is using it
                                #ifdef BLUETOOTH_CODE
                                if(UART2_IfEnable() && Val->Flag.Access_BLE)
                                    if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED) //Connected
                                        SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_BLE);
                                #endif
                            }
                        }
                        break;

                        //Button Press infromation from LPS (12V / 230V)
                        case 0x31:
                            if(Block_3_ID_3.NumValue & (1<<17))
                                BUZ_SetMelody(BUZ_BIP);
                        break;

                        //Update Status from Main Processor
                        case 0x32:  
                            App_UpdateFlags.Byte[0] = Buffer->Msg_Buffer[1];
                        break;

                        //Tone Request from LPS
                        case 0x33:
                            if(Buffer->Msg_Buffer[1] < NUMBER_OF_TONES)
                                BUZ_SetMelody(Buffer->Msg_Buffer[1]);
                        break;

                        //Error buffer
                        case 0x41:
                        {
                            //Send to BLE:
                            #ifdef BLUETOOTH_CODE
                            if(UART2_IfEnable())
                            {
                                if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED) //Connected
                                    SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_BLE);
                            }
                            #endif

                            //Save old error buffer in a temporary array
                            unsigned char LPSErrorBuffer_OLD[8];
                            memcpy(LPSErrorBuffer_OLD, LPSErrorBuffer, 8);

                            //Save new error buffer in error buffer array.
                            memcpy(LPSErrorBuffer, &Buffer->Msg_Buffer[1], 8);

                            //Clear All Old Errors
                            unsigned char i;
                            for(i = 0; i<8; i++){
                                //If not present 
                                if(!Error_CheckForErrorInBuf(LPSErrorBuffer_OLD[i]))
                                {
                                    ErrorFlags[LPSErrorBuffer_OLD[i]].Active = false;
                                    ErrorFlags[LPSErrorBuffer_OLD[i]].Minimized = false;
                                }
                            }

                            //Set New Errors
                            for(i = 0; i<8; i++)
                            {
                                Menu_ERRORS[i].ST_Value->Data->Flag.NumValue_Received = true;
                                
                                //If there is a error in buffer
                                if(LPSErrorBuffer[i] > 0)
                                {
                                    ErrorFlags[LPSErrorBuffer[i]].Active = true;
                                    Menu_ERRORS[i].Show = MENU_SHOW;
                                    Menu_ERRORS[i].ST_Value->Data->NumValue = LPSErrorBuffer[i];
                                    Menu_ERRORS[i].S8A_Title = ErrorList[LPSErrorBuffer[i]].Title;
                                }
                                else
                                    Menu_ERRORS[i].Show = false;
                            }
                        }
                        break;

                        //Model Name
                        case 0x42:
                            //Send to BLE: (Model defined by lookup using serial number)
                            //#ifdef BLUETOOTH_CODE 
                            //    if(Block_200_ID_101.NumValue > 0) //Connected
                            //        SendMSG (&Buffer->Msg_Buffer[1], Buffer->Msg_Counter-2, UART_BLE);;
                            //#endif

                            memcpy(ModelValue, &Buffer->Msg_Buffer[1], Buffer->Msg_Buffer[1]+1);
                        break;

                        case 0x06:
                            if((Buffer->Msg_Buffer[1] == 1)&&(Buffer->Msg_Buffer[2] == 2)&&(Buffer->Msg_Buffer[3] == 3)&&(Buffer->Msg_Buffer[4] == 4))
                                //\TODO: The reset must happen within 10 ms (fails at 35 ms + margin), because the controlboard bootloader expects it in bootmode then
                                /* This should clear the screen and show that is has entered bootmode, however it bricks the bootloader due to comment above */
                                /*UART1_Disable();
                                UART2_Disable();
                                __builtin_disable_interrupts();
                                ERC240_Display_Square(0, 0, 239, 127);
                                ERC240_Display_Square(1, 1, 238, 126);
                                ERC240_Display_String(10,105, ENG_BootMode,0,'L',(unsigned char*)S11N_Helvetica15x17);
                                //waitms(30); //WDT cleared inside function*/
                                /* Temporary way of handling bootmode: Just put display to sleep (ONLY WORKS ON NEW HW 02:40>=)*/
                                ERC240_EnterSleep();
                                LCD_RST_Clear();
                                RCON_SoftwareReset();
                        break;
                        
                        default:
                            SendMSG (&Buffer->Msg_Buffer[0], Buffer->Msg_Counter, UART_BLE);
                        break;
                    }
                    NewData = true;
                }
                
                #ifdef BLUETOOTH_CODE
                if(Buffer->Uart_Module == UART_BLE)
                {
                    switch (Buffer->Msg_Buffer[0])
                    {
                        //Requested value - RESPONSE
                        case 0x20:
                        {    
                            Value *Val = Values_GetValue(Buffer->Msg_Buffer[1],Buffer->Msg_Buffer[2]);
                            
                            //Set value into RAM
                            if(Val != NULL)
                            {
                                // If request is for error buffer, ensure that it is up-to-date
                                if((Val->Block == 200) && (Val->Prefix == ERROR) && !(Val->Flag.NumValue_Received)) {
                                    unsigned char Data = 0x41;
                                    SendMSG(&Data, 1, UART_CTRL);
                                }
                                
                                if(Buffer->Msg_Counter == 7){
                                    Val->NumValue = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16) | (long)(Buffer->Msg_Buffer[4] << 8) | (long)Buffer->Msg_Buffer[3];
                                    Val->Flag.NumValue_Received = true;
                                    GlobalState.Update_Graphic = true;

                                    //Forward value to CRTL if CTRL is using it
                                    if( Val->Flag.Access_CTRL)
                                        SendMSG (Buffer->Msg_Buffer, Buffer->Msg_Counter, UART_CTRL);
                                }
                                //Request
                                else
                                {
                                    //Forward request to controlboard
                                    if(Val->Flag.Owned_CTRL)
                                        SendMSG (&Buffer->Msg_Buffer[0], Buffer->Msg_Counter, UART_CTRL);
                                    //Responde to request
                                    else if(Val->Flag.Access_BLE)
                                        Values_Send_ByValue(Buffer->Msg_Buffer[0], Val, UART_BLE);
                                }
                            }
                        }
                        break;
                        
                        //Receive Passkey and MAC
                        case 0x50:
                        {    
                            Value *Val = Values_GetValue(Buffer->Msg_Buffer[1], Buffer->Msg_Buffer[2]);
                            
                            if(Val != NULL){
                                Val->NumValue = (long)(Buffer->Msg_Buffer[6] << 24) | (long)(Buffer->Msg_Buffer[5] << 16) | (long)(Buffer->Msg_Buffer[4] << 8) | (long)Buffer->Msg_Buffer[3]; 
                                Val->Flag.NumValue_Received = true;
                                
                                if(Val->ID == 102)
                                    BUZ_SetMelody(BUZ_BIP);
                            }
                        }
                        break;
                        
                        //Function Control
                        case 0x80:
                        {
                            if(Buffer->Msg_Buffer[2] == 0xCB)
                            {
                                unsigned char ON_12VDC[7] = {0x50, 0x00, 0x9F, 0x00, 0x00, 0x01, 0x00};
                                unsigned char OFF_12VDC[7] = {0x50, 0x00, 0x9F, 0x00, 0x00, 0x00, 0x00};
                                if(Block_0_ID_203.NumValue >= OS_READY_TO_START)
                                    SendMSG (OFF_12VDC, 7, UART_CTRL);
                                else
                                    SendMSG (ON_12VDC, 7, UART_CTRL);                       
                            }
                            else if(Buffer->Msg_Buffer[2] == 0xC9)
                            {
                                unsigned char ON_230VAC[7] = {0x50, 0x00, 0x9E, 0x00, 0x00, 0x01, 0x00};
                                unsigned char OFF_230VAC[7] = {0x50, 0x00, 0x9E, 0x00, 0x00, 0x00, 0x00};
                                if(Block_0_ID_201.NumValue >= OS_READY_TO_START)
                                    SendMSG (OFF_230VAC, 7, UART_CTRL);
                                else
                                    SendMSG (ON_230VAC, 7, UART_CTRL);
                            }
                        }
                        break;
                        
                        default:
                            SendMSG (&Buffer->Msg_Buffer[0], Buffer->Msg_Counter, UART_CTRL);
                        break;
                    }
                }
                #endif
            }
            //Add else statment for bad CRC here
        }
        //Add data to buffer if SOH flag is set
        else if(Buffer->SOH_Detected)
        {
            Buffer->DLE_Detected = false;            
            Buffer->Msg_Buffer[Buffer->Msg_Counter] = ReadByte;
            Buffer->Msg_Counter++;
            
            if(Buffer->Msg_Counter >= 20) { // In case SOH is detected but not EOT, buffer could overflow. This detects and resets.
                Buffer->Msg_Counter = 0;
                Buffer->SOH_Detected = false;
            }
        } 
    }
    return NewData;
}

void SendMSG (uint8_t* SRC, uint8_t SRC_Length, uint8_t UartSelect)
{
    //Create container for the framed message
    uint8_t DST[RX_MSG_SIZE]; //TODO: Can overflow when DLE is included
    size_t DST_Counter = 0;
    
    //Add Starting Of Heading (SOH)
    DST[DST_Counter] = 0x01;
    DST_Counter++;

    //Add data with Data Link Escape (DLE)
    uint8_t i;
    for(i=0; i<SRC_Length; i++)
    {
        if((SRC[i]==0x04)||(SRC[i]==0x01)||(SRC[i]==0x10))
        {
            DST[DST_Counter] = 0x10;
            DST_Counter++;
        }
        DST[DST_Counter] = SRC[i];
        DST_Counter++;
    }
    
    //Add CRC
    unsigned short CRC16;
    CRC16 = CalculateCrc(SRC, SRC_Length);
    
    uint8_t CRC16_Arr[2];
    CRC16_Arr[0] = (CRC16);
    CRC16_Arr[1] = (CRC16>>8);

    for(i=0; i<2; i++)
    {
        if((CRC16_Arr[i]==0x04)||(CRC16_Arr[i]==0x01)||(CRC16_Arr[i]==0x10))
        {
            DST[DST_Counter] = 0x10;
            DST_Counter++;
        }
        
        DST[DST_Counter] = CRC16_Arr[i];
        DST_Counter++;
    }   
    
    //Add End Of Transmission (EOT)
    DST[DST_Counter] = 0x04;
    DST_Counter++;
    
    if((DST[1] == 0) && (DST[2] == 103))
    {
        DST_Counter++;
        DST_Counter--;
    }
    
    #ifdef BLUETOOTH_CODE
    //Send Message
    if(UartSelect == UART_CTRL)
        UART1_Write(DST, DST_Counter);
    else if(UartSelect == UART_BLE)
        UART2_Write(DST, DST_Counter); 
    else
    {
        UART1_Write(DST, DST_Counter);
        UART2_Write(DST, DST_Counter);
    } 
    #else
    UART1_Write(DST, DST_Counter);
    #endif
}