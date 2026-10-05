#include "Values.h"
#include "ERC240128FS.h"
#include "Text.h"
#include "Functions.h"
#include "Button.h"
#include "UART_App.h"

#ifdef BLUETOOTH_CODE
void PassKey_ShowPopup()
{
    //Clear Area
    ERC240_Display_EraseArea_Square(15,10,225,121);

    //Frame
    ERC240_Display_XLine(15,9,210);
    ERC240_Display_XLine(15,121,210);
    ERC240_Display_XLine(16,122,209);
    ERC240_Display_YLine(15,10,111);
    ERC240_Display_YLine(224,10,111);
    ERC240_Display_YLine(225,11,111);

    //Title
    ERC240_Display_StringTextBox(120, 15, 100, ENG_PassKey, (unsigned char *)S10B_Small_Fonts13x13, 0, 'C');

    //Passkey
    ERC240_Display_String(120,40,(unsigned char*)PasskeyToString(&Block_200_ID_102),0,'C',(unsigned char *)S18B_Helvetica24x29);

    //Description
    ERC240_Display_StringTextBox(120, 100, 200, ENG_DESC_PresToContinue, (unsigned char *)S10_Small_Fonts12x13, 0, 'C');
    
    //Show Popup
    ERC240_Show_MemoryBuffer();
}
  
bool PassKey_CheckForPopup()
{
    if(Block_200_ID_102.Flag.NumValue_Received)
        return true;
    return false;
}

void PassKey_ClearPopup()
{
    Block_200_ID_102.Flag.NumValue_Received = false;
}

#endif