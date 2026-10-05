#include "ERC240128FS.h"
#include "Menu_LockScreen.h"
#include "Text.h"
#include "Button.h"
#include "Values.h"
#include "Timer.h"
#include "State_Statemachine.h"
#include "UART_BIOS.h"

bool SettingsUnlocked = false;

LockMenu ActiveLock = {.SelectedCharacter = 0, .Characters[0] = '0', .Characters[1] = '0', .Characters[2] = '0', .Characters[3] = '0',};

//Numbers Position
#define XPOSITION_Numbers       71
#define YPOSIION_Numbers        51

//Arrow Position
#define XPOSITION_Arrow         68
#define YPOSIION_ArrowUpper     40
#define YPOSIION_ArrowLower     65

//Inverted Area - Selection
#define XPOSIION_InvertStart    70
#define XPOSIION_InvertEnd      80
#define YPOSIION_InvertStart    51
#define YPOSIION_InvertEnd      67

//X Spacing
#define SPACING                 30

void ClearLockValue()
{
    ActiveLock.Characters[0] = '0';
    ActiveLock.Characters[1] = '0';
    ActiveLock.Characters[2] = '0';
    ActiveLock.Characters[3] = '0';
    ActiveLock.SelectedCharacter = 0;
}

bool Menu_SettingLock_Logic(bool Change)
{
    bool CloseView = false;
    //Get lock value
    if(KeypadLPS.OK.Press.Short)
    {
        if(ActiveLock.SelectedCharacter >= 3)
        {
            //If inputed code is equal to stored in unit than unlock
            long PressedCode = (((ActiveLock.Characters[0]-48)*1000)+((ActiveLock.Characters[1]-48)*100)+((ActiveLock.Characters[2]-48)*10)+(ActiveLock.Characters[3]-48))<<16;
            long StoredCode  = Block_3_ID_4.NumValue;
            
            if(Change)
            {
                //Set new lock value
                Block_3_ID_4.NumValue = PressedCode;
                Block_3_ID_4.Flag.NumValue_Received = true;
                Values_Send_ByValue(0x50, &Block_3_ID_4, UART_CTRL);
                SettingsUnlocked = false;
            }
            else if (PressedCode == StoredCode)
            {
                Timer_StartTimer(&Timer_CLK100ms_SettingLock);
                SettingsUnlocked = true;
            }
            ClearLockValue();
            CloseView = true;
        }
        else
            ActiveLock.SelectedCharacter++;
    }
    //C
    else if(KeypadLPS.BACK.Press.Short)
    {
        ClearLockValue();
        CloseView = true;
    }
    else if(KeypadLPS.UP.Press.Short)
    {
        if(ActiveLock.Characters[ActiveLock.SelectedCharacter]>='9')
            ActiveLock.Characters[ActiveLock.SelectedCharacter] = '0';
        else
            ActiveLock.Characters[ActiveLock.SelectedCharacter]++;
    }
    else if(KeypadLPS.DOWN.Press.Short)
    {
        if(ActiveLock.Characters[ActiveLock.SelectedCharacter]<='0')
            ActiveLock.Characters[ActiveLock.SelectedCharacter] = '9';
        else
            ActiveLock.Characters[ActiveLock.SelectedCharacter]--;
    }
    
    return CloseView;
}

void Menu_SettingLock_GraphicPresentation()
{
    //Clear Area
    ERC240_Display_EraseArea_Square(15,10,225,121);
    long StoredCode  = Block_3_ID_4.NumValue;
    
    if(SettingsUnlocked)
    {
        if(StoredCode == 0)
        {
            //NO CODE STORED
            //Title
            ERC240_Display_String(120,25,ENG_SetPin,0,'C',(unsigned char*)S10B_Small_Fonts13x13); 
            //Description
            ERC240_Display_String(120,80, ENG_DESC_SetPin,0,'C',(unsigned char*)S6_Small_Fonts8x10);
        }
        else
        {
            //CODE STORED
            //Title
            ERC240_Display_String(120,25, ENG_ChangePin,0,'C',(unsigned char*)S10B_Small_Fonts13x13); 
            //Description
            ERC240_Display_String(120,80, ENG_DESC_ChangePin,0,'C',(unsigned char*)S6_Small_Fonts8x10); 
        }
        //Comment - "0000 = No Lock"
        ERC240_Display_String(120,95, ENG_LockOff,0,'C',(unsigned char*)S6_Small_Fonts8x10);
    }
    else
    {
        //Title
        ERC240_Display_String(120,25,ENG_EnterPin,0,'C',(unsigned char*)S10B_Small_Fonts13x13); 
        //Description
        ERC240_Display_String(120,80,ENG_DESC_EnterPin,0,'C',(unsigned char*)S6_Small_Fonts8x10);
    }
    
    
    //Write Code Numbers and Arrows
    unsigned char i;
    for(i=0; i<=3; i++)
    {
       //Draw Arrows   
        ERC240_Display_Character(XPOSITION_Arrow + (SPACING*i)  , YPOSIION_ArrowUpper   , 12, (unsigned char*)ToggleBox14x13);
        ERC240_Display_Character(XPOSITION_Numbers + (SPACING*i), YPOSIION_Numbers      , ActiveLock.Characters[i], (unsigned char*)S11N_Helvetica15x17);
        ERC240_Display_Character(XPOSITION_Arrow + (SPACING*i)  , YPOSIION_ArrowLower   , 13, (unsigned char*)ToggleBox14x13); 
    }
   
    //Mark Selected Value
    ERC240_Display_InvertArea_Square(XPOSIION_InvertStart + (SPACING*ActiveLock.SelectedCharacter), YPOSIION_InvertStart, XPOSIION_InvertEnd + (SPACING*ActiveLock.SelectedCharacter), YPOSIION_InvertEnd);
    
    //Left Buttom Navigation
    ERC240_Display_Character(16, 100, 3, (unsigned char*)Navigation49x21); 
    
    //Right Buttom Navigation
    ERC240_Display_Character(175, 100, 2, (unsigned char*)Navigation49x21);
    
    //Frame
    ERC240_Display_XLine(15,9,210);
    ERC240_Display_XLine(15,121,210);
    ERC240_Display_XLine(16,122,209);
    ERC240_Display_YLine(15,10,111);
    ERC240_Display_YLine(224,10,111);
    ERC240_Display_YLine(225,11,111);
        
    //Show Graphic
    //ERC240_Show_MemoryBuffer();
}


