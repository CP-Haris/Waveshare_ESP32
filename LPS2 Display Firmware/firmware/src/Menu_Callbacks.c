#include "Menu_Application.h"
#include "Menu_Callbacks.h"
#include "Values.h"
#include "UART_App.h"

void CLB_Change_DCIN_StartVolt(Value *SelectedSetting)
{
    long MinValue = Block_30_ID_13.NumValue + 0x7FFF;
    if(SelectedSetting->NumValue <= MinValue)
        SelectedSetting->NumValue = MinValue;  
}

void CLB_Change_DCIN_StopVolt(Value *SelectedSetting)
{
    long MaxValue = Block_30_ID_12.NumValue - 0x7FFF;
    if(SelectedSetting->NumValue >= MaxValue)
        SelectedSetting->NumValue = MaxValue;   
}

void CLB_Change_IO_C1StartVolt(Value *SelectedSetting)
{
    long MinValue = Block_4_ID_3.NumValue + 6553.6;
    if(SelectedSetting->NumValue <= MinValue)
        SelectedSetting->NumValue = MinValue;  
}

void CLB_Change_IO_C1StopVolt(Value *SelectedSetting)
{
    long MaxValue = Block_4_ID_0.NumValue - 6553.6;
    if(SelectedSetting->NumValue >= MaxValue)
        SelectedSetting->NumValue = MaxValue;   
}

void CLB_Change_IO_C1CustomLvl(Value *SelectedSetting)
{
    if(SelectedSetting->NumValue > 0)
    {
        Block_4_ID_0.NumValue = 13<<16;
        Values_Send_ByValue(SET_VAL, &Block_4_ID_0, UART_CTRL);
        Block_4_ID_3.NumValue = 12<<16;
        Values_Send_ByValue(SET_VAL, &Block_4_ID_3, UART_CTRL);
    }
    else
    {
        Block_4_ID_0.NumValue = 4<<16;
        Values_Send_ByValue(SET_VAL, &Block_4_ID_0, UART_CTRL);
        Block_4_ID_3.NumValue = 3<<16;
        Values_Send_ByValue(SET_VAL, &Block_4_ID_3, UART_CTRL);
    }
}