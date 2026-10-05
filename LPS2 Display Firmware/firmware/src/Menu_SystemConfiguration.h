#ifndef _MENU_SYSTEMCONFIGURATION_H_
#define _MENU_SYSTEMCONFIGURATION_H_

#include <stdbool.h> 
#include "Values.h"

typedef struct __attribute__((packed)) Setting {
    Value   *Value;
    long    SetValue;
} Setting;

void Config_RunSetting(Value *SelectedSetting);
void Config_UpdateCurrentSetting(Value *SelectedSetting);

#endif