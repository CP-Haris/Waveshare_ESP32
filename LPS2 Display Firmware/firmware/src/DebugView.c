#include "../src/config/default/definitions.h"
#include "ERC240128FS.h"
#include "Text.h"
#include "Functions.h"
#include "Button.h"

bool DebugView_CheckButtonPress()
{
    if(KeypadLPS.UP.Press.Short)
        LED_G_12V_Set();
    if(KeypadLPS.DOWN.Press.Short)
        LED_R_12V_Set();
    if(KeypadLPS.OK.Press.Short)
        LED_G_230V_Set();
    if(KeypadLPS.BACK.Press.Short)
        LED_R_230V_Set();
    
    if (KeypadLPS.UP.Short_TMRLimit == 5)
        LED_G_12V_Clear();
    if (KeypadLPS.DOWN.Short_TMRLimit == 5)
        LED_R_12V_Clear();
    if (KeypadLPS.OK.Short_TMRLimit == 5)
        LED_G_230V_Clear();
    if (KeypadLPS.BACK.Short_TMRLimit == 5)
        LED_R_230V_Clear();
    
    /*if(KeypadLPS.UP.Press.Any)
        LED_G_12V_Set();
    else
        LED_G_12V_Clear();
    
    if(KeypadLPS.DOWN.Press.Any)
        LED_R_12V_Set();
    else
        LED_R_12V_Clear();
    
    if(KeypadLPS.OK.Press.Any)
        LED_G_230V_Set();
    else
        LED_G_230V_Clear();
    
    if(KeypadLPS.BACK.Press.Any)
        LED_R_230V_Set();
    else
        LED_R_230V_Clear();*/
    
    if(KEYPAD_ACTIVITY)
        return true;
    else
        return false;
}

void DebugView_GraphicPresentation()
{
    ERC240_Clear_MemoryBuffer();
    ERC240_Display_Square(0, 0, 239, 127);
    ERC240_Display_Square(1, 1, 238, 126);
    //ERC240_Display_SplashScreen(SPLASH_LPS_V2);
    ERC240_Display_String(10,105, ENG_SVersionDisp,0,'L',(unsigned char*)S11N_Helvetica15x17);
    ERC240_Display_String(230,105,(unsigned char*)VersionValToString(&Block_200_ID_0, true),0,'R',(unsigned char*)S11N_Helvetica15x17);
    //ERC240_Show_MemoryBuffer();
}
