#include "Values.h"

#ifndef _Timer_H_
	#define _Timer_H_
    
    //Definition of function pointer for timer interrupt
    typedef void (*TMR_SW_ISR)() ; 
    
    //Definition of timer object
    typedef struct Timer {
        long Value;
        long Limit;
        bool Toggler;
        bool Flag;
        bool Active;
        bool Continues;
        TMR_SW_ISR ISR;
    } Timer; 
    
    void Timer_StopTimer(volatile struct Timer *Timer);
    void Timer_StartTimer(volatile struct Timer *Timer);
    void Timer_TMR2_100ms_Interrupt();
    void Timer_TMR1_10ms_Interrupt();

    //Software timers
    extern volatile Timer Timer_CLK100ms_MenuTimeout;
    extern volatile Timer Timer_CLK100ms_SplashScreen;
    extern volatile Timer Timer_CLK100ms_Request;
    extern volatile Timer Timer_CLK100ms_Blinking;
    extern volatile Timer Timer_CLK100ms_Communication;
    extern volatile Timer Timer_CLK100ms_SettingLock;
    extern volatile Timer Timer_CLK100ms_InitLoop;
    extern volatile Timer Timer_CLK100ms_230VAC_ON_Blinking;
    extern volatile Timer Timer_CLK100ms_12VDC_ON_Blinking;
    extern volatile Timer Timer_CLK10ms_SystemConfigTimeout;
    extern volatile Timer Timer_CLK10ms_Communication;
    extern volatile Timer Timer_CLK100ms_LED_Blinking;
    //extern volatile Timer Timer_CLK100ms_1SecGP;
    extern volatile Timer Timer_CLK100ms_ReInit;
    extern volatile Timer Timer_CLK100ms_PowerDown;
    extern volatile Timer Timer_CLK100ms_PassKeyOff;
    extern volatile Timer Timer_CLK100ms_DebugTimeout;

    extern volatile bool Blink12VDC;
    extern volatile bool Blink230VAC;
    
#endif