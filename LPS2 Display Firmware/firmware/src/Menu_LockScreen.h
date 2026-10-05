extern bool SettingsUnlocked;



#ifndef LOCKSCREEN_H
    #define LOCKSCREEEN_H   

typedef struct LockMenu {
    unsigned char SelectedCharacter;
    unsigned char Characters[4];
} LockMenu;

bool Menu_SettingLock_Logic(bool Change);
void ClearLockValue();

#endif