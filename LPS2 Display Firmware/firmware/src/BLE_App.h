#include <stdbool.h>                    // Defines true

#ifndef _BLE_APP_H  
#define _BLE_APP_H

#define BLE_STATUS_NOT_CONNECTED 5
#define BLE_STATUS_CONNECTED_DEV1 0
#define BLE_STATUS_CONNECTED_DEV2 1
#define BLE_STATUS_CONNECTED_DEV3 2
#define BLE_STATUS_CONNECTED_DEV4 3
#define BLE_STATUS_CONNECTED_DEV5 4

void BLE_SetPowerSetting(bool Enable);
void BLE_UpdatePowerSetting();
void BLE_RemoveBoundedDevice(unsigned char DevNumber);
void BLE_RemoveAllBoundedDevices();

//How to get feedback if advertisment is active?
void BLE_EnableAdvertising(bool Enable);

#endif