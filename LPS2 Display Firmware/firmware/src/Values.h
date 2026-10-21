#include <stdbool.h>                    // Defines true
#include "State_Statemachine.h"
#include "Menu_Error.h"
#include "peripheral/gpio/plib_gpio.h"
#include "Menu_MultiSettings.h"

#ifndef _MENU_VALUES_H_
#define _MENU_VALUES_H_

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
#define SET_BIT(var, pos) ((var) |= (1 << (pos)))
#define CLEAR_BIT(var, pos) ((var) &= ~(1 << (pos)))

//#define ENABLE_TEST_VALS

//#define DEMO
//99 = Prototype
//01 = Device 1
//02 = Device 2
//03 = Device 3
//04 = Device 4

//Do not add zero in front
#if defined(__32MX130F256D__)
    #define UC_VERSION 20000 //Device NR2
#elif defined(__32MX170F256D__)
    #define UC_VERSION 30000 //Device NR3
#elif defined(__32MX230F256D__)
    #define UC_VERSION 40000 //Device NR4
#endif

#define SOFTWARE_VERSION    UC_VERSION+426 //Do not add zero in front
//#define SOFTWARE_VERSION    UC_VERSION+1494  //DEBUG RELEASE

//Use OS_ON as the critaria if something is running
//----------------------------------------------------------------
//	Defined Function Operating states
//----------------------------------------------------------------
#define	OS_DISABLED					0xFFFF0000
#define	OS_OFF						0x00000
#define	OS_WAKEUP					0x10000
#define	OS_READY_TO_START			0x20000
#define	OS_STARTING					0x30000
#define OS_STOPPING					0x40000
#define OS_ON						0x50000


//----------------------------------------------------------------
//	Defined Testmode states
//----------------------------------------------------------------
#define TM_NORMAL               0x00000
#define TM_BIOS                 0x20000
#define TM_APPLICATION          0x30000
#define TM_NOPROTECTION         0x40000

//If failure level is above WARNING the it is blocked.
//ACIN and DCIN can still run when errorlevel is FL_EMPTY
//----------------------------------------------------------------
//	Defined Failure Levels
//----------------------------------------------------------------
#define	FL_OK						0x00000
#define	FL_WARNING					0x10000
#define FL_SIMPLE_FAILURE			0x20000
#define FL_EMPTY					0x30000
#define FL_CRITICAL_FAILURE         0x40000

//----------------------------------------------------------------
//	Defined Battery operation state
//----------------------------------------------------------------

//Battery Statuses
#define BS_UNKNOWN              0x0
#define BS_IDLE                 0x10000		//No charging and no current.
#define BS_DISCHARGE            0x20000		//Discharging.
#define BS_CHARGE               0x30000		//Charging.
#define BS_FULL                 0x40000		//Charging and battery full.
#define BS_BALANCING            0x50000     //Charging but battery is balancing and recommended to leaving LPS charging.
#define BS_EMPTY                0xFFFF0000  //Battery Empty



//////////////// CONTROL BOARD VALUES //////////////////////////////
//PREFIX Defines - CTRL
#define VOLTAGE         1
#define CURRENT         2
#define TEMPERATURE     3
#define PROCENT         4
#define TIME_HEAD       5
#define TIME_HHMM       6
#define TIME_HHMMSS     7
#define ERROR           8
#define POWER           9
#define SERIAL          11
#define VERSION_LONG    12
#define VERSION_SHORT   13
#define DATE            14
#define STRING          15
#define STATE_OPERATION 16
#define STATE_FAILURE   17
#define STATE_BATTERY   18
#define OHM             19
#define KWH             20
#define MASKED_BITMAP   21
#define STATE_TESTMODE  23
#define MAC             24
#define PASSKEY         25


//Global Operation Status
//#define SYS_OPSTATE           211
#define SYS_TESTMODE          212
#define BATSTATUS             210
#define WAKEUPFLAGS           157

//Energy Meter
#define NRGMETER_ACIN         140
#define NRGMETER_DCIN         142
#define NRGMETER_DCOUT        144
#define NRGMETER_SOLAR        146

//IO Voltages
#define IO1_REMOTE            70
#define IO2_DATA              71
#define IO3_DATAFRONT         72
#define C2_TERMINAL           73
#define C1_TERMINAL           74

//AC OUTPUT VALUES
#define ACOUT_OPSTATE         201
#define ACOUT_FLSTATE         205
#define ACOUT_WATT            107
#define ACOUT_VOLT            105
#define ACOUT_AMP             106
#define ACOUT_AUTO_TIMEGLOBAL 226
//SETTINGS - ACOutput 
#define ACOUT_SAVER_TIME       1
#define ACOUT_SAVER_LIMIT      2

//AC INPUT VALUES
#define ACIN_OPSTATE          200
#define ACIN_FLSTATE          204
#define ACIN_WATT             104
#define ACIN_VOLT             102
#define ACIN_AMP              103
// SETTINGS - ACInput 
#define ACIN_MAX_CURRENT      2

//SOLAR VALUES
#define SOLAR_OPSTATE         208
#define SOLAR_FLSTATE         209
#define SOLAR_AMP             78
#define SOLAR_WATT            79
// SETTINGS - SOLAR
#define SOLAR_SET_OPERATION_MODE  0
#define SOLAR_SET_INPUT_VOLT  4
#define SOLAR_SELFLEARN_OC_VOLT     0
#define SOLAR_SELFLEARN_MPP_VOLT    1
#define SOLAR_SELFLEARN_START_VOLT  2

//DC OUTPUT VALUES
#define DCOUT_OPSTATE         203
#define DCOUT_FLSTATE         207
#define DCOUT_WATT            113
#define DCOUT_VOLT            111
#define DCOUT_AMP             112
#define DCOUT_AUTO_TIMEGLOBAL 225
// SETTINGS - DCOutput
#define DCOUT_STB_TIME        0
#define DCOUT_SAVER_TIME      1
#define DCOUT_SAVER_CURRENT   2



//DC INPUT VALUES
#define DCIN_OPSTATE          202
#define DCIN_FLSTATE          206
#define DCIN_WATT             110
#define DCIN_VOLT             108
#define DCIN_AMP              109
#define DCIN_OPERATING_VOLTAGE  170
// SETTINGS - DCIN 
#define DCIN_CONTROL_BITMAP      0
#define DCIN_SET_OPERATION_MODE  1
#define DCIN_SET_INPUT_CURR      7
#define DCIN_SET_START_VOLTAGE   12
#define DCIN_SET_STOP_VOLTAGE    13

//BATTERY STATUS VALUES
#define BATSTATUS_REMTIME     120
#define BATSTATUS_SOC         119
#define BATSTATUS_WATT        127
#define BATSTATUS_VOLT        100
#define BATSTATUS_AMP         101
#define BATSTATUS_TEMP        114
#define BATSTATUS_CELL1       1 
#define BATSTATUS_CELL2       2 
#define BATSTATUS_CELL3       3 
#define BATSTATUS_CELL4       4 
#define BATSTATUS_CYCLES      124


//TEMPERATURE VALUES
#define TEMPERATURE_IGBT      116
#define TEMPERATURE_TRAFO     117
#define TEMPERATURE_CELL12    121
#define TEMPERATURE_CELL23    122
#define TEMPERATURE_CELL34    123

//ABOUT VALUES
#define ABOUT_SERIAL          220
#define ABOUT_MANUDATE        221
#define ABOUT_HWVERS          222
#define ABOUT_SWVERSLPS       223

//NON Indexed Values (Does not use the index number for anything!)
#define ABOUT_MODEL           219

// SETTINGS - DISPLAY 
#define DISP_LIGHT_CRG        0
#define DISP_LIGHT_DISCRG     1
#define DISP_BITMAP1           3
#define DISP_LOCK             4
#define DISP_CONTRAST         5
#define DISP_SPLASH           6
#define DISP_VIEW             7
#define DISP_BLE_NAME         10


////////////////// FUNTIONS           ////////////////////////////////////////
#define FUNC_JUMPSTART        1
#define FUNC_ACIN_MAX         100
#define FUNC_ACIN_MIN         101
#define FUNC_ACIN_HYST        102
#define FUNC_ACOUT_MAX        110
#define FUNC_ACOUT_MIN        111
#define FUNC_ACOUT_HYST       112
#define FUNC_DCIN_MAX         120
#define FUNC_DCIN_MIN         121
#define FUNC_DCIN_HYST        122
#define FUNC_DCOUT_MAX        130
#define FUNC_DCOUT_MIN        131
#define FUNC_DCOUT_HYST       132

////////////////// POWER BOARD VALUES ////////////////////////////////////////
#define PWR_HWVERS             222
#define PWR_SWVERS             223
#define PWR_224ASK_MADS        224
#define PWR_225ASK_MADS        225
#define BOOT_VERSION           230

////////////////// DCDC BOARD VALUES ////////////////////////////////////////
#define DCDC_HWVERS             222
#define DCDC_SWVERS             223

////////////////// Internal_Variables - BLOCK 200//////////////////////////////
#define INTERNAL_ERR_BUFF0      2  
#define INTERNAL_ERR_BUFF1      3
#define INTERNAL_ERR_BUFF2      4
#define INTERNAL_ERR_BUFF3      5
#define INTERNAL_ERR_BUFF4      6
#define INTERNAL_ERR_BUFF5      7
#define INTERNAL_ERR_BUFF6      8
#define INTERNAL_ERR_BUFF7      9
#define INTERNAL_SW_VERSION     0
#define INTERNAL_RCON           1
#define INTERNAL_COM_TIMER      10
#define INTERNAL_STATE          11
#define INTERNAL_SUPPORT        12
#define INTERNAL_HW_VERSION     13
#define INTERNAL_BOOT_VERSION   14

////////////////// Test Vals //////////////////////////////
#define TESTVAL_0               0
#define TESTVAL_1               1
#define TESTVAL_2               2
#define TESTVAL_3               3
#define TESTVAL_4               4
#define TESTVAL_5               5
#define TESTVAL_6               6
#define TESTVAL_7               7
#define TESTVAL_8               8
#define TESTVAL_9               9
#define TESTVAL_10             10
#define TESTVAL_11             11
#define TESTVAL_12             12
#define TESTVAL_13             13
#define TESTVAL_14             14
#define TESTVAL_15             15
#define TESTVAL_ENABLE         255

#define BIT                     true
#define VALUE                   false

//Check
#define GET_VAL             0x20
#define GET_MIN_MAX         0x22
#define GET_DEFAULT         0x24
#define SET_VAL             0x50

#define POPUP_SETTING                0
#define POPUP_SETTING_MULTI          4
//#define POPUP_QR_REGISTER          1
#define POPUP_ERROR                  2
#define POPUP_LOCK_SET               3

#define MENU_HIDE 0
#define MENU_SHOW 1
#define MENU_HIDE_IF_NULL_OR_0 2

////////////////////////////// STRUCTS ////////////////////////////////////////
union APP_UPDATE_FLAGS_UNION {
    struct {
        unsigned Update                        :1;            //Bit 0        - Timer for 1 sec and in case of events
        unsigned Init                          :1;            //Bit 1        - Used to init display for Old firmware
        unsigned Stop                          :1;            //Bit 2        - Used to stop display for Old firmware
        unsigned KeyPressPwr                   :1;            //Bit 3        - Power button Key Press
        unsigned KeyPressDisp                  :1;            //Bit 4        - Display controlled button Key Press
        unsigned NotUsed                       :1;            //Bit 5        - Unused
        unsigned DeepSleep                     :1;            //Bit 6        - Display must enter deepsleep mode; not standby
        unsigned State_ON                      :1;            //Bit 7        - ON / OFF bit for display - controlled by App_ucOperatingState > OS_SLEEP
    };
    unsigned char Byte[1];
};
typedef union APP_UPDATE_FLAGS_UNION             APP_UPDATE_FLAGS;

typedef union APP_WAKEUP_FLAGS_UNION {
	struct {
		unsigned Wakeup_Mains					:1;		//Bit 0		- OK
		unsigned Wakeup_Update					:1;		//Bit 1					// (Old usage: LPS I = Power button)
		unsigned Wakeup_IN_Terminal				:1;		//Bit 2		- OK		//C1 input (D+ or Clamp 15)
		unsigned G3_Combi_230vacMains			:1;		//Bit 3					//(Enable/Disable)	Communication controlled. Message from G3 combi accepting 230vac mains voltage
		unsigned Wakeup_Blocked					:1;		//Bit 4					//
		unsigned Wakeup_Solar					:1;		//Bit 5					//Reserved for later use
		unsigned Wakeup_Button_230V				:1;		//Bit 6		- OK
		unsigned Wakeup_Button_12V				:1;		//Bit 7		- OK
 
		unsigned Wakeup_IO_Terminal				:1;		//Bit 8		- OK		//C2
		unsigned Wakeup_IO_1					:1;		//Bit 9		- OK
		unsigned Wakeup_IO_2					:1;		//Bit 10	- OK
		unsigned Wakeup_IO_3					:1;		//Bit 11	- OK
 
		unsigned Output_IO_1					:1;		//Bit 12
		unsigned Output_IO_2					:1;		//Bit 13
		unsigned Output_IO_3					:1;		//Bit 14
		unsigned NC_Bit_Signed					:1;		//Bit 15				//Bit cannot be used 
	};
	unsigned char Byte[2];
	unsigned int Int;
} APP_WAKEUP_FLAGS;

typedef struct __attribute__((packed)) Value {
    union {
        long NumValue;           // Access the value as a long
        unsigned char NumValue_Bytes[4]; // Access the value as 4 bytes
    };
    long NumMin;
    long NumMax;
    long NumDefault;
    unsigned char Prefix;
    unsigned char Block;
    unsigned char ID;
    struct {
        unsigned char BlockedBySetting  : 1;
        unsigned char NumMinMax_Received : 1;
        unsigned char NumValue_Received : 1;
        unsigned char NumDefault_Received : 1;
        unsigned char Access_BLE : 1;        //Returns value to BLE  (UART 2)
        unsigned char Access_CTRL : 1;                    //Returns value to CTRL (UART 1)
        unsigned char Owned_BLE : 1;           //Indicates if Bluetooth is owner; in that case controlboard cannot change the Block ID data.
        unsigned char Owned_CTRL : 1;
    } Flag;
} Value;
 
//TODO: Change all long to SLONG
typedef union slong {
    unsigned char Bytes[4];
    struct {
        unsigned int ILow;
        signed int IHigh;
    };
    signed long L;
} SLONG;

#define EXTENDED_POINTERS
typedef void (*PTR_Function)();
typedef struct __attribute__((packed)) Menu_Popup {   

#ifdef EXTENDED_POINTERS
    const PTR_Function Function_Entry;
#endif
    const PTR_Function Function_Cancel;
    const PTR_Function Function_Change;
    const PTR_Function Function_Accept;
    const unsigned char *Description;
    const long StepFactor_Slow;
    const long StepFactor_Fast;
    const long JumpToMaxVal;
    const unsigned char Type;
    const bool CodeProtect;
}Menu_Popup;

typedef struct __attribute__((packed)) Menu_Match {   
    const long MatchVal;              //Used for matching bits (Bitmap)
    const unsigned char *MatchString; //Display String on match
}Menu_Match;

typedef struct __attribute__((packed)) Menu_Value {
    Value * const Data;  
    const unsigned char *NumbToString_NoMatch;
    const struct Menu_Match *NumbToString;
    const unsigned char Decimals;
    const bool Show;
}Menu_Value;

typedef struct __attribute__((packed)) Menu {
    const unsigned char     *S8A_Title; 
    const Menu_Value        *ST_Value;
    const Menu_Popup        *ST_Popup;
    struct Menu             **ST_Children;
    unsigned char           MarkerChildIndex;       //Count with hidden menues
    unsigned char           MarkerWindowIndex;
    unsigned char           Show;
    unsigned char           Shift;
} Menu;


////////////////////////EXTERN VARIABLES////////////////////////////////////////
extern Value Block_0_ID_70;
extern Value Block_0_ID_71;
extern Value Block_0_ID_72;
extern Value Block_0_ID_73;
extern Value Block_0_ID_74;
extern Value Block_0_ID_140;
extern Value Block_0_ID_142;
extern Value Block_0_ID_144;
extern Value Block_0_ID_146;
extern Value Block_0_ID_201;
extern Value Block_0_ID_205;
extern Value Block_0_ID_107;
extern Value Block_0_ID_105;
extern Value Block_0_ID_106;
extern Value Block_0_ID_226;
extern Value Block_0_ID_200;
extern Value Block_0_ID_204;
extern Value Block_0_ID_104;
extern Value Block_0_ID_102;
extern Value Block_0_ID_103;
extern Value Block_0_ID_208;
extern Value Block_0_ID_209;
extern Value Block_0_ID_78;
extern Value Block_0_ID_79;
extern Value Block_0_ID_203;
extern Value Block_0_ID_207;
extern Value Block_0_ID_113;
extern Value Block_0_ID_111;
extern Value Block_0_ID_112;
extern Value Block_0_ID_225;
extern Value Block_0_ID_202;
extern Value Block_0_ID_206;
extern Value Block_0_ID_110;
extern Value Block_0_ID_108;
extern Value Block_0_ID_109;
extern Value Block_0_ID_170;
extern Value Block_0_ID_120;
extern Value Block_0_ID_119;
extern Value Block_0_ID_127;
extern Value Block_0_ID_100;
extern Value Block_0_ID_101;
extern Value Block_0_ID_114;
extern Value Block_0_ID_1;
extern Value Block_0_ID_2;
extern Value Block_0_ID_3;
extern Value Block_0_ID_4;
extern Value Block_0_ID_124;
extern Value Block_0_ID_116;
extern Value Block_0_ID_117;
extern Value Block_0_ID_121;
extern Value Block_0_ID_122;
extern Value Block_0_ID_123;
extern Value Block_0_ID_220;
extern Value Block_0_ID_221;
extern Value Block_0_ID_222;
extern Value Block_0_ID_223;
extern Value Block_0_ID_157;
extern Value Block_0_ID_210;
extern Value Block_0_ID_212;
extern Value Block_0_ID_230;

extern Value Block_1_ID_1;
extern Value Block_1_ID_2;
extern Value Block_1_ID_3;
extern Value Block_1_ID_100;
extern Value Block_1_ID_101;
extern Value Block_1_ID_102;
extern Value Block_1_ID_110;
extern Value Block_1_ID_111;
extern Value Block_1_ID_112;
extern Value Block_1_ID_120;
extern Value Block_1_ID_121;
extern Value Block_1_ID_122;
extern Value Block_1_ID_123;
extern Value Block_1_ID_124;
extern Value Block_1_ID_125;
extern Value Block_1_ID_130;
extern Value Block_1_ID_131;
extern Value Block_1_ID_132;

extern Value Block_3_ID_3;
extern Value Block_3_ID_0;
extern Value Block_3_ID_1;
extern Value Block_3_ID_4;
extern Value Block_3_ID_5;
extern Value Block_3_ID_6;
extern Value Block_3_ID_7;
extern Value Block_3_ID_10;

extern Value Block_4_ID_0;
extern Value Block_4_ID_3;

extern Value Block_6_ID_2;
extern Value Block_6_ID_3;
extern Value Block_6_ID_6;

extern Value Block_7_ID_0;

extern Value Block_30_ID_0;
extern Value Block_30_ID_1;
extern Value Block_30_ID_7;
extern Value Block_30_ID_12;
extern Value Block_30_ID_13;

extern Value Block_31_ID_0;
extern Value Block_31_ID_1;
extern Value Block_31_ID_2;
extern Value Block_31_ID_3;
extern Value Block_31_ID_4;
extern Value Block_31_ID_5;
extern Value Block_31_ID_6;
extern Value Block_31_ID_7;
extern Value Block_31_ID_8;
extern Value Block_31_ID_10;

extern Value Block_40_ID_0;
extern Value Block_40_ID_1;
extern Value Block_40_ID_2;

extern Value Block_50_ID_0;
extern Value Block_50_ID_1;
extern Value Block_50_ID_2;

extern Value Block_60_ID_2;

extern Value Block_70_ID_0;

extern Value Block_71_ID_0;
extern Value Block_71_ID_1;
extern Value Block_71_ID_2;

extern Value Block_82_ID_0;
extern Value Block_82_ID_1;
extern Value Block_82_ID_2;
extern Value Block_82_ID_3;
extern Value Block_82_ID_4;
extern Value Block_82_ID_5;
extern Value Block_82_ID_6;
extern Value Block_82_ID_7;
extern Value Block_82_ID_8;
extern Value Block_82_ID_9;
extern Value Block_82_ID_10;
extern Value Block_82_ID_11;
extern Value Block_82_ID_12;
extern Value Block_82_ID_13;
extern Value Block_82_ID_14;
extern Value Block_82_ID_15;
extern Value Block_82_ID_16;
extern Value Block_82_ID_17;
extern Value Block_82_ID_18;
extern Value Block_82_ID_19;
extern Value Block_82_ID_20;
extern Value Block_82_ID_21;
extern Value Block_82_ID_22;
extern Value Block_82_ID_23;
extern Value Block_82_ID_24;
extern Value Block_82_ID_25;
extern Value Block_82_ID_26;
extern Value Block_82_ID_27;
extern Value Block_82_ID_28;
extern Value Block_82_ID_29;
extern Value Block_82_ID_30;

extern Value Block_83_ID_0;
extern Value Block_83_ID_1;
extern Value Block_83_ID_2;
extern Value Block_83_ID_3;
extern Value Block_83_ID_4;
extern Value Block_83_ID_5;
extern Value Block_83_ID_6;
extern Value Block_83_ID_7;
extern Value Block_83_ID_8;
extern Value Block_83_ID_9;
extern Value Block_83_ID_10;
extern Value Block_83_ID_11;
extern Value Block_83_ID_12;
extern Value Block_83_ID_13;
extern Value Block_83_ID_14;
extern Value Block_83_ID_15;
extern Value Block_83_ID_16;
extern Value Block_83_ID_17;
extern Value Block_83_ID_18;
extern Value Block_83_ID_19;
extern Value Block_83_ID_20;
extern Value Block_83_ID_21;
extern Value Block_83_ID_22;
extern Value Block_83_ID_23;
extern Value Block_83_ID_24;
extern Value Block_83_ID_25;
extern Value Block_83_ID_26;
extern Value Block_83_ID_27;
extern Value Block_83_ID_28;
extern Value Block_83_ID_29;
extern Value Block_83_ID_30;

extern Value Block_200_ID_0;
extern Value Block_200_ID_1;
extern Value Block_200_ID_2;
extern Value Block_200_ID_3;
extern Value Block_200_ID_4;
extern Value Block_200_ID_5;
extern Value Block_200_ID_6;
extern Value Block_200_ID_7;
extern Value Block_200_ID_8;
extern Value Block_200_ID_9;
extern Value Block_200_ID_10;
extern Value Block_200_ID_11;
extern Value Block_200_ID_12;
extern Value Block_200_ID_13;

extern Value Block_200_ID_100; 
extern Value Block_200_ID_101;
extern Value Block_200_ID_102;
extern Value Block_200_ID_103;
extern Value Block_200_ID_110; 
extern Value Block_200_ID_111; 
extern Value Block_200_ID_112; 
extern Value Block_200_ID_113;
extern Value Block_200_ID_114;
extern Value Block_200_ID_115;
extern Value Block_200_ID_116;
extern Value Block_200_ID_117;
extern Value Block_200_ID_118;
extern Value Block_200_ID_119;


extern Value Block_241_ID_119; //Total system SOC
extern Value Block_241_ID_127; // System Power
extern Value Block_242_ID_119; //Capacity Extension SOC

extern Value Block_250_ID_222;
extern Value Block_250_ID_223;
extern Value Block_250_ID_224;
extern Value Block_250_ID_225;
extern Value Block_250_ID_230;

extern Value Block_251_ID_223;
extern Value Block_251_ID_230;

extern Value Block_252_ID_230;

extern Value Block_255_ID_0;
extern Value Block_255_ID_1;
extern Value Block_255_ID_2;
extern Value Block_255_ID_3;
extern Value Block_255_ID_4;
extern Value Block_255_ID_5;
extern Value Block_255_ID_6;
extern Value Block_255_ID_7;
extern Value Block_255_ID_8;
extern Value Block_255_ID_9;
extern Value Block_255_ID_10;
extern Value Block_255_ID_11;
extern Value Block_255_ID_12;
extern Value Block_255_ID_13;
extern Value Block_255_ID_14;
extern Value Block_255_ID_15;
extern Value Block_255_ID_255;

extern volatile unsigned long Test[8];
extern volatile APP_UPDATE_FLAGS App_UpdateFlags;
//extern const Value *LockPointer; 
extern unsigned char ModelValue[20];
//extern WAKEUP_FLAGS Wakeup_Flags;
    
/////////////////////////FUNCTION PROTOTYPES////////////////////////////////////
//int Values_FindValueInBlock(Value *BlockArray, int ArraySize, unsigned char SearchIndex);
Value *Values_GetValue(unsigned char U8_block, unsigned char U8_index);
void Values_Send_ByValue(uint8_t CMD, Value *Val, uint8_t UartSelect);
bool Values_RequestByValue(Value *Val, bool UpdateIfSet, char SelectValue);
bool Values_RequestInitial();
//extern volatile APP_WAKEUP_FLAGS 				APP_WAKEUP_FLAGS;
#endif