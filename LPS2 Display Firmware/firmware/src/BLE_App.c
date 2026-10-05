

#include "BLE_App.h"
#include "Values.h"
#include "UART_App.h"

#ifdef BLUETOOTH_CODE

#define BLE_CMD_X70     0x70
#define X70_DELETE_ALL_DEVICES  0x01
#define X70_DELETE_DEVICES      0x02
#define X70_SET_ADVERTISMENT    0x03
#define X70_GET_ALL_BONDED_DEVICES 0x04

void BLE_SetPowerSetting(bool Enable)
{
    if(Enable)
    {
        SET_BIT(Block_3_ID_3.NumValue, 20);
        BLE_ON_Clear(); // ON
        UART2_Init(115200, true);
    }
    else
    {
        CLEAR_BIT(Block_3_ID_3.NumValue, 20);
        BLE_ON_Set();
        UART2_Disable();
    }
    Values_Send_ByValue(0x50, &Block_3_ID_3, UART_CTRL);
}

void BLE_UpdatePowerSetting()
{
    if(CHECK_BIT(Block_3_ID_3.NumValue, 20)) {
        BLE_ON_Clear();
        UART2_Init(115200, true);
    }
    else{
        BLE_ON_Set();
        UART2_Disable();
    }  
}

void BLE_RemoveBoundedDevice(unsigned char DevNumber)
{
    unsigned char Data[3] = {BLE_CMD_X70, X70_DELETE_DEVICES, DevNumber};
    SendMSG(&Data[0], sizeof(Data), UART_BLE);
}

void BLE_RemoveAllBoundedDevices()
{
    unsigned char Data[2] = {BLE_CMD_X70, X70_DELETE_ALL_DEVICES};
    SendMSG(&Data[0], sizeof(Data), UART_BLE);
}


void BLE_RequestBoundedDevices()
{
    unsigned char Data[3] = {BLE_CMD_X70, X70_GET_ALL_BONDED_DEVICES};
    SendMSG(&Data[0], sizeof(Data), UART_BLE);
}
#endif