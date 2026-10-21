#include "definitions.h" 

#ifndef Buzzer_H
#define Buzzer_H

//Tone          Frequency         
#define C_6     1047
#define Db_6    1109
#define D_6     1175
#define Eb_6    1245
#define E_6     1319
#define F_6     1397
#define Gb_6    1480
#define G_6     1568
#define Ab_6    1661
#define A_6     1760
#define Bb_6    1865
#define B_6     1976
#define C_7     2093
#define Db_7    2217
#define D_7     2349
#define Eb_7    2489
#define E_7     2637
#define F_7     2794
#define Gb_7    2960
#define G_7     3136
#define Ab_7    3322
#define A_7     3520
#define Bb_7    3729
#define B_7     3951
#define C_8     4186

//OFF
#define BUZ_OFF             -1

//Melodies
#define BUZ_HELLO           0
#define BUZ_GOODBYE         1
#define BUZ_FUNCTION_ON     2
#define BUZ_FUNCTION_OFF    3
#define BUZ_ERROR           4
#define BUZ_WARNING         5
#define BUZ_BIP             6
#define BUZ_INFO_C6         7
#define BUZ_INFO_D6         8
#define BUZ_INFO_E6         9
#define BUZ_INFO_F6         10
#define BUZ_INFO_G6         11
#define BUZ_INFO_A6         12
#define BUZ_INFO_B6         13
#define BUZ_INFO_C7         14
#define BUZ_INFO_D7         15
#define BUZ_INFO_E7         16
#define BUZ_INFO_F7         17
#define BUZ_INFO_G7         18
#define BUZ_INFO_A7         19
#define BUZ_INFO_B7         20
#define BUZ_INFO_C8         21
#define BUZ_MARIO           22

#define NUMBER_OF_TONES     23


//Tones:        Tone Data that is used to set the PWM
//Length:       Used Tone data lengt
typedef struct BUZ_Melody {
        const unsigned short *Tones;  
        const unsigned char Length;
    } BUZ_Melody;

    void BUZ_INT_Player();
    void BUZ_SetMelody(char Melody);
    void BUZ_SetTone(unsigned short Frequency);
#endif