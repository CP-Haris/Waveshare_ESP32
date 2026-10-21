#include "definitions.h"   
#include "RTC.h"
#include "ERC240128FS.h"
#include "Values.h"
#include "Timer.h"
#include "Button.h"
#include "Menu_Error.h"
#include "Menu_Application.h"
#include "Menu_Settings.h"
#include "Menu_Functions.h"
#include "LED.h"
#include "Buzzer.h"
#include "MEM_FLASH.h"
#include "State_Statemachine.h"
#include "Functions.h"
#include "UART_BIOS.h"
#include "UART_App.h"
#include "State_Controller.h"
#include "device.h"
#include "Text.h"
#include "BLE_App.h"

//CHANGE PROCESSOR
//APP CHANGES
//Processor in project
//Ensure it is correctly defined in plib_gpio.h (Line 47-58)
//Ensure correct Sofware verison in Values.h
//Include correct linker
//Set correct loadable boot hex file

//BOOTLOADER CHANGES
//define in bootloader_config.h
//Processor in project
//Include correct linker

inline void Main_ResetController()
{
    //Poll for communication error
    
    unsigned int status = U1STA;
    UART_ERROR errors = (UART_ERROR)(status & (_U1STA_OERR_MASK | _U1STA_FERR_MASK | _U1STA_PERR_MASK));
    if(errors>0)
    {
        RCON_SoftwareReset();
    }

    //Reset LPS if following BTNS combination is pressed
    if((BTN_UP & BTN_DOWN & BTN_BACK))
    {
        Block_3_ID_7.NumValue = VIEW_NEW;
        Block_3_ID_7.Flag.NumValue_Received = true;
        Values_Send_ByValue(0x50, &Block_3_ID_7, UART_CTRL);
        
        Value Bit = Block_3_ID_3; 
        Bit.NumValue |= (1<<19);
        Values_Send_ByValue(0x50, &Bit, UART_CTRL);
        
        while(!U1STAbits.TRMT){WDTCONbits.WDTCLR = 1;};
        U1MODEbits.ON = 0;
        U1STAbits.OERR = 0;
        U1STAbits.PERR = 0;
        U1STAbits.FERR = 0;
        
        RCON_SoftwareReset();
    }

    //Reset WDT and put to sleep
    WDTCONbits.WDTCLR = 1;
    if(GlobalState.Update_Logic == false)
        asm volatile ( "wait" );
}


int main ( void )
{      
    //Initialize Peripherals
    SYS_Initialize (NULL);
    
    //Timer (100mS) 
    TMR2_CallbackRegister(&Timer_TMR2_100ms_Interrupt, NULL);
    TMR2_Start();   //100ms Timer for Software Timers

     //Init Controlboard communication
    UART1_Init(115200, true);
    
    //Timer (10mS)
    TMR1_CallbackRegister(&Timer_TMR1_10ms_Interrupt, NULL);
    TMR1_Start();   //10ms  Timer for Buttons
    
    //Play Tone Interrupt (125ms)
    TMR4_CallbackRegister(&BUZ_INT_Player, NULL);   
    TMR4_Start();   //125ms Timer for TONES
    TMR3_Start();   //250us Timer for BUZER
    
    //Store reset cause in INTERNAL_RCON value
    Block_200_ID_1.NumValue = (unsigned long)(RCON & 0b0000001111111111);
    
    //Init Bluetooth communication
    #ifdef BLUETOOTH_CODE        
        // Bluetooth is setup after requesting initial values (Values_RequestInitial()) to know whether or not to turn it on
        // The Bluetooth on/off bit is stored on the controlboard
    
        /*
        // This implementation used "persistent" Block 200 ID 100, but the bootloader wipes the RAM
        // Initialize Bluetooth power control in case it's a power-out and/or brown-out reset
        Block_200_ID_100.Flag.Owned_BLE = true;
        Block_200_ID_100.Block = 200;
        Block_200_ID_100.ID = 100;
        Block_200_ID_100.Prefix = MASKED_BITMAP;

        if (Block_200_ID_1.NumValue & 0b11) {
            BLE_SetPowerSetting(true);
        } else { // On software reset, value has already been initialized
            BLE_UpdatePowerSetting();
        }*/
    #else
        BLE_ON_Set(); //Inverse logic (P-Channel)
    #endif

    //Clear reset flag
    RCON_ResetCauseClear(RESET_REASON_ALL);
    
    //Get NVM Bootloader value from 0x9D003FF0(Virtual) 0x1D003FF0(Physical)
    Block_252_ID_230.NumValue = *(const long*)(0x9D003FF0);
    Block_252_ID_230.Flag.NumValue_Received = true;
    
    //Send reset cause
    Values_Send_ByValue(0x52, &Block_200_ID_1, UART_CTRL);
    
    //Send Software Version - Request start application
    Values_Send_ByValue(0x52, &Block_200_ID_0, UART_CTRL);
    
    //Send Bootloader Version
    Values_Send_ByValue(0x52, &Block_252_ID_230, UART_CTRL);
    
    //Set Initial State 
    GlobaState_Initial(MAINSTATE_INIT);
    
    while(true)
    {
        //Read RX data. Request logic update if new data is received.       
        if(COM_GetRXData(&UART1_Buffer))
            GlobalState.Update_Logic = true;
        
        #ifdef BLUETOOTH_CODE
        if(COM_GetRXData(&UART2_Buffer) && (GlobalState_GetActual() != MAINSTATE_OFF))
            GlobalState.Update_Logic = true;
        #endif

        //Change the logic state of interface. Change_State variable is cleared by State_Update function.        
        if(GlobalState.Update_Logic || KEYPAD_ACTIVITY)
            State_UpdateLogic();

        //Request values for active state
        if(GlobalState.Update_Request)
        {
            //Send Active State Info
            GlobaState_SendActiveState(UART_CTRL);
            #ifdef BLUETOOTH_CODE
            GlobaState_SendActiveState(UART_BLE);
            #endif
            
            //Send Communication Timeout Timer (Only to Controlboard)
            GlobaState_SendCommunicationTimer(UART_CTRL);
            
            //Request Statemachine values
            State_PeriodicalRequest();
        }
        
        //Update GUI
        if(GlobalState.Update_Graphic)
            State_UpdateGraphics();
        
        Main_ResetController();
    }
    
    /* Execution should not come here during normal operation */
    return ( EXIT_FAILURE );
}