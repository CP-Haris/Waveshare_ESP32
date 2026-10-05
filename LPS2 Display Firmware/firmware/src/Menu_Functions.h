#include "Values.h"

#ifndef _MENU_FUNCTIONS_H_
#define _MENU_FUNCTIONS_H_

#define MARKER_RESET 0
#define MARKER_UP 1
#define MARKER_DOWN 2
#define MARKER_NONE 4


//Private
void Menu_ShowPath();



 //Public   
void Menu_InitMenu();
Menu* Menu_GetCurrentMenu(void);
bool Menu_SetNextMenu(Menu *NextMenu);
bool Menu_SetPreviousMenu();
void Menu_Reset();
void Menu_UpdateGraphics(void);
void Menu_Remove_ItemOrMenu(struct Menu *ST_Menu);
void Menu_Add_Relation (struct Menu *Parrent, struct Menu *Child);
void Menu_MoveMarker(unsigned char MarkerDirection);
void Menu_ShowNumbricVal(Menu *ActiveMenu, unsigned char U8_YLocation, unsigned char U8_XLocation, char Adjust, const unsigned char *Font);
Menu* Menu_GetSelectedChild(struct Menu *ST_Menu);
bool Menu_ShowMenu(Menu* SelectedMenu);
bool Menu_UpdateMarker(struct Menu *ST_Menu, unsigned char* NumberOfVisibleItems, unsigned char* MarkerPosition, unsigned char* Scroll);

#endif