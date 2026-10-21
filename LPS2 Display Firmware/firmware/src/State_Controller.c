#include "State_Controller.h"
#include "Values.h"
#include "Timer.h"
#include "UART_BIOS.h"


volatile StateController GlobalState;
void GlobaState_SendCommunicationTimer(unsigned char Uart)
{
    //Send State Info to LPS using UART 
    Block_200_ID_10.NumValue = Timer_CLK100ms_Communication.Value*6554;
    Block_200_ID_10.Flag.NumValue_Received = true;
    Values_Send_ByValue(0x52, &Block_200_ID_10, Uart);
}
void GlobaState_SendActiveState(unsigned char Uart)
{
    //Send State Info to LPS using UART 
    Block_200_ID_11.NumValue = (long)(GlobalState.State[GlobalState.StateIndex]);
    Block_200_ID_11.Flag.NumValue_Received = true;
    Values_Send_ByValue(0x52, &Block_200_ID_11, Uart);
}
void GlobaState_Initial(long InitialState)
{
    GlobalState.Update_Logic = true;
    GlobalState.CallerState = InitialState;
    GlobalState.StateIndex = 0;
    GlobalState.State[0] = InitialState;
}
void GlobaState_Next(long NextState)
{
    //State is an object containing a array of states. To control the state "StateIndex" is used. 
    GlobalState.CallerState = GlobalState.State[GlobalState.StateIndex];
    GlobalState.StateIndex++;
    GlobalState.State[GlobalState.StateIndex]=NextState;
    GlobalState.Update_Logic = true;
    GlobalState.Update_Graphic = true;
    GlobalState.Update_Request = true;
}
void GlobaState_Previous()
{
    if(GlobalState.StateIndex > 0)
    {
        GlobalState.CallerState = GlobalState.State[GlobalState.StateIndex];
        GlobalState.StateIndex--;
        GlobalState.Update_Logic = true;
        GlobalState.Update_Graphic = true;
        GlobalState.Update_Request = true;
    }
}
long GlobalState_GetActual()
{
    return GlobalState.State[GlobalState.StateIndex];
}

long GlobalState_GetCaller()
{
    return GlobalState.CallerState;
}