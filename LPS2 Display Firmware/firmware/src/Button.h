#include <stdbool.h>                    // Defines true

#define BTN_UP      !PORTCbits.RC4
#define BTN_DOWN    !PORTCbits.RC5
#define BTN_OK      !PORTCbits.RC3
#define BTN_BACK    !PORTAbits.RA9

#ifndef _BUTTON_H_
	#define _BUTTON_H_

# define KEYPAD_ACTIVITY    (KeypadLPS.BACK.Press.Any | KeypadLPS.DOWN.Press.Any | KeypadLPS.OK.Press.Any | KeypadLPS.UP.Press.Any)

    typedef struct Button {
        unsigned char Press_Inform[3];
        union {
            struct {
                unsigned char Short : 1; // Bit-wise access to Short (first bit)
                unsigned char Long  : 1; // Bit-wise access to Long (second bit)
                unsigned char Reserved : 6; // Reserved for any extra bits if needed
            };
            unsigned char Any; // Full-byte access to Duration
        } Press;
        unsigned char Short_TMRValue;
        unsigned char Short_TMRLimit;
        unsigned char Long_TMRValue;
        unsigned char Long_TMRLimit;
    }Button;
    
    //Keypad Counter
    typedef struct Keypad {
        Button UP;
        Button DOWN;
        Button BACK;
        Button OK;
    }Keypad; 
    

    void Button_Update();
    void Button_Clear();
    void Button_TimerController(bool ButtonSwitch, struct Button *BTN);

    extern Keypad KeypadLPS;
#endif
