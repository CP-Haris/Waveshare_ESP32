#include "Timer.h"
#include "definitions.h"
#include "Buzzer.h"
#include "ERC240128FS.h"
#include "Button.h"
#include "Menu_Application.h"
#include "Menu_LockScreen.h"
#include "State_Controller.h"
#include "UART_BIOS.h"

//Interrupt Prototype
void Timer_SW_ISR_SplashScreen();
void Timer_SW_ISR_UART_Request();
void Timer_SW_ISR_SettingLock();
void Timer_SW_ISR_Blinking();
void Timer_SW_ISR_LED_Blinking();
void Timer_SW_ISR_StartTX();
void Timer_SW_ISR_ReInit();
void Timer_SW_ISR_PassKeyOff();
void Timer_SW_ISR_CommunicationWDT();
void Timer_SW_ISR_Init();
void Timer_SW_ISR_DebugTimeout();


//Defined in Menu
extern bool SettingsLocked;

//Software Timers
volatile Timer Timer_CLK100ms_MenuTimeout            = {.Limit = 3000,                                           .Continues = false};  //5min
volatile Timer Timer_CLK100ms_SplashScreen           = {.Limit = 20,    .ISR = Timer_SW_ISR_SplashScreen,        .Continues = false};  //2s
volatile Timer Timer_CLK100ms_Request                = {.Limit = 10,    .ISR = Timer_SW_ISR_UART_Request,        .Continues = false};  //1s
volatile Timer Timer_CLK100ms_Blinking               = {.Limit = 4,     .ISR = Timer_SW_ISR_Blinking,            .Continues = false};  //400ms
volatile Timer Timer_CLK100ms_Communication          = {.Limit = 100,   .ISR = Timer_SW_ISR_CommunicationWDT,    .Continues = false};  //10s
volatile Timer Timer_CLK100ms_SettingLock            = {.Limit = 6000,    /*.ISR = Timer_SW_ISR_SettingLock,*/   .Continues = false};  //10min
volatile Timer Timer_CLK100ms_InitLoop               = {.Limit = 2,     .ISR = Timer_SW_ISR_Init,                .Continues = false};  //200ms - Changed to 200ms due to not working display 12-02-2024
//volatile Timer Timer_CLK100ms_1SecGP                 = {.Limit = 2,                                             .Continues = false}; //Added 12-02-2024 due to non working display
volatile Timer Timer_CLK10ms_SystemConfigTimeout     = {.Limit = 5,                                              .Continues = false};  //50ms
volatile Timer Timer_CLK10ms_Communication           = {.Limit = 5,     .ISR = Timer_SW_ISR_StartTX,             .Continues = false};  //50ms
volatile Timer Timer_CLK100ms_LED_Blinking           = {.Active = true,  .Limit = 1,     .ISR = Timer_SW_ISR_LED_Blinking,        .Continues = true}; //100ms
volatile Timer Timer_CLK100ms_ReInit                 = {.Limit = 20,    .ISR = Timer_SW_ISR_ReInit,              .Continues = false};  //2s
volatile Timer Timer_CLK100ms_PowerDown              = {.Limit = 80,                                             .Continues = false};  //8s
volatile Timer Timer_CLK100ms_PassKeyOff             = {.Limit = 600,   .ISR = Timer_SW_ISR_PassKeyOff,          .Continues = false};  //1min
volatile Timer Timer_CLK100ms_DebugTimeout           = {.Limit = 6000,  .ISR = Timer_SW_ISR_DebugTimeout,        .Continues = false};  //10min

void Timer_Controller(volatile struct Timer *SW_Timer)
{
    if(SW_Timer->Active)
    {
        SW_Timer->Value++; 
        
        //Timer elased
        if(SW_Timer->Value >= SW_Timer->Limit)
        {
            SW_Timer->Active = SW_Timer->Continues;
            SW_Timer->Toggler = !SW_Timer->Toggler;
            SW_Timer->Flag = true;
            SW_Timer->Value = 0;
            
            //Execute software ISR if set
            if(SW_Timer->ISR != NULL)
                SW_Timer->ISR();
        }
    }
}

void Timer_StartTimer(volatile struct Timer *Timer)
{
    Timer->Active = true;
    Timer->Flag = false;
    Timer->Value = 0;
}

void Timer_StopTimer(volatile struct Timer *Timer)
{
    Timer->Active = false;
    Timer->Flag = false;
    Timer->Value = 0;
}

//// HARDWARE INTERUPTS ////
//100ms interrupt
void Timer_TMR2_100ms_Interrupt()
{       
        Timer_Controller(&Timer_CLK100ms_InitLoop);
        Timer_Controller(&Timer_CLK100ms_MenuTimeout);
        Timer_Controller(&Timer_CLK100ms_SplashScreen);
        Timer_Controller(&Timer_CLK100ms_Request);
        Timer_Controller(&Timer_CLK100ms_Blinking);
        Timer_Controller(&Timer_CLK100ms_Communication);  
        Timer_Controller(&Timer_CLK100ms_SettingLock);
        Timer_Controller(&Timer_CLK100ms_LED_Blinking);
        //Timer_Controller(&Timer_CLK100ms_1SecGP); //12-02-2024
        Timer_Controller(&Timer_CLK100ms_ReInit);
        Timer_Controller(&Timer_CLK100ms_PowerDown);
        Timer_Controller(&Timer_CLK100ms_PassKeyOff);
        Timer_Controller(&Timer_CLK100ms_DebugTimeout);
}
//10ms interrupt
void Timer_TMR1_10ms_Interrupt()
{  
    Timer_Controller(&Timer_CLK10ms_SystemConfigTimeout);
    Timer_Controller(&Timer_CLK10ms_Communication);
    
    //NO beeping on display off or transitioning
    if((GlobalState.State[GlobalState.StateIndex] != MAINSTATE_OFF) && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_POWERDOWN)
        && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_INIT))
    {
        Button_TimerController(BTN_UP, &KeypadLPS.UP);
        Button_TimerController(BTN_DOWN, &KeypadLPS.DOWN);
        Button_TimerController(BTN_BACK, &KeypadLPS.BACK);
        Button_TimerController(BTN_OK, &KeypadLPS.OK); 
    }
}

//// SOFTWARE INTERUPTS ////
void Timer_SW_ISR_UART_Request()
{
    GlobalState.Update_Request = true;
    Timer_StartTimer(&Timer_CLK100ms_Request);
}

void Timer_SW_ISR_SplashScreen()
{
    GlobalState.Update_Request = true;
}

//Not Used. Polling is used instead in UpdateLogic Menu
void Timer_SW_ISR_SettingLock()
{
    SettingsUnlocked = false;
}

void Timer_SW_ISR_Blinking()
{
    GlobalState.Update_Logic = true;
}

// This function is only triggered if there is no communication from the controlboard for 10 seconds.
// This may be due to noise or periodic errors which require a reset, or it may be due to entering sleep mode.
// Either way, the display processor should also enter sleep mode.
// If communication persists, it will wake and reset.
void Timer_SW_ISR_CommunicationWDT()
{
    System_EnterSleep();
}

void Timer_SW_ISR_Init() {
    GlobalState.Update_Logic = true;
}

void Timer_SW_ISR_DebugTimeout() {
    GlobalState.Update_Logic = true;
}

void Timer_SW_ISR_StartTX()
{
#ifdef BLUETOOTH_CODE
    // Check if any data is pending for transmission 
    if (UART_WritePendingBytesGet(&UART2_Buffer) > 0)
    {
        UART2_Buffer.CharLoopCount = 0;
        /* Enable TX interrupt as data is pending for transmission */
        UART2_TX_INT_ENABLE();
    }
#endif
    
    // Check if any data is pending for transmission 
    //if (UART_WritePendingBytesGet(&UART1_Buffer) > 0)
    //{
        UART1_Buffer.CharLoopCount = 0;
        /* Enable TX interrupt as data is pending for transmission */
        UART1_TX_INT_ENABLE();
    //}
}

int Blink = 0;
volatile bool Blink12VDC = false;
volatile bool Blink230VAC = false;
void Timer_SW_ISR_LED_Blinking()
{
    if(Blink230VAC)
    {
        if(Blink < 4){
            LED_G_230V_Set();
            LED_R_230V_Clear();
        }
        else if (Blink < 10){
            if((Block_0_ID_205.NumValue >= FL_SIMPLE_FAILURE) && (Block_0_ID_201.NumValue >= OS_WAKEUP))
                LED_R_230V_Set();
            LED_G_230V_Clear();
        }
        else{
            LED_G_230V_Set();
            LED_R_230V_Clear();
        }
    }
    
    if(Blink12VDC)
    {
        if(Blink < 4){
            LED_G_12V_Set();
            LED_R_12V_Clear();
        }
        else if (Blink < 10){
            if((Block_0_ID_207.NumValue >= FL_SIMPLE_FAILURE) && (Block_0_ID_203.NumValue >= OS_WAKEUP))
                LED_R_12V_Set();
            LED_G_12V_Clear();
        }
        else{
            LED_G_12V_Set();
            LED_R_12V_Clear();
        }
    }
    
    if(Blink == 10)
        Blink = 0;
    else
        Blink++;
}

void Timer_SW_ISR_ReInit() {
    //Re-initializes display every 2 seconds in case of accidental display-off due to ESD or mechanical shock
    if(!GlobalState.Update_Graphic) {
        LCD_COMMAND;
        ERC240_ReInit();
        ERC240_ExitSleep();
        LCD_DATA;
    }
    
    Timer_StartTimer(&Timer_CLK100ms_ReInit);
}

void Timer_SW_ISR_PassKeyOff() {
    GlobalState.Update_Logic = true;
}