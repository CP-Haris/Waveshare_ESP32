#include "Menu_SystemConfiguration.h"
#include "Menu_Settings.h"
#include "Functions.h"
#include "UART_App.h"
#include "Timer.h"
#include "Text.h"


#define MAX_RETRIES 5

volatile Setting ListOfSetting[70];
volatile unsigned char Index = 0;
volatile unsigned char Size = 0;


volatile long CurrentSetting = 0;
volatile long NewSetting = 0;

void Config_UpdateCurrentSetting(Value *SelectedSetting)
{
    CurrentSetting = SelectedSetting->NumValue;
}

//Setting 1
void Config_LoadSetting(long SettingNumber)
{
    Index = 0;
    Size = 0;
    switch (SettingNumber)
    {
        case (1 << 16): //Succes
        case (1 << 20): //Failed to set
        case (1 << 24): //Failed to clear
        {   // Capacity Extension
            ListOfSetting[0] =  (Setting){ .SetValue = 0,           .Value = &Block_6_ID_3};
            ListOfSetting[1] =  (Setting){ .SetValue = 1619001344,  .Value = &Block_6_ID_6};

            ListOfSetting[2] =  (Setting){ .SetValue = 196608,      .Value = &Block_82_ID_0};
            ListOfSetting[3] =  (Setting){ .SetValue = 0,           .Value = &Block_82_ID_1};
            ListOfSetting[4] =  (Setting){ .SetValue = 7798784,     .Value = &Block_82_ID_2};
            ListOfSetting[5] =  (Setting){ .SetValue = 589824,      .Value = &Block_82_ID_3};
            ListOfSetting[6] =  (Setting){ .SetValue = 327680,      .Value = &Block_82_ID_4};
            ListOfSetting[7] =  (Setting){ .SetValue = 62259,       .Value = &Block_82_ID_5};
            ListOfSetting[8] =  (Setting){ .SetValue = 0,           .Value = &Block_82_ID_6};
            ListOfSetting[9] =  (Setting){ .SetValue = 13107200,    .Value = &Block_82_ID_7};
            ListOfSetting[10] = (Setting){ .SetValue = 589824,      .Value = &Block_82_ID_8};
            ListOfSetting[11] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_9};
            ListOfSetting[12] = (Setting){ .SetValue = 131072,      .Value = &Block_82_ID_10};
            ListOfSetting[13] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_11};
            ListOfSetting[14] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_12};
            ListOfSetting[15] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_13};
            ListOfSetting[16] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_14};
            ListOfSetting[17] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_15};
            ListOfSetting[18] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_16};
            ListOfSetting[19] = (Setting){ .SetValue = 7798784,     .Value = &Block_82_ID_17};
            ListOfSetting[20] = (Setting){ .SetValue = 196608,      .Value = &Block_82_ID_18};
            ListOfSetting[21] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_19};
            ListOfSetting[22] = (Setting){ .SetValue = 62914,       .Value = &Block_82_ID_20};
            ListOfSetting[23] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_21};
            ListOfSetting[24] = (Setting){ .SetValue = 13107200,    .Value = &Block_82_ID_22};
            ListOfSetting[25] = (Setting){ .SetValue = 196608,      .Value = &Block_82_ID_23};
            ListOfSetting[26] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_24};
            ListOfSetting[27] = (Setting){ .SetValue = 131072,      .Value = &Block_82_ID_25};
            ListOfSetting[28] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_26};
            ListOfSetting[29] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_27};
            ListOfSetting[30] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_28};
            ListOfSetting[31] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_29};
            ListOfSetting[32] = (Setting){ .SetValue = 0,           .Value = &Block_82_ID_30};

            ListOfSetting[33] = (Setting){ .SetValue = 65536,       .Value = &Block_83_ID_0};
            ListOfSetting[34] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_1};
            ListOfSetting[35] = (Setting){ .SetValue = 7798784,     .Value = &Block_83_ID_2};
            ListOfSetting[36] = (Setting){ .SetValue = 196608,      .Value = &Block_83_ID_3};
            ListOfSetting[37] = (Setting){ .SetValue = 327680,      .Value = &Block_83_ID_4};
            ListOfSetting[38] = (Setting){ .SetValue = 64225,       .Value = &Block_83_ID_5};
            ListOfSetting[39] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_6};
            ListOfSetting[40] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_7};
            ListOfSetting[41] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_8};
            ListOfSetting[42] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_9};
            ListOfSetting[43] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_10};
            ListOfSetting[44] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_11};
            ListOfSetting[45] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_12};
            ListOfSetting[46] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_13};
            ListOfSetting[47] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_14};
            ListOfSetting[48] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_15};
            ListOfSetting[49] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_16};
            ListOfSetting[50] = (Setting){ .SetValue = 7798784,     .Value = &Block_83_ID_17};
            ListOfSetting[51] = (Setting){ .SetValue = 589824,      .Value = &Block_83_ID_18};
            ListOfSetting[52] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_19};
            ListOfSetting[53] = (Setting){ .SetValue = 62915,       .Value = &Block_83_ID_20};
            ListOfSetting[54] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_21};
            ListOfSetting[55] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_22};
            ListOfSetting[56] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_23};
            ListOfSetting[57] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_24};
            ListOfSetting[58] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_25};
            ListOfSetting[59] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_26};
            ListOfSetting[60] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_27};
            ListOfSetting[61] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_28};
            ListOfSetting[62] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_29};
            ListOfSetting[63] = (Setting){ .SetValue = 0,           .Value = &Block_83_ID_30};
            Size = 64;
        }
        break;
    }
}

bool Config_RemoveSetting(long SettingNumber)
{
    Config_LoadSetting(SettingNumber);
    
    bool Default_Received = false;
    unsigned char retry = 0;
    unsigned char Title[8] = {7,'W','A','I','T','.','.','.'};
    unsigned char Description[25] = {24,'R','e','m','o','v','i','n','g',' ','c','u','r','r','e','n','t',' ','s','e','t','t','i','n','g'};
    Menu_Setting_Message(Title, Description);
            
    while(Index != Size)
    {
        COM_GetRXData(&UART1_Buffer);
        WDTCONbits.WDTCLR = 1;

        if(Default_Received)
        {
            if((ListOfSetting[Index].Value->NumValue == ListOfSetting[Index].Value->NumDefault) && ListOfSetting[Index].Value->Flag.NumValue_Received)
            {
                Default_Received = false;
                ListOfSetting[Index].Value->Flag.BlockedBySetting = 0;
                ListOfSetting[Index].Value->Flag.NumValue_Received = 0;
                retry = 0;
                Index = Index + 1;
            }       
            else if(!Timer_CLK10ms_SystemConfigTimeout.Active)
            {
                Timer_StartTimer(&Timer_CLK10ms_SystemConfigTimeout);
                
                //Set Default value as NumValue
                unsigned char Data[7];
                Data[0]=0x50;
                Data[1]=ListOfSetting[Index].Value->Block;
                Data[2]=ListOfSetting[Index].Value->ID;
                Data[3]=(unsigned char)(ListOfSetting[Index].Value->NumDefault);
                Data[4]=(unsigned char)(ListOfSetting[Index].Value->NumDefault>>8);
                Data[5]=(unsigned char)(ListOfSetting[Index].Value->NumDefault>>16);
                Data[6]=(unsigned char)(ListOfSetting[Index].Value->NumDefault>>24);
                SendMSG(Data, 7, UART_CTRL);
                
                //Not needed, the LPS sends an echo of the value when a SET is made
                //Values_RequestByValue(ListOfSetting[Index].Value, false, GET_VAL);
                
                retry = retry + 1;
                if (retry > MAX_RETRIES)   
                    break;
            }
        }
        else if(!Timer_CLK10ms_SystemConfigTimeout.Active)
        {
            Timer_StartTimer(&Timer_CLK10ms_SystemConfigTimeout);
            Default_Received = Values_RequestByValue(ListOfSetting[Index].Value, false, GET_DEFAULT);
            retry = retry + 1;
            if (retry > MAX_RETRIES)
                break;
        }
    }
    
    if(Index == Size)
        return true;
    else
        return false;
}

bool Config_SetSetting (long SettingNumber)
{
    Config_LoadSetting (SettingNumber);
    unsigned char retry = 0;

    unsigned char Title[8] = {7,'W','A','I','T','.','.','.'};
    unsigned char Description[21] = {20,'A','p','p','l','y','i','n','g',' ','n','e','w',' ','s','e','t','t','i','n','g'};
    Menu_Setting_Message(Title, Description);
    
    while(Index != Size)
    {
        COM_GetRXData(&UART1_Buffer);
        WDTCONbits.WDTCLR = 1;

        if((ListOfSetting[Index].SetValue == ListOfSetting[Index].Value->NumValue) && ListOfSetting[Index].Value->Flag.NumValue_Received)
        {
            ListOfSetting[Index].Value->Flag.NumValue_Received = 0;
            ListOfSetting[Index].Value->Flag.BlockedBySetting = 1;
            retry = 0;
            Index = Index + 1;
        }       
        else if(!Timer_CLK10ms_SystemConfigTimeout.Active)
        {
            Timer_StartTimer(&Timer_CLK10ms_SystemConfigTimeout);

            //Send Setting (Write)
            unsigned char Data[7];
            Data[0]=0x50;
            Data[1]=ListOfSetting[Index].Value->Block;
            Data[2]=ListOfSetting[Index].Value->ID;
            Data[3]=(unsigned char)(ListOfSetting[Index].SetValue);
            Data[4]=(unsigned char)(ListOfSetting[Index].SetValue>>8);
            Data[5]=(unsigned char)(ListOfSetting[Index].SetValue>>16);
            Data[6]=(unsigned char)(ListOfSetting[Index].SetValue>>24);
            SendMSG(Data, 7, UART_CTRL);

            //Reqest Setting (Read) - Not needed, the LPS sends an echo of the value when a SET is made
            //Values_RequestByValue(ListOfSetting[Index].Value, false, GET_VAL);
            
            //Increment retry counter
            retry = retry + 1;
            if (retry > MAX_RETRIES)
            {
                break;
            }
        }
    }
    
    if(Index == Size)
        return true;
    else
        return false;
}

//Function accept callback (Menu Popup)
void Config_RunSetting(Value *SelectedSetting)
{
    NewSetting = SelectedSetting->NumValue;
    //Check if there is a change in selection
    if(NewSetting != CurrentSetting)
    {
        //Erase current configuration
        if(Config_RemoveSetting(CurrentSetting))
        {
            //Send the new configuration if it is not 0 = None
            if(NewSetting != 0)
            {
                //Set New configuration
                if(Config_SetSetting(NewSetting))
                {
                    //0x000X0000 (Represents successed set value 0 to 16 val)
                    Block_7_ID_0.NumValue = NewSetting;
                    Values_Send_ByValue(SET_VAL, &Block_7_ID_0, UART_CTRL);
                }  
                else
                {
                    //0x00X00000 (Represents failed to set 0 to 16 val)
                    Block_7_ID_0.NumValue = NewSetting << 4;
                    Values_Send_ByValue(SET_VAL, &Block_7_ID_0, UART_CTRL);
                }
            }
        }
        else
        {
            //0x0X000000 (Represents failed to clear 0 to 16 val)
            Block_7_ID_0.NumValue = NewSetting << 8;
            Values_Send_ByValue(SET_VAL, &Block_7_ID_0, UART_CTRL);
        }
    }
}
    



