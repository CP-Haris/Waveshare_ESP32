#include "Button.h"
#include "UART_App.h"
#include "Timer.h"
#include "Buzzer.h"
#include "Values.h"

//Create Keypad and init setting
Keypad KeypadLPS = {.BACK.Long_TMRLimit = 200,  .BACK.Short_TMRLimit = 5,   .BACK.Press_Inform = {0x30,'L','P'},
                    .OK.Long_TMRLimit = 200,    .OK.Short_TMRLimit = 5,     .OK.Press_Inform = {0x30,'O','P'},
                    .UP.Long_TMRLimit = 200,    .UP.Short_TMRLimit = 5,     .UP.Press_Inform = {0x30,'U','P'},
                    .DOWN.Long_TMRLimit = 200,  .DOWN.Short_TMRLimit = 5,   .DOWN.Press_Inform = {0x30,'D','P'}};

void Button_Clear()
{
    KeypadLPS.BACK.Press.Any = 0;
    KeypadLPS.DOWN.Press.Any = 0;
    KeypadLPS.UP.Press.Any = 0;
    KeypadLPS.OK.Press.Any = 0;
}

void Button_TimerController(bool ButtonSwitch, struct Button *BTN)
{
    //Buttons are active low
    if(ButtonSwitch)
    {
        BTN->Short_TMRValue++;
        BTN->Long_TMRValue++;

        if(BTN->Long_TMRValue == BTN->Long_TMRLimit)
        {
            BTN->Press.Long = true;
            BTN->Long_TMRLimit = 25; 
            BTN->Long_TMRValue = 0;
            BTN->Press.Short = true;
            BTN->Short_TMRValue = 0;
            
            //Inform
            if(Block_3_ID_3.NumValue & (1<<17))
                BUZ_SetMelody(BUZ_BIP);
            SendMSG (&BTN->Press_Inform[0], 3, UART_CTRL);
           
        } 
        else if(BTN->Short_TMRValue == BTN->Short_TMRLimit)
        {
            BTN->Press.Short = true; 
            BTN->Short_TMRLimit = 25; 
            BTN->Short_TMRValue = 0;
            
            //Inform
            if(Block_3_ID_3.NumValue & (1<<17))
                BUZ_SetMelody(BUZ_BIP);
            SendMSG (&BTN->Press_Inform[0], 3, UART_CTRL);
        } 
    }
    else
    {
        BTN->Long_TMRLimit = 200; 
        BTN->Short_TMRLimit = 5; 
        BTN->Short_TMRValue = 0;
        BTN->Long_TMRValue = 0;
    }
}