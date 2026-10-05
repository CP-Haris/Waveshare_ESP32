#include "Values.h"
#include "Menu_Application.h"
#include "Menu_Settings.h"
#include "Menu_SystemConfiguration.h"
#include "Menu_Callbacks.h"
#include "Values_Functions.h"
#include "Settings.h"
#include "ERC240128FS.h"
#include "Menu_Error.h"
#include "Text.h"
#include "BLE_App.h"
#include "Functions.h"

#define SHIFT_LVL1 14
//Menues whith
extern Menu* References_MAIN[];
extern Menu* References_GENERAL[];
extern Menu* References_IOVOLT[];
extern Menu* References_ACOUT[];
extern Menu* References_ACIN[];
extern Menu* References_DCOUT[];
extern Menu* References_DCIN[];
extern Menu* References_DCIN_StarterBattery[];
extern Menu* References_SOLAR[];
extern Menu* References_SOUND[];
extern Menu* References_DISPLAY[];
#ifdef BLUETOOTH_CODE
extern Menu* References_BLE[];
extern Menu* References_BLE_Attached[];
#endif
extern Menu* References_STATUS[];
extern Menu* References_TEMP[];
extern Menu* References_ABOUT[];
extern Menu* References_STORAGE[];
extern Menu* References_BOOTLOADERS[];
extern Menu* References_NRGMETER[];
extern Menu* References_ERROR[];
extern Menu* References_TESTVALS[];
//extern Menu* References_CONFIGS[];
//extern Menu* References_EXTENSION[];




//General Matches
const struct Menu_Match Match_On_Off[] = {
    {.MatchVal = 0 << 16, .MatchString = ENG_OS_Off},
    {NULL}
};

// Main Menu
Menu Menu_MAIN      = {.Show = true, .S8A_Title = ENG_MainMenu, .ST_Children = References_MAIN};
Menu Menu_GENERAL   = {.Show = true, .S8A_Title = ENG_General, .ST_Children = References_GENERAL};


//IO voltage Menu
Menu Menu_IOVOLT    = {.Show = true, .S8A_Title = ENG_IOVolt, .ST_Children = References_IOVOLT};

const Menu_Value Presentation_IOVOLT_Remote = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_70};
Menu Menu_IOVOLT_Remote = {.Show = true, .S8A_Title = ENG_Remote, .ST_Value = &Presentation_IOVOLT_Remote};

const Menu_Value Presentation_IOVOLT_Data = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_71};
Menu Menu_IOVOLT_Data = {.Show = true, .S8A_Title = ENG_Data, .ST_Value = &Presentation_IOVOLT_Data};

const Menu_Value Presentation_IOVOLT_DataFront = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_72};
Menu Menu_IOVOLT_DataFront = {.Show = true, .S8A_Title = ENG_DataFront, .ST_Value = &Presentation_IOVOLT_DataFront};

const Menu_Value Presentation_IOVOLT_C2Terminal = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_73};
Menu Menu_IOVOLT_C2Terminal = {.Show = true, .S8A_Title = ENG_C2Terminal, .ST_Value = &Presentation_IOVOLT_C2Terminal};

const Menu_Value Presentation_IOVOLT_C1Terminal = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_74};
Menu Menu_IOVOLT_C1Terminal = {.Show = true, .S8A_Title = ENG_C1Terminal, .ST_Value = &Presentation_IOVOLT_C1Terminal};


const struct Menu_Match Match_IOVOLT_C1[] = {
    {.MatchVal = 1 << 18, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_IOVOLT_DCOutIfC1 = {.Description = ENG_DCOutC1, .CodeProtect = false};
const Menu_Value Presentation_IOVOLT_DCOutIfC1 = {.Show = true, .NumbToString = Match_IOVOLT_C1, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_6_ID_6};
Menu Menu_IOVOLT_DCOutIfC1 = {.Show = true, .S8A_Title = ENG_DCOutC1, .ST_Popup = &Setting_IOVOLT_DCOutIfC1, .ST_Value = &Presentation_IOVOLT_DCOutIfC1};

const Menu_Popup Setting_IOVOLT_ACOutIfC1 = {.Description = ENG_ACOutC1, .CodeProtect = false};
const Menu_Value Presentation_IOVOLT_ACOutIfC1 = {.Show = true, .NumbToString = Match_IOVOLT_C1, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_6_ID_2};
Menu Menu_IOVOLT_ACOutIfC1 = {.Show = true, .S8A_Title = ENG_ACOutC1, .ST_Popup = &Setting_IOVOLT_ACOutIfC1, .ST_Value = &Presentation_IOVOLT_ACOutIfC1};

const struct Menu_Match Match_IOVOLT_C1CustomLvl[] = {
    {.MatchVal = 1 << 16, .MatchString = ENG_IO_Custom},
    {NULL}
};
const Menu_Popup Setting_IOVOLT_C1CustomLvl = {.Function_Change = CLB_Change_IO_C1CustomLvl, .StepFactor_Fast = 1 * 65536, .StepFactor_Slow = 1 * 65536, .CodeProtect = false};
const Menu_Value Presentation_IOVOLT_C1CustomLvl = {.Show = true, .NumbToString = Match_IOVOLT_C1CustomLvl, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_200_ID_13};
Menu Menu_IOVOLT_C1CustomLvl = {.Show = true, .S8A_Title = ENG_IO_C1CustomLvl, .ST_Popup = &Setting_IOVOLT_C1CustomLvl, .ST_Value = &Presentation_IOVOLT_C1CustomLvl};

const Menu_Popup Setting_IOVOLT_C1CustomActive = {.Function_Change = CLB_Change_IO_C1StartVolt, .Description = ENG_DESC_DCIn_StartVoltage, .StepFactor_Fast = 1 * 6553.6, .StepFactor_Slow = 1 * 6553.6, .CodeProtect = false};
const Menu_Value Presentation_IOVOLT_C1CustomActive = {.Show = true, .Data = &Block_4_ID_0, .Decimals = 1};
Menu Menu_IOVOLT_C1CustomActive = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_DCIn_StartVoltage, .ST_Popup = &Setting_IOVOLT_C1CustomActive, .ST_Value = &Presentation_IOVOLT_C1CustomActive};

const Menu_Popup Setting_IOVOLT_C1CustomDeactive = {.Function_Change = CLB_Change_IO_C1StopVolt, .Description = ENG_DESC_DCIn_StopVoltage, .StepFactor_Fast = 1 * 6553.6, .StepFactor_Slow = 1 * 6553.6, .CodeProtect = false};
const Menu_Value Presentation_IOVOLT_C1CustomDeactive = {.Show = true, .Data = &Block_4_ID_3, .Decimals = 1};
Menu Menu_IOVOLT_C1CustomDeactive = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_DCIn_StopVoltage, .ST_Popup = &Setting_IOVOLT_C1CustomDeactive, .ST_Value = &Presentation_IOVOLT_C1CustomDeactive};

//Energy meter Menu
Menu Menu_NRGMETER  = {.Show = true, .S8A_Title = ENG_EnergyMeter, .ST_Children = References_NRGMETER};

const Menu_Value Presentation_NRGMETER_ACIn = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_140};
Menu Menu_NRGMETER_ACIn = {.Show = true, .S8A_Title = ENG_ACIn_NRG, .ST_Value = &Presentation_NRGMETER_ACIn};

const Menu_Value Presentation_NRGMETER_DCIn = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_142};
Menu Menu_NRGMETER_DCIn = {.Show = true, .S8A_Title = ENG_DCIn, .ST_Value = &Presentation_NRGMETER_DCIn};

const Menu_Value Presentation_NRGMETER_DCOut = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_144};
Menu Menu_NRGMETER_DCOut = {.Show = true, .S8A_Title = ENG_DCOut_CHG, .ST_Value = &Presentation_NRGMETER_DCOut};

const Menu_Value Presentation_NRGMETER_Solar = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_146};
Menu Menu_NRGMETER_Solar = {.Show = true, .S8A_Title = ENG_Solar, .ST_Value = &Presentation_NRGMETER_Solar};


//AC Out Menu
Menu Menu_ACOUT     = {.Show = true, .S8A_Title = ENG_ACOut, .ST_Children = References_ACOUT};

const Menu_Value Presentation_ACOUT_OperationStatus = {.Show = true, .Data = &Block_0_ID_201};
Menu Menu_ACOUT_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_ACOUT_OperationStatus};

const Menu_Value Presentation_ACOUT_Power = {.Show = true, .Data = &Block_0_ID_107};
Menu Menu_ACOUT_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_ACOUT_Power};

const Menu_Value Presentation_ACOUT_Voltage = {.Decimals = 1, .Show = true, .Data = &Block_0_ID_105};
Menu Menu_ACOUT_Voltage = {.Show = true, .S8A_Title = ENG_Voltage, .ST_Value = &Presentation_ACOUT_Voltage};

const Menu_Value Presentation_ACOUT_Current = {.Decimals = 1, .Show = true, .Data = &Block_0_ID_106};
Menu Menu_ACOUT_Current = {.Show = true, .S8A_Title = ENG_Current, .ST_Value = &Presentation_ACOUT_Current};

const Menu_Popup Setting_ACOUT_Auto_PowerDownDelay = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.0166666 * 65536, .Description = ENG_DESC_ACOutTime, .CodeProtect = false};
const Menu_Value Presentation_ACOUT_Auto_PowerDownDelay = {.Show = true, .Data = &Block_50_ID_1};
Menu Menu_ACOUT_Auto_PowerDownDelay = {.Show = true, .S8A_Title = ENG_Auto_PowerDownDelay, .ST_Popup = &Setting_ACOUT_Auto_PowerDownDelay, .ST_Value = &Presentation_ACOUT_Auto_PowerDownDelay};

const Menu_Popup Setting_ACOUT_Auto_PowerDownLoad = {.StepFactor_Fast = 10 * 65536, .StepFactor_Slow = 1 * 65536, .Description = ENG_DESC_ACOutWatt, .CodeProtect = false};
const Menu_Value Presentation_ACOUT_Auto_PowerDownLoad = {.Show = true, .Data = &Block_50_ID_2};
Menu Menu_ACOUT_Auto_PowerDownLoad = {.Show = true, .S8A_Title = ENG_Auto_PowerDownLoad, .ST_Popup = &Setting_ACOUT_Auto_PowerDownLoad, .ST_Value = &Presentation_ACOUT_Auto_PowerDownLoad};

const Menu_Popup Setting_ACOUT_Inveter_Cuttoff = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_ACOut_Cuttoff, .CodeProtect = false};
const Menu_Value Presentation_ACOUT_Inverter_Cuttoff = {.Show = true, .Data = &Block_50_ID_0};
Menu Menu_ACOUT_Inverter_Cutoff = {.Show = true, .S8A_Title = ENG_ACOut_Cutoff, .ST_Popup = &Setting_ACOUT_Inveter_Cuttoff, .ST_Value = &Presentation_ACOUT_Inverter_Cuttoff};


//DC Out Menu
Menu Menu_DCOUT = {.Show = true, .S8A_Title = ENG_DCOut, .ST_Children = References_DCOUT};

const Menu_Value Presentation_DCOUT_OperationStatus = {.Show = true, .Data = &Block_0_ID_203};
Menu Menu_DCOUT_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_DCOUT_OperationStatus};

const Menu_Value Presentation_DCOUT_Power = {.Show = true, .Data = &Block_0_ID_113};
Menu Menu_DCOUT_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_DCOUT_Power};

const Menu_Value Presentation_DCOUT_Voltage = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_111};
Menu Menu_DCOUT_Voltage = {.Show = true, .S8A_Title = ENG_Voltage, .ST_Value = &Presentation_DCOUT_Voltage};

const Menu_Value Presentation_DCOUT_Current = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_112};
Menu Menu_DCOUT_Current = {.Show = true, .S8A_Title = ENG_Current, .ST_Value = &Presentation_DCOUT_Current};

const Menu_Popup Setting_DCOUT_ShutdownDelay = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.0166666 * 65536, .Description = ENG_DESC_DCOutTime, .CodeProtect = true};
const Menu_Value Presentation_DCOUT_ShutdownDelay = {.Show = true, .NumbToString = Match_On_Off, .Data = &Block_40_ID_0};
Menu Menu_DCOUT_ShutdownDelay = {.Show = true, .S8A_Title = ENG_ShutdownDelay, .ST_Popup = &Setting_DCOUT_ShutdownDelay, .ST_Value = &Presentation_DCOUT_ShutdownDelay};

//Added 28-02-2024
const Menu_Popup Setting_DCOUT_SaverTime = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.0166666 * 65536, .Description = ENG_DESC_ACOutTime, .CodeProtect = true};
const Menu_Value Presentation_DCOUT_SaverTime = {.Show = true, .Data = &Block_40_ID_1, .NumbToString = Match_On_Off};
Menu Menu_DCOUT_SaverTime = {.Show = true, .S8A_Title = ENG_Auto_PowerDownDelay, .ST_Value = &Presentation_DCOUT_SaverTime, .ST_Popup = &Setting_DCOUT_SaverTime};

//Added 28-02-2024
const Menu_Popup Setting_DCOUT_SaverAmp = {.StepFactor_Fast = 10 * 65536, .StepFactor_Slow = 1 * 65536, .Description = ENG_DESC_ACOutWatt, .CodeProtect = false};
const Menu_Value Presentation_DCOUT_SaverAmp = {.Show = true, .Data = &Block_40_ID_2};
Menu Menu_DCOUT_SaverAmp = {.Show = true, .S8A_Title = ENG_Auto_PowerDownLoad, .ST_Value = &Presentation_DCOUT_SaverAmp, .ST_Popup = &Setting_DCOUT_SaverAmp};

//AC IN Menu
Menu Menu_ACIN = {.Show = true, .S8A_Title = ENG_ACIn, .ST_Children = References_ACIN};

const Menu_Value Presentation_ACIN_OperationStatus = {.Show = true, .Data = &Block_0_ID_200};
Menu Menu_ACIN_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_ACIN_OperationStatus};

const Menu_Value Presentation_ACIN_Power = {.Show = true, .Data = &Block_0_ID_104};
Menu Menu_ACIN_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_ACIN_Power};

const Menu_Value Presentation_ACIN_Voltage = {.Decimals = 1, .Show = true, .Data = &Block_0_ID_102};
Menu Menu_ACIN_Voltage = {.Show = true, .S8A_Title = ENG_Voltage, .ST_Value = &Presentation_ACIN_Voltage};

const Menu_Value Presentation_ACIN_Current = {.Decimals = 1, .Show = true, .Data = &Block_0_ID_103};
Menu Menu_ACIN_Current = {.Show = true, .S8A_Title = ENG_Current, .ST_Value = &Presentation_ACIN_Current};

const Menu_Popup Setting_ACIN_MaxCurrent = {.StepFactor_Fast = 1 * 65536, .StepFactor_Slow = 1 * 65536, .Description = ENG_DESC_ACInMaxCurrent, .CodeProtect = false};
const Menu_Value Presentation_ACIN_MaxCurrent = {.Show = true, .Data = &Block_60_ID_2};
Menu Menu_ACIN_MaxCurrent = {.Show = true, .S8A_Title = ENG_MaxCurrent, .ST_Popup = &Setting_ACIN_MaxCurrent, .ST_Value = &Presentation_ACIN_MaxCurrent};


 //DC IN Menu
Menu Menu_DCIN = {.Show = true, .S8A_Title = ENG_DCIn, .ST_Children = References_DCIN};

const Menu_Value Presentation_DCIN_OperationStatus = {.Show = true, .Data = &Block_0_ID_202};
Menu Menu_DCIN_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_DCIN_OperationStatus};

const Menu_Value Presentation_DCIN_Power = {.Show = true, .Data = &Block_0_ID_110};
Menu Menu_DCIN_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_DCIN_Power};

const Menu_Value Presentation_DCIN_Voltage = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_108};
Menu Menu_DCIN_Voltage = {.Show = true, .S8A_Title = ENG_Voltage, .ST_Value = &Presentation_DCIN_Voltage};

const Menu_Value Presentation_DCIN_Current = {.Decimals = 2, .Show = MENU_SHOW, .Data = &Block_0_ID_109};
Menu Menu_DCIN_Current = {.Show = true, .S8A_Title = ENG_Current, .ST_Value = &Presentation_DCIN_Current};

const struct Menu_Match Match_DCIN_Jumpstart[] = {
    {.MatchVal = 0 << 16, .MatchString = ENG_OS_Off},
    {.MatchVal = 0.0833333 * 65536, .MatchString = ENG_Active},
    {NULL}
};
const Menu_Popup Setting_DCIN_Jumpstart = {.StepFactor_Fast = 0.0833333 * 65536, .StepFactor_Slow = 0.0833333 * 65536, .Description = ENG_DESC_Jumpstart, .CodeProtect = false};
const Menu_Value Presentation_DCIN_Jumpstart = {.NumbToString = Match_DCIN_Jumpstart, .Show = MENU_SHOW, .Data = &Block_1_ID_1};
Menu Menu_DCIN_Jumpstart = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_Jumpstart, .ST_Popup = &Setting_DCIN_Jumpstart, .ST_Value = &Presentation_DCIN_Jumpstart};

const struct Menu_Match MatchBit_DCIN_JumpAndStarter[] = {
    {.MatchVal = 1 << 20, .MatchString = ENG_Disabled},
    {NULL}
};
const Menu_Popup Setting_DCIN_Jumpstart_GlobalDisable = {.Description = ENG_DESC_JumpstartFunc, .CodeProtect = true};
const Menu_Value Presentation_DCIN_Jumpstart_GlobalDisable = {.Show = true, .NumbToString = MatchBit_DCIN_JumpAndStarter, .Data = &Block_30_ID_0, .NumbToString_NoMatch = ENG_Enabled};
Menu Menu_DCIN_Jumpstart_GlobalDisable = {.Show = true, .S8A_Title = ENG_JumpstartFunc, .ST_Popup = &Setting_DCIN_Jumpstart_GlobalDisable, .ST_Value = &Presentation_DCIN_Jumpstart_GlobalDisable};


const struct Menu_Match Match_DCIN_OperatingVoltage[] = {
    {.MatchVal = 1 << 16, .MatchString = ENG_12V},
    {.MatchVal = 1 << 17, .MatchString = ENG_24V},
    {NULL}
};
const Menu_Value Presentation_DCIN_OperatingVoltage = {.Show = true, .NumbToString = Match_DCIN_OperatingVoltage, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_0_ID_170};
Menu Menu_DCIN_OperatingVoltage = {.Show = true, .S8A_Title = ENG_DCINSelectedVolt, .ST_Value = &Presentation_DCIN_OperatingVoltage};

const struct Menu_Match Match_DCIN_SetOperatingVoltage[] = {
    {.MatchVal = 0 << 16, .MatchString = ENG_Auto},
    {.MatchVal = 1 << 16, .MatchString = ENG_12V},
    {.MatchVal = 2 << 16, .MatchString = ENG_24V},
    {NULL}
};
//Value Missing in Values (&Block_30_Indexer[X] for voltage setting)
const Menu_Popup Setting_DCIN_SetOperatingVoltage = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_DESC_DCIN_SetInputVoltage, .CodeProtect = true};
const Menu_Value Presentation_DCIN_SetOperatingVoltage = {.Show = true, .NumbToString = Match_DCIN_SetOperatingVoltage , .Data = &Block_30_ID_1};
Menu Menu_DCIN_SetOperatingVoltage = {.Show = true, .S8A_Title = ENG_SetVolt, .ST_Popup = &Setting_DCIN_SetOperatingVoltage, .ST_Value = &Presentation_DCIN_SetOperatingVoltage};

const Menu_Popup Setting_DCIN_SetChargeCurrent = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_DESC_DCIN_SetInputCurrent, .CodeProtect = true};
const Menu_Value Presentation_DCIN_SetChargeCurrent = {.Show = true, .Data = &Block_30_ID_7};
Menu Menu_DCIN_SetChargeCurrent = {.Show = true, .S8A_Title = ENG_DCINSetCurr, .ST_Popup = &Setting_DCIN_SetChargeCurrent, .ST_Value = &Presentation_DCIN_SetChargeCurrent};

//Added 28-02-2024
const Menu_Popup Setting_DCIN_StartVolt = {.Function_Change = CLB_Change_DCIN_StartVolt, .StepFactor_Fast = 1 << 16, .StepFactor_Slow = 6553, .Description = ENG_DESC_DCIn_StartVoltage, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StartVolt12V = {.Show = true, .Decimals = 2, .Data = &Block_30_ID_12};
Menu Menu_DCIN_StartVolt12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_DCIn_StartVoltage, .ST_Value = &Presentation_DCIN_StartVolt12V, .ST_Popup = &Setting_DCIN_StartVolt};


//Added 28-02-2024
const Menu_Popup Setting_DCIN_StopVolt = {.Function_Change = CLB_Change_DCIN_StopVolt, .StepFactor_Fast = 1 << 16, .StepFactor_Slow = 6553, .Description = ENG_DESC_DCIn_StartVoltage, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StopVolt12V = {.Show = true, .Decimals = 2, .Data = &Block_30_ID_13};
Menu Menu_DCIN_StopVolt12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_DCIn_StopVoltage, .ST_Value = &Presentation_DCIN_StopVolt12V, .ST_Popup = &Setting_DCIN_StopVolt};



 //DC IN Menu - Starter Battery
Menu Menu_DCIN_StarterBattery = {.Show = true, .S8A_Title = ENG_ChargeOfStarterBat, .ST_Children = References_DCIN_StarterBattery};

const struct Menu_Match MatchBit_DCIN_StartBatEnable[] = {
    {.MatchVal = 1 << 16, .MatchString = ENG_Enabled},
    {NULL}
};
const Menu_Popup Setting_DCIN_StartBatEnable = {.Description = ENG_ChargeOfStarterBat, .CodeProtect = false};
const Menu_Value Presentation_DCIN_StartBatEnable = {.Show = true, .NumbToString = MatchBit_DCIN_StartBatEnable, .NumbToString_NoMatch = ENG_Disabled, .Data = &Block_31_ID_0};
Menu Menu_DCIN_StartBatEnable = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Popup = &Setting_DCIN_StartBatEnable, .ST_Value = &Presentation_DCIN_StartBatEnable};

const Menu_Popup Setting_DCIN_StartBatChgCurrent = {.StepFactor_Fast = 5 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_ChargeCurrent, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StartBatChgCurrent12V = {.Show = true, .Data = &Block_31_ID_1};
Menu Menu_DCIN_StartBatChgCurrent12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_ChargeCurrent, .ST_Popup = &Setting_DCIN_StartBatChgCurrent, .ST_Value = &Presentation_DCIN_StartBatChgCurrent12V};
const Menu_Value Presentation_DCIN_StartBatChgCurrent24V = {.Show = true, .Data = &Block_31_ID_5};
Menu Menu_DCIN_StartBatChgCurrent24V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_ChargeCurrent, .ST_Popup = &Setting_DCIN_StartBatChgCurrent, .ST_Value = &Presentation_DCIN_StartBatChgCurrent24V};

const Menu_Popup Setting_DCIN_StartBatChgVoltage = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 6553, .Description = ENG_ChargeVoltage, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StartBatChgVoltage12V = {.Show = true, .Decimals = 2, .Data = &Block_31_ID_2};
Menu Menu_DCIN_StartBatChgVoltage12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_ChargeVoltage, .ST_Value = &Presentation_DCIN_StartBatChgVoltage12V, .ST_Popup = &Setting_DCIN_StartBatChgVoltage};
const Menu_Value Presentation_DCIN_StartBatChgVoltage24V = {.Show = true, .Decimals = 2, .Data = &Block_31_ID_6};
Menu Menu_DCIN_StartBatChgVoltage24V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_ChargeVoltage, .ST_Value = &Presentation_DCIN_StartBatChgVoltage24V, .ST_Popup = &Setting_DCIN_StartBatChgVoltage};

const Menu_Popup Setting_DCIN_StartBatCutOffCurrent = {.StepFactor_Fast = 5 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_CutOffCurrent, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StartBatCutOffCurrent12V = {.Show = true, .Data = &Block_31_ID_3};
Menu Menu_DCIN_StartBatCutOffCurrent12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_CutOffCurrent, .ST_Popup = &Setting_DCIN_StartBatCutOffCurrent, .ST_Value = &Presentation_DCIN_StartBatCutOffCurrent12V};
const Menu_Value Presentation_DCIN_StartBatCutOffCurrent24V = {.Show = true, .Data = &Block_31_ID_7};
Menu Menu_DCIN_StartBatCutOffCurrent24V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_CutOffCurrent, .ST_Popup = &Setting_DCIN_StartBatCutOffCurrent, .ST_Value = &Presentation_DCIN_StartBatCutOffCurrent24V};

const Menu_Popup Setting_DCIN_StartBatMaintenance = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 6553, .Description = ENG_MaintenanceVoltage, .CodeProtect = true};
const Menu_Value Presentation_DCIN_StartBatMaintenance12V = {.Show = true, .Decimals = 2, .Data = &Block_31_ID_4};
Menu Menu_DCIN_StartBatMaintenance12V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_MaintenanceVoltage, .ST_Value = &Presentation_DCIN_StartBatMaintenance12V, .ST_Popup = &Setting_DCIN_StartBatMaintenance};
const Menu_Value Presentation_DCIN_StartBatMaintenance24V = {.Show = true, .Decimals = 2, .Data = &Block_31_ID_8};
Menu Menu_DCIN_StartBatMaintenance24V = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_MaintenanceVoltage, .ST_Value = &Presentation_DCIN_StartBatMaintenance24V, .ST_Popup = &Setting_DCIN_StartBatMaintenance};

const Menu_Popup Setting_DCIN_CutOffTimer = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_Cutoff_Timer, .CodeProtect = true};
const Menu_Value Presentation_DCIN_CutOffTimer = {.Show = true, .Data = &Block_31_ID_10};
Menu Menu_DCIN_CutOffTimer = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_Cutoff_Timer, .ST_Value = &Presentation_DCIN_CutOffTimer, .ST_Popup = &Setting_DCIN_CutOffTimer};

//Solar Menu
Menu Menu_SOLAR = {.Show = true, .S8A_Title = ENG_Solar, .ST_Children = References_SOLAR};

const Menu_Value Presentation_SOLAR_OperationStatus = {.Show = true, .Data = &Block_0_ID_208};
Menu Menu_SOLAR_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_SOLAR_OperationStatus};

const Menu_Value Presentation_SOLAR_Power = {.Show = true, .Data = &Block_0_ID_79};
Menu Menu_SOLAR_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_SOLAR_Power};

const Menu_Value Presentation_SOLAR_Voltage = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_73};
Menu Menu_SOLAR_Voltage = {.Show = true, .S8A_Title = ENG_InputVoltage, .ST_Value = &Presentation_SOLAR_Voltage};

const Menu_Value Presentation_SOLAR_Current = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_78};
Menu Menu_SOLAR_Current = {.Show = true, .S8A_Title = ENG_OutputCurrent, .ST_Value = &Presentation_SOLAR_Current};


const struct Menu_Match Match_SOLAR_SetOperation[] = {
    {.MatchVal = 0 << 16, .MatchString = ENG_OS_Off},
    {.MatchVal = 1 << 16, .MatchString = ENG_Auto},
    {.MatchVal = 2 << 16, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_SOLAR_SetOperation = {.StepFactor_Fast = 1 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_DESC_SOLAR_Operation, .CodeProtect = true};
const Menu_Value Presentation_SOLAR_SetOperation = {.Show = true, .NumbToString = &Match_SOLAR_SetOperation[0], .Data = &Block_70_ID_0};
Menu Menu_SOLAR_SetOperation = {.Show = true, .S8A_Title = ENG_SOLARSetOperation, .ST_Popup = &Setting_SOLAR_SetOperation, .ST_Value = &Presentation_SOLAR_SetOperation};

const Menu_Value Presentation_SOLAR_SetOCVoltage = {.Show = true, .Decimals = 2, .Data = &Block_71_ID_0};
Menu Menu_SOLAR_SetOCVoltage = {.Show = true, .S8A_Title = ENG_SOLAR_OCVolt, .ST_Value = &Presentation_SOLAR_SetOCVoltage};

const Menu_Value Presentation_SOLAR_SetMPPVoltage = {.Show = true, .Decimals = 2, .Data = &Block_71_ID_1};
Menu Menu_SOLAR_SetMPPVoltage = {.Show = true, .S8A_Title = ENG_SOLAR_MPPVolt, .ST_Value = &Presentation_SOLAR_SetMPPVoltage};

const Menu_Value Presentation_SOLAR_SetStartVoltage = {.Show = true, .Decimals = 2, .Data = &Block_71_ID_2};
Menu Menu_SOLAR_SetStartVoltage = {.Show = true, .S8A_Title = ENG_SOLAR_StartVolt, .ST_Value = &Presentation_SOLAR_SetStartVoltage};


//Status Menu
Menu Menu_STATUS = {.Show = true, .S8A_Title = ENG_BatteryStatus, .ST_Children = References_STATUS};

const Menu_Value Presentation_STATUS_OperationStatus = {.Show = true, .Data = &Block_0_ID_210};
Menu Menu_STATUS_OperationStatus = {.Show = true, .S8A_Title = ENG_OperationStatus, .ST_Value = &Presentation_STATUS_OperationStatus};

const Menu_Value Presentation_STATUS_RemainingOperation = {.Show = true, .Data = &Block_0_ID_120};
Menu Menu_STATUS_RemainingOperation = {.Show = true, .S8A_Title = ENG_RemainingOperation, .ST_Value = &Presentation_STATUS_RemainingOperation};

const Menu_Value Presentation_STATUS_SOC = {.Show = true, .Data = &Block_0_ID_119};
Menu Menu_STATUS_SOC = {.Show = true, .S8A_Title = ENG_SOC, .ST_Value = &Presentation_STATUS_SOC};

const Menu_Value Presentation_STATUS_Power = {.Show = true, .Data = &Block_0_ID_127};
Menu Menu_STATUS_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Value = &Presentation_STATUS_Power};

const Menu_Value Presentation_STATUS_Voltage = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_100};
Menu Menu_STATUS_Voltage = {.Show = true, .S8A_Title = ENG_Voltage, .ST_Value = &Presentation_STATUS_Voltage};

const Menu_Value Presentation_STATUS_Current = {.Decimals = 2, .Show = true, .Data = &Block_0_ID_101};
Menu Menu_STATUS_Current = {.Show = true, .S8A_Title = ENG_Current, .ST_Value = &Presentation_STATUS_Current};

const Menu_Value Presentation_STATUS_Temperature = {.Decimals = 1, .Show = true, .Data = &Block_0_ID_114};
Menu Menu_STATUS_Temperature = {.Show = true, .S8A_Title = ENG_Temperature, .ST_Value = &Presentation_STATUS_Temperature};

const Menu_Value Presentation_STATUS_Cell1Vol = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_1};
Menu Menu_STATUS_Cell1Vol = {.Show = true, .S8A_Title = ENG_Cell1Vol, .ST_Value = &Presentation_STATUS_Cell1Vol};

const Menu_Value Presentation_STATUS_Cell2Vol = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_2};
Menu Menu_STATUS_Cell2Vol = {.Show = true, .S8A_Title = ENG_Cell2Vol, .ST_Value = &Presentation_STATUS_Cell2Vol};

const Menu_Value Presentation_STATUS_Cell3Vol = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_3};
Menu Menu_STATUS_Cell3Vol = {.Show = true, .S8A_Title = ENG_Cell3Vol, .ST_Value = &Presentation_STATUS_Cell3Vol};

const Menu_Value Presentation_STATUS_Cell4Vol = {.Decimals = 3, .Show = true, .Data = &Block_0_ID_4};
Menu Menu_STATUS_Cell4Vol = {.Show = true, .S8A_Title = ENG_Cell4Vol, .ST_Value = &Presentation_STATUS_Cell4Vol};

const Menu_Value Presentation_STATUS_Cycles = {.Show = true, .Data = &Block_0_ID_124};
Menu Menu_STATUS_Cycles = {.Show = true, .S8A_Title = ENG_Cycles, .ST_Value = &Presentation_STATUS_Cycles};


//Temperature Menu
Menu Menu_TEMP = {.Show = true, .S8A_Title = ENG_Temperature, .ST_Children = References_TEMP};

const Menu_Value Presentation_TEMP_Trafo = {.Show = true, .Decimals = 1, .Data = &Block_0_ID_117};
Menu Menu_TEMP_Trafo = {.Show = true, .S8A_Title = ENG_Trafo, .ST_Value = &Presentation_TEMP_Trafo};

const Menu_Value Presentation_TEMP_IGBT = {.Show = true, .Decimals = 1, .Data = &Block_0_ID_116};
Menu Menu_TEMP_IGBT = {.Show = true, .S8A_Title = ENG_IGBT, .ST_Value = &Presentation_TEMP_IGBT};

const Menu_Value Presentation_TEMP_TempCell12 = {.Show = true, .Decimals = 1, .Data = &Block_0_ID_121};
Menu Menu_TEMP_TempCell12 = {.Show = true, .S8A_Title = ENG_TempCell12, .ST_Value = &Presentation_TEMP_TempCell12};

const Menu_Value Presentation_TEMP_TempCell23 = {.Show = true, .Decimals = 1, .Data = &Block_0_ID_122};
Menu Menu_TEMP_TempCell23 = {.Show = true, .S8A_Title = ENG_TempCell23, .ST_Value = &Presentation_TEMP_TempCell23};

const Menu_Value Presentation_TEMP_TempCell34 = {.Show = true, .Decimals = 1, .Data = &Block_0_ID_123};
Menu Menu_TEMP_TempCell34 = {.Show = true, .S8A_Title = ENG_TempCell34, .ST_Value = &Presentation_TEMP_TempCell34};


//About Menu
Menu Menu_ABOUT = {.Show = true, .S8A_Title = ENG_About, .ST_Children = References_ABOUT};

const Menu_Value Presentation_ABOUT_Serial = {.Show = true, .Data = &Block_0_ID_220};
Menu Menu_ABOUT_Serial = {.Show = true, .S8A_Title = ENG_Serial, .ST_Value = &Presentation_ABOUT_Serial};

const Menu_Value Presentation_ABOUT_ManufactureDate = {.Show = true, .Data = &Block_0_ID_221};
Menu Menu_ABOUT_ManufactureDate = {.Show = true, .S8A_Title = ENG_ManufactureDate, .ST_Value = &Presentation_ABOUT_ManufactureDate};

const Menu_Value Presentation_ABOUT_HVersion = {.Show = true, .Data = &Block_0_ID_222};
Menu Menu_ABOUT_HVersion = {.Show = true, .S8A_Title = ENG_HVersion, .ST_Value = &Presentation_ABOUT_HVersion};

const Menu_Value Presentation_ABOUT_SVersionUnit = {.Show = true, .Data = &Block_0_ID_223};
Menu Menu_ABOUT_SVersionUnit = {.Show = true, .S8A_Title = ENG_SVersionUnit, .ST_Value = &Presentation_ABOUT_SVersionUnit};

const Menu_Value Presentation_ABOUT_SVersionDisp = {.Show = true, .Data = &Block_200_ID_0};
Menu Menu_ABOUT_SVersionDisp = {.Show = true, .S8A_Title = ENG_SVersionDisp, .ST_Value = &Presentation_ABOUT_SVersionDisp};

const Menu_Value Presentation_ABOUT_SVersionPwr = {.Show = true, .Data = &Block_250_ID_223};
Menu Menu_ABOUT_SVersionPwr = {.Show = true, .S8A_Title = ENG_SVersionPwr, .ST_Value = &Presentation_ABOUT_SVersionPwr};

const Menu_Value Presentation_ABOUT_SVersionDCDC = {.Show = true, .Data = &Block_251_ID_223};
Menu Menu_ABOUT_SVersionDCDC = {.Show = true, .S8A_Title = ENG_SVersionDCDC, .ST_Value = &Presentation_ABOUT_SVersionDCDC};


//Storage Menu
Menu Menu_STORAGE = {.Show = true, .S8A_Title = ENG_Storage, .ST_Children = References_STORAGE};

// \TODO: Needs Block ID for storage mode delay
const Menu_Popup Setting_STORAGE_DelayTime = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.0166666 * 65536, .Description = ENG_DESC_StorageTime, .CodeProtect = true};
const Menu_Value Presentation_STORAGE_Delay = {.Show = true, .Data = &Block_40_ID_1, .NumbToString = Match_On_Off};
Menu Menu_STORAGE_Delay = {.Show = true, .S8A_Title = ENG_StorageDelay, .ST_Popup = &Setting_STORAGE_DelayTime, .ST_Value = &Presentation_STORAGE_Delay};

//const Menu_Value Presentation_STORAGE_Enter = {.Show = true}; // \TODO: Must be linked to some message or function or something
//Menu Menu_STORAGE_Enter = {.Show = true, .S8A_Title = ENG_StorageEnter, .ST_Value = &Presentation_STORAGE_Enter};

//Bootloaders Menu
Menu Menu_ABOUT_BOOTLOADERS = {.Show = true, .S8A_Title = ENG_Bootloaders, .ST_Children = References_BOOTLOADERS};

const Menu_Value Presentation_BOOTLOADER_VersionUnit = {.Show = true, .Data = &Block_0_ID_230};
Menu Menu_BOOTLOADER_VersionUnit = {.Show = true, .S8A_Title = ENG_BVersionUnit, .ST_Value = &Presentation_BOOTLOADER_VersionUnit};

const Menu_Value Presentation_BOOTLOADER_VersionPwr = {.Show = true, .Data = &Block_250_ID_230};
Menu Menu_BOOTLOADER_VersionPwr = {.Show = true, .S8A_Title = ENG_BVersionPwr, .ST_Value = &Presentation_BOOTLOADER_VersionPwr};

const Menu_Value Presentation_BOOTLOADER_VersionDcDc = {.Show = true, .Data = &Block_251_ID_230};
Menu Menu_BOOTLOADER_VersionDcDc = {.Show = true, .S8A_Title = ENG_BVersionDcDc, .ST_Value = &Presentation_BOOTLOADER_VersionDcDc};

const Menu_Value Presentation_BOOTLOADER_VersionDisp = {.Show = true, .Data = &Block_252_ID_230, .Decimals = 10};
Menu Menu_BOOTLOADER_VersionDisp = {.Show = true, .S8A_Title = ENG_BVersionDisp, .ST_Value = &Presentation_BOOTLOADER_VersionDisp};


//Display menu
Menu Menu_DISPLAY = {.Show = true, .S8A_Title = ENG_Display, .ST_Children = References_DISPLAY};

const struct Menu_Match Match_DISPLAY_Backlight[] = {
    {.MatchVal = 0 << 16, .MatchString = ENG_OS_Off},
    {.MatchVal = 9.10195 * 65536, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_DISPLAY_Backlight_Charge = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.016666 * 65536, .JumpToMaxVal = 1 * 65536, .Description = ENG_DESC_Backlight, .CodeProtect = false};
const Menu_Value Presentation_DISPLAY_Backlight_Charge = {.NumbToString = Match_DISPLAY_Backlight, .Show = true, .Data = &Block_3_ID_0};
Menu Menu_DISPLAY_Backlight_Charge = {.Show = true, .S8A_Title = ENG_Backlight_Charge, .ST_Popup = &Setting_DISPLAY_Backlight_Charge, .ST_Value = &Presentation_DISPLAY_Backlight_Charge};

const Menu_Popup Setting_DISPLAY_Backlight_Discharge = {.StepFactor_Fast = 0.16666 * 65536, .StepFactor_Slow = 0.016666 * 65536, .JumpToMaxVal = 1<<16, .Description = ENG_DESC_Backlight, .CodeProtect = false};
const Menu_Value Presentation_DISPLAY_Backlight_Discharge = {.NumbToString = Match_DISPLAY_Backlight, .Show = true, .Data = &Block_3_ID_1};
Menu Menu_DISPLAY_Backlight_Discharge = {.Show = true, .S8A_Title = ENG_Backlight_Discharge, .ST_Popup = &Setting_DISPLAY_Backlight_Discharge, .ST_Value = &Presentation_DISPLAY_Backlight_Discharge};

const Menu_Popup Setting_DISPLAY_Contrast = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_Contrast, .Function_Change = ERC240_Set_Contrast, .Function_Cancel = ERC240_Set_Contrast, .CodeProtect = false};
const Menu_Value Presentation_DISPLAY_Contrast = {.Show = true, .Data = &Block_3_ID_5};
Menu Menu_DISPLAY_Contrast = {.Show = true, .S8A_Title = ENG_Contrast, .ST_Popup = &Setting_DISPLAY_Contrast, .ST_Value = &Presentation_DISPLAY_Contrast};

const Menu_Popup Setting_DISPLAY_Lock = {.Type = POPUP_LOCK_SET, .CodeProtect = true};
const Menu_Value Presentation_DISPLAY_Lock = {.Show = false, .Data = &Block_3_ID_4};
Menu Menu_DISPLAY_Lock = {.Show = true, .S8A_Title = ENG_ParamProtect, .ST_Popup = &Setting_DISPLAY_Lock, .ST_Value = &Presentation_DISPLAY_Lock};

//Bluetooth Menu
#ifdef BLUETOOTH_CODE
Menu Menu_BLE = {.Show = false, .S8A_Title = ENG_BLE_Bluetooth, .ST_Children = References_BLE};

const struct Menu_Match Match_BLE_Power[] = {
    {.MatchVal = 1 << 16, .MatchString = ENG_OS_On},
    {NULL}
};

const Menu_Popup Setting_BLE_Power = {.Description = ENG_DESC_EnableDisable, .CodeProtect = false, .Function_Accept = BLE_UpdatePowerSetting};
const Menu_Value Presentation_BLE_Power = {.Show = true, .NumbToString = Match_BLE_Power, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_200_ID_100};
Menu Menu_BLE_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Popup = &Setting_BLE_Power, .ST_Value = &Presentation_BLE_Power};
        
const struct Menu_Match Match_BLE_Status[] = {
    {.MatchVal = 0, .MatchString = ENG_ConDev1},
    {.MatchVal = 1, .MatchString = ENG_ConDev2},
    {.MatchVal = 2, .MatchString = ENG_ConDev3},
    {.MatchVal = 3, .MatchString = ENG_ConDev4},
    {.MatchVal = 4, .MatchString = ENG_ConDev5},
    {.MatchVal = 5, .MatchString = ENG_Disconnected},
    {NULL}
};
const Menu_Value Presentation_BLE_Status = {.Show = true, .NumbToString = Match_BLE_Status, .Data = &Block_200_ID_101};
Menu Menu_BLE_Status = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_ConnectionStatus, .ST_Value = &Presentation_BLE_Status};

Menu Menu_BLE_Attached = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_BLE_BoundedDevices, .ST_Children = References_BLE_Attached};

const Menu_Popup Setting_BLE_Attached_D1 = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_DeleteOK};
const Menu_Value Presentation_BLE_Attached_D1 = {.Show = true, .Data = &Block_200_ID_111};
Menu Menu_BLE_Attached_D1 = {.Show = MENU_HIDE_IF_NULL_OR_0, .S8A_Title = ENG_BLE_Dev1, .ST_Popup = &Setting_BLE_Attached_D1, .ST_Value = &Presentation_BLE_Attached_D1};

const Menu_Popup Setting_BLE_Attached_D2 = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_DeleteOK};
const Menu_Value Presentation_BLE_Attached_D2 = {.Show = true, .Data = &Block_200_ID_113};
Menu Menu_BLE_Attached_D2 = {.Show = MENU_HIDE_IF_NULL_OR_0, .S8A_Title = ENG_BLE_Dev2, .ST_Popup = &Setting_BLE_Attached_D2, .ST_Value = &Presentation_BLE_Attached_D2};

const Menu_Popup Setting_BLE_Attached_D3 = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_DeleteOK};
const Menu_Value Presentation_BLE_Attached_D3 = {.Show = true, .Data = &Block_200_ID_115};
Menu Menu_BLE_Attached_D3 = {.Show = MENU_HIDE_IF_NULL_OR_0, .S8A_Title = ENG_BLE_Dev3, .ST_Popup = &Setting_BLE_Attached_D3, .ST_Value = &Presentation_BLE_Attached_D3};

const Menu_Popup Setting_BLE_Attached_D4 = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_DeleteOK};
const Menu_Value Presentation_BLE_Attached_D4 = {.Show = true, .Data = &Block_200_ID_117};
Menu Menu_BLE_Attached_D4 = {.Show = MENU_HIDE_IF_NULL_OR_0, .S8A_Title = ENG_BLE_Dev4, .ST_Popup = &Setting_BLE_Attached_D4, .ST_Value = &Presentation_BLE_Attached_D4};

const Menu_Popup Setting_BLE_Attached_D5 = {.StepFactor_Fast = 6553.6, .StepFactor_Slow = 655.36, .Description = ENG_DESC_DeleteOK};
const Menu_Value Presentation_BLE_Attached_D5 = {.Show = true, .Data = &Block_200_ID_119};
Menu Menu_BLE_Attached_D5 = {.Show = MENU_HIDE_IF_NULL_OR_0, .S8A_Title = ENG_BLE_Dev5, .ST_Popup = &Setting_BLE_Attached_D5, .ST_Value = &Presentation_BLE_Attached_D5};

const Menu_Value Presentation_BLE_Version = {.Show = true, .Data = &Block_200_ID_103};
Menu Menu_BLE_Version = {.Shift = SHIFT_LVL1, .Show = true, .S8A_Title = ENG_BLE_FWVersion, .ST_Value = &Presentation_BLE_Version};

#endif

//Sound Menu
Menu Menu_SOUND = {.Show = true, .S8A_Title = ENG_Sound, .ST_Children = References_SOUND};

const struct Menu_Match Match_Sound_Power[] = {
    {.MatchVal = 1 << 16, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_SOUND_Power = {.Description = ENG_DESC_Sound, .CodeProtect = false};
const Menu_Value Presentation_SOUND_Power = {.Show = true, .NumbToString = Match_Sound_Power, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_3_ID_3};
Menu Menu_SOUND_Power = {.Show = true, .S8A_Title = ENG_Power, .ST_Popup = &Setting_SOUND_Power, .ST_Value = &Presentation_SOUND_Power};

const struct Menu_Match Match_Sound_Button[] = {
    {.MatchVal = 1 << 17, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_SOUND_Button = {.Description = ENG_DESC_Sound, .CodeProtect = false};
const Menu_Value Presentation_SOUND_Button = {.Show = true, .NumbToString = Match_Sound_Button, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_3_ID_3};
Menu Menu_SOUND_Button = {.Show = true, .S8A_Title = ENG_Button, .ST_Popup = &Setting_SOUND_Button, .ST_Value = &Presentation_SOUND_Button};

const struct Menu_Match Match_Sound_Error[] = {
    {.MatchVal = 1 << 18, .MatchString = ENG_OS_On},
    {NULL}
};
const Menu_Popup Setting_SOUND_Error = {.Description = ENG_DESC_Sound, .CodeProtect = false};
const Menu_Value Presentation_SOUND_Error = {.Show = true, .NumbToString = Match_Sound_Error, .NumbToString_NoMatch = ENG_OS_Off, .Data = &Block_3_ID_3};
Menu Menu_SOUND_Error = {.Show = true, .S8A_Title = ENG_FL_Simple, .ST_Popup = &Setting_SOUND_Error, .ST_Value = &Presentation_SOUND_Error};

//Menu Menu_CONFIGS   = {.S8A_Title = ENG_CONFIG, .ST_Children = References_CONFIGS, .Show = true,};
const struct Menu_Match Match_Setting[] = {
    {.MatchVal = 1 << 24, .MatchString = ENG_FL_Simple},
    {.MatchVal = 1 << 20, .MatchString = ENG_FL_Simple},
    {.MatchVal = 0, .MatchString = ENG_None},
    {.MatchVal = 1 << 16, .MatchString = ENG_CONFIG_Extension},
    {NULL}
};
const Menu_Popup Setting_Match_CONFIGS_SELECT = {.Function_Entry = Config_UpdateCurrentSetting, .Function_Accept = Config_RunSetting, .StepFactor_Fast = 1 << 16, .StepFactor_Slow = 1 << 16, .Description = ENG_DESC_Config, .CodeProtect = true};
const Menu_Value Presentation_CONFIGS_SELECT = {.NumbToString = &Match_Setting[0], .Show = true, .Data = &Block_7_ID_0};
Menu Menu_CONFIGS_SELECT = {.Show = true, .S8A_Title = ENG_CONFIG_Short, .ST_Popup = &Setting_Match_CONFIGS_SELECT, .ST_Value = &Presentation_CONFIGS_SELECT};

const Menu_Value Presentation_CONFIGS_SOC_TOTAL = {.Show = true, .Data = &Block_241_ID_119};
Menu Menu_CONFIGS_SOC_TOTAL = {.Show = true, .S8A_Title = ENG_SOC_System_Total, .ST_Value = &Presentation_CONFIGS_SOC_TOTAL};

const Menu_Value Presentation_CONFIGS_SOC_EXT = {.Show = true, .Data = &Block_242_ID_119};
Menu Menu_CONFIGS_SOC_EXT = {.Show = true, .S8A_Title = ENG_SOC_System_Extension, .ST_Value = &Presentation_CONFIGS_SOC_EXT};

//Error Menu
Menu Menu_ERROR     = {.S8A_Title = ENG_Errors, .ST_Children = References_ERROR, .Show = true,};
const Menu_Value Presentation_ERROR_Buffer[8] = {
    [0] = {.Show = true,  .Data = &Block_200_ID_2},
    [1] = {.Show = true,  .Data = &Block_200_ID_3},
    [2] = {.Show = true,  .Data = &Block_200_ID_4},
    [3] = {.Show = true,  .Data = &Block_200_ID_5},
    [4] = {.Show = true,  .Data = &Block_200_ID_6},
    [5] = {.Show = true,  .Data = &Block_200_ID_7},
    [6] = {.Show = true,  .Data = &Block_200_ID_8},
    [7] = {.Show = true,  .Data = &Block_200_ID_9}
};
Menu Menu_ERRORS[8] = {
    [0] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[0]},
    [1] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[1]},
    [2] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[2]},
    [3] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[3]},
    [4] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[4]},
    [5] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[5]},
    [6] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[6]},
    [7] =   {.Show = true, .ST_Value = &Presentation_ERROR_Buffer[7]}
}; 

#ifdef ENABLE_TEST_VALS
//Test value menu
Menu Menu_TESTVALS     = {.Show = false, .S8A_Title = ENG_TestVals, .ST_Children = References_TESTVALS};
const Menu_Value Presentation_TESTVAL[16] = {
    [0] = {.Show = true,  .Data = &Block_255_ID_0, .Decimals = 3}/*,
    [1] = {.Show = true,  .Data = &Block_255_ID_1, .Decimals = 3},
    [2] = {.Show = true,  .Data = &Block_255_ID_2, .Decimals = 3},
    [3] = {.Show = true,  .Data = &Block_255_ID_3, .Decimals = 3},
    [4] = {.Show = true,  .Data = &Block_255_ID_4, .Decimals = 3},
    [5] = {.Show = true,  .Data = &Block_255_ID_5, .Decimals = 3},
    [6] = {.Show = true,  .Data = &Block_255_ID_6, .Decimals = 3},
    [7] = {.Show = true,  .Data = &Block_255_ID_7, .Decimals = 3},
    [8] = {.Show = true,  .Data = &Block_255_ID_8, .Decimals = 3},
    [9] = {.Show = true,  .Data = &Block_255_ID_9, .Decimals = 3},
    [10] = {.Show = true,  .Data = &Block_255_ID_10, .Decimals = 3},
    [11] = {.Show = true,  .Data = &Block_255_ID_11, .Decimals = 3},
    [12] = {.Show = true,  .Data = &Block_255_ID_12, .Decimals = 3},
    [13] = {.Show = true,  .Data = &Block_255_ID_13, .Decimals = 3},
    [14] = {.Show = true,  .Data = &Block_255_ID_15, .Decimals = 3},
    [15] = {.Show = true,  .Data = &Block_255_ID_15, .Decimals = 3}*/
};
Menu Menu_TESTVAL[16] = {
    [0] =   {.Show = true,.S8A_Title = ENG_Test0, .ST_Value = &Presentation_TESTVAL[0]}/*,
    [1] =   {.Show = true,.S8A_Title = ENG_Test1, .ST_Value = &Presentation_TESTVAL[1]},
    [2] =   {.Show = true,.S8A_Title = ENG_Test2, .ST_Value = &Presentation_TESTVAL[2]},
    [3] =   {.Show = true,.S8A_Title = ENG_Test3, .ST_Value = &Presentation_TESTVAL[3]},
    [4] =   {.Show = true,.S8A_Title = ENG_Test4, .ST_Value = &Presentation_TESTVAL[4]},
    [5] =   {.Show = true,.S8A_Title = ENG_Test5, .ST_Value = &Presentation_TESTVAL[5]},
    [6] =   {.Show = true,.S8A_Title = ENG_Test6, .ST_Value = &Presentation_TESTVAL[6]},
    [7] =   {.Show = true,.S8A_Title = ENG_Test7, .ST_Value = &Presentation_TESTVAL[7]},
    [8] =   {.Show = true,.S8A_Title = ENG_Test8, .ST_Value = &Presentation_TESTVAL[8]},
    [9] =   {.Show = true,.S8A_Title = ENG_Test9, .ST_Value = &Presentation_TESTVAL[9]},
    [10] =   {.Show = true,.S8A_Title = ENG_Test10, .ST_Value = &Presentation_TESTVAL[10]},
    [11] =   {.Show = true,.S8A_Title = ENG_Test11, .ST_Value = &Presentation_TESTVAL[11]},
    [12] =   {.Show = true,.S8A_Title = ENG_Test12, .ST_Value = &Presentation_TESTVAL[12]},
    [13] =   {.Show = true,.S8A_Title = ENG_Test13, .ST_Value = &Presentation_TESTVAL[13]},
    [14] =   {.Show = true,.S8A_Title = ENG_Test14, .ST_Value = &Presentation_TESTVAL[14]},
    [15] =   {.Show = true,.S8A_Title = ENG_Test15, .ST_Value = &Presentation_TESTVAL[15]}*/
}; 
#endif
///REFERENCES - CHILDREN///
//Add a NULL at the end of a menu definition for indication of end.
Menu* References_MAIN[] = {
    //#ifdef BLUETOOTH_CODE
    //&Menu_BLE,
    //#endif
    &Menu_ACOUT, 
    &Menu_ACIN, 
    &Menu_DCOUT, 
    &Menu_DCIN, 
    &Menu_SOLAR, 
    &Menu_GENERAL, 
    #ifdef ENABLE_TEST_VALS
    &Menu_TESTVALS,
    #endif
    NULL
};
Menu* References_GENERAL[] = {
    &Menu_STATUS,
    &Menu_NRGMETER,
    &Menu_TEMP,
    &Menu_IOVOLT,
    &Menu_ERROR,
    &Menu_DISPLAY,
    &Menu_SOUND,
    #ifdef BLUETOOTH_CODE
    &Menu_BLE,
    #endif
    &Menu_ABOUT,
    &Menu_STORAGE,
    &Menu_CONFIGS_SELECT,
    NULL
};
Menu* References_IOVOLT[] = {
    &Menu_IOVOLT_Remote,
    &Menu_IOVOLT_Data,
    &Menu_IOVOLT_DataFront,
    &Menu_IOVOLT_C1Terminal,
    &Menu_IOVOLT_C2Terminal,
    &Menu_IOVOLT_DCOutIfC1,
    &Menu_IOVOLT_ACOutIfC1,
    //&Menu_IOVOLT_C1CustomLvl,         //Commented out 03-12-2024. Can provoke error 93
    //&Menu_IOVOLT_C1CustomActive,      //Commented out 03-12-2024. Can provoke error 93
    //&Menu_IOVOLT_C1CustomDeactive,    //Commented out 03-12-2024. Can provoke error 93
    NULL
};
Menu* References_ACOUT[] = {
    &Menu_ACOUT_OperationStatus, 
    &Menu_ACOUT_Power, 
    &Menu_ACOUT_Voltage, 
    &Menu_ACOUT_Current,
    &Menu_ACOUT_Auto_PowerDownDelay, 
    &Menu_ACOUT_Auto_PowerDownLoad,
    &Menu_ACOUT_Inverter_Cutoff,
    NULL
};
Menu* References_ACIN[] = {
    &Menu_ACIN_OperationStatus, 
    &Menu_ACIN_Power, 
    &Menu_ACIN_Voltage, 
    &Menu_ACIN_Current, 
    &Menu_ACIN_MaxCurrent,
    NULL
};
Menu* References_DCOUT[] = {
    &Menu_DCOUT_OperationStatus, 
    &Menu_DCOUT_Power, 
    &Menu_DCOUT_Voltage, 
    &Menu_DCOUT_Current, 
    &Menu_DCOUT_ShutdownDelay,
    &Menu_DCOUT_SaverTime,
    &Menu_DCOUT_SaverAmp,
    NULL
};

Menu* References_DCIN_StarterBattery[] = {
    &Menu_DCIN_StartBatEnable,
    &Menu_DCIN_StartBatChgCurrent12V,
    &Menu_DCIN_StartBatChgCurrent24V,
    &Menu_DCIN_StartBatChgVoltage12V,
    &Menu_DCIN_StartBatChgVoltage24V,
    &Menu_DCIN_StartBatCutOffCurrent12V,
    &Menu_DCIN_StartBatCutOffCurrent24V,
    &Menu_DCIN_CutOffTimer,
    &Menu_DCIN_StartBatMaintenance12V,
    &Menu_DCIN_StartBatMaintenance24V,
    NULL
};

Menu* References_DCIN[] = {
    &Menu_DCIN_OperationStatus, 
    &Menu_DCIN_Power, 
    &Menu_DCIN_Voltage, 
    &Menu_DCIN_Current, 
    &Menu_DCIN_OperatingVoltage, 
    &Menu_DCIN_SetChargeCurrent, 
    &Menu_DCIN_SetOperatingVoltage, 
    &Menu_DCIN_StartVolt12V,
    &Menu_DCIN_StopVolt12V,
    &Menu_DCIN_Jumpstart_GlobalDisable,
    &Menu_DCIN_Jumpstart, 
    &Menu_DCIN_StarterBattery,
    NULL
};
Menu* References_SOLAR[] = {
    &Menu_SOLAR_OperationStatus, 
    &Menu_SOLAR_Power, 
    &Menu_SOLAR_Voltage, 
    &Menu_SOLAR_Current, 
    &Menu_SOLAR_SetOperation, 
    &Menu_SOLAR_SetOCVoltage, 
    &Menu_SOLAR_SetMPPVoltage, 
    &Menu_SOLAR_SetStartVoltage,
    NULL
};
Menu* References_SOUND[] = {
    &Menu_SOUND_Power,
    &Menu_SOUND_Button,
    &Menu_SOUND_Error,
    NULL
};
Menu* References_DISPLAY[] = {
    &Menu_DISPLAY_Backlight_Charge,
    &Menu_DISPLAY_Backlight_Discharge,
    &Menu_DISPLAY_Lock,
    &Menu_DISPLAY_Contrast,
    NULL
};

#ifdef BLUETOOTH_CODE
Menu* References_BLE[] = {
    &Menu_BLE_Power,
    &Menu_BLE_Status,
    &Menu_BLE_Attached,
    &Menu_BLE_Version,
    NULL
};

Menu* References_BLE_Attached[] = {
    &Menu_BLE_Attached_D1,
    &Menu_BLE_Attached_D2,
    &Menu_BLE_Attached_D3,
    &Menu_BLE_Attached_D4,
    &Menu_BLE_Attached_D5,
    NULL
};
#endif


Menu* References_STATUS[] = {
    &Menu_STATUS_OperationStatus,
    &Menu_STATUS_RemainingOperation,
    &Menu_STATUS_SOC,
    &Menu_CONFIGS_SOC_EXT,
    &Menu_CONFIGS_SOC_TOTAL,
    &Menu_STATUS_Power,
    &Menu_STATUS_Voltage,
    &Menu_STATUS_Current,
    &Menu_STATUS_Temperature,
    &Menu_STATUS_Cell1Vol,
    &Menu_STATUS_Cell2Vol,
    &Menu_STATUS_Cell3Vol,
    &Menu_STATUS_Cell4Vol,
    &Menu_STATUS_Cycles,
    NULL
};
Menu* References_TEMP[] = {
    &Menu_TEMP_Trafo,
    &Menu_TEMP_IGBT,
    &Menu_TEMP_TempCell12,
    &Menu_TEMP_TempCell23,
    &Menu_TEMP_TempCell34,
    NULL
};
Menu* References_ABOUT[] = {
    &Menu_ABOUT_Serial,
    &Menu_ABOUT_ManufactureDate,
    &Menu_ABOUT_HVersion,
    &Menu_ABOUT_SVersionUnit,
    &Menu_ABOUT_SVersionDisp,
    &Menu_ABOUT_SVersionPwr,
    &Menu_ABOUT_SVersionDCDC,
    &Menu_ABOUT_BOOTLOADERS,
    NULL
};
Menu* References_STORAGE[] = {
    &Menu_STORAGE_Delay,
    //&Menu_STORAGE_Enter,
    NULL
};Menu* References_BOOTLOADERS[] = {
    &Menu_BOOTLOADER_VersionUnit,
    &Menu_BOOTLOADER_VersionDisp,
    &Menu_BOOTLOADER_VersionPwr,
    &Menu_BOOTLOADER_VersionDcDc,
    NULL
};
Menu* References_NRGMETER[] = {
    &Menu_NRGMETER_ACIn,
    &Menu_NRGMETER_DCIn,
    &Menu_NRGMETER_DCOut,
    &Menu_NRGMETER_Solar,
    NULL
};
Menu* References_ERROR[] = {
    &Menu_ERRORS[0],
    &Menu_ERRORS[1],
    &Menu_ERRORS[2],
    &Menu_ERRORS[3],
    &Menu_ERRORS[4],
    &Menu_ERRORS[5],
    &Menu_ERRORS[6],
    &Menu_ERRORS[7],
    NULL
};

#ifdef ENABLE_TEST_VALS
Menu* References_TESTVALS[] = {
    &Menu_TESTVAL[0],/*
    &Menu_TESTVAL[1],
    &Menu_TESTVAL[2],
    &Menu_TESTVAL[3],
    &Menu_TESTVAL[4],
    &Menu_TESTVAL[5],
    &Menu_TESTVAL[6],
    &Menu_TESTVAL[7],
    &Menu_TESTVAL[8],
    &Menu_TESTVAL[9],
    &Menu_TESTVAL[10],
    &Menu_TESTVAL[11],
    &Menu_TESTVAL[12],
    &Menu_TESTVAL[13],
    &Menu_TESTVAL[14],
    &Menu_TESTVAL[15],*/
    NULL
};
#endif


