#include "MainView_Old.h"
#include "Menu_Application.h"
#include "Values.h"
#include "Timer.h"
#include "Power.h"
#include "Menu_Error.h"
#include "Text.h"
#include "BLE_App.h"

#define DOT     1
#define RIGHT   2
#define LEFT    3
#define UP      4
#define DOWN    5

#define NOBLINK false
#define BLINK   true
//#define DEMO

void MainView_ShowFunctionPowerBar(unsigned char POSX, unsigned char POSY, unsigned char Width, unsigned char Hight, long Value, long MaxValue, long MinValue, bool EnableBlinking)
{
    long absMaxValue;
    if (Value < 0)
        absMaxValue = abs(MinValue);
    else
        absMaxValue = abs(MaxValue);
    
    //Get absolute value
    long absValue = abs(Value);

    //Check if overload
    bool Overload = absValue > absMaxValue;
    
    //Activate timer if not active if there is a overload
    if(Overload && !Timer_CLK100ms_Blinking.Active)
        Timer_StartTimer(&Timer_CLK100ms_Blinking);
    
    
    if(!(Overload & !Timer_CLK100ms_Blinking.Toggler) || !EnableBlinking)
    {           
        //Calculate powerbar length
        unsigned char PowerBarLength;
        if(absValue >= absMaxValue)
            PowerBarLength = Width;
        else
            PowerBarLength = ((absValue>>16)*Width/(absMaxValue>>16));
        
        //Invert powerbar length
        ERC240_Display_InvertArea_Square(POSX, POSY, POSX+PowerBarLength, POSY+Hight);
    }   
}

void MainView_ShowFuncitonIcon(unsigned char POSX, unsigned char POSY, unsigned char Icon_OK, unsigned char Icon_ERROR, const unsigned char *Title, const Value *OPSTATE, const Value *FLSTATE, bool AcceptEmpty)
{
    //Create Frame and Header text
    ERC240_Display_String(POSX+32, POSY+3, (unsigned char*)Title, 0, 'C', (unsigned char*)S6_Small_Fonts8x10);
    ERC240_Display_Character(POSX, POSY, 0, (unsigned char*)ComplexFrame63x57);
    
    #if defined (DEMO)
        ERC240_Display_Character(POSX+15, POSY+17, Icon_OK, (unsigned char*)MainIcons35x26); //OK
        //ERC240_Display_Character(POSX+15, POSY+17, Icon_ERROR, (unsigned char*)MainIcons35x26); //ERROR
    #else

    //Create Icon inside the Frame
    if ((FLSTATE->NumValue < FL_SIMPLE_FAILURE) || (AcceptEmpty && (FLSTATE->NumValue == FL_EMPTY)))
        ERC240_Display_Character(POSX+15, POSY+17, Icon_OK, (unsigned char*)MainIcons35x26);
    else
        ERC240_Display_Character(POSX+15, POSY+17, Icon_ERROR, (unsigned char*)MainIcons35x26);
    #endif
}

void MainView_ShowTimer(unsigned char POSX, unsigned char POSY, const Value *TIMER)
{
    #if defined (DEMO)
        //TIMER->Data->NumValue = 5<<8;
        ERC240_Display_String(83,POSY,(unsigned char*)TimeValToString(TIMER),0,'C',(unsigned char*)S6_Small_Fonts8x10);
        //ERC240_Display_String(156,POSY,(unsigned char*)TimeValToString(TIMER),0,'C',(unsigned char*)S6_Small_Fonts8x10);
    #else
    if (TIMER->NumValue > 0)
        ERC240_Display_String(POSX,POSY,(unsigned char*)TimeValToString(TIMER),0,'C',(unsigned char*)S6_Small_Fonts8x10);
    #endif
}

void MainView_ShowACIN()
{
    MainView_ShowFuncitonIcon (175, 0, 6, 7, ENG_ACIn_Charge, &Block_0_ID_200, &Block_0_ID_204, true);
   
    #if defined (DEMO)
        if(Block_200_ID_13.NumValue != 1<<16)
            ERC240_Display_Character(137, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(147, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(157, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(167, 34, DOT, (unsigned char*)ComplexSmallIcons9x9); 
    #else
   
    if(Block_0_ID_200.NumValue >= OS_READY_TO_START)
    {
        if(abs(Block_0_ID_104.NumValue) > Block_1_ID_102.NumValue)
        {
            //Show Arrows
            if(Block_0_ID_104.NumValue > 0)
            {
                //Arrows Pointing Right
                if(Block_7_ID_0.NumValue != 1<<16)
                    ERC240_Display_Character(137, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(147, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9); 
            }
            else
            {
                //Arrows Pointing Left
                if(Block_7_ID_0.NumValue != 1<<16)
                    ERC240_Display_Character(137, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(147, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9); 
            };
            
            //Show Powerbar
            MainView_ShowFunctionPowerBar   (177, 49, 60, 5, Block_0_ID_104.NumValue, Block_1_ID_100.NumValue,  Block_1_ID_101.NumValue, NOBLINK);
        }
        else
        {   
            //Dots
            if(Block_7_ID_0.NumValue != 1<<16)
                ERC240_Display_Character(137, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(147, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(167, 34, DOT, (unsigned char*)ComplexSmallIcons9x9); 
        }
    }
    #endif
}

void MainView_ShowACOUT()
{
    MainView_ShowFuncitonIcon       (175, 71, 0, 1, ENG_ACOut, &Block_0_ID_201, &Block_0_ID_205, false);
    MainView_ShowTimer              (156, 96, &Block_0_ID_226);
#if defined (DEMO)
    //Dots
    ERC240_Display_Character(137, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(147, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(157, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(167, 105, DOT, (unsigned char*)ComplexSmallIcons9x9); 
#else
    if(Block_0_ID_200.NumValue >= OS_READY_TO_START)
    {
        if(abs(Block_0_ID_107.NumValue) > Block_1_ID_112.NumValue)
        {
            if(Block_0_ID_107.NumValue > 0)
            {
                //Arrows Pointing left
                ERC240_Display_Character(167, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9); 
                
                //Arrows Pointing Down
                ERC240_Display_Character(157, 49, DOWN, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 63, DOWN, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 77, DOWN, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 91, DOWN, (unsigned char*)ComplexSmallIcons9x9);
                
                //Arrows Pointing Right
                ERC240_Display_Character(157, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);   
            }
            else
            {
                //Arrows Pointing Right
                ERC240_Display_Character(167, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9); 
                
                //Arrows Pointing up
                ERC240_Display_Character(157, 49, UP, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 63, UP, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 77, UP, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 91, UP, (unsigned char*)ComplexSmallIcons9x9);
                
                //Arrows Pointing Left
                ERC240_Display_Character(157, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
            }
            
            MainView_ShowFunctionPowerBar(177, 120, 60, 5, Block_0_ID_107.NumValue, Block_1_ID_110.NumValue,  Block_1_ID_111.NumValue, BLINK);
        }
        else
        {
            //Dots
            ERC240_Display_Character(167, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            //Dots
            ERC240_Display_Character(157, 49, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 63, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 77, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 91, DOT, (unsigned char*)ComplexSmallIcons9x9);
            //Dots
            ERC240_Display_Character(157, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(167, 105, DOT, (unsigned char*)ComplexSmallIcons9x9); 
        }
    }
    else if(Block_0_ID_201.NumValue >= OS_READY_TO_START)
    {
        if(abs(Block_0_ID_107.NumValue) > Block_1_ID_112.NumValue)
        {
            if(Block_0_ID_107.NumValue > 0)
            {
                //Arrows Pointing Right
                ERC240_Display_Character(137, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(147, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);  
            }
            else
            {
                //Arrows Pointing Left
                ERC240_Display_Character(137, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(147, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(157, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(167, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9); 
            }
            
            MainView_ShowFunctionPowerBar(177, 120, 60, 5, Block_0_ID_107.NumValue, Block_1_ID_110.NumValue,  Block_1_ID_111.NumValue, BLINK);
        }
        else
        {
            //Dots
            ERC240_Display_Character(137, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(147, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(157, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(167, 105, DOT, (unsigned char*)ComplexSmallIcons9x9); 
        }
    }
#endif
}

void MainView_ShowDCIN()
{
    MainView_ShowFuncitonIcon       (0, 0, 4, 5, ENG_DCIn_Charge, &Block_0_ID_202, &Block_0_ID_206,true);
    MainView_ShowTimer (83, 25, &Block_1_ID_1);
    
    #if defined (DEMO)
    //Dots
    ERC240_Display_Character(64, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(74, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(84, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
    ERC240_Display_Character(94, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
    #else
    if(Block_0_ID_202.NumValue >= OS_READY_TO_START)
    {
        if(abs(Block_0_ID_109.NumValue) > Block_1_ID_122.NumValue)
        {
            if(Block_0_ID_109.NumValue > 0)
            {
                //Arrows Pointing Right
                ERC240_Display_Character(64, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(74, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(84, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(94, 34, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                
            }
            else
            {
                //Arrows Pointing Left
                ERC240_Display_Character(64, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(74, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(84, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(94, 34, LEFT, (unsigned char*)ComplexSmallIcons9x9);
            }
            
            /******* CALCULATION METHOD: A (GT-1787)   *********/
            MainView_ShowFunctionPowerBar(2, 49, 60, 5, Block_0_ID_110.NumValue, Block_1_ID_123.NumValue, Block_1_ID_124.NumValue, NOBLINK);
            
            /******* CALCULATION METHOD: C (GT-1787)   *********/
            // Calculate the maximum possible DC IN power with the applied voltage and current limit setting
            /*
            long DCIN_Max_Power = Block_0_ID_108.NumValue * Block_1_ID_120.NumValue;
            
            // If the power is below 560W, it is the maximum attainable, and will be the endpoint of the bar
            if(DCIN_Max_Power < 560)
                MainView_ShowFunctionPowerBar(2, 49, 60, 5, Block_0_ID_110.NumValue, DCIN_Max_Power,  Block_1_ID_121.NumValue, NOBLINK);
            else // If the power is above 560W, the DC-DC converter will limit it, and 560W will be the endpoint of the bar
                MainView_ShowFunctionPowerBar(2, 49, 60, 5, Block_0_ID_110.NumValue, 560,  Block_1_ID_121.NumValue, NOBLINK);
            */
        }
        else
        {
            //Dots
            ERC240_Display_Character(64, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(74, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(84, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(94, 34, DOT, (unsigned char*)ComplexSmallIcons9x9);
        }
    }
    #endif
}

void MainView_ShowDCOUT()
{
    MainView_ShowFuncitonIcon       (0, 71, 4, 5, ENG_DCOut, &Block_0_ID_203, &Block_0_ID_207, false);
    MainView_ShowTimer              (83, 96, &Block_0_ID_225);
    
    #if defined (DEMO)
        //Dots
        ERC240_Display_Character(65, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(75, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(85, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
        ERC240_Display_Character(95, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
    #else

    if(Block_0_ID_203.NumValue >= OS_READY_TO_START)
    {
        if(abs(Block_0_ID_112.NumValue) > Block_1_ID_132.NumValue)
        {
            if(Block_0_ID_112.NumValue > 0)
            {
                //Arrows Pointing Right
                ERC240_Display_Character(64, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(74, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(84, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(94, 105, RIGHT, (unsigned char*)ComplexSmallIcons9x9);
            }
            else
            {
                //Arrows Pointing Left
                ERC240_Display_Character(64, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(74, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(84, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
                ERC240_Display_Character(94, 105, LEFT, (unsigned char*)ComplexSmallIcons9x9);
            }
            
            MainView_ShowFunctionPowerBar   (2, 120, 60, 5, Block_0_ID_112.NumValue, Block_1_ID_130.NumValue,  Block_1_ID_131.NumValue, BLINK);
        }
        else
        {
            //Dots
            ERC240_Display_Character(64, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(74, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(84, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
            ERC240_Display_Character(94, 105, DOT, (unsigned char*)ComplexSmallIcons9x9);
        }
    }
    #endif
}

void MainView_ShowBattery(unsigned char POSX, unsigned char POSY, unsigned char Width, unsigned char Hight, unsigned char IconX, unsigned char IconY)
{      
    unsigned int GrayedOutHight = (unsigned int)(Block_0_ID_119.NumValue*(Hight-2))>>16;

    //Battery - Top Part
    ERC240_Display_InvertArea_Square(POSX+(Width*0.5)-8,POSY-8,POSX+(Width*0.5)+8,POSY-2);
    
    //LPS Battery icons
    //Set Charging icon based on Battery status (0,210)
    if ((Block_0_ID_210.NumValue >= BS_CHARGE) && (Block_0_ID_210.NumValue != BS_FULL))       
        ERC240_Display_Character(IconX, IconY, 1, (unsigned char*)ComplexBatteryIcons16x44); 
    //Set Connect-to-charge icon based on SOC (0,119)
    else if (Block_0_ID_119.NumValue < 13107)
        ERC240_Display_Character(IconX, IconY, 0, (unsigned char*)ComplexBatteryIcons16x44); 
    
    //Draw Battery
    ERC240_Display_InvertArea_Square(POSX,POSY,POSX+Width,POSY+Hight);
    ERC240_Display_InvertArea_Square(POSX+2,POSY+2,POSX+Width-2,POSY+Hight-2-GrayedOutHight);
}

void MainView_ShowBattery_Ext(unsigned char POSX, unsigned char POSY, unsigned char Width, unsigned char Hight, unsigned char IconX, unsigned char IconY)
{      
    //Block_242_ID_119 is extension battery capacity soc
    unsigned int GrayedOutHight = (unsigned int)((Block_242_ID_119.NumValue*(Hight-2))>>16);

    unsigned char POSX_EXT = POSX + 13;
    unsigned char POSY_EXT = POSY - 12;
    //Battery - Top Part
    ERC240_Display_InvertArea_Square(POSX_EXT+(Width*0.5)-8,POSY_EXT-8,POSX_EXT+(Width*0.5)+8,POSY_EXT-2);
      
    //Draw Battery
    ERC240_Display_InvertArea_Square(POSX_EXT,POSY_EXT,POSX_EXT+Width,POSY_EXT+Hight);
    ERC240_Display_InvertArea_Square(POSX_EXT+2,POSY_EXT+2,POSX_EXT+Width-2,POSY_EXT+Hight-2-GrayedOutHight);
    
    //Delete area for normal battery
    ERC240_Display_EraseArea_Square(POSX_EXT,POSY_EXT+3,POSX_EXT+11,POSY_EXT+10);
    ERC240_Display_EraseArea_Square(POSX_EXT,POSY_EXT+11,POSX_EXT+18,POSY_EXT+Hight+8);
    
    MainView_ShowBattery(POSX, POSY, Width, Hight, IconX, IconY);
}

void MainView_ShowSolar()
{
    #if defined (DEMO)
        ERC240_Display_Character(70, 55, 8, (unsigned char*)SideIcons28x28);
    #else
    if(Block_0_ID_208.NumValue >=  OS_READY_TO_START)
        ERC240_Display_Character(70, 55, 8, (unsigned char*)SideIcons28x28);
        
    #endif
}

void MainView_ShowOperationTime()
{
    //Show Remaining time and SOC and status based on battery state (0,210)
    if(Block_0_ID_210.NumValue == BS_FULL) {
        //Status in Text
        ERC240_Display_String(119, 10, ENG_UndefinedTime, 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
        ERC240_Display_String(119, 115, ENG_SOC100, 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);      
    } 
    else if(Block_0_ID_210.NumValue == BS_EMPTY){
        ERC240_Display_String(119, 10, ENG_UndefinedTime, 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
        ERC240_Display_String(119, 115, (unsigned char*)SimpleValToString(&Block_0_ID_119, 0), 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
    } 
    else {
        ERC240_Display_String(119, 10, (unsigned char*)TimeValToString(&Block_0_ID_120), 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
        ERC240_Display_String(119, 115, (unsigned char*)SimpleValToString(&Block_0_ID_119, 0), 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
    }
}

void MainView_ShowOperationTime_Ext()
{
    //Show SOC
    ERC240_Display_String(119, 115, (unsigned char*)SimpleValToString(&Block_241_ID_119, 0), 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
    
    // Power level commented out as it is misleading
    //ERC240_Display_String(119, 10, (unsigned char*)SimpleValToString(&Block_241_ID_127,0), 0, 'C', (unsigned char*)S10B_Small_Fonts13x13);
}

void MainView_UpdateContent()
{       
    ERC240_Clear_MemoryBuffer();
    
    //Show Battery    
    if(Block_7_ID_0.NumValue == (1<<16))
    {
        //Timer Icon or "TESTMODE"
        if(Block_0_ID_212.NumValue > TM_NORMAL)
            ERC240_Display_String(120,1,(unsigned char*)OperationValToString(&Block_0_ID_212),0,'C',(unsigned char*)S6_Small_Fonts8x10);
    
        MainView_ShowOperationTime_Ext();
        MainView_ShowBattery_Ext(104, 45, 30, 60, 112, 50);
    }
    else
    {
        //Timer Icon or "TESTMODE"
        if(Block_0_ID_212.NumValue > TM_NORMAL)
            ERC240_Display_String(120,1,(unsigned char*)OperationValToString(&Block_0_ID_212),0,'C',(unsigned char*)S6_Small_Fonts8x10);
        else {
            ERC240_Display_Character(115,1, 0, (unsigned char*)ComplexSmallIcons9x9);
            
            #ifdef BLUETOOTH_CODE
            if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED && CHECK_BIT(Block_3_ID_3.NumValue, 20))
                ERC240_Display_Character(105,1, 6, (unsigned char*)ComplexSmallIcons9x9); //Bluetooth - show only if on and connected to device
            #endif
        }
        
        MainView_ShowOperationTime();
        MainView_ShowBattery(103, 35, 33, 75, 112, 50);
    }
    
    MainView_ShowDCIN();
    MainView_ShowDCOUT();
    MainView_ShowACOUT();
    MainView_ShowSolar();
    
    if (!((unsigned char)((Block_0_ID_223.NumValue / 100) % 100) == 11)) {
        MainView_ShowACIN();
    }
    
    //ERC240_Show_MemoryBuffer();
}