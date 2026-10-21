#include "Menu_Functions.h"

#ifndef _MENU_SETTING_H_
#define _MENU_SETTING_H_

bool Menu_Setting_ValueController(struct Menu *Item);
void Menu_Setting_GraphicPresentation(struct Menu *Item);
void Menu_SettingLock_GraphicPresentation();
void Menu_Setting_ClearFirstEntry();
void Menu_Setting_SetFirstEntry();
bool Menu_Setting_Logic(struct Menu *Item);
void Menu_Setting_Message(unsigned char *Title, unsigned char *Desctiprion);

#endif