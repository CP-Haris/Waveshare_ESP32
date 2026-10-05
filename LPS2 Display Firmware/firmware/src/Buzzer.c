#include "Buzzer.h"
#include "Values.h"
//

const unsigned short  Melody_Mario[]         = {E_7,E_7,0,E_7,0,C_7,E_7,0,G_7,G_7,G_7,0,G_6};
const unsigned short  Melody_Bip[]           = {D_7};
const unsigned short  Melody_BipHigh[]       = {C_8};
const unsigned short  Melody_Warning[]       = {C_6,0,C_7,C_7};
const unsigned short  Melody_Error[]         = {B_7,B_7,0,B_7,B_7};
const unsigned short  Melody_Function_Off[]  = {C_7,C_7,C_6};
const unsigned short  Melody_Function_On[]   = {C_6,C_6,C_7};
const unsigned short  Melody_Goodby[]        = {G_6,F_6,E_6,D_6,C_6};
const unsigned short  Melody_Hello[]         = {C_6,D_6,E_6,F_6,G_6};;
const unsigned short  Melody_Info_C6[]          = {C_6,0,C_6};
const unsigned short  Melody_Info_D6[]          = {D_6,0,D_6};
const unsigned short  Melody_Info_E6[]          = {E_6,0,E_6};
const unsigned short  Melody_Info_F6[]          = {F_6,0,F_6};
const unsigned short  Melody_Info_G6[]          = {G_6,0,G_6};
const unsigned short  Melody_Info_A6[]          = {A_6,0,A_6};
const unsigned short  Melody_Info_B6[]          = {B_6,0,B_6};
const unsigned short  Melody_Info_C7[]          = {C_7,0,C_7};
const unsigned short  Melody_Info_D7[]          = {D_7,0,D_7};
const unsigned short  Melody_Info_E7[]          = {E_7,0,E_7};
const unsigned short  Melody_Info_F7[]          = {F_7,0,F_7};
const unsigned short  Melody_Info_G7[]          = {G_7,0,G_7};
const unsigned short  Melody_Info_A7[]          = {A_7,0,A_7};
const unsigned short  Melody_Info_B7[]          = {B_7,0,B_7};
const unsigned short  Melody_Info_C8[]          = {C_8,C_8,0,C_8,C_8};


char ActiveToneIndex = -1;
unsigned char PlayedTones = 0;
//Melody Container
const BUZ_Melody Tones[NUMBER_OF_TONES] =
{
    [BUZ_HELLO]         = {.Length = 5, .Tones = Melody_Hello},
    [BUZ_GOODBYE]       = {.Length = 5, .Tones = Melody_Goodby},
    [BUZ_FUNCTION_ON]   = {.Length = 3, .Tones = Melody_Function_On},
    [BUZ_FUNCTION_OFF]  = {.Length = 3, .Tones = Melody_Function_Off},
    [BUZ_ERROR]         = {.Length = 5, .Tones = Melody_Error},
    [BUZ_WARNING]       = {.Length = 7, .Tones = Melody_Warning},
    [BUZ_BIP]           = {.Length = 1, .Tones = Melody_Bip},
    [BUZ_INFO_C6]       = {.Length = 3, .Tones = Melody_Info_C6},
    [BUZ_INFO_D6]       = {.Length = 3, .Tones = Melody_Info_D6},
    [BUZ_INFO_E6]       = {.Length = 3, .Tones = Melody_Info_E6},
    [BUZ_INFO_F6]       = {.Length = 3, .Tones = Melody_Info_F6},
    [BUZ_INFO_G6]       = {.Length = 3, .Tones = Melody_Info_G6},
    [BUZ_INFO_A6]       = {.Length = 3, .Tones = Melody_Info_A6},
    [BUZ_INFO_B6]       = {.Length = 3, .Tones = Melody_Info_B6},
    [BUZ_INFO_C7]       = {.Length = 3, .Tones = Melody_Info_C7},
    [BUZ_INFO_D7]       = {.Length = 3, .Tones = Melody_Info_D7},
    [BUZ_INFO_E7]       = {.Length = 3, .Tones = Melody_Info_E7},
    [BUZ_INFO_F7]       = {.Length = 3, .Tones = Melody_Info_F7},
    [BUZ_INFO_G7]       = {.Length = 3, .Tones = Melody_Info_G7},
    [BUZ_INFO_A7]       = {.Length = 3, .Tones = Melody_Info_A7},
    [BUZ_INFO_B7]       = {.Length = 3, .Tones = Melody_Info_B7},
    [BUZ_INFO_C8]       = {.Length = 5, .Tones = Melody_Info_C8},
    [BUZ_MARIO]         = {.Length = 13, .Tones = Melody_Mario}
};

void BUZ_SetMelody(char Melody)
{
    if(ActiveToneIndex <= 0)
    {
        if(Block_0_ID_212.NumValue == TM_NORMAL)
        {
            ActiveToneIndex = Melody;
            PlayedTones = 0;
        }
    }
}

//Debug C_6 = 1047Hz
void BUZ_SetTone(unsigned short Frequency)
{
    //Set PWM Values
    if(Frequency>0)
    {
        //Set local values
        unsigned int TMR_Periode = TMR3_FrequencyGet()/Frequency;
        unsigned int TMR_DeadTime =  TMR_Periode/4;
        unsigned int TMR_HalfPeriod = TMR_Periode/2;
        
        //Stop PWM generator
        OCMP1_Disable(); 
        OCMP2_Disable();
        TMR3_Stop();
        
        //Initialize PWM generator
        TMR3_PeriodSet(TMR_Periode);
        OCMP1_CompareValueSet(0);
        OCMP1_CompareSecondaryValueSet(TMR_HalfPeriod-TMR_DeadTime);
        OCMP2_CompareValueSet(TMR_HalfPeriod);
        OCMP2_CompareSecondaryValueSet (TMR_Periode-TMR_DeadTime);
        
        //Activate PWM generator
        OCMP1_Enable();
        OCMP2_Enable();
        TMR3_Start();
    }
    else
    {
        //Stop PWM generator
        OCMP1_Disable(); 
        OCMP2_Disable();
        TMR3_Stop();
    }
}

//Interrup Service routine for TMR4. Triggered every 125ms.
void BUZ_INT_Player()
{
    // If tone is set to be played
    if(ActiveToneIndex >= 0)
    {               
        //
        if(PlayedTones < Tones[(unsigned char)ActiveToneIndex].Length)
        {
            //Play tone while all notes are not played
            BUZ_SetTone(Tones[(unsigned char)ActiveToneIndex].Tones[PlayedTones]);
            PlayedTones++;
        }
        else
        {
            //Stop PWM generator
            OCMP1_Disable(); 
            OCMP2_Disable();
            TMR3_Stop();
            
            //Stop sound and reset counter           
            ActiveToneIndex = BUZ_OFF;
            PlayedTones = 0;
        }
    }
}