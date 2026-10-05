#include "Menu_Settings.h"
#include "Settings.h"
#include "Button.h"
#include "ERC240128FS.h"
#include "Functions.h"
#include <string.h>
#include "State_Controller.h"
#include "Text.h"
#include "UART_App.h"

//Value defines
#define ValuePos_X  120
#define ValuePos_Y  45
#define ValueFont   (unsigned char *)S10B_Small_Fonts13x13
unsigned char LogicEntryCounter = 0;
unsigned char GraphicEntryCounter = 0;
long InitialValue;


bool Menu_Setting_ValueController(struct Menu *Item)
{
    bool GoToPreviousState = false;
    long lStepFactor = 0;
    long long lHighResStepFactor;
    unsigned long ulDiv;
    unsigned long long ullStepCount;
    unsigned long long  ullRounding;
    
    // This added in order to show info pop-ups which can be accepted to send information
    if (Item->ST_Value == NULL) {
        if(KeypadLPS.OK.Press.Short) {
            GoToPreviousState = true;
        
            //Accept Value pointer function
            if(Item->ST_Popup->Function_Accept != NULL) Item->ST_Popup->Function_Accept();
        
        } else if(KeypadLPS.BACK.Press.Short) {
            GoToPreviousState = true;

            //Cancel pointer function
            if(Item->ST_Popup->Function_Cancel != NULL) Item->ST_Popup->Function_Cancel();
        }
        
        return GoToPreviousState;
    }
    
    if(LogicEntryCounter == 1)
    {
        //Save initial value (value before setting change)
        InitialValue = Item->ST_Value->Data->NumValue;
        Values_RequestByValue(Item->ST_Value->Data, false, GET_MIN_MAX);

        #ifdef EXTENDED_POINTERS
        //Entry pointer function
        if(Item->ST_Popup->Function_Entry != NULL)
            Item->ST_Popup->Function_Entry(Item->ST_Value->Data);
        #endif
    }

    
    if(KeypadLPS.UP.Press.Short || KeypadLPS.UP.Press.Long)
    {
        if(Item->ST_Value->Data->Prefix == MASKED_BITMAP)
        {
            Item->ST_Value->Data->NumValue |= Item->ST_Value->NumbToString[0].MatchVal;
        }
        else if(Item->ST_Value->Data->Prefix == MAC)
        {
            //Do Nothing
        }
        else
        {
            //Get next value
            if(KeypadLPS.UP.Press.Long) 
                lStepFactor = Item->ST_Popup->StepFactor_Fast;
            else 
                lStepFactor = Item->ST_Popup->StepFactor_Slow;
            long NewValue = Item->ST_Value->Data->NumValue + lStepFactor;

            //Rounding next value
            if (lStepFactor < 0x10000) 
            {
                ulDiv = 0xFFFFFFFF / lStepFactor;
                if ((ulDiv & 0x0000FFFF) > 0x8000) ulDiv += 0x10000;    //Afrund stepcount til heltal
                ulDiv = ulDiv >> 16;                                    //Fjern decimaler på StepCount
                lHighResStepFactor = (0xFFFFFFFF / ulDiv);
                ullStepCount = NewValue & 0x0000FFFF;
                ullStepCount = (ullStepCount<<32) / lHighResStepFactor;
                if ((ullStepCount & 0x0000FFFF) > 0x8000) {
                    ullStepCount = (ullStepCount >> 16) + 1;
                } else {
                    ullStepCount = (ullStepCount >> 16);
                }
                ullRounding = ullStepCount * lHighResStepFactor;
                if ((ullRounding & 0x0000FFFF) > 0x8000) {
                    ullRounding = (ullRounding >> 16) + 1;
                } else {
                    ullRounding = (ullRounding >> 16);
                }
                NewValue = (NewValue & 0xFFFF0000) + ullRounding;
            }
            
            ///INCREMENT VALUE
            //Jump to max if certan value
            if((NewValue > Item->ST_Popup->JumpToMaxVal) && (Item->ST_Popup->JumpToMaxVal > 0))
                Item->ST_Value->Data->NumValue = Item->ST_Value->Data->NumMax;
            
            //Out of range (Max)
            else if ((NewValue > Item->ST_Value->Data->NumMax) && (Item->ST_Value->Data->NumMax > 0))
                Item->ST_Value->Data->NumValue = Item->ST_Value->Data->NumMax;
            
            //Out of range (Min)
            else if (NewValue < Item->ST_Value->Data->NumMin)
                Item->ST_Value->Data->NumValue = Item->ST_Value->Data->NumMin;
            
            //Increment
            else
                Item->ST_Value->Data->NumValue = NewValue;
        }
        
        //Value Change pointer function
        if(Item->ST_Popup->Function_Change != NULL)
            Item->ST_Popup->Function_Change(Item->ST_Value->Data);
        
    }
    
    else if(KeypadLPS.DOWN.Press.Short || KeypadLPS.DOWN.Press.Long)
    {
        if(Item->ST_Value->Data->Prefix == MASKED_BITMAP)
        {
            Item->ST_Value->Data->NumValue &= ~(Item->ST_Value->NumbToString[0].MatchVal);
        }
        else if(Item->ST_Value->Data->Prefix == MAC)
        {
            //Do Nothing
        }
        else
        {
            //Get next value
            if(KeypadLPS.DOWN.Press.Long)
                lStepFactor = Item->ST_Popup->StepFactor_Fast;
            else
                lStepFactor = Item->ST_Popup->StepFactor_Slow;
            long NewValue = Item->ST_Value->Data->NumValue - lStepFactor;

            //Rounding next value
            if (lStepFactor < 0x10000) 
            {
                ulDiv = 0xFFFFFFFF / lStepFactor;
                if ((ulDiv & 0x0000FFFF) > 0x8000) ulDiv += 0x10000;    //Afrund stepcount til heltal
                ulDiv = ulDiv >> 16;                                    //Fjern decimaler på StepCount
                lHighResStepFactor = (0xFFFFFFFF / ulDiv);
                ullStepCount = NewValue & 0x0000FFFF;
                ullStepCount = (ullStepCount<<32) / lHighResStepFactor;
                if ((ullStepCount & 0x0000FFFF) > 0x8000) {
                    ullStepCount = (ullStepCount >> 16) + 1;
                } else {
                    ullStepCount = (ullStepCount >> 16);
                }
                ullRounding = ullStepCount * lHighResStepFactor;
                if ((ullRounding & 0x0000FFFF) > 0x8000) {
                    ullRounding = (ullRounding >> 16) + 1;
                } else {
                    ullRounding = (ullRounding >> 16);
                }
                NewValue = (NewValue & 0xFFFF0000) + ullRounding;
            }
           
            ///INCREMENT VALUE
            //Jump to max if certan value
            if((NewValue > Item->ST_Popup->JumpToMaxVal) && (Item->ST_Popup->JumpToMaxVal > 0))
                Item->ST_Value->Data->NumValue = Item->ST_Popup->JumpToMaxVal;
            //Out of range (Max)
            else if ((NewValue > Item->ST_Value->Data->NumMax) && (Item->ST_Value->Data->NumMax > 0))
                Item->ST_Value->Data->NumValue = Item->ST_Value->Data->NumMax;
            //Out of range (Min)
            else if (NewValue < Item->ST_Value->Data->NumMin)
                Item->ST_Value->Data->NumValue = Item->ST_Value->Data->NumMin;
            //Decrement
            else
                Item->ST_Value->Data->NumValue = NewValue;
            
        }
        
        //Value Change pointer function
        if(Item->ST_Popup->Function_Change != NULL)
            Item->ST_Popup->Function_Change(Item->ST_Value->Data);
        
    }
    //Set new value
    else if(KeypadLPS.OK.Press.Short)
    {
        GoToPreviousState = true;
        #ifdef BLUETOOTH_CODE
        if(Item->ST_Value->Data->Prefix == MAC)
        {
            unsigned char ClearDevice[] = {0x70, 0x02, 0x00};
            switch(Item->ST_Value->Data->ID)
            {
                case 110:
                case 111:
                    Block_200_ID_110.NumValue = NULL;
                    Block_200_ID_111.NumValue = NULL;
                    ClearDevice[2] = 0;
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                break;

                case 112:
                case 113:
                    Block_200_ID_112.NumValue = NULL;
                    Block_200_ID_113.NumValue = NULL;
                    ClearDevice[2] = 1;
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                break;

                case 114:
                case 115:
                    Block_200_ID_114.NumValue = NULL;
                    Block_200_ID_115.NumValue = NULL;
                    ClearDevice[2] = 2;
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                break;

                case 116:
                case 117:
                    Block_200_ID_116.NumValue = NULL;
                    Block_200_ID_117.NumValue = NULL;
                    ClearDevice[2] = 3;
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                break;

                case 118:
                case 119:
                    Block_200_ID_118.NumValue = NULL;
                    Block_200_ID_119.NumValue = NULL;
                    ClearDevice[2] = 4;
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                    SendMSG (ClearDevice, 3, UART_BLE);
                break;
            }
        }
        if(Item->ST_Value->Data->Flag.Owned_BLE)
            Values_Send_ByValue(SET_VAL, (Value*)Item->ST_Value->Data, UART_BLE);
        #endif
        
        if(Item->ST_Value->Data->Flag.Owned_CTRL)
            Values_Send_ByValue(SET_VAL, (Value*)Item->ST_Value->Data, UART_CTRL);
        
        
        //Accept Value pointer function
        if(Item->ST_Popup->Function_Accept != NULL)
            Item->ST_Popup->Function_Accept(Item->ST_Value->Data);
    }
    //Exit Setting
    else if(KeypadLPS.BACK.Press.Short)
    {
        GoToPreviousState = true;
        Item->ST_Value->Data->NumValue = InitialValue;
        
        //Cancel pointer function
        if(Item->ST_Popup->Function_Cancel != NULL)
            Item->ST_Popup->Function_Cancel(Item->ST_Value->Data);
    }
    return GoToPreviousState;
}

bool Menu_Setting_Logic(struct Menu *Item)
{
    if(LogicEntryCounter == 0xFF)
        LogicEntryCounter = 2;
    else
        LogicEntryCounter++;
    
    bool GoToPreviousState = false;
    if(Item->ST_Popup->Type == POPUP_SETTING) {
        GoToPreviousState = Menu_Setting_ValueController(Item);

        if(KEYPAD_ACTIVITY)
            GlobalState.Update_Graphic = true; 
    }
    
    if(GoToPreviousState)
    {
        GraphicEntryCounter = 0;
        LogicEntryCounter = 0;
    }
    return GoToPreviousState;
}      

void Menu_Setting_GraphicPresentation(struct Menu *Item)
{
    if(GraphicEntryCounter == 0xFF)
        GraphicEntryCounter = 2;
    else
        GraphicEntryCounter++;    
    
    if(Item->ST_Popup->Type == POPUP_SETTING)
    {
        //Clear Area
        ERC240_Display_EraseArea_Square(15,10,225,121);

        //Title
        ERC240_Display_String(120,30,Item->S8A_Title,0,'C',(unsigned char*)S10B_Small_Fonts13x13);
        
        if(Item->ST_Value != NULL) {
            if(!(Item->ST_Value->Data->Prefix == MAC))
            {
                //Left Buttom Navigation
                ERC240_Display_Character(16, 100, 3, (unsigned char*)Navigation49x21); 

                //Right Buttom Navigation
                ERC240_Display_Character(175, 100, 2, (unsigned char*)Navigation49x21);
            }
        }
        
        //Frame
        ERC240_Display_XLine(15,9,210);
        ERC240_Display_XLine(15,121,210);
        ERC240_Display_XLine(16,122,209);
        ERC240_Display_YLine(15,10,111);
        ERC240_Display_YLine(224,10,111);
        ERC240_Display_YLine(225,11,111);

        //Show value if value is active
        Menu_ShowNumbricVal(Item, ValuePos_Y, ValuePos_X, 'C', ValueFont);

        //Description
        if(Item->ST_Popup->Description != NULL)
            ERC240_Display_StringTextBox(ValuePos_X, ValuePos_Y+20, 150, Item->ST_Popup->Description, (unsigned char*)S6_Small_Fonts8x10, 0, 'C');

        //Show Graphic
        //ERC240_Show_MemoryBuffer();
    }        
}


void Menu_Setting_Message(unsigned char *Title, unsigned char *Desctiprion)
{
    //Clear Area
    ERC240_Display_EraseArea_Square(15,10,225,121);

    //Title
    ERC240_Display_String(120,30,Title,0,'C',(unsigned char*)S10B_Small_Fonts13x13);
    ERC240_Display_StringTextBox(ValuePos_X, ValuePos_Y+20, 150, Desctiprion, (unsigned char*)S6_Small_Fonts8x10, 0, 'C');
    
    //Frame
    ERC240_Display_XLine(15,9,210);
    ERC240_Display_XLine(15,121,210);
    ERC240_Display_XLine(16,122,209);
    ERC240_Display_YLine(15,10,111);
    ERC240_Display_YLine(224,10,111);
    ERC240_Display_YLine(225,11,111);

    //Show Graphic
    ERC240_Show_MemoryBuffer();
}




