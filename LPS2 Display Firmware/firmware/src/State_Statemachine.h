#include "definitions.h"

#ifndef _STATE_STATEMACHINE_H    /* Guard against multiple inclusion */
#define _STATE_STATEMACHINE_H

typedef struct PowerOptionValues {
    bool PowerViewActive;
    unsigned char DCOut;
    bool DCOutWakeup;
    unsigned char ACOut;
    bool ACOutWakeup;
    unsigned char DCIn;
    unsigned char ACIn;
    unsigned char MarkerLocation;
    unsigned char Solar;
} PowerOptionValue;

extern PowerOptionValue PowerValues;

//Functions
void RequestInitialValues();
void State_UpdateLED();
void State_PeriodicalRequest();
void State_SendBLEData();
void State_UpdateLogic();
void State_UpdateGraphics();

void System_EnterSleep(void);


#endif
