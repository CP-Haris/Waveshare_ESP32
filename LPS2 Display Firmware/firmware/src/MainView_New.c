#include "MainView_New.h"
#include "MainView_Old.h"
#include "Menu_Application.h"
#include "Values.h"
#include "Timer.h"
#include "Power.h"
#include "Menu_Error.h"
#include "Text.h"
#include "BLE_App.h"
//SOC BATSTATUS_SOC = 65535 -> 100%

void V2_MainView_Operation_Extension()
{      
    ERC240_Display_String(119,3,(unsigned char*)SimpleValToString(&Block_241_ID_127,0),0,'C',(unsigned char*)S18B_Helvetica24x29); 
}

void V2_MainView_Operation()
{      
    //LPS Is Charging
    if ((Block_0_ID_210.NumValue == BS_CHARGE) || (Block_0_ID_210.NumValue == BS_BALANCING))
    {
        //Status in Text: Write Remaining time or testmode
        if(Block_0_ID_212.NumValue > TM_NORMAL)
            ERC240_Display_String(119,3,(unsigned char*)OperationValToString(&Block_0_ID_212),0,'C',(unsigned char*)S14N_Helvetica19x22);
        else
            ERC240_Display_String(119, 3, ENG_CHG_Time, 0, 'C', (unsigned char*)S14N_Helvetica19x22);

        //Time - Remaining time
        ERC240_Display_String(119,25, (unsigned char*)TimeValToString(&Block_0_ID_120), 0, 'C', (unsigned char*)S18B_Helvetica24x29);
    }
    
    //LPS is FULL
    else if(Block_0_ID_210.NumValue == BS_FULL)
    {
        //Status in Text: Write Remaining time or testmode
        if(Block_0_ID_212.NumValue > TM_NORMAL)
            ERC240_Display_String(119,3,(unsigned char*)OperationValToString(&Block_0_ID_212),0,'C',(unsigned char*)S14N_Helvetica19x22);
        else
            ERC240_Display_String(119, 3, ENG_Full, 0, 'C', (unsigned char*)S14N_Helvetica19x22);

        //Time
        ERC240_Display_String(119,25, ENG_UndefinedTime, 0, 'C', (unsigned char*)S18B_Helvetica24x29);            
    }
    //LPS is Discharging
    else 
    {
        //Status in Text: Write Remaining time or testmode
        if(Block_0_ID_212.NumValue > TM_NORMAL)
            ERC240_Display_String(119,3,(unsigned char*)OperationValToString(&Block_0_ID_212),0,'C',(unsigned char*)S14N_Helvetica19x22);
        else if(Block_0_ID_210.NumValue == BS_EMPTY)
            ERC240_Display_String(119, 3, ENG_FL_Empty, 0, 'C', (unsigned char*)S14N_Helvetica19x22);
        else
            ERC240_Display_String(119, 3, ENG_TimeLeft, 0, 'C', (unsigned char*)S14N_Helvetica19x22);
        
        //Time
        if(Block_0_ID_210.NumValue == BS_EMPTY)
            ERC240_Display_String(119, 25, ENG_UndefinedTime, 0, 'C', (unsigned char*)S18B_Helvetica24x29);
        else
            ERC240_Display_String(119,25, (unsigned char*)TimeValToString(&Block_0_ID_120), 0, 'C', (unsigned char*)S18B_Helvetica24x29);       
    } 
}
void V2_MainView_SOC()
{
    //If errror
    
        if(LPSErrorBuffer[0] > 0){
            if(Block_7_ID_0.NumValue == 1<<16){ //Check if capacity extension is active
                ERC240_Display_String(158, 70, (unsigned char*)SimpleValToString(&Block_241_ID_119,0), 0, 'R', (unsigned char*)S28B_Helvetica37x44);
                ERC240_Display_Character(150, 63, 1, (unsigned char*)ComplexFrame63x57); //Draw Error Icon
            }
            else{
                ERC240_Display_String(153, 70, (unsigned char*)SimpleValToString(&Block_0_ID_119,0), 0, 'R', (unsigned char*)S28B_Helvetica37x44);
                ERC240_Display_Character(145, 63, 1, (unsigned char*)ComplexFrame63x57); //Draw Error Icon
            }
        }
        else  
            //Write SOC - Big
            if(Block_7_ID_0.NumValue == 1<<16) //Check if capacity extension is active
                ERC240_Display_String(207, 55, (unsigned char*)SimpleValToString(&Block_241_ID_119,0), 0, 'R', (unsigned char*)Helvetica_LT_Std59x70);
            else
                ERC240_Display_String(200, 55, (unsigned char*)SimpleValToString(&Block_0_ID_119,0), 0, 'R', (unsigned char*)Helvetica_LT_Std59x70);
}
void V2_MainView_SideIcons()
{
#if defined(DEMO)
    //Show All ICONS - DC SIDE
    ERC240_Display_Character(0, 5, 11, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(0, 35, 0, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(0, 65, 1, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(0, 95, 2, (unsigned char*)SideIcons28x28);
    
    //Show All ICONS - AC SIDE
    ERC240_Display_Character(212, 5, 7, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(212, 35, 3, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(212, 65, 4, (unsigned char*)SideIcons28x28);
    ERC240_Display_Character(212, 95, 5, (unsigned char*)SideIcons28x28);  
#else   
    //DC IN
    if(Block_0_ID_202.NumValue >=  OS_READY_TO_START)
        ERC240_Display_Character(0, 5, 11, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(0, 5, 6, (unsigned char*)SideIcons28x28);
    
    //DC OUT - Is conditional on
    if(Blink12VDC)
        ERC240_Display_Character(0, 35, 0, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(0, 35, 6, (unsigned char*)SideIcons28x28);
    
    //SOLAR
    if(Block_0_ID_208.NumValue >=  OS_READY_TO_START)
        ERC240_Display_Character(0, 65, 1, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(0, 65, 6, (unsigned char*)SideIcons28x28);
    
    //DCOUT - Is timed
    if((Block_0_ID_225.NumValue > 0) || (Block_1_ID_1.NumValue > 0))
        ERC240_Display_Character(0, 95, 2, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(0, 95, 6, (unsigned char*)SideIcons28x28);
    
    
    //Bluetooth - show only if on and connected to device
    #ifdef BLUETOOTH_CODE
    if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED && CHECK_BIT(Block_3_ID_3.NumValue, 20))
        ERC240_Display_Character(212, 5, 12, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(212, 5, 7, (unsigned char*)SideIcons28x28);
    #endif
    
    //AC OUT - Contitional On
    if(Blink230VAC) //Show icon for remote wakeup
        ERC240_Display_Character(212, 35, 3, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(212, 35, 7, (unsigned char*)SideIcons28x28);
    
    //AC OUT - AC Input plugged
    if(Block_0_ID_200.NumValue >= OS_READY_TO_START)
        ERC240_Display_Character(212, 65, 4, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(212, 65, 7, (unsigned char*)SideIcons28x28);

    //AC OUT - Is timed
    if(Block_0_ID_226.NumValue > 0)
        ERC240_Display_Character(212, 95, 5, (unsigned char*)SideIcons28x28);
    else
        ERC240_Display_Character(212, 95, 7, (unsigned char*)SideIcons28x28);
#endif
}
void V2_MainView_UpdateContent()
{
    //Clear image buffer
    ERC240_Clear_MemoryBuffer(); 
    V2_MainView_SideIcons();
    V2_MainView_SOC();
    
    if(Block_7_ID_0.NumValue == (1<<16))
    {
        MainView_ShowBattery_Ext(30, 70, 30, 50, 37, 74);
        //V2_MainView_Operation_Extension();
    }
    else
    {
        MainView_ShowBattery(30, 60, 33, 60, 39, 70);
        V2_MainView_Operation();
    }
    
    //ERC240_Show_MemoryBuffer(); 
}


