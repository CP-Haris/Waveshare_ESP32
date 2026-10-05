#include "Values.h"
#include "Values_Functions.h"
#include "UART_App.h"
#include "Menu_Functions.h"
#include "Menu_Application.h"
#include "Button.h"
#include "Timer.h"
#include "Buzzer.h"
#include "ERC240128FS.h"
#include "BLE_App.h"

volatile APP_UPDATE_FLAGS   App_UpdateFlags;
unsigned char ModelValue[20]; //Model Number

//Control Variables - Block 0
Value Block_0_ID_70     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 70, .Prefix = VOLTAGE};            // IO1_REMOTE
Value Block_0_ID_71     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 71, .Prefix = VOLTAGE};            // IO2_DATA
Value Block_0_ID_72     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 72, .Prefix = VOLTAGE};            // IO3_DATAFRONT
Value Block_0_ID_73     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 73, .Prefix = VOLTAGE};            // C2_TERMINAL
Value Block_0_ID_74     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 74, .Prefix = VOLTAGE};            // C1_TERMINAL
Value Block_0_ID_140    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 140, .Prefix = KWH};                // NRGMETER_ACIN
Value Block_0_ID_142    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 142, .Prefix = KWH};                // NRGMETER_DCIN
Value Block_0_ID_144    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 144, .Prefix = KWH};                // NRGMETER_DCOUT
Value Block_0_ID_146    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 146, .Prefix = KWH};                // NRGMETER_SOLAR
Value Block_0_ID_201    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 201, .Prefix = STATE_OPERATION};    // ACOUT_OPSTATE
Value Block_0_ID_205    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 205, .Prefix = STATE_FAILURE};      // ACOUT_FLSTATE
Value Block_0_ID_107    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 107, .Prefix = POWER};              // ACOUT_WATT
Value Block_0_ID_105    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 105, .Prefix = VOLTAGE};            // ACOUT_VOLT
Value Block_0_ID_106    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 106, .Prefix = CURRENT};            // ACOUT_AMP
Value Block_0_ID_226    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 226, .Prefix = TIME_HHMMSS};        // ACOUT_AUTO_TIMEGLOBAL
Value Block_0_ID_200    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 200, .Prefix = STATE_OPERATION};    // ACIN_OPSTATE
Value Block_0_ID_204    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 204, .Prefix = STATE_FAILURE};      // ACIN_FLSTATE
Value Block_0_ID_104    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 104, .Prefix = POWER};              // ACIN_WATT
Value Block_0_ID_102    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 102, .Prefix = VOLTAGE};            // ACIN_VOLT
Value Block_0_ID_103    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 103, .Prefix = CURRENT};            // ACIN_AMP
Value Block_0_ID_208    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 208, .Prefix = STATE_OPERATION};    // SOLAR_OPSTATE
Value Block_0_ID_209    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 209, .Prefix = STATE_FAILURE};      // SOLAR_FLSTATE
Value Block_0_ID_78     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 78, .Prefix = CURRENT};            // SOLAR_AMP
Value Block_0_ID_79     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 79, .Prefix = POWER};              // SOLAR_WATT
Value Block_0_ID_203    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 203, .Prefix = STATE_OPERATION};    // DCOUT_OPSTATE
Value Block_0_ID_207    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 207, .Prefix = STATE_FAILURE};      // DCOUT_FLSTATE
Value Block_0_ID_113    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 113, .Prefix = POWER};              // DCOUT_WATT
Value Block_0_ID_111    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 111, .Prefix = VOLTAGE};            // DCOUT_VOLT
Value Block_0_ID_112    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 112, .Prefix = CURRENT};            // DCOUT_AMP
Value Block_0_ID_225    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 225, .Prefix = TIME_HHMMSS};        // DCOUT_AUTO_TIMEGLOBAL
Value Block_0_ID_202    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 202, .Prefix = STATE_OPERATION};    // DCIN_OPSTATE
Value Block_0_ID_206    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 206, .Prefix = STATE_FAILURE};      // DCIN_FLSTATE
Value Block_0_ID_110    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 110, .Prefix = POWER};              // DCIN_WATT
Value Block_0_ID_108    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 108, .Prefix = VOLTAGE};            // DCIN_VOLT
Value Block_0_ID_109    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 109, .Prefix = CURRENT};            // DCIN_AMP
Value Block_0_ID_170    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 170, .Prefix = MASKED_BITMAP};      // DCIN_OPERATING_VOLTAGE
Value Block_0_ID_120    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 120, .Prefix = TIME_HEAD, .NumValue = 0xFFFE8000};          // BATSTATUS_REMTIME
Value Block_0_ID_119    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 119, .Prefix = PROCENT};            // BATSTATUS_SOC
Value Block_0_ID_127    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 127, .Prefix = POWER};              // BATSTATUS_WATT
Value Block_0_ID_100    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 100, .Prefix = VOLTAGE};            // BATSTATUS_VOLT
Value Block_0_ID_101    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 101, .Prefix = CURRENT};            // BATSTATUS_AMP
Value Block_0_ID_114    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 114, .Prefix = TEMPERATURE};        // BATSTATUS_TEMP
Value Block_0_ID_1      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 1,   .Prefix = VOLTAGE};            // BATSTATUS_CELL1
Value Block_0_ID_2      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 2, .Prefix = VOLTAGE};            // BATSTATUS_CELL2
Value Block_0_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 3, .Prefix = VOLTAGE};            // BATSTATUS_CELL3
Value Block_0_ID_4      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 4, .Prefix = VOLTAGE};            // BATSTATUS_CELL4
Value Block_0_ID_124    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 124, .Prefix = NULL};               // BATSTATUS_CYCLES
Value Block_0_ID_116    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 116, .Prefix = TEMPERATURE};        // TEMPERATURE_IGBT
Value Block_0_ID_117    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 117, .Prefix = TEMPERATURE};        // TEMPERATURE_TRAFO
Value Block_0_ID_121    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 121, .Prefix = TEMPERATURE};        // TEMPERATURE_CELL12
Value Block_0_ID_122    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 122, .Prefix = TEMPERATURE};        // TEMPERATURE_CELL23
Value Block_0_ID_123    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 123, .Prefix = TEMPERATURE};        // TEMPERATURE_CELL34
Value Block_0_ID_220    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 220, .Prefix = SERIAL};             // ABOUT_SERIAL
Value Block_0_ID_221    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 221, .Prefix = DATE};               // ABOUT_MANUDATE
Value Block_0_ID_222    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 222, .Prefix = VERSION_SHORT};      // ABOUT_HWVERS
Value Block_0_ID_223    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 223, .Prefix = VERSION_LONG};       // ABOUT_SWVERSLPS
Value Block_0_ID_157    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 157, .Prefix = NULL};               // WAKEUPFLAGS
Value Block_0_ID_210    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 210, .Prefix = STATE_BATTERY};      // BATSTATUS
Value Block_0_ID_212    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 212, .Prefix = STATE_TESTMODE};     // SYS_TESTMODE
Value Block_0_ID_230    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 0, .ID = 230, .Prefix = VERSION_SHORT};      // BOOT_VERSION

//Functions variables
Value Block_1_ID_1      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 1, .Prefix = TIME_HHMMSS, .NumMax = 0.0833333 * 65536,    .NumMin = 0, .Flag.NumMinMax_Received = true};   // FUNC_JUMPSTART
Value Block_1_ID_2      = {.Flag.Access_CTRL = true, .Block = 1, .ID = 2, .Prefix = NULL, .NumValue = 0};                                                                                                // Bluetooth present
Value Block_1_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 3,   .Prefix = MASKED_BITMAP,  .NumValue = 0b0};                                           // Sleep mode enable
Value Block_1_ID_100    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 100, .Prefix = POWER, .NumValue = 760<<16};                                     // FUNC_ACIN_MAX
Value Block_1_ID_101    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 101, .Prefix = POWER, .NumValue = 0};                                           // FUNC_ACIN_MIN
Value Block_1_ID_102    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 102, .Prefix = POWER, .NumValue = 25<<16};                                      // FUNC_ACIN_HYST
Value Block_1_ID_110    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 110, .Prefix = POWER, .NumValue = 1425<<16};                                    // FUNC_ACOUT_MAX
Value Block_1_ID_111    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 111, .Prefix = POWER, .NumValue = 0};                                           // FUNC_ACOUT_MIN
Value Block_1_ID_112    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 112, .Prefix = POWER, .NumValue = 25<<16};                                      // FUNC_ACOUT_HYST
Value Block_1_ID_120    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 120, .Prefix = CURRENT, .NumValue = 45<<16};                                    // FUNC_DCIN_MAX
Value Block_1_ID_121    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 121, .Prefix = CURRENT, .NumValue = 0};                                         // FUNC_DCIN_MIN
Value Block_1_ID_122    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 122, .Prefix = CURRENT, .NumValue = 1<<16};                                     // FUNC_DCIN_HYST
Value Block_1_ID_123    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 123, .Prefix = POWER, .NumValue = 520<<16};                                     // FUNC_DCIN_WATT_MAX
Value Block_1_ID_124    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 124, .Prefix = POWER, .NumValue = -520<<16};                                    // FUNC_DCIN_WATT_MIN
Value Block_1_ID_125    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 125, .Prefix = POWER, .NumValue = 10<<16};                                      // FUNC_DCIN_WATT_HYST
Value Block_1_ID_130    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 130, .Prefix = CURRENT, .NumValue = 90<<16};                                    // FUNC_DCOUT_MAX
Value Block_1_ID_131    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 131, .Prefix = CURRENT, .NumValue = 0xFF4C<<16};                                // FUNC_DCOUT_MIN
Value Block_1_ID_132    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 1, .ID = 132, .Prefix = CURRENT, .NumValue = 1<<16};                                     // FUNC_DCOUT_HYST

//Display Variables
Value Block_3_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 3, .Prefix = MASKED_BITMAP, .NumValue = 0b00111<<16};                             // DISP_BITMAP1 Bit 0 = Power Sound, Bit 1 = Button Sound, Bit 2 = Error Soun, Bit 3 = Show QR, Bit 4 = Active Bluetooth
Value Block_3_ID_0      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 0, .Prefix = TIME_HHMMSS};            // DISP_LIGHT_CRG
Value Block_3_ID_1      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 1, .Prefix = TIME_HHMMSS};            // DISP_LIGHT_DISCRG
Value Block_3_ID_4      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 4, .Prefix = NULL, .NumMax = 1<<16, .NumMin = 0, .Flag.NumMinMax_Received = true};                              // DISP_LOCK
Value Block_3_ID_5      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 5, .Prefix = PROCENT, .NumValue = 655.36*50};                                  // DISP_CONTRAST
Value Block_3_ID_6      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 6, .Prefix = NULL, .NumValue = SPLASH_LPS_V2<<16};                             // DISP_SPLASH
Value Block_3_ID_7      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 3, .ID = 7, .Prefix = NULL, .NumValue = VIEW_NEW};                                      // DISP_VIEW

Value Block_4_ID_0      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 4, .ID = 0, .Prefix = VOLTAGE};//, .NumMax = 14<<16, .NumMin = 12<<16, .Flag.NumMinMax_Received = true};                             // C1 Wakeup level - Activate
Value Block_4_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 4, .ID = 3, .Prefix = VOLTAGE};//, .NumMax = 14<<16, .NumMin = 12<<16, .Flag.NumMinMax_Received = true};                             // C1 Wakeup levels - Deactivate


Value Block_6_ID_6      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 6, .ID = 6, .Prefix = MASKED_BITMAP};                             // Activate DC out on C1 signal: Block ID 5,6 Bit 2
Value Block_6_ID_2      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 6, .ID = 2, .Prefix = MASKED_BITMAP};                             // Activate AC out on C1 signal: Block ID 5,2 Bit 2
Value Block_6_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 6, .ID = 3};

Value Block_7_ID_0      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 7, .ID = 0, .Prefix = NULL, .NumMax = 0x00010000, .NumMin = 0, .Flag.NumMinMax_Received = true};

//Setting DCInput Variables
Value Block_30_ID_0     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 30, .ID = 0, .Prefix = MASKED_BITMAP};   // DCIN_CONTROL_BITMAP - Jumpsatart Control
Value Block_30_ID_1     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 30, .ID = 1, .Prefix = NULL, .NumMax = 2<<16, .NumMin = 0, .Flag.NumMinMax_Received = true};   // DCIN_SET_OPERATION_MODE
Value Block_30_ID_7     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 30, .ID = 7, .Prefix = CURRENT};         // DCIN_SET_INPUT_CURR
Value Block_30_ID_12    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 30, .ID = 12, .Prefix = VOLTAGE};       // DCIN_SET_START_VOLTAGE (added 28-02-2024) 
Value Block_30_ID_13    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 30, .ID = 13, .Prefix = VOLTAGE};       // DCIN_SET_STOP_VOLTAGE (added 28-02-2024)

//Setting DCInput Variables
Value Block_31_ID_0     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 0, .Prefix = MASKED_BITMAP};                                  //ENABLE_CHARGE_OF_STARTBAT 
//12V Vals
Value Block_31_ID_1     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 1, .Prefix = CURRENT};      
Value Block_31_ID_2     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 2, .Prefix = VOLTAGE};   
Value Block_31_ID_3     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31,  .ID = 3, .Prefix = CURRENT};                                   
Value Block_31_ID_4     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31,  .ID = 4, .Prefix = VOLTAGE};   

//24 Vals
Value Block_31_ID_5     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 5, .Prefix = CURRENT};      
Value Block_31_ID_6     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 6, .Prefix = VOLTAGE};   
Value Block_31_ID_7     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31,  .ID = 7, .Prefix = CURRENT};                                   
Value Block_31_ID_8     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31,  .ID = 8, .Prefix = VOLTAGE};
Value Block_31_ID_10    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 31, .ID = 10,  .Prefix = TIME_HHMMSS};

//DC Output Setting
Value Block_40_ID_0     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 40, .ID = 0, .Prefix = TIME_HHMMSS};   // DCOUT_STB_TIME
Value Block_40_ID_1     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 40, .ID = 1, .Prefix = TIME_HHMMSS};   // DCOUT_SAVER_TIME (added 28-02-2024)
Value Block_40_ID_2     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 40, .ID = 2, .Prefix = CURRENT};       // DCOUT_SAVER_CURRENT (added 28-02-2024)

//AC Output Settings
Value Block_50_ID_0     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 50, .ID = 0, .Prefix = PROCENT};      //INVERTER CUTTOFF
Value Block_50_ID_1     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 50, .ID = 1, .Prefix = TIME_HHMMSS};  // ACOUT_SAVER_TIME
Value Block_50_ID_2     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 50, .ID = 2, .Prefix = POWER};        // ACOUT_SAVER_LIMIT

//AC Input Settings
Value Block_60_ID_2     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 60, .ID = 2, .Prefix = CURRENT};  // ACIN_MAX_CURRENT

//Solar Variables 1
Value Block_70_ID_0    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 70, .ID = 0, .Prefix = NULL};  // SOLAR_SET_OPERATION_MODE

//Solar Variables 2
Value Block_71_ID_0     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 71, .ID = 0, .Prefix = VOLTAGE};  // SOLAR_SELFLEARN_OC_VOLT
Value Block_71_ID_1     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 71, .ID = 1, .Prefix = VOLTAGE};  // SOLAR_SELFLEARN_MPP_VOLT
Value Block_71_ID_2     = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 71, .ID = 2, .Prefix = VOLTAGE};  // SOLAR_SELFLEARN_START_VOLT

Value Block_82_ID_0      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 0 };
Value Block_82_ID_1      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 1};
Value Block_82_ID_2      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 2};
Value Block_82_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 3};
Value Block_82_ID_4      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 4};
Value Block_82_ID_5      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 5};
Value Block_82_ID_6      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 6};
Value Block_82_ID_7      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 7};
Value Block_82_ID_8      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 8};
Value Block_82_ID_9      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 9};
Value Block_82_ID_10      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 10};
Value Block_82_ID_11      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 11};
Value Block_82_ID_12      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 12};
Value Block_82_ID_13      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 13};
Value Block_82_ID_14      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 14};
Value Block_82_ID_15      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 15};
Value Block_82_ID_16      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 16};
Value Block_82_ID_17      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 17};
Value Block_82_ID_18      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 18};
Value Block_82_ID_19      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 19};
Value Block_82_ID_20      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 20};
Value Block_82_ID_21      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 21};
Value Block_82_ID_22      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 22};
Value Block_82_ID_23      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 23};
Value Block_82_ID_24      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 24};
Value Block_82_ID_25      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 25};
Value Block_82_ID_26      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 26};
Value Block_82_ID_27      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 27};
Value Block_82_ID_28      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 28};
Value Block_82_ID_29      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 29};
Value Block_82_ID_30      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 82, .ID = 30};

Value Block_83_ID_0      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 0 };
Value Block_83_ID_1      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 1};
Value Block_83_ID_2      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 2};
Value Block_83_ID_3      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 3};
Value Block_83_ID_4      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 4};
Value Block_83_ID_5      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 5};
Value Block_83_ID_6      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 6};
Value Block_83_ID_7      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 7};
Value Block_83_ID_8      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 8};
Value Block_83_ID_9      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 9};
Value Block_83_ID_10      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 10};
Value Block_83_ID_11      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 11};
Value Block_83_ID_12      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 12};
Value Block_83_ID_13      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 13};
Value Block_83_ID_14      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 14};
Value Block_83_ID_15      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 15};
Value Block_83_ID_16      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 16};
Value Block_83_ID_17      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 17};
Value Block_83_ID_18      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 18};
Value Block_83_ID_19      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 19};
Value Block_83_ID_20      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 20};
Value Block_83_ID_21      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 21};
Value Block_83_ID_22      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 22};
Value Block_83_ID_23      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 23};
Value Block_83_ID_24      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 24};
Value Block_83_ID_25      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 25};
Value Block_83_ID_26      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 26};
Value Block_83_ID_27      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 27};
Value Block_83_ID_28      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 28};
Value Block_83_ID_29      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 29};
Value Block_83_ID_30      = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 83, .ID = 30};

//Internal Variables - Block 200
Value Block_200_ID_0    = {.Block = 200, .ID = 0, .Prefix = VERSION_LONG, .NumValue = SOFTWARE_VERSION};     //INTERNAL_SW_VERSION
Value Block_200_ID_1    = {.Block = 200, .ID = 1, .Prefix = MASKED_BITMAP};            //INTERNAL_RCON
Value Block_200_ID_2    = {.Flag.Access_BLE = true, .Block = 200, .ID = 2, .Prefix = ERROR};            //INTERNAL_ERR_BUFF0
Value Block_200_ID_3    = {.Flag.Access_BLE = true, .Block = 200, .ID = 3, .Prefix = ERROR};            //INTERNAL_ERR_BUFF1
Value Block_200_ID_4    = {.Flag.Access_BLE = true, .Block = 200, .ID = 4, .Prefix = ERROR};            //INTERNAL_ERR_BUFF2
Value Block_200_ID_5    = {.Flag.Access_BLE = true, .Block = 200, .ID = 5, .Prefix = ERROR};            //INTERNAL_ERR_BUFF3
Value Block_200_ID_6    = {.Flag.Access_BLE = true, .Block = 200, .ID = 6, .Prefix = ERROR};            //INTERNAL_ERR_BUFF4
Value Block_200_ID_7    = {.Flag.Access_BLE = true, .Block = 200, .ID = 7, .Prefix = ERROR};            //INTERNAL_ERR_BUFF5
Value Block_200_ID_8    = {.Flag.Access_BLE = true, .Block = 200, .ID = 8, .Prefix = ERROR};            //INTERNAL_ERR_BUFF6
Value Block_200_ID_9    = {.Flag.Access_BLE = true, .Block = 200, .ID = 9, .Prefix = ERROR};            //INTERNAL_ERR_BUFF7
Value Block_200_ID_10   = {.Block = 200, .ID = 10, .Prefix = TIME_HHMMSS};           //INTERNAL_COM_TIMER
Value Block_200_ID_11   = {.Block = 200, .ID = 11, .Prefix = NULL};           //INTERNAL_STATE
Value Block_200_ID_12   = {.Block = 200, .ID = 12, .Prefix = NULL};           //INTERNAL_SUPPORT
Value Block_200_ID_13   = {.Block = 200, .ID = 13, .Prefix = NULL};           //C1CustomValues

#ifdef BLUETOOTH_CODE
//Bluetooth Variables
//volatile Value Block_200_ID_100 __attribute__ ((persistent, aligned(32))); //Bluetooth On (Bit 0) --- persistent to save value when going into standby; OVERWRITTEN BY BOOTLOADER
Value Block_200_ID_100  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 100, .Prefix = MASKED_BITMAP, .NumValue = 1<<16}; //Bluetooth On (Bit 0)
Value Block_200_ID_101  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 101, .Prefix = NULL, .NumValue = BLE_STATUS_NOT_CONNECTED}; //5 = disconnected
Value Block_200_ID_102  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 102, .Prefix = NULL}; //PassKey
Value Block_200_ID_103  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 103, .Prefix = VERSION_SHORT}; //Bluetooth Firmware Version

Value Block_200_ID_110  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 110, .Prefix = MAC}; //White listed MAC 1A
Value Block_200_ID_111  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 111, .Prefix = MAC}; //White listed MAC 1B
Value Block_200_ID_112  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 112, .Prefix = MAC}; //White listed MAC 2A
Value Block_200_ID_113  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 113, .Prefix = MAC}; //White listed MAC 2B
Value Block_200_ID_114  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 114, .Prefix = MAC}; //White listed MAC 3A
Value Block_200_ID_115  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 115, .Prefix = MAC}; //White listed MAC 3B
Value Block_200_ID_116  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 116, .Prefix = MAC}; //White listed MAC 4A
Value Block_200_ID_117  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 117, .Prefix = MAC}; //White listed MAC 4B
Value Block_200_ID_118  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 118, .Prefix = MAC}; //White listed MAC 5A
Value Block_200_ID_119  = {.Flag.Owned_BLE = true, .Block = 200, .ID = 119, .Prefix = MAC}; //White listed MAC 5B
#endif

//System Values
Value Block_241_ID_119  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 241, .ID = 119, .Prefix = PROCENT};       //Total system SOC
Value Block_241_ID_127  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 241, .ID = 127, .Prefix = POWER};
Value Block_242_ID_119  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 242, .ID = 119, .Prefix = PROCENT};       //Capacity Extension SOC

//Power Board Values - Block 250
Value Block_250_ID_222  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 250, .ID = 222, .Prefix = VERSION_SHORT};            //PWR_HWVERS
Value Block_250_ID_223  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 250, .ID = 223, .Prefix = VERSION_LONG};            //PWR_SWVERS
Value Block_250_ID_224  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 250, .ID = 224, .Prefix = NULL};                     //ASK Mads
Value Block_250_ID_225  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 250, .ID = 225, .Prefix = NULL};                     //ASK Mads
Value Block_250_ID_230  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 250, .ID = 230, .Prefix = VERSION_SHORT};            //PWR_BOOTVERSION

//DCDC Converter Variables - Block 251
Value Block_251_ID_223  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 251, .ID = 223, .Prefix = VERSION_LONG};            //DCDC_SWVERS
Value Block_251_ID_230  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 251, .ID = 230, .Prefix = VERSION_SHORT};            //BOOT_VERSION

//Display Values
Value Block_252_ID_230  = {.Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 252, .ID = 230, .Prefix = VERSION_SHORT}; // BOOT_VERSION

//Test Values
#ifdef ENABLE_TEST_VALS
Value Block_255_ID_0    = {.Block = 255, .ID = 0, .Prefix = NULL};   // TESTVAL_0
/*Value Block_255_ID_1    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 1, .Prefix = NULL};   // TESTVAL_1
Value Block_255_ID_2    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 2, .Prefix = NULL};   // TESTVAL_2
Value Block_255_ID_3    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 3, .Prefix = NULL};   // TESTVAL_3
Value Block_255_ID_4    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 4, .Prefix = NULL};   // TESTVAL_4
Value Block_255_ID_5    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 5, .Prefix = NULL};   // TESTVAL_5
Value Block_255_ID_6    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 6, .Prefix = NULL};   // TESTVAL_6
Value Block_255_ID_7    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 7, .Prefix = NULL};   // TESTVAL_7
Value Block_255_ID_8    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 8, .Prefix = NULL};   // TESTVAL_8
Value Block_255_ID_9    = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 9, .Prefix = NULL};   // TESTVAL_9
Value Block_255_ID_10   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 10, .Prefix = NULL};  // TESTVAL_10
Value Block_255_ID_11   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 11, .Prefix = NULL};  // TESTVAL_11
Value Block_255_ID_12   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 12, .Prefix = NULL};  // TESTVAL_12
Value Block_255_ID_13   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 13, .Prefix = NULL};  // TESTVAL_13
Value Block_255_ID_14   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 14, .Prefix = NULL};  // TESTVAL_14
Value Block_255_ID_15   = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 15, .Prefix = NULL};  // TESTVAL_15
Value Block_255_ID_255  = {.Flag.Owned_CTRL = true, .Flag.Access_BLE = true, .Flag.Access_CTRL = true, .Block = 255, .ID = 255, .Prefix = NULL}; // TESTVAL_ENABLE*/
#endif

Value* Values_GetValue(unsigned char U8_block, unsigned char U8_index) {
    // Declare a pointer to the Value structure
    Value* value = NULL;

    // Switch-case structure to handle different block numbers
    switch (U8_block) {
        case 0: // Block 0
            switch (U8_index) {
                case 70: value = &Block_0_ID_70; break;
                case 71: value = &Block_0_ID_71; break;
                case 72: value = &Block_0_ID_72; break;
                case 73: value = &Block_0_ID_73; break;
                case 74: value = &Block_0_ID_74; break;
                case 140: value = &Block_0_ID_140; break;
                case 142: value = &Block_0_ID_142; break;
                case 144: value = &Block_0_ID_144; break;
                case 146: value = &Block_0_ID_146; break;
                case 201: value = &Block_0_ID_201; break;
                case 205: value = &Block_0_ID_205; break;
                case 107: value = &Block_0_ID_107; break;
                case 105: value = &Block_0_ID_105; break;
                case 106: value = &Block_0_ID_106; break;
                case 226: value = &Block_0_ID_226; break;
                case 200: value = &Block_0_ID_200; break;
                case 204: value = &Block_0_ID_204; break;
                case 104: value = &Block_0_ID_104; break;
                case 102: value = &Block_0_ID_102; break;
                case 103: value = &Block_0_ID_103; break;
                case 208: value = &Block_0_ID_208; break;
                case 209: value = &Block_0_ID_209; break;
                case 78: value = &Block_0_ID_78; break;
                case 79: value = &Block_0_ID_79; break;
                case 203: value = &Block_0_ID_203; break;
                case 207: value = &Block_0_ID_207; break;
                case 113: value = &Block_0_ID_113; break;
                case 111: value = &Block_0_ID_111; break;
                case 112: value = &Block_0_ID_112; break;
                case 225: value = &Block_0_ID_225; break;
                case 202: value = &Block_0_ID_202; break;
                case 206: value = &Block_0_ID_206; break;
                case 110: value = &Block_0_ID_110; break;
                case 108: value = &Block_0_ID_108; break;
                case 109: value = &Block_0_ID_109; break;
                case 170: value = &Block_0_ID_170; break;
                case 120: value = &Block_0_ID_120; break;
                case 119: value = &Block_0_ID_119; break;
                case 127: value = &Block_0_ID_127; break;
                case 100: value = &Block_0_ID_100; break;
                case 101: value = &Block_0_ID_101; break;
                case 114: value = &Block_0_ID_114; break;
                case 1: value = &Block_0_ID_1; break;
                case 2: value = &Block_0_ID_2; break;
                case 3: value = &Block_0_ID_3; break;
                case 4: value = &Block_0_ID_4; break;
                case 124: value = &Block_0_ID_124; break;
                case 116: value = &Block_0_ID_116; break;
                case 117: value = &Block_0_ID_117; break;
                case 121: value = &Block_0_ID_121; break;
                case 122: value = &Block_0_ID_122; break;
                case 123: value = &Block_0_ID_123; break;
                case 220: value = &Block_0_ID_220; break;
                case 221: value = &Block_0_ID_221; break;
                case 222: value = &Block_0_ID_222; break;
                case 223: value = &Block_0_ID_223; break;
                case 157: value = &Block_0_ID_157; break;
                case 210: value = &Block_0_ID_210; break;
                case 212: value = &Block_0_ID_212; break;
                case 230: value = &Block_0_ID_230; break;
            }
            break;
        case 1: // Block 1
            switch (U8_index) {
                case 1: value = &Block_1_ID_1; break;
                case 2: value = &Block_1_ID_2; break;
                case 100: value = &Block_1_ID_100; break;
                case 101: value = &Block_1_ID_101; break;
                case 102: value = &Block_1_ID_102; break;
                case 110: value = &Block_1_ID_110; break;
                case 111: value = &Block_1_ID_111; break;
                case 112: value = &Block_1_ID_112; break;
                case 120: value = &Block_1_ID_120; break;
                case 121: value = &Block_1_ID_121; break;
                case 122: value = &Block_1_ID_122; break;
                case 123: value = &Block_1_ID_123; break;
                case 124: value = &Block_1_ID_124; break;
                case 125: value = &Block_1_ID_125; break;
                case 130: value = &Block_1_ID_130; break;
                case 131: value = &Block_1_ID_131; break;
                case 132: value = &Block_1_ID_132; break;
            }
            break;
        case 3: // Block 3
            switch (U8_index) {
                case 3: value = &Block_3_ID_3; break;
                case 0: value = &Block_3_ID_0; break;
                case 1: value = &Block_3_ID_1; break;
                case 4: value = &Block_3_ID_4; break;
                case 5: value = &Block_3_ID_5; break;
                case 6: value = &Block_3_ID_6; break;
                case 7: value = &Block_3_ID_7; break;
            }
            break;
        case 4: // Block 4
            switch (U8_index) {
                case 0: value = &Block_4_ID_0; break;
                case 3: value = &Block_4_ID_3; break;
            }
            break;
        case 6:
            switch (U8_index) {
                case 2: value = &Block_6_ID_2; break; // AC Out on C1
                case 3: value = &Block_6_ID_3; break;
                case 6: value = &Block_6_ID_6; break;
            }
            break;
        case 7: // Block 6
            switch (U8_index) {
                case 0: value = &Block_7_ID_0; break;
            }
            break;
        case 30: // Block 30
            switch (U8_index) {
                case 0: value = &Block_30_ID_0; break;
                case 1: value = &Block_30_ID_1; break;
                case 7: value = &Block_30_ID_7; break;
                case 12: value = &Block_30_ID_12; break;
                case 13: value = &Block_30_ID_13; break;
            }
            break;
        case 31: //Block 31
            switch(U8_index) {
                case 0: value = &Block_31_ID_0; break;    
                case 1: value = &Block_31_ID_1; break;  
                case 2: value = &Block_31_ID_2; break;
                case 3: value = &Block_31_ID_3; break;
                case 4: value = &Block_31_ID_4; break;
                case 5: value = &Block_31_ID_5; break;  
                case 6: value = &Block_31_ID_6; break;
                case 7: value = &Block_31_ID_7; break;
                case 8: value = &Block_31_ID_8; break;
                case 10: value = &Block_31_ID_10; break;
            }
            break;
        case 40: // Block 40
            switch (U8_index) {
                case 0: value = &Block_40_ID_0; break;
                case 1: value = &Block_40_ID_1; break;
                case 2: value = &Block_40_ID_2; break;
            }
            break;
        case 50: // Block 50
            switch (U8_index) {
                case 0: value = &Block_50_ID_0; break;
                case 1: value = &Block_50_ID_1; break;
                case 2: value = &Block_50_ID_2; break;
            }
            break;
        case 60: // Block 60
            switch (U8_index) {
                case 2: value = &Block_60_ID_2; break;
            }
            break;
        case 70: // Block 70
            switch (U8_index) {
                case 0: value = &Block_70_ID_0; break;
            }
            break;
        case 71: // Block 71
            switch (U8_index) {
                case 0: value = &Block_71_ID_0; break;
                case 1: value = &Block_71_ID_1; break;
                case 2: value = &Block_71_ID_2; break;
            }
            break;
        
        case 82: 
            switch (U8_index) {
                case 0: value = &Block_82_ID_0; break;
                case 1: value = &Block_82_ID_1; break;
                case 2: value = &Block_82_ID_2; break;
                case 3: value = &Block_82_ID_3; break;
                case 4: value = &Block_82_ID_4; break;
                case 5: value = &Block_82_ID_5; break;
                case 6: value = &Block_82_ID_6; break;
                case 7: value = &Block_82_ID_7; break;
                case 8: value = &Block_82_ID_8; break;
                case 9: value = &Block_82_ID_9; break;
                case 10: value = &Block_82_ID_10; break;
                case 11: value = &Block_82_ID_11; break;
                case 12: value = &Block_82_ID_12; break;
                case 13: value = &Block_82_ID_13; break;
                case 14: value = &Block_82_ID_14; break;
                case 15: value = &Block_82_ID_15; break;
                case 16: value = &Block_82_ID_16; break;
                case 17: value = &Block_82_ID_17; break;
                case 18: value = &Block_82_ID_18; break;
                case 19: value = &Block_82_ID_19; break;
                case 20: value = &Block_82_ID_20; break;
                case 21: value = &Block_82_ID_21; break;
                case 22: value = &Block_82_ID_22; break;
                case 23: value = &Block_82_ID_23; break;
                case 24: value = &Block_82_ID_24; break;
                case 25: value = &Block_82_ID_25; break;
                case 26: value = &Block_82_ID_26; break;
                case 27: value = &Block_82_ID_27; break;
                case 28: value = &Block_82_ID_28; break;
                case 29: value = &Block_82_ID_29; break;
                case 30: value = &Block_82_ID_30; break;
            }
            break;
        case 83:
            switch (U8_index) {
                case 0: value = &Block_83_ID_0; break;
                case 1: value = &Block_83_ID_1; break;
                case 2: value = &Block_83_ID_2; break;
                case 3: value = &Block_83_ID_3; break;
                case 4: value = &Block_83_ID_4; break;
                case 5: value = &Block_83_ID_5; break;
                case 6: value = &Block_83_ID_6; break;
                case 7: value = &Block_83_ID_7; break;
                case 8: value = &Block_83_ID_8; break;
                case 9: value = &Block_83_ID_9; break;
                case 10: value = &Block_83_ID_10; break;
                case 11: value = &Block_83_ID_11; break;
                case 12: value = &Block_83_ID_12; break;
                case 13: value = &Block_83_ID_13; break;
                case 14: value = &Block_83_ID_14; break;
                case 15: value = &Block_83_ID_15; break;
                case 16: value = &Block_83_ID_16; break;
                case 17: value = &Block_83_ID_17; break;
                case 18: value = &Block_83_ID_18; break;
                case 19: value = &Block_83_ID_19; break;
                case 20: value = &Block_83_ID_20; break;
                case 21: value = &Block_83_ID_21; break;
                case 22: value = &Block_83_ID_22; break;
                case 23: value = &Block_83_ID_23; break;
                case 24: value = &Block_83_ID_24; break;
                case 25: value = &Block_83_ID_25; break;
                case 26: value = &Block_83_ID_26; break;
                case 27: value = &Block_83_ID_27; break;
                case 28: value = &Block_83_ID_28; break;
                case 29: value = &Block_83_ID_29; break;
                case 30: value = &Block_83_ID_30; break;
            }
            break;
        case 200: // Block 200
            switch (U8_index) {
                case 0: value = &Block_200_ID_0; break;
                case 1: value = &Block_200_ID_1; break;
                case 2: value = &Block_200_ID_2; break;
                case 3: value = &Block_200_ID_3; break;
                case 4: value = &Block_200_ID_4; break;
                case 5: value = &Block_200_ID_5; break;
                case 6: value = &Block_200_ID_6; break;
                case 7: value = &Block_200_ID_7; break;
                case 8: value = &Block_200_ID_8; break;
                case 9: value = &Block_200_ID_9; break;
                case 10: value = &Block_200_ID_10; break;
                case 11: value = &Block_200_ID_11; break;
                case 12: value = &Block_200_ID_12; break;
                
                #ifdef BLUETOOTH_CODE
                case 100: value = &Block_200_ID_100; break;
                case 101: value = &Block_200_ID_101; break;
                case 102: value = &Block_200_ID_102; break;
                case 103: value = &Block_200_ID_103; break;
                case 110: value = &Block_200_ID_110; break;
                case 111: value = &Block_200_ID_111; break;
                case 112: value = &Block_200_ID_112; break;
                case 113: value = &Block_200_ID_113; break;
                case 114: value = &Block_200_ID_114; break;
                case 115: value = &Block_200_ID_115; break;
                case 116: value = &Block_200_ID_116; break;
                case 117: value = &Block_200_ID_117; break;
                case 118: value = &Block_200_ID_118; break;
                case 119: value = &Block_200_ID_119; break;
                #endif
            }
            break;
        case 241:
            switch (U8_index) {
                case 119: value = &Block_241_ID_119; break;
                case 127: value = &Block_241_ID_127; break;
            }
            break;
        case 242:
            switch (U8_index) {
                case 119: value = &Block_242_ID_119; break;
            }
            break;
        case 250: // Block 250
            switch (U8_index) {
                case 222: value = &Block_250_ID_222; break;
                case 223: value = &Block_250_ID_223; break;
                case 224: value = &Block_250_ID_224; break;
                case 225: value = &Block_250_ID_225; break;
                case 230: value = &Block_250_ID_230; break;
            }
            break;
        case 251: // Block 251
            switch (U8_index) {
                case 223: value = &Block_251_ID_223; break;
                case 230: value = &Block_251_ID_230; break;
            }
            break;
        case 252: // Block 252
            switch (U8_index) {
                case 230: value = &Block_252_ID_230; break;
            }
            break;
        #ifdef ENABLE_TEST_VALS
        case 255: // Block 255
            switch (U8_index) {
                case 0: value = &Block_255_ID_0; break;
                /*case 1: value = &Block_255_ID_1; break;
                case 2: value = &Block_255_ID_2; break;
                case 3: value = &Block_255_ID_3; break;
                case 4: value = &Block_255_ID_4; break;
                case 5: value = &Block_255_ID_5; break;
                case 6: value = &Block_255_ID_6; break;
                case 7: value = &Block_255_ID_7; break;
                case 8: value = &Block_255_ID_8; break;
                case 9: value = &Block_255_ID_9; break;
                case 10: value = &Block_255_ID_10; break;
                case 11: value = &Block_255_ID_11; break;
                case 12: value = &Block_255_ID_12; break;
                case 13: value = &Block_255_ID_13; break;
                case 14: value = &Block_255_ID_14; break;
                case 15: value = &Block_255_ID_15; break;
                case 255: value = &Block_255_ID_255; break;*/
            }
            break;
        #endif
    }

    // Return the pointer to the Value structure
    return value;
}

void Values_Send_ByValue(uint8_t CMD, Value *Val, uint8_t UartSelect)
{
    unsigned char Data[7];
    Data[0]=CMD;
    Data[1]=Val->Block;
    Data[2]=Val->ID;
    Data[3]=(unsigned char)(Val->NumValue);
    Data[4]=(unsigned char)(Val->NumValue>>8);
    Data[5]=(unsigned char)(Val->NumValue>>16);
    Data[6]=(unsigned char)(Val->NumValue>>24);
    SendMSG(&Data[0], sizeof(Data), UartSelect);
}

bool Values_RequestByValue(Value *Val, bool UpdateIfSet, char SelectValue)
{
    // Ensure communication goes to correct module
    char UART = UART_UNDEFINED;
    if((Val->Flag.Owned_CTRL) || (Val->Prefix == ERROR)) {
        UART = UART_CTRL;
        //Return "true" if value already received and update is not required
        switch(SelectValue)
        {
            case GET_VAL:
                if(Val->Flag.NumValue_Received && !UpdateIfSet)
                    return true;
            break;

            case GET_MIN_MAX:
                if(Val->Flag.NumMinMax_Received && !UpdateIfSet)
                    return true;
            break;

            case GET_DEFAULT:
                if(Val->Flag.NumDefault_Received && !UpdateIfSet)
                    return true;
            break;

            default:
                return false;
            break;
        }
    }
    #ifdef BLUETOOTH_CODE
    else if (Val->Flag.Owned_BLE){
        UART = UART_BLE;
        if(Block_200_ID_101.NumValue < BLE_STATUS_NOT_CONNECTED){
            if(SelectValue == GET_VAL){
                if(Val->Flag.NumValue_Received && !UpdateIfSet)
                    return true;
            }
            else
                return false;   
        }
        else //if(Val != &Block_200_ID_103)
           return false;
    }
    #endif
    else
        return false;

    //If requested value is powerboard software version request multipe values
    if((Val->Block == 250) && (Val->ID == PWR_SWVERS)){
        unsigned char Data[3]={SelectValue,250,223};
        SendMSG(&Data[0], sizeof(Data), UART);

        unsigned char Data2[3]={SelectValue,250,224};
        SendMSG(&Data2[0], sizeof(Data2), UART);

        unsigned char Data3[3]={SelectValue,250,225};
        SendMSG(&Data3[0], sizeof(Data3), UART);
    }
    else if (Val->Prefix == ERROR) {
        unsigned char Data = 0x41;
        SendMSG(&Data, 1, UART_CTRL);
    }
    else {
        Val->Flag.NumDefault_Received = false;
        unsigned char Data[3]={SelectValue,Val->Block,Val->ID};
        SendMSG(&Data[0], sizeof(Data), UART);
    }
    
    //Return false to incicate that value is still not received
    return false;
}

bool Values_RequestInitial()
{
    bool InitOK = true;
    
    //Request Model Number
    if(ModelValue[0]==0)
    {
        InitOK = false;
        unsigned char Data = 0x42;
        SendMSG(&Data, sizeof(Data), UART_CTRL);
    }
    
    //Request Hardware Revision
    InitOK &= Values_RequestByValue(&Block_0_ID_222, false, GET_VAL);
    //Request Software Revision
    InitOK &= Values_RequestByValue(&Block_0_ID_223, false, GET_VAL);
    
    
    //Request System Operation State
    InitOK &= Values_RequestByValue(&Block_0_ID_212, false, GET_VAL);
    
    InitOK &= Values_RequestByValue(&Block_0_ID_119, false, GET_VAL);
    
    //Request Main View Skin (Simple / Complex)
    InitOK &= Values_RequestByValue(&Block_3_ID_7, false, GET_VAL);
    
    //Request Main View power bounderies
    
    // AC In Power
    InitOK &= Values_RequestByValue(&Block_1_ID_100, false, GET_VAL); 
    InitOK &= Values_RequestByValue(&Block_1_ID_101, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_102, false, GET_VAL); 

    // AC Out Power
    InitOK &= Values_RequestByValue(&Block_1_ID_110, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_111, false, GET_VAL); 
    InitOK &= Values_RequestByValue(&Block_1_ID_112, false, GET_VAL); 
    
    // DC IN Current
    InitOK &= Values_RequestByValue(&Block_1_ID_120, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_121, false, GET_VAL); 
    InitOK &= Values_RequestByValue(&Block_1_ID_122, false, GET_VAL);
    
    // DC IN Power
    InitOK &= Values_RequestByValue(&Block_1_ID_123, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_124, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_125, false, GET_VAL);
    
    // DC Out Current
    InitOK &= Values_RequestByValue(&Block_1_ID_130, false, GET_VAL); 
    InitOK &= Values_RequestByValue(&Block_1_ID_131, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_1_ID_132, false, GET_VAL); 
    
    //Active on Control Board Version > Version 22 
    InitOK &= Values_RequestByValue(&Block_3_ID_5, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_3_ID_3, false, GET_VAL);
    InitOK &= Values_RequestByValue(&Block_3_ID_4, false, GET_VAL);
    
    //Max In Current
    InitOK &= Values_RequestByValue(&Block_60_ID_2, false, GET_MIN_MAX);
    
    //Active on Control Board Version > Version XX
    InitOK &= Values_RequestByValue(&Block_3_ID_6, false, GET_VAL);
    
    //Error buffer needs to be instantiated, otherwise display will crash when entering the error buffer menu.
    //\TODO: Find out why the error buffer menu crashes if error buffer is not instantiated
    InitOK &= Values_RequestByValue(&Block_200_ID_2, false, GET_VAL);
    
    //Enable Test Variables
    #ifdef ENABLE_TEST_VALS
    /*InitOK &= Values_RequestByValue(&Block_255_ID_255, false, GET_VAL);*/
    #endif
    
    return InitOK;
}

