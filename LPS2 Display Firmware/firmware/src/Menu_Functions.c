#include "Menu_Functions.h"
#include "Menu_Application.h"
#include "Menu_LockScreen.h"
#include "Settings.h"
#include "Functions.h"
#include "ERC240128FS.h"
#include "Text.h"
#include "Buzzer.h"
#include "BLE_App.h"

//Menu window and scroll setting
#define SCROLL_LENGTH           76
#define SCROLL_WIDTH            8
#define SCROLL_XPOS_START       229
#define SCROLL_YPOS_START       30
#define FIRST_ITEM_YPOS         31
#define SCREENEND_WITH_SCROLL   224
#define SCREENEND_NO_SCROLL     234
#define ITEMS_IN_WINDOW_MAX     5   
#define ITEM_HEIGHT             15

//Fonts
#define TitleFont   (unsigned char*)S10B_Small_Fonts13x13
#define MenuFont    (unsigned char*)S10_Small_Fonts12x13
#define PathFont    (unsigned char*)S6_Small_Fonts8x10

//Menu navigation pointers
#define MENU_QUE_SIZE 10
Menu *MenuQue[MENU_QUE_SIZE] = {&Menu_MAIN};
unsigned char MenuQueIndexer = 0;

void Menu_InitMenu()
{
    MenuQueIndexer = 0;
    MenuQue[0] = &Menu_MAIN;
}

Menu* Menu_GetCurrentMenu(void)
{
    return MenuQue[MenuQueIndexer];
}


//Returns -1 if no mached string is stored. Else returns index for string
static char FindMatchValueIndex(const Menu_Value *Val, long NumValue)
{
    unsigned char i;
    for (i=0; Val->NumbToString[i].MatchString != NULL; i++)
    {
        long MatchVal = (long)Val->NumbToString[i].MatchVal;
        if(MatchVal == NumValue)
            return (char)i;
    }
    return -1;
}

void Menu_UpdateVisualizationData()
{
    //Check software version xx:[XX]:xx. [XX] defines LPS variant, which have differences in menus to be shown and max charging power.
    //Value 14 is SE unit
    if((unsigned char)((Block_0_ID_223.NumValue / 100) % 100) == 14) {
        Menu_SOLAR.Show = MENU_HIDE;
        Menu_DCIN_StarterBattery.Show = MENU_HIDE;
        Menu_IOVOLT_Data.Show = MENU_HIDE;
        Menu_IOVOLT_DataFront.Show = MENU_HIDE;
        Menu_CONFIGS_SELECT.Show = MENU_HIDE;
        Menu_NRGMETER_Solar.Show = MENU_HIDE;
        
        // Change default maximum value of AC charging (limited to 610 W)
        if (!Block_1_ID_100.Flag.NumDefault_Received)
            Block_1_ID_100.NumValue = 600<<16;
        
        // Change default maximum value of DC Charging (limited to 12V 25A; 24V 12,5A = 300W on SE)
        if (!Block_1_ID_123.Flag.NumDefault_Received)
            Block_1_ID_123.NumValue = 300<<16;   // FUNC_DCIN_WATT_MAX
        if (!Block_1_ID_124.Flag.NumDefault_Received)
            Block_1_ID_124.NumValue = -300<<16;  // FUNC_DCIN_WATT_MIN
    }
    //Value 11 is 1100 W unit
    else if((unsigned char)((Block_0_ID_223.NumValue / 100) % 100) == 11) {
        Menu_SOLAR.Show = MENU_HIDE;
        Menu_ACIN.Show = MENU_HIDE;
        Menu_IOVOLT_Data.Show = MENU_HIDE;
        Menu_IOVOLT_DataFront.Show = MENU_HIDE;
        Menu_IOVOLT_C2Terminal.Show = MENU_HIDE;
        Menu_CONFIGS_SELECT.Show = MENU_HIDE;
        Menu_NRGMETER_Solar.Show = MENU_HIDE;
        Menu_NRGMETER_ACIn.Show = MENU_HIDE;
        
        // Change default maximum value of DC Charging (limited to 12V 25A; 24V 12,5A = 300W on 1100 W)
        if (!Block_1_ID_123.Flag.NumDefault_Received)
            Block_1_ID_123.NumValue = 300<<16;   // FUNC_DCIN_WATT_MAX
        if (!Block_1_ID_124.Flag.NumDefault_Received)
            Block_1_ID_124.NumValue = -300<<16;  // FUNC_DCIN_WATT_MIN
        
        // Change default maximum value of DC output current (limited to 120 A on 1100 W)
        if (!Block_1_ID_131.Flag.NumDefault_Received)
            Block_1_ID_131.NumValue = 0xFF87<<16;
    }
    // Value 15 is 1500 W unit
    else if((unsigned char)((Block_0_ID_223.NumValue / 100) % 100) == 15) {
        // Change default maximum value of AC charging (limited to 610 W)
        if (!Block_1_ID_100.Flag.NumDefault_Received)
            Block_1_ID_100.NumValue = 600<<16;
        
        // Change default maximum value of DC Charging (limited to 12V 25A; 24V 12,5A = 300W on 1100 W)
        if (!Block_1_ID_123.Flag.NumDefault_Received)
            Block_1_ID_123.NumValue = 300<<16;   // FUNC_DCIN_WATT_MAX
        if (!Block_1_ID_124.Flag.NumDefault_Received)
            Block_1_ID_124.NumValue = -300<<16;  // FUNC_DCIN_WATT_MIN
    }
    else {
        Menu_SOLAR.Show = MENU_SHOW;
        Menu_ACIN.Show = MENU_SHOW;
        Menu_DCIN_StarterBattery.Show = MENU_SHOW;
        Menu_IOVOLT_Data.Show = MENU_SHOW;
        Menu_IOVOLT_DataFront.Show = MENU_SHOW;
        Menu_IOVOLT_C2Terminal.Show = MENU_SHOW;
        Menu_CONFIGS_SELECT.Show = MENU_SHOW;
        Menu_NRGMETER_Solar.Show = MENU_SHOW;
        Menu_NRGMETER_ACIn.Show = MENU_SHOW;
    }

    //Check if DC operational voltage is 24
    //If yes, hide Start Voltage, Stop Voltage, Charge of starter Battery
    if(Block_0_ID_170.NumValue & (1 << 17))
    {
        Menu_DCIN_StartVolt12V.Show = false;
        Menu_DCIN_StopVolt12V.Show = false;
    }
    else
    {
        Menu_DCIN_StartVolt12V.Show = true;
        Menu_DCIN_StopVolt12V.Show = true;
    }
    
    //Check if values are bigger that 5V
    if((Block_4_ID_0.NumValue >= 5<<16) || (Block_4_ID_3.NumValue >= 5<<16))
    {
        Block_200_ID_13.NumValue = 65536;
        Menu_IOVOLT_C1CustomActive.Show = true;
        Menu_IOVOLT_C1CustomDeactive.Show = true;
    }
    else
    {
        Block_200_ID_13.NumValue = 0;
        Menu_IOVOLT_C1CustomActive.Show = false;
        Menu_IOVOLT_C1CustomDeactive.Show = false;
    }
    
    //Show "Charge of Starter Battery"
    if(Menu_DCIN_StartBatEnable.ST_Value->NumbToString[0].MatchVal & Menu_DCIN_StartBatEnable.ST_Value->Data->NumValue)
    {
        Menu_DCIN_CutOffTimer.Show = MENU_SHOW;
        if(Block_0_ID_170.NumValue & (1 << 17))
        {
            Menu_DCIN_StartBatChgCurrent12V.Show = MENU_HIDE;
            Menu_DCIN_StartBatChgVoltage12V.Show = MENU_HIDE;
            Menu_DCIN_StartBatCutOffCurrent12V.Show = MENU_HIDE;
            Menu_DCIN_StartBatMaintenance12V.Show = MENU_HIDE;
            Menu_DCIN_StartBatChgCurrent24V.Show = MENU_SHOW;
            Menu_DCIN_StartBatChgVoltage24V.Show = MENU_SHOW;
            Menu_DCIN_StartBatCutOffCurrent24V.Show = MENU_SHOW;
            Menu_DCIN_StartBatMaintenance24V.Show = MENU_SHOW;
        }
        else
        {
            Menu_DCIN_StartBatChgCurrent24V.Show = MENU_HIDE;
            Menu_DCIN_StartBatChgVoltage24V.Show = MENU_HIDE;
            Menu_DCIN_StartBatCutOffCurrent24V.Show = MENU_HIDE;
            Menu_DCIN_StartBatMaintenance24V.Show = MENU_HIDE;
            Menu_DCIN_StartBatChgCurrent12V.Show = MENU_SHOW;
            Menu_DCIN_StartBatChgVoltage12V.Show = MENU_SHOW;
            Menu_DCIN_StartBatCutOffCurrent12V.Show = MENU_SHOW;
            Menu_DCIN_StartBatMaintenance12V.Show = MENU_SHOW;
        }
    }
    else
    {
        Menu_DCIN_StartBatChgCurrent12V.Show = MENU_HIDE;
        Menu_DCIN_StartBatChgVoltage12V.Show = MENU_HIDE;
        Menu_DCIN_StartBatCutOffCurrent12V.Show = MENU_HIDE;
        Menu_DCIN_StartBatMaintenance12V.Show = MENU_HIDE;
        Menu_DCIN_StartBatChgCurrent24V.Show = MENU_HIDE;
        Menu_DCIN_StartBatChgVoltage24V.Show = MENU_HIDE;
        Menu_DCIN_StartBatCutOffCurrent24V.Show = MENU_HIDE;
        Menu_DCIN_StartBatMaintenance24V.Show = MENU_HIDE;
        Menu_DCIN_CutOffTimer.Show = MENU_HIDE;
    }
    
    //Jumpstart Enable
    if(CHECK_BIT(Block_30_ID_0.NumValue, 20))
        Menu_DCIN_Jumpstart.Show = MENU_HIDE;
    else
        Menu_DCIN_Jumpstart.Show = MENU_SHOW;
    
    #ifdef BLUETOOTH_CODE
    //Show Bluetooth subfunctions
    if(CHECK_BIT(Block_3_ID_3.NumValue, 20))
    {
        Menu_BLE_Attached.Show = MENU_SHOW;
        Menu_BLE_Status.Show = MENU_SHOW;
        Menu_BLE_Version.Show = MENU_SHOW;
    }
    else
    {
        Menu_BLE_Attached.Show = MENU_HIDE;
        Menu_BLE_Status.Show = MENU_HIDE;
        Menu_BLE_Version.Show = MENU_HIDE;
    }
    #endif

    if(Block_7_ID_0.NumValue == (1<<16))
    {
        Menu_CONFIGS_SOC_TOTAL.Show = MENU_SHOW;
        Menu_CONFIGS_SOC_EXT.Show = MENU_SHOW;
    }
    else
    {
        Menu_CONFIGS_SOC_TOTAL.Show = MENU_HIDE;
        Menu_CONFIGS_SOC_EXT.Show = MENU_HIDE;
    }
    
    if (Block_0_ID_200.NumValue == OS_ON) //Operation state AC In
        Menu_ACIN_Current.Show = true;
    else
        Menu_ACIN_Current.Show = false;

        
    #ifdef ENABLE_TEST_VALS
    //Enable Test Values
    //if(Block_255_ID_255.NumValue > 0)
        Menu_TESTVALS.Show = MENU_SHOW;
    //else
    //    Menu_TESTVALS.Show = MENU_HIDE;
    #endif
}

void Menu_ShowNumbricVal(Menu *ActiveMenu, unsigned char U8_YLocation, unsigned char U8_XLocation, char Adjust, const unsigned char *Font)
{ 
   //Show value if value is active
    
    if(ActiveMenu->ST_Value != NULL) {
        if(ActiveMenu->ST_Value->Show)
        {
            // Dereference ST_Value pointer to access members
            const Menu_Value *MenuValue = ActiveMenu->ST_Value;

            bool ValueIsMasked = false;
            //Check if pointer is loaded
            if((MenuValue->NumbToString != NULL)&&(MenuValue->Data->Prefix != MASKED_BITMAP)){
                char MatchIndex = FindMatchValueIndex(MenuValue, MenuValue->Data->NumValue);
                if(MatchIndex >= 0)
                {
                    ERC240_Display_String(U8_XLocation,U8_YLocation,MenuValue->NumbToString[(unsigned char)MatchIndex].MatchString,0,Adjust,Font);
                    ValueIsMasked = true;
                }
                else if(MenuValue->NumbToString_NoMatch != NULL)
                {
                    ERC240_Display_String(U8_XLocation,U8_YLocation,MenuValue->NumbToString_NoMatch,0,Adjust,Font);
                    ValueIsMasked = true;
                }
            }

            if(!ValueIsMasked)  
            {
                //Check for prefix
                switch(MenuValue->Data->Prefix)
                {
                    case TIME_HEAD:
                    case TIME_HHMMSS:
                    case TIME_HHMM:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)TimeValToString(MenuValue->Data),0,Adjust,Font);
                    break;

                    case STATE_BATTERY:
                    case STATE_FAILURE:
                    case STATE_OPERATION:
                    case STATE_TESTMODE:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)OperationValToString(MenuValue->Data),0,Adjust,Font);
                    break;

                    case SERIAL:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)SerialValToString(MenuValue->Data),0,Adjust,Font);
                    break;

                    case VERSION_LONG:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)VersionValToString(MenuValue->Data, true),0,Adjust,Font);
                    break;

                    case VERSION_SHORT:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)VersionValToString(MenuValue->Data, false),0,Adjust,Font);
                    break;              

                    case DATE:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)DateValToString(MenuValue->Data),0,Adjust,Font);
                    break;

                    case MASKED_BITMAP:
                        if(MenuValue->NumbToString != NULL){
                            //MatchIndex = -1 if no match found
                            if(MenuValue->NumbToString[0].MatchVal & MenuValue->Data->NumValue)
                                ERC240_Display_String(U8_XLocation,U8_YLocation,MenuValue->NumbToString[0].MatchString,0,Adjust,Font);
                            else
                                ERC240_Display_String(U8_XLocation,U8_YLocation,MenuValue->NumbToString_NoMatch,0,Adjust,Font);
                        }
                    break;

                    #ifdef BLUETOOTH_CODE
                    case MAC:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)MacValToString(MenuValue->Data),0,Adjust,Font);
                    break;

                    case PASSKEY:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)PasskeyToString(MenuValue->Data),0,Adjust,Font);
                    break;
                    #endif

                    default:
                        ERC240_Display_String(U8_XLocation,U8_YLocation,(unsigned char*)SimpleValToString(MenuValue->Data, MenuValue->Decimals),0,Adjust,Font);
                    break;
                }
            }
        }
    }
}

void Menu_ShowPath()
{
    //Path Lines
    ERC240_Display_XLine(0,116,240);
    ERC240_Display_XLine(0,127,240);
    
    uint8_t U8_PathIndex = 0;
    uint8_t U8_PathStringLocator = 0;
    uint8_t Arrow[] = {3,' ','>',' '};
    
    //Write Initial menu text
    U8_PathStringLocator = ERC240_Display_String(U8_PathStringLocator, 117, MenuQue[U8_PathIndex]->S8A_Title, 0, 'L', PathFont);
    U8_PathIndex++;
    
    //Make complete pathtext by reading marker location of first parrent
    while(U8_PathIndex <= MenuQueIndexer)
    {
        U8_PathStringLocator += ERC240_Display_String(U8_PathStringLocator, 117, Arrow, 0, 'L', PathFont);  
        U8_PathStringLocator += ERC240_Display_String(U8_PathStringLocator, 117, MenuQue[U8_PathIndex]->S8A_Title, 0, 'L', PathFont);
        U8_PathIndex++;
    }
}

void Menu_ShowChildInMenu(struct Menu *Child, unsigned char Location, unsigned char RightOffset)
{
    int YPosition = FIRST_ITEM_YPOS+(Location*ITEM_HEIGHT);
    unsigned char Shift = 6 + Child->Shift;

    //MENU - LEFT ICON
    if(Child->ST_Children != NULL)
        //Draw Icon - Menu arrow 
        Shift = Shift+ERC240_Display_Character(Shift-4, YPosition, 8, (unsigned char*)ToggleBox14x13)+2;
    else if(Child->ST_Popup != NULL)
    {       
        //Draw icon - Locked popup
        if(Child->ST_Popup->CodeProtect && !SettingsUnlocked)
            Shift = Shift+ERC240_Display_Character(Shift, YPosition, 17, (unsigned char*)ToggleBox14x13)+2;
        //Draw icon - Settings
        else if((Child->ST_Popup->Type == POPUP_SETTING)|(Child->ST_Popup->Type == POPUP_SETTING_MULTI))
            Shift = Shift+ERC240_Display_Character(Shift, YPosition, 14, (unsigned char*)ToggleBox14x13)+2;
        //Draw icon - Password screen
        else if(Child->ST_Popup->Type == POPUP_LOCK_SET)
            Shift = Shift+ERC240_Display_Character(Shift, YPosition, 9, (unsigned char*)ToggleBox14x13)+2;
    }
    //ERROR - LEFT ICON
    else if(Child->ST_Value != NULL)
    {
        //Show error icon if the item is an ERROR and shift left space
        if(Child->ST_Value->Data->Prefix == ERROR){
            //Add icon
            if(ErrorList[Child->ST_Value->Data->NumValue].ErrorLevel > FL_WARNING)
               Shift = Shift+ERC240_Display_Character(Shift, YPosition, 15, (unsigned char*)ToggleBox14x13)+2;
            else
               Shift = Shift+ERC240_Display_Character(Shift, YPosition, 16, (unsigned char*)ToggleBox14x13)+2;
        }   
    }

    //Write Left text - Title
    ERC240_Display_String(Shift,YPosition, Child->S8A_Title,0,'L', MenuFont);
    
    //Write Right text - Value
    if(Child->ST_Value != NULL) {
        if(Child->ST_Value->Data->Prefix == ERROR)
            ERC240_Display_String(RightOffset,YPosition,(unsigned char*)U8ToString(Child->ST_Value->Data->NumValue),0,'R',MenuFont);
        else
            Menu_ShowNumbricVal((struct Menu*)Child, YPosition, RightOffset, 'R', MenuFont);
    }
}

void Menu_DrawMarker(struct Menu *CurrentMenu, unsigned char Location, unsigned char RightOffset)
{
    if(CurrentMenu->ST_Children[CurrentMenu->MarkerChildIndex] != NULL)
    {
        unsigned char AdjustedLocation = FIRST_ITEM_YPOS+(Location*ITEM_HEIGHT);
        ERC240_Display_InvertArea_Square(3, AdjustedLocation, RightOffset+1, AdjustedLocation+14);
    }
}

void Menu_DrawScroll(unsigned char WindowPosition, unsigned char VisibleItems)
{
    //Draw Scroll Bar
    unsigned char U8_numberOfScrolls =  VisibleItems-ITEMS_IN_WINDOW_MAX;
    if(U8_numberOfScrolls > 0)
    {    
       unsigned char U8_sizeOfScroll    = SCROLL_LENGTH/(U8_numberOfScrolls+1);
       unsigned char YStart             = SCROLL_YPOS_START+(WindowPosition*U8_sizeOfScroll);
       unsigned char YEnd               = SCROLL_YPOS_START+(WindowPosition*U8_sizeOfScroll)+U8_sizeOfScroll;
       ERC240_Display_InvertArea_Square(SCROLL_XPOS_START,YStart,SCROLL_XPOS_START+SCROLL_WIDTH,YEnd);
    }    
}

bool Menu_ShowMenu(Menu* SelectedMenu)
{
    if(SelectedMenu->ST_Value != NULL) {
        if((SelectedMenu->Show == MENU_SHOW) || (SelectedMenu->Show == MENU_HIDE_IF_NULL_OR_0 && SelectedMenu->ST_Value->Data->NumValue != NULL))
            return true;
    } else {
        if(SelectedMenu->Show == MENU_SHOW)
            return true;
    }
    
    return false;
}

//#define ITEMS_IN_WINDOW_MAX     5  
//Used for compensating dynamic menu when items are hidden. Updates the window and marker position
bool Menu_UpdateMarker(struct Menu *ST_Menu, unsigned char* NumberOfVisibleItems, unsigned char* MarkerPosition, unsigned char* Scroll)
{
    bool Detected_MarkedItem = false;
    bool Detected_Window = false;
    bool DrawMarker = false;
    unsigned char VisibleItems = 0;
    unsigned char VisibleMarkedItem = 0;
    unsigned char VisibleFirstItemInWindow = 0;
    
    //Move from away from hidden marker
    unsigned char i = ST_Menu->MarkerChildIndex;
    if(!Menu_ShowMenu(ST_Menu->ST_Children[i]))
    {
        while (ST_Menu->ST_Children[i+1] != NULL) {
            i++;
            if(Menu_ShowMenu(ST_Menu->ST_Children[i])) {
                ST_Menu->MarkerChildIndex = i;
                break;
            }
        }

        if(!Menu_ShowMenu(ST_Menu->ST_Children[i]))
        {
            while (i > 0) {
                i--;
                if(Menu_ShowMenu(ST_Menu->ST_Children[i])){
                    ST_Menu->MarkerChildIndex = i;
                    break;
                }
            } 
        }
    }
    
    //If moved away sucessfully
    if(Menu_ShowMenu(ST_Menu->ST_Children[ST_Menu->MarkerChildIndex]))
    {
        DrawMarker = true;
        i = 0;

        //Go through all visible items in menu
        while (ST_Menu->ST_Children[i] != NULL) {
            if(Menu_ShowMenu(ST_Menu->ST_Children[i])){

                //Set at what number of visible items the currently sellected is at
                if(i == ST_Menu->MarkerChildIndex){
                    Detected_MarkedItem = true;
                    VisibleMarkedItem = VisibleItems;
                }
                //Set at what number of visible items the beginning of the window is
                if(i == ST_Menu->MarkerWindowIndex){
                    Detected_Window = true;
                    VisibleFirstItemInWindow = VisibleItems;
                }

                //Adjust Windown if item is out of bounds (Decrease)
                if ((VisibleMarkedItem < VisibleFirstItemInWindow) && Detected_MarkedItem)
                {
                    VisibleFirstItemInWindow--;
                    ST_Menu->MarkerWindowIndex--;
                    //Skip until shown item
                    while (!Menu_ShowMenu(ST_Menu->ST_Children[ST_Menu->MarkerWindowIndex]))
                        ST_Menu->MarkerWindowIndex--;
                }

                //Adjust Windown if item is out of bounds (Increase)
                else if((VisibleMarkedItem >= VisibleFirstItemInWindow + ITEMS_IN_WINDOW_MAX) && Detected_Window)
                {
                    VisibleFirstItemInWindow++;
                    ST_Menu->MarkerWindowIndex++;  

                    while (!Menu_ShowMenu(ST_Menu->ST_Children[ST_Menu->MarkerWindowIndex]))
                        ST_Menu->MarkerWindowIndex++;
                }
                VisibleItems++;
            }
            //Increment to next menu if current selected menu is on a hidden item.
            else {
                if(i == ST_Menu->MarkerChildIndex)
                    ST_Menu->MarkerChildIndex--;
            }      
            i++;
        }

        //Adjust if tailing items are hidden
        if(ST_Menu->ST_Children[i] != NULL)
        {
            while(VisibleFirstItemInWindow+ITEMS_IN_WINDOW_MAX > VisibleItems)
            {
                VisibleFirstItemInWindow--;
                ST_Menu->MarkerWindowIndex--;
                //Skip until shown item
                while (!Menu_ShowMenu(ST_Menu->ST_Children[ST_Menu->MarkerWindowIndex]))
                    ST_Menu->MarkerWindowIndex--;
            }
        }

        *NumberOfVisibleItems = VisibleItems;
        *MarkerPosition = VisibleMarkedItem - VisibleFirstItemInWindow;
        *Scroll = VisibleFirstItemInWindow;
    }
    return DrawMarker;
}

//Called By Graphic Statemachine
void Menu_UpdateGraphics(void) {
    Menu_UpdateVisualizationData();
    ERC240_Clear_MemoryBuffer();

    Menu* CurrentMenu = Menu_GetCurrentMenu();   

    // Write Title
    ERC240_Display_String(120, 4, CurrentMenu->S8A_Title, 0, 'C', TitleFont);         

    // Draw menu content lines
    ERC240_Display_XLine(0, 24, 239);
    ERC240_Display_XLine(0, 25, 239);
    ERC240_Display_XLine(0, 112, 239);
    ERC240_Display_XLine(0, 111, 239);

    // Variables to hold marker and scroll info
    unsigned char MarkedChild = 0;
    unsigned char ChildrenToShow = 0;
    unsigned char Scroll = 0;

    // Update marker and window based on the current menu state
    bool DrawMenuItems = Menu_UpdateMarker(CurrentMenu, &ChildrenToShow, &MarkedChild, &Scroll);
    
    if(DrawMenuItems)
    {
        // Show visible menu items
        unsigned char ShownItems = 0;
        unsigned char IndexOfChildren = CurrentMenu->MarkerWindowIndex;
        while ((CurrentMenu->ST_Children[IndexOfChildren] != NULL) && (ShownItems < ITEMS_IN_WINDOW_MAX)) {
            if (Menu_ShowMenu(CurrentMenu->ST_Children[IndexOfChildren])) {
                // Check if scroll is required
                if (ChildrenToShow > ITEMS_IN_WINDOW_MAX) {
                    Menu_ShowChildInMenu(CurrentMenu->ST_Children[IndexOfChildren], ShownItems, SCREENEND_WITH_SCROLL);
                } else {
                    Menu_ShowChildInMenu(CurrentMenu->ST_Children[IndexOfChildren], ShownItems, SCREENEND_NO_SCROLL);
                }
                ShownItems++;
            }
            IndexOfChildren++;
        }

        // Draw marker and scroll
        if (ChildrenToShow > ITEMS_IN_WINDOW_MAX) {
            Menu_DrawMarker(CurrentMenu, MarkedChild, SCREENEND_WITH_SCROLL);
            Menu_DrawScroll(Scroll, ChildrenToShow);
        } else {
            Menu_DrawMarker(CurrentMenu, MarkedChild, SCREENEND_NO_SCROLL);
        }
    }
    
    Menu_ShowPath();
    //ERC240_Show_MemoryBuffer();
}

//USED FOR LOGIC OF MENU
bool Menu_SetNextMenu(Menu *NextMenu)
{
    if(MenuQueIndexer < MENU_QUE_SIZE)
    {
        MenuQueIndexer++;
        MenuQue[MenuQueIndexer] = NextMenu;
        return true;
    }
    return false;
}

bool Menu_SetPreviousMenu()
{
    if(MenuQueIndexer > 0)
    {
        MenuQue[MenuQueIndexer]->MarkerChildIndex = 0;
        MenuQue[MenuQueIndexer]->MarkerWindowIndex = 0;
        MenuQueIndexer--;
        return true;
    }
    return false;
}

Menu* Menu_GetSelectedChild(struct Menu *ST_Menu)
{
    //error - Return calling menu
    return ST_Menu->ST_Children[ST_Menu->MarkerChildIndex];
}


//Used when navigation inside the menu is changed
void Menu_MoveMarker(unsigned char MarkerDirection)
{
    Menu* CurrentMenu = Menu_GetCurrentMenu();
       
    switch(MarkerDirection)
    {
        case MARKER_RESET:
        {
            CurrentMenu->MarkerChildIndex = 0;
        }
        break;
        
        case MARKER_UP: 
        {           
            //Decrease to next shown child. Do noting if no child exists.
            unsigned char i = CurrentMenu->MarkerChildIndex;
            while (i > 0) {
                i--;
                if(Menu_ShowMenu(CurrentMenu->ST_Children[i])){
                    CurrentMenu->MarkerChildIndex = i;
                    break;
                }
            }           
        }
        break;
        
        case MARKER_DOWN: //OK
        {        
            //Increase to next shown child. Do noting if no child exists.
            unsigned char i = CurrentMenu->MarkerChildIndex;
            while (CurrentMenu->ST_Children[i+1] != NULL) {
                i++;
                if(Menu_ShowMenu(CurrentMenu->ST_Children[i])) {
                    CurrentMenu->MarkerChildIndex = i;
                    break;
                }
            }
        }
        break;         
    }
}