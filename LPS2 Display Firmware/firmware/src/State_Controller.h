#include <stdbool.h>

#ifndef _STATE_CONTROLLER_H
#define _STATE_CONTROLLER_H

//Main Statemashine

#define MAINSTATE_DEBUG                 0x00100
#define MAINSTATE_INIT                  0x00010
#define MAINSTATE_POWERDOWN             0x00200
#define MAINSTATE_OFF                   0x01000
#define MAINSTATE_SPLASH                0x11000
#define MAINSTATE_MAINVIEW              0x30000
#define MAINSTATE_ERROR                 0x50000
#define MAINSTATE_MENU                  0x60000
#define MAINSTATE_SETTING               0x70000
#define MAINSTATE_SETTING_LOCK          0x71000
#define MAINSTATE_PASSKEY               0x72000


typedef struct StateController {
    long State[10];
    long CallerState;
    unsigned char StateIndex;
    bool Update_Logic;
    bool Update_Request;
    bool Update_Graphic;
    //bool FirstEntry;
    unsigned char InitTryCounter;
}StateController;

extern volatile StateController GlobalState;

void GlobaState_SendCommunicationTimer(unsigned char Uart);
void GlobaState_SendActiveState(unsigned char Uart);
void GlobaState_Initial(long InitialState);
void GlobaState_Next(long NextState);
void GlobaState_Previous();
long GlobalState_GetActual();
long GlobalState_GetCaller();
bool State_IsFirstEntry();

#endif