#include "State_Controller.h"
#include "Button.h"
#include "Menu_Error.h"
#include "Values.h"
#include "Timer.h"
#include "Menu_Application.h"
#include "Menu_Settings.h"
#include "Settings.h"
#include "ERC240128FS.h"
#include "MainView_Old.h"
#include "MainView_New.h"
#include "LED.h"
#include "Buzzer.h"
#include "Menu_LockScreen.h"
#include "InitView.h"
#include "peripheral/gpio/plib_gpio.h"
#include "Menu_BLE_Passkey.h"
#include "BLE_App.h"
#include "Text.h"

#include "UART_App.h"

int InitTryCounter = 0;
void State_UpdateLogic()
{
    do { 
        //Clear update flag
        GlobalState.Update_Logic = false;
        long CurrentState = GlobalState_GetActual();
        
        if(CurrentState > MAINSTATE_SPLASH)
        {
            //Override state if OFF state is received while ON
            if (!App_UpdateFlags.State_ON)
                //GlobalState.State[GlobalState.StateIndex] = MAINSTATE_SPLASH;
                GlobaState_Next(MAINSTATE_SPLASH);
            else if (App_UpdateFlags.State_ON)
            {
                //Check for popups
                #ifdef BLUETOOTH_CODE
                if((CurrentState != MAINSTATE_ERROR) && (CurrentState != MAINSTATE_PASSKEY))
                #else
                if(CurrentState != MAINSTATE_ERROR)
                #endif
                {
                    if(Error_CheckForPopup())
                        GlobaState_Next(MAINSTATE_ERROR);
                }
            }
            
            #ifdef BLUETOOTH_CODE
            if(CurrentState != MAINSTATE_PASSKEY)
            {
                if(PassKey_CheckForPopup()) {
                    GlobaState_Next(MAINSTATE_PASSKEY);
                    Timer_StartTimer(&Timer_CLK100ms_PassKeyOff);
                }
            }
            
            //If Bluetooth module not present or functional, remove Bluetooth menu and signal controlboard to turn off UART during sleep
            if(CHECK_BIT(Block_3_ID_3.NumValue, 20) && !BLE_ACTIVE_Get()) {
                if(Menu_BLE.Show) { // These are also default values
                    Menu_BLE.Show = false;
                    Block_1_ID_2.NumValue = 0;
                    Values_Send_ByValue(SET_VAL, &Block_1_ID_2, UART_CTRL);
                }
            } else {
                if(!Menu_BLE.Show) {
                    Menu_BLE.Show = true;
                    Block_1_ID_2.NumValue = 1;
                    Values_Send_ByValue(SET_VAL, &Block_1_ID_2, UART_CTRL);
                }
            }
            #endif
        }
        
        //Perform Logic Action
        switch(CurrentState)
        {
            case MAINSTATE_DEBUG:
                DebugView_CheckButtonPress();
                GlobalState.Update_Graphic = true;
                Timer_StopTimer(&Timer_CLK100ms_Communication);
                
                //Debug mode is left after 15 minutes
                if(Timer_CLK100ms_DebugTimeout.Flag) {
                    GlobaState_Next(MAINSTATE_POWERDOWN);
                    
                    LED_R_230V_Clear();
                    LED_G_230V_Clear();
                    LED_R_12V_Clear();
                    LED_G_12V_Clear();

                    Timer_StartTimer(&Timer_CLK100ms_PowerDown);
                    //Timer_StartTimer(&Timer_CLK100ms_Request);
                    Timer_StartTimer(&Timer_CLK100ms_Communication);

                    // Remove graphics from screen
                    ERC240_Clear_MemoryBuffer();
                    ERC240_Show_MemoryBuffer();

                    // Enter sleep mode to discharge bias capacitors and reduce current consumption
                    LCD_COMMAND;
                    ERC240_EnterSleep();
                    LCD_RST_Clear();
                }
            break;
            
            case MAINSTATE_INIT:
                if (Timer_CLK100ms_InitLoop.Flag)
                {
                    Timer_StopTimer(&Timer_CLK100ms_InitLoop);
                    
                    if(Values_RequestInitial()){
                        // Update BLE power depending on last saved setting
                        #ifdef BLUETOOTH_CODE
                        BLE_UpdatePowerSetting();
                        #endif
                        
                        //Initialize Application
                        if (App_UpdateFlags.State_ON) {
                            GlobaState_Next(MAINSTATE_SPLASH);
                        } else { // This added in case microcontroller loses communication connection while off
                            GlobaState_Next(MAINSTATE_POWERDOWN);
                            Timer_StartTimer(&Timer_CLK100ms_PowerDown);
                            
                            // Remove graphics from screen
                            ERC240_Clear_MemoryBuffer();
                            ERC240_Show_MemoryBuffer();
                            
                            // Enter sleep mode to discharge bias capacitors and reduce current consumption
                            LCD_COMMAND;
                            ERC240_EnterSleep();
                            LCD_RST_Clear();
                        }
                    }
                    else if(InitTryCounter > 3){
                        //Initialize Debug
                        Button_Clear();
                        Timer_StopTimer(&Timer_CLK100ms_Communication);
                        
                        UART1_Disable();
                        UART_BufferInit(&UART1_Buffer);
                        
                        if(!UART_ReadCountGet(&UART1_Buffer)) {
                            InitTryCounter = 0;
                            
                            ERC240_Init(true);
                            GlobaState_Next(MAINSTATE_DEBUG);
                            
                            UART1_Init(115200, true);
                            UART1_Enable();
                            
                            BLE_ON_Set();
                            
                            Timer_StartTimer(&Timer_CLK100ms_DebugTimeout);
                            Timer_StartTimer(&Timer_CLK100ms_Request);
                        } else {
                            GlobalState.Update_Logic = true;
                        }
                        
                    }
                    else {    
                        //Try to initialize again  
                        Timer_StartTimer(&Timer_CLK100ms_InitLoop);
                        InitTryCounter++;
                    }
                }
                else if(!Timer_CLK100ms_InitLoop.Active){
                    Timer_StartTimer(&Timer_CLK100ms_InitLoop);
                    LED_R_230V_Clear();
                    LED_G_230V_Clear();
                    LED_R_12V_Clear();
                    LED_G_12V_Clear();
                    Values_RequestInitial();
                }
            break;
            
            case MAINSTATE_POWERDOWN:
                if (App_UpdateFlags.State_ON) {
                    Error_ClearAllMinimized();
                    GlobaState_Initial(MAINSTATE_OFF);
                    GlobaState_Next(MAINSTATE_SPLASH);
                    Timer_StopTimer(&Timer_CLK100ms_PowerDown);
                    
                    LCD_COMMAND;
                    ERC240_ExitSleep(); // Exit sleep mode to restart capacitors and allow graphics
                    LCD_RST_Set();
                    
                    Block_1_ID_3.NumValue = 0; // Definitely not going into sleep mode now
                }
                
                // If power-down time elapsed, go into off-mode (only if not entering sleep mode)
                if(!App_UpdateFlags.DeepSleep) {
                    Timer_StopTimer(&Timer_CLK100ms_Communication); // If transitioning to standby mode, don't allow it to enter sleep mode.
                    if(Timer_CLK100ms_PowerDown.Flag) {
                        if((Block_1_ID_2.NumValue) && (CHECK_BIT(Block_3_ID_3.NumValue, 20))) { // If BLE is active and turned on, just disable as many peripherals as possible
                            GlobaState_Initial(MAINSTATE_OFF);

                            // Turn off peripherals
                            TMR4_Stop();
                            TMR3_Stop();
                            TMR2_Stop(); // 100 ms timer also used for communication WDT; hence this is disabled in standby mode
                            OCMP1_Disable();
                            OCMP2_Disable();

                            LCD_CS_Set();
                            LCD_RD_Set();
                            LCD_WR_Set();

                        } else { // If BLE is inactive, go into low-power sleep mode; nothing to do but wait for controlboard
                            System_EnterSleep();
                        }
                    }
                }
            break;
            
            // Off tries to reach minimum current consumption without entering sleep, as this breaks BLE
            // Total: 3.5 mA
            // ----------------
            // BLE chip: 0.3 mA
            // Display: 0.4 mA (it should be possible to go lower, but for some reason it doesn't)
            // Microcontroller: 2.8 mA (typ. 2 mA in idle @ 10 MHz, plus spending time for UART communication)
            case MAINSTATE_OFF:
                // If turned back on, ensure everything is restarted
                if (App_UpdateFlags.State_ON) {
                    TMR2_Start();
                    TMR4_Start();
                    TMR3_Start();
                    
                    LCD_CS_Clear();
                    //LCD_RD_Clear();
                    //LCD_WR_Clear();
                    
                    LCD_COMMAND;
                    ERC240_ExitSleep();
                    LCD_RST_Set();
                    
                    GlobaState_Next(MAINSTATE_SPLASH);
                }
            break;
            
            case MAINSTATE_SPLASH:
                //Change splashscreen state if splashscreen timer is not active
                if(!Timer_CLK100ms_SplashScreen.Active){
                    //Turn Off splashscreen if splashscreen timer is overflow and change state
                    if(Timer_CLK100ms_SplashScreen.Flag)
                    {
                        Timer_StopTimer(&Timer_CLK100ms_SplashScreen);
                        Button_Clear();

                        //Initialize state and menu
                        GlobaState_Initial(MAINSTATE_MAINVIEW);
                        Menu_InitMenu();
                        
                        if(Block_3_ID_3.NumValue & (1<<16))
                            BUZ_SetMelody(BUZ_HELLO);
                    }
                    //Activate splashscreen and request graphichal change
                    else
                    {
                        long CallerState = GlobalState_GetCaller();
                        if ((CallerState == MAINSTATE_OFF) || (CallerState == MAINSTATE_INIT) || (CallerState == MAINSTATE_POWERDOWN))
                        {
                            ERC240_Init(true);
                            Timer_StartTimer(&Timer_CLK100ms_SplashScreen);
                            Timer_StartTimer(&Timer_CLK100ms_Request);
                            Timer_StartTimer(&Timer_CLK100ms_ReInit);
                            GlobalState.Update_Graphic = true;
                        }
                        else if (CallerState > MAINSTATE_SPLASH)
                        {
                            GlobaState_Next(MAINSTATE_POWERDOWN);
                            if(Block_3_ID_3.NumValue & (1<<16))
                                BUZ_SetMelody(BUZ_GOODBYE);
                            
                            LED_R_230V_Clear();
                            LED_G_230V_Clear();
                            LED_R_12V_Clear();
                            LED_G_12V_Clear();
                            
                            Timer_StartTimer(&Timer_CLK100ms_PowerDown);
                            
                            // Remove graphics from screen
                            ERC240_Clear_MemoryBuffer();
                            ERC240_Show_MemoryBuffer();
                            
                            // Enter sleep mode to discharge bias capacitors and reduce current consumption
                            LCD_COMMAND;
                            ERC240_EnterSleep();
                            LCD_RST_Clear();
                            
                            // Send state of Bluetooth (present or not) to allow controlboard to shut down UART
                            Values_Send_ByValue(SET_VAL, &Block_1_ID_2, UART_CTRL);
                        }
                    }
                }
            break;
            case MAINSTATE_MAINVIEW:    
                if(KeypadLPS.OK.Press.Short)
                {
                    Menu_MoveMarker(MARKER_RESET);
                    GlobaState_Next(MAINSTATE_MENU);
                    Timer_StartTimer(&Timer_CLK100ms_MenuTimeout);
                }
                else if(KeypadLPS.DOWN.Press.Short || KeypadLPS.UP.Press.Short)
                {
                    //Change Main View Skin
                    if(Block_3_ID_7.NumValue == VIEW_NEW)
                        Block_3_ID_7.NumValue = VIEW_OLD;
                    else
                        Block_3_ID_7.NumValue = VIEW_NEW;
                    
                    //Send change to LPS eeprom and update GUI
                    GlobalState.Update_Graphic = true;
                    Values_Send_ByValue(0x50, &Block_3_ID_7, UART_CTRL);
                }
            break;

            case MAINSTATE_MENU: {              
                //Check if lock is activated -> Not active if code 0000
                long StoredCode = Block_3_ID_4.NumValue;
                if(StoredCode == 0x00)
                    SettingsUnlocked = true;
                else if (!Timer_CLK100ms_SettingLock.Active) 
                    SettingsUnlocked = false;
                 
                //Leave Menu if no buttons are pressed for time defined by "Timer_MenuTimeout"
                if(KEYPAD_ACTIVITY)
                    Timer_StartTimer(&Timer_CLK100ms_MenuTimeout);
                else if(Timer_CLK100ms_MenuTimeout.Flag){
                    Timer_StopTimer(&Timer_CLK100ms_MenuTimeout);
                    Menu_InitMenu();
                    GlobaState_Previous(&GlobalState);
                }
                
                //Adjust Marker if UP/DOWN is Pressed
                if(KeypadLPS.DOWN.Press.Short){               
                    Menu_MoveMarker(MARKER_DOWN);
                    GlobalState.Update_Graphic = true;
                } 
                else if(KeypadLPS.UP.Press.Short){
                    Menu_MoveMarker(MARKER_UP);
                    GlobalState.Update_Graphic = true;
                }
               
                //If OK is pressed go to the marked item
                else if(KeypadLPS.OK.Press.Short){
                    
                    Menu* MarkedMenu = Menu_GetSelectedChild(Menu_GetCurrentMenu());
                    if(MarkedMenu != NULL){
                        if(Menu_ShowMenu(MarkedMenu))
                        {
                            //If the selected value is a setting
                            if(MarkedMenu->ST_Popup != NULL)
                            {
                                //Go to setting state
                                GlobaState_Next(MAINSTATE_SETTING_LOCK);
                            }

                            //If the selected item has Children go to selected child
                            else if(MarkedMenu->ST_Children != NULL) {
                                Menu_SetNextMenu(MarkedMenu);
                                Menu_MoveMarker(MARKER_RESET);
                                GlobalState.Update_Graphic = true;
                                GlobalState.Update_Request = true;
                            }

                            //If the selected value is a Error pointer
                            else if(MarkedMenu->ST_Value != NULL)
                            {
                                if(MarkedMenu->ST_Value->Data->Prefix == ERROR){
                                    ErrorFlags[MarkedMenu->ST_Value->Data->NumValue].Minimized = false;
                                    GlobalState.Update_Graphic = true;
                                } 
                            }
                        }
                    }
                }
                
                //If BACK is pressed go to the marked item
                else if(KeypadLPS.BACK.Press.Short){
                    //If the selected item has Parrent
                    if(Menu_SetPreviousMenu()){
                        GlobalState.Update_Logic = true;
                        GlobalState.Update_Graphic = true;
                        GlobalState.Update_Request = true;
                    }
                    else{
                        GlobaState_Previous();
                    }
                }
            }
            break;
            
            case MAINSTATE_ERROR:
                if(KEYPAD_ACTIVITY){
                    if(KeypadLPS.BACK.Press.Short){
                        Error_ClearPopup(false);
                        GlobaState_Previous();
                    }
                    else if(KeypadLPS.OK.Press.Short){
                        Error_ClearPopup(true);
                        GlobaState_Previous();
                    }
                }
                else if (Error_CheckCleared())
                    GlobaState_Previous();
            
            break;
            
            #ifdef BLUETOOTH_CODE
            case MAINSTATE_PASSKEY:
                if(KEYPAD_ACTIVITY){
                    if(KeypadLPS.BACK.Press.Short || KeypadLPS.OK.Press.Short){
                        PassKey_ClearPopup();
                        GlobaState_Previous();
                        Timer_StopTimer(&Timer_CLK100ms_PassKeyOff);
                    }
                }
                else if ((Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED) || (!Timer_CLK100ms_PassKeyOff.Active))
                {
                    PassKey_ClearPopup();
                    GlobaState_Previous();
                    Timer_StopTimer(&Timer_CLK100ms_PassKeyOff);
                }
            break;
            #endif

            case MAINSTATE_SETTING_LOCK:
            {
                //Go to Menu if comming from a setting
                Menu* MarkedMenu = Menu_GetSelectedChild(Menu_GetCurrentMenu());
                
                if(GlobalState_GetCaller() == MAINSTATE_SETTING)
                    GlobaState_Previous();
                                
                //Go to lockscreen if setting is code protected and not unlocked.
                else if ((MarkedMenu->ST_Popup->CodeProtect  && !SettingsUnlocked) || (MarkedMenu->ST_Popup->Type == POPUP_LOCK_SET))
                {
                    if(KEYPAD_ACTIVITY)
                    {
                        GlobalState.Update_Graphic = true;
                       
                        //If lockscreen is not finished (waiting for 4 characters)
                        if (Menu_SettingLock_Logic(SettingsUnlocked))
                        {
                            if(MarkedMenu->ST_Popup->Type != POPUP_LOCK_SET)
                            {
                                if(SettingsUnlocked) //If unlocked
                                    GlobaState_Next(MAINSTATE_SETTING);
                                else
                                    GlobaState_Previous();
                            }
                            else if(!SettingsUnlocked || KeypadLPS.BACK.Press.Short)
                                GlobaState_Previous();
                        }
                    }
                }
                //Go to setting
                else 
                    GlobaState_Next(MAINSTATE_SETTING);
            }
            break;  
            
            case MAINSTATE_SETTING:
                //Handles Setting logic and returns bool if to go to previous state
                if(Menu_Setting_Logic(Menu_GetSelectedChild(Menu_GetCurrentMenu())))
                    GlobaState_Previous();
            break;  
        }
        Button_Clear();
    } while (GlobalState.Update_Logic);
}

void State_UpdateGraphics()
{
    long State = GlobalState_GetActual();
    
    // DO NOT ADD MAINSTATE_POWERDOWN, THE LEDS WILL KEEP BLINKING
    if((State != MAINSTATE_INIT) && (State != MAINSTATE_OFF) && (State != MAINSTATE_DEBUG)) {
        State_UpdateLED();
    }
    
    switch(State)
    {
        case MAINSTATE_DEBUG:
            DebugView_GraphicPresentation();
        break;
        
        case MAINSTATE_INIT:
        case MAINSTATE_POWERDOWN:
        case MAINSTATE_OFF:
            LED_R_230V_Clear();
            LED_G_230V_Clear();
            LED_R_12V_Clear();
            LED_G_12V_Clear();
            //ERC240_Clear_MemoryBuffer();
        break;           
            
        case MAINSTATE_SPLASH:
            if(Timer_CLK100ms_SplashScreen.Active)
            {
                ERC240_Clear_MemoryBuffer();
                ERC240_Display_SplashScreen((unsigned char)(Block_3_ID_6.NumValue>>16));
            }
        break;
        
        case MAINSTATE_ERROR:
            Error_ShowPopup();
        break;
        
        #ifdef BLUETOOTH_CODE
        case MAINSTATE_PASSKEY:
            PassKey_ShowPopup();
        break;
        #endif
        
        case MAINSTATE_MAINVIEW:
            if(Block_3_ID_7.NumValue == VIEW_NEW)
                V2_MainView_UpdateContent();
            else
                MainView_UpdateContent();
        break;

        case MAINSTATE_MENU:
            Menu_UpdateGraphics();
        break;

        case MAINSTATE_SETTING:
            Menu_Setting_GraphicPresentation(Menu_GetSelectedChild(Menu_GetCurrentMenu()));
        break;
        
        case MAINSTATE_SETTING_LOCK:
            Menu_SettingLock_GraphicPresentation();
        break;  
        
    }
    
    // Only update display when something is actually written to it
    if((State != MAINSTATE_INIT) && (State != MAINSTATE_OFF) && (State != MAINSTATE_POWERDOWN)) {
        ERC240_Show_MemoryBuffer();
    }

    GlobalState.Update_Graphic = false;
}

void State_PeriodicalRequest()
{
    GlobalState.Update_Request = false;
    
    //Check Always
    //Fetch operating and wakeup states in most modes. Not when initializing or fully off.
    //\TODO: Why not in POWERDOWN or DEBUG? To ensure these modes can be left?
    //if((GlobalState.State[GlobalState.StateIndex] != MAINSTATE_SETTING) && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_INIT) && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_OFF))
    if((GlobalState.State[GlobalState.StateIndex] != MAINSTATE_INIT) && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_OFF)
            && (GlobalState.State[GlobalState.StateIndex] != MAINSTATE_DEBUG))
    {
        Values_RequestByValue(&Block_0_ID_157, true, GET_VAL);      //157 - WAKEUPFLAGS
        Values_RequestByValue(&Block_0_ID_203, true, GET_VAL);      //203 - DCOUT_OPSTATE
        Values_RequestByValue(&Block_0_ID_207, true, GET_VAL);      //207 - DCOUT_FLSTATE
        Values_RequestByValue(&Block_0_ID_201, true, GET_VAL);      //201 - ACOUT_OPSTATE
        Values_RequestByValue(&Block_0_ID_205, true, GET_VAL);      //205 - ACOUT_FLSTATE
        
        Values_RequestByValue(&Block_0_ID_212, true, GET_VAL);      //212 - SYS_TESTMODE
        Values_RequestByValue(&Block_0_ID_202, true, GET_VAL);      //202 - DCIN_OPSTATE
        Values_RequestByValue(&Block_0_ID_206, true, GET_VAL);      //206 - DCIN_FLSTATE
        Values_RequestByValue(&Block_0_ID_200, true, GET_VAL);      //200 - ACIN_OPSTATE
        Values_RequestByValue(&Block_0_ID_204, true, GET_VAL);      //204 - ACIN_FLSTATE
    }
    
    switch(GlobalState.State[GlobalState.StateIndex])
    {
        case MAINSTATE_MENU:
        { 
            Menu ** MenuChildren = Menu_GetCurrentMenu()->ST_Children;
            //Check if menu has children
            if(MenuChildren != NULL)
            {
                //Loop through all children
                unsigned char ChildNumber = 0;
                while(MenuChildren[ChildNumber] != NULL)
                {
                    //Check if child has a value attached and request new value over uart
                    if(MenuChildren[ChildNumber]->ST_Value != NULL) {
                        if(MenuChildren[ChildNumber]->Show != false)
                        {
                            Values_RequestByValue(MenuChildren[ChildNumber]->ST_Value->Data, true, GET_VAL);
                        }
                    }
                    ChildNumber++;
                }
                
                //TODO: (Quick fix) Make a better fix on how to change name of LPS SOC when extension is enables
                if(Block_7_ID_0.NumValue == 1<<16)
                    Menu_STATUS_SOC.S8A_Title = ENG_SOC_LPS;
                else
                    Menu_STATUS_SOC.S8A_Title = ENG_SOC;
            }
        }
        break;
        
        case MAINSTATE_SETTING:
        {
            if(Menu_GetSelectedChild(Menu_GetCurrentMenu())->ST_Value != NULL) {
                Values_RequestByValue(Menu_GetSelectedChild(Menu_GetCurrentMenu())->ST_Value->Data, false, GET_MIN_MAX);
            }
        }
        break;

        case MAINSTATE_MAINVIEW:
        {   
            Values_RequestByValue(&Block_0_ID_107, true, GET_VAL);  //ACOUT_WATT
            Values_RequestByValue(&Block_0_ID_104, true, GET_VAL);  //ACIN_WATT
            Values_RequestByValue(&Block_0_ID_112, true, GET_VAL);  //DCOUT_AMP
            Values_RequestByValue(&Block_0_ID_109, true, GET_VAL);  //DCIN_AMP
            Values_RequestByValue(&Block_0_ID_113, true, GET_VAL);  //DCOUT_WATT
            Values_RequestByValue(&Block_0_ID_110, true, GET_VAL);  //DCIN_WATT
            Values_RequestByValue(&Block_0_ID_210, true, GET_VAL);  //BATSTATUS
            Values_RequestByValue(&Block_0_ID_119, true, GET_VAL);  //BATSTATUS_SOC
            Values_RequestByValue(&Block_0_ID_127, true, GET_VAL);  //BATSTATUS_WATT
            Values_RequestByValue(&Block_0_ID_120, true, GET_VAL);  //BATSTATUS_REMTIME
            Values_RequestByValue(&Block_0_ID_208, true, GET_VAL);  //SOLAR_OPSTATE
            Values_RequestByValue(&Block_0_ID_79, true, GET_VAL);   //Solar_WATT
            Values_RequestByValue(&Block_0_ID_225, true, GET_VAL);  //DCOUT_AUTO_TIMEGLOBAL
            Values_RequestByValue(&Block_0_ID_226, true, GET_VAL);  //ACOUT_AUTO_TIMEGLOBAL
            Values_RequestByValue(&Block_1_ID_1, true, GET_VAL);    //FUNC_JUMPSTART
            Values_RequestByValue(&Block_7_ID_0, true, GET_VAL);
            if(Block_7_ID_0.NumValue == 1<<16)
            {
                Values_RequestByValue(&Block_241_ID_119, true, GET_VAL);
                Values_RequestByValue(&Block_242_ID_119, true, GET_VAL);
                Values_RequestByValue(&Block_241_ID_127, true, GET_VAL);
            }
        }
        break;
        
        case MAINSTATE_PASSKEY:
        {
            //Enable backlight
            SendMSG(KeypadLPS.UP.Press_Inform, 3, UART_CTRL);
        }
        break;
        
        default:
            break;
    }
}

//TODO:
//Remove State_UpdateLED function and use timerISR 
//Have the timer running at all times and do a evaluation on each state (Blink On stat and blink off state)
void State_UpdateLED()
{
    ///////////////////////// AC OUT //////////////////////////////////////////
    //AC OUT - Blocked due to failure
    // Block_0_ID_205 = Failurestate ACOUT
    // Block_0_ID_201 = Operation state AC Out
    // Block_0_ID_200 = Operation state AC In
    if((Block_0_ID_205.NumValue >= FL_SIMPLE_FAILURE) && (Block_0_ID_201.NumValue >= OS_WAKEUP))
    {
        if (Block_0_ID_200.NumValue == OS_ON) //Operation state AC In
            Blink230VAC = true;
        else
        {
            Blink230VAC = false;
            LED_R_230V_Set();
            LED_G_230V_Clear();
        }
    } 
    //AC OUT - Active
    else if (Block_0_ID_201.NumValue >= OS_READY_TO_START) // Operation state AC Out
    {
        //Check if wakeup button is not present
        if (!((Block_0_ID_157.NumValue >> 16) & (1 << 6))) 
            Blink230VAC = true;
        else 
        {
            //Debug OPSTATE ACOUT
            Blink230VAC = false;
            LED_R_230V_Clear();
            LED_G_230V_Set();
        }
    }
    //AC OUT - Off
    else
    {
        if (Block_0_ID_200.NumValue == OS_ON) //Operation state AC In
            Blink230VAC = true;
        else
        {
            Blink230VAC = false;
            LED_R_230V_Clear();
            LED_G_230V_Clear();
        }
    }
    
    ///////////////////////// DC OUT //////////////////////////////////////////
    //DC OUT - Blocked due to failure
    // Block_0_ID_207 = Failurestate DCOUT
    // Block_0_ID_203 = Operation state DC Out
    if((Block_0_ID_207.NumValue >= FL_SIMPLE_FAILURE) && (Block_0_ID_203.NumValue >= OS_WAKEUP))
    {
        if (!((Block_0_ID_157.NumValue >> 16) & (1 << 7))) 
            Blink12VDC = true;
        else //
        {
            Blink12VDC = false;
            LED_R_12V_Set();
            LED_G_12V_Clear();
        }
    }

    //DC OUT - Active
    else if (Block_0_ID_203.NumValue >= OS_READY_TO_START)   //Operation state DC Out  
    {
        //Check if wakeup butteon is not present
        if (!((Block_0_ID_157.NumValue >> 16) & (1 << 7))) 
            Blink12VDC = true;
        else
        {
            Blink12VDC = false;
            LED_R_12V_Clear();
            LED_G_12V_Set();
        }
    }
    //DC OUT - Off
    else
    {
        Blink12VDC = false;
        LED_R_12V_Clear();
        LED_G_12V_Clear();
    }
}

// Enter deep sleep mode to reduce power consumption
void System_EnterSleep(void) {
    __builtin_disable_interrupts(); // Just in case
    WDT_Clear();
    //\TODO: Find out if this actually works with the used #pragma. Trying to change the pragma just caused issues.
    WDT_Disable();
    
    // Clear screen to ensure nothing is shown
    ERC240_Clear_MemoryBuffer();
    ERC240_Show_MemoryBuffer();

    // Send display to sleep mode to discharge bias capacitors and reduce current consumption
    LCD_COMMAND;
    ERC240_EnterSleep();
    LCD_RST_Clear();
    
    // Turn off all peripherals
    TMR4_Stop();
    TMR3_Stop();
    TMR2_Stop();
    TMR1_Stop();
    OCMP1_Disable();
    OCMP2_Disable();
    UART1_Disable();
    #ifdef BLUETOOTH_CODE
    UART2_Disable();
    #endif

    // Disable all peripheral power
    // Unlock system for configuration
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.PMDLOCK = 0;
    
    PMD1 = 0x1101;
    PMD2 = 0x7;
    PMD3 = 0x1f001f;
    PMD4 = 0x1f;
    PMD5 = 0x1030303;
    PMD6 = 0x10003;
    
    // Lock back the system after PPS configuration
    // DO NOT LOCK (Reason #1: OSCCON needs unlocked system, reason #2: I fear that PMDLOCK affects bootloader)
    /*SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    CFGCONbits.PMDLOCK = 1;
     
    SYSKEY = 0x00000000;*/
    
    // Suspend all I/O pins and enable pull-up or pull-down as required
    GPIO_Sleep_Deinitialize();
    
    #ifdef PROCESSOR_PIC32MX170F256D_HW0130
        #error HW Rev 1 untested; change IOs, test, and approve (current consumption, CN wake from UART etc.)
    #endif
    
    // Enable change notice interrupt for controlboard UART RX
    CNCONCbits.ON = true; // Enable change notice C module
    CNCONCbits.SIDL = false; // Allow change notice during idle
    CNENA = 0x0;
    CNENB = 0x0;
    CNENC = 0x0;
    CNENCbits.CNIEC1 = true; // Change Notice Enable -- for C1 (UART RX)
    PORTC; // Read PORTC to clear mismatch
    
    //IPC8SET = 0x40000 | 0x0;  /* CHANGE_NOTICE:  Priority 1 / Subpriority 0 */
    //IFS1bits.CNCIF = false; // Clear interrupt flag just in case
    EVIC_SourceStatusClear(INT_SOURCE_CHANGE_NOTICE_C);
    //IEC1bits.CNCIE = true; // Enable change notice C interrupt vector
    EVIC_SourceEnable(INT_SOURCE_CHANGE_NOTICE_C);
    
    __builtin_enable_interrupts(); // Just in case
    
    // Send microcontroller to sleep mode
    OSCCONSET = 0x10;
    asm volatile ( "wait" );
    asm volatile ( "nop" );
    
    // When change registered on UART RX, the interrupt CHANGE_NOTICE_Handler will trigger
}
