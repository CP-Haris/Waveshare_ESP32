#include <stdbool.h>                    // Defines true
#include "Values.h"

#ifndef _ERC240128FS_H_
#define _ERC240128FS_H_

#define LCD_DATA    LCD_CD_Set()       
#define LCD_COMMAND LCD_CD_Clear()         

#define SPLASH_LPS_V1   0
#define SPLASH_LPS_V2   1
#define SPLASH_ZEBRA    2

#define VIEW_OLD  0<<16
#define VIEW_NEW  1<<16


typedef union {
  struct {
    unsigned char D0:1;
    unsigned char D1:1;
    unsigned char D2:1;
    unsigned char D3:1;
    unsigned char D4:1;
    unsigned char D5:1;
    unsigned char D6:1;
    unsigned char D7:1;
  };
  struct {
    unsigned char BYTE:8;
  };
} DATAbits_t;
    
//-- PROTOTYPES --//
void Init_PMP(void);

void waitms(unsigned int milliseconds);

void ERC240_EnterSleep();
void ERC240_ExitSleep();
void ERC240_ResetMTP();
void ERC240_ReInit();

void ERC240_Init(bool LongDelay);
void ERC240_Display_Address(void);
void ERC240_Clear_MemoryBuffer();
void ERC240_Display_Byte(unsigned char x, unsigned char y, unsigned char byte);
void ERC240_Show_MemoryBuffer();
void ERC240_Set_Contrast(Value *VAL_Contrast);
unsigned char ERC240_LengthOfCharecter(unsigned char Charecter, const unsigned char *Font);
unsigned char ERC240_Display_Character(unsigned char x, unsigned char y, unsigned char Charecter, const unsigned char *Font);
unsigned char ERC240_Display_String(int x, int y, const unsigned char *String, unsigned char CharSpacing, unsigned char adjust, const unsigned char *Font);
void ERC240_Display_StringTextBox(int x, int y, int BoundaryLength, const unsigned char *String, const unsigned char *Font, unsigned char LineSpace, unsigned Adjust);
void ERC240_Display_XLine(unsigned char x, unsigned char y, unsigned char Length);
void ERC240_Display_YLine(unsigned char x, unsigned char y, unsigned char Length);
void ERC240_Display_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
void ERC240_Display_EraseArea_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
void ERC240_Display_InvertArea_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
void ERC240_Display_SplashScreen(unsigned char Splash);
void ERC240_Display_Pixel(unsigned char x, unsigned char y);

//-- END OF PROTOTYPES --//
extern bool DisplayOpdated;

//LCD Buffer
extern volatile unsigned char LCD_Buffer[]; //Was volatile

//Fonts - V1
extern const unsigned char S6_Small_Fonts8x10[];
extern const unsigned char S10_Small_Fonts12x13[];
extern const unsigned char S10B_Small_Fonts13x13[];


//Icons - V1
extern const unsigned char MainIcons35x26[];
extern const unsigned char Navigation49x21[];
extern const unsigned char SplashScreen240x54[];
extern const unsigned char ToggleBox14x13[];
extern const unsigned char ComplexBatteryIcons16x44[];
extern const unsigned char ComplexFrame63x57[];
extern const unsigned char ComplexSmallIcons9x9[];


//Fonts - V2
extern const unsigned char Helvetica_LT_Std59x70[];
extern const unsigned char S11N_Helvetica15x17[];
extern const unsigned char S14N_Helvetica19x22[];
extern const unsigned char S18B_Helvetica24x29[];
extern const unsigned char S28B_Helvetica37x44[];  //Not all characters!

//Icons - V2
//extern const unsigned char QR100x100[];
extern const unsigned char ICON_27X58[];
extern const unsigned char SideIcons28x28[];


#endif