#include <xc.h> // include processor files - each processor file is guarded.  
#include "definitions.h" 
// Default setting after reset:
//* 32.768 kHz on pin CLKOUT active
//* 24 hour mode is selected
//* Offset register is set to 0
//* No alarms set
//* Timer disabled
//* No interrupts enabled

#ifndef RTC_H
#define	RTC_H

#define RTC_READ        true
#define RTC_WRITE       false
#define RTC_CMD_UNLOCK  0b001
//Years
#define January     0b00000001
#define February    0b00000010
#define March       0b00000011
#define April       0b00000100
#define May         0b00000101
#define June        0b00000110
#define July        0b00000111
#define August      0b00001000
#define September   0b00001001
#define October     0b00010000
#define November    0b00010001
#define December    0b00010010

//Days
#define Sunday      0b00000000
#define Monday      0b00000001
#define Tuesday     0b00000010
#define Wednesday   0b00000011
#define Thursday    0b00000100
#define Friday      0b00000101
#define Saturday    0b00000110

//Structs are defined from LSB first

//Protocol only suports 24H mode. Remember to set it correct!
typedef union __attribute__((packed)) tagRTC_HEADER {
	struct __attribute__((packed)){
		unsigned ADDRESS                :4;
		unsigned UNLOCK                 :3;
		unsigned RW                     :1;
	};
	unsigned char Byte;
} RTC_HEADER;

//CONTROL 1 Register - Adress 0x00
typedef union __attribute__((packed)) tagRTC_CONTROL1 {
	struct __attribute__((packed)){
		unsigned NOT_USED_0             :1;
		unsigned CIE                    :1;
		unsigned FORMAT                 :1;
		unsigned NOT_USED_3             :1;
		unsigned SR                     :1;
		unsigned STOP                   :1;
		unsigned NOT_USED_6             :1;
		unsigned EXT_TEST               :1;
	};
	uint8_t Byte;
} RTC_CONTROL1;

//CONTROL 2 Register - Adress 0x01
typedef union __attribute__((packed)) tagRTC_CONTROL2 {
	struct __attribute__((packed)){
		unsigned TIE                    :1;
		unsigned AIE                    :1;
		unsigned TF                     :1;
		unsigned AF                     :1;
		unsigned TI_TP                  :1;
		unsigned MSF                    :1;
		unsigned SI         	   		:1;
		unsigned MI                     :1;
	};
	uint8_t Byte;
} RTC_CONTROL2;

//SECONDS Register - Adress 0x02
typedef union __attribute__((packed)) tagRTC_SECONDS {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :3;
		unsigned OS                     :1;
	};
	uint8_t Byte;
} RTC_SECONDS;

//MINUTES Register - Adress 0x03
typedef union __attribute__((packed)) tagRTC_MINUTES {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :3;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_MINUTES;

//HOURS24 Register - Adress 0x04
typedef union __attribute__((packed)) tagRTC_HOURS24 {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :2;
        unsigned NOT_USED_6             :1;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_HOURS24;

//DAYS Register - Adress 0x05
typedef union __attribute__((packed)) tagRTC_DAYS {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :2;
        unsigned NOT_USED_6             :1;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_DAYS;

//WEEKDAYS Register - Adress 0x06
typedef union __attribute__((packed)) tagRTC_WEEKDAYS {
	struct __attribute__((packed)){
		unsigned WEEKDAY                :3;
		unsigned NOT_USED_3             :1;
        unsigned NOT_USED_4             :1;
		unsigned NOT_USED_5             :1;
        unsigned NOT_USED_6             :1;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_WEEKDAYS;

//MONTHS Register - Adress 0x07
typedef union __attribute__((packed)) tagRTC_MONTHS {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :1;
        unsigned NOT_USED_5             :1;
        unsigned NOT_USED_6             :1;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_MONTHS;

//YEARS Register - Adress 0x08
typedef union __attribute__((packed)) tagRTC_YEARS {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :4;
	};
	uint8_t Byte;
} RTC_YEARS;

//MINUTE_ALARM Register - Adress 0x09
typedef union __attribute__((packed)) tagRTC_MINUTE_ALARM {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :3;
        unsigned AE_M                   :1;
	};
	uint8_t Byte;
} RTC_MINUTE_ALARM;

//HOUR24_ALARM Register - Adress 0x0A
typedef union __attribute__((packed)) tagRTC_HOUR24_ALARM {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :2;
        unsigned NOT_USED_6             :1;
        unsigned AE_H                   :1;
	};
	uint8_t Byte;
} RTC_HOUR24_ALARM;

//DAY_ALARM Register - Adress 0x0B
typedef union __attribute__((packed)) tagRTC_DAY_ALARM {
	struct __attribute__((packed)){
		unsigned VALx1                  :4;
		unsigned VALx10                 :2;
        unsigned NOT_USED_6             :1;
        unsigned AE_D                   :1;
	};
	uint8_t Byte;
} RTC_DAY_ALARM;

//WEEKDAY_ALARM Register - Adress 0x0C
typedef union __attribute__((packed)) tagRTC_WEEKDAY_ALARM {
	struct __attribute__((packed)){
		unsigned WEEKDAY                :3;
        unsigned NOT_USED_3             :1;
        unsigned NOT_USED_4             :1;
        unsigned NOT_USED_5             :1;
        unsigned NOT_USED_6             :1;
        unsigned AE_W                   :1;
	};
	uint8_t Byte;
} RTC_WEEKDAY_ALARM;

//OFFSET Register - Adress 0x0D
typedef union __attribute__((packed)) tagRTC_OFFSET {
	struct __attribute__((packed)){
		unsigned TODO0                  :1;
		unsigned TODO1                  :1;
		unsigned TODO2                  :1;
		unsigned TODO3                  :1;
		unsigned TODO4                  :1;
		unsigned TODO5                  :1;
		unsigned TODO6      	   		:1;
		unsigned TODO7                  :1;
	};
	uint8_t Byte;
} RTC_OFFSET;

//TIMER_CLKOUT Register - Adress 0x0E
typedef union __attribute__((packed)) tagRTC_TIMER_CLKOUT {
	struct __attribute__((packed)){
		unsigned CTD                    :2;
		unsigned NOT_USED_2             :1;
		unsigned TE                     :1;
		unsigned COF         	   		:3;
		unsigned NOT_USED_7             :1;
	};
	uint8_t Byte;
} RTC_TIMER_CLKOUT;

//COUNTDOWN_TIMER Register - Adress 0x0F
typedef union __attribute__((packed)) tagRTC_COUNTDOWN_TIMER {
	uint8_t COUNTDOWN_TIMER;
} RTC_COUNTDOWN_TIMER;

//COUNTDOWN_TIMER Register - Adress 0x0F
typedef union __attribute__((packed)){
    struct __attribute__((packed)){
        RTC_CONTROL1 CONTROL1;
        RTC_CONTROL2 CONTROL2;
        RTC_SECONDS SECONDS;
        RTC_MINUTES MINUTES;
        RTC_HOURS24 HOURS;
        RTC_DAYS DAYS;
        RTC_WEEKDAYS WEEKDAYS;
        RTC_MONTHS MONTHS;
        RTC_YEARS YEARS;
        RTC_MINUTE_ALARM MINUTE_ALARM;
        RTC_HOUR24_ALARM HOUR_ALARM;
        RTC_DAY_ALARM DAY_ALARM;
        RTC_WEEKDAY_ALARM WEEKDAY_ALARM;
        RTC_OFFSET OFFSET;
        RTC_TIMER_CLKOUT TIMER_CLKOUT;
        RTC_COUNTDOWN_TIMER COUNTDOWN_TIMER;
        };
        uint8_t Byte[16];
        /*
        RTC_COUNTDOWN_TIMER COUNTDOWN_TIMER;
        RTC_TIMER_CLKOUT TIMER_CLKOUT;
        RTC_OFFSET OFFSET;
        RTC_WEEKDAY_ALARM WEEKDAY_ALARM;
        RTC_DAY_ALARM DAY_ALARM;
        RTC_HOUR24_ALARM HOUR_ALARM;
        RTC_MINUTE_ALARM MINUTE_ALARM;
        RTC_YEARS YEARS;
        RTC_MONTHS MONTHS;
        RTC_WEEKDAYS WEEKDAYS;
        RTC_DAYS DAYS;
        RTC_HOURS24 HOURS;
        RTC_MINUTES MINUTES;
        RTC_SECONDS SECONDS;
        RTC_CONTROL2 CONTROL2;
        RTC_CONTROL1 CONTROL1;*/
} RTC_DATA_CONTAINER;

//Refresh Functions
void RTC_RW(void* buffer, uint8_t length, bool read, uint8_t rtc_addr);


//Get Date
uint8_t RTC_Get_Seconds();
uint8_t RTC_Get_Minutes();
uint8_t RTC_Get_Hours24();
uint8_t RTC_Get_Days();
uint8_t RTC_Get_Weekday();
uint8_t RTC_Get_Month();
uint8_t RTC_Get_Year();

//Set Date
void RTC_Set_Seconds(uint8_t Seconds);
void RTC_Set_Minutes(uint8_t Minutes);
void RTC_Set_Hours24(uint8_t Hours24);
void RTC_Set_Days(uint8_t Days);
void RTC_Set_Weekday(uint8_t Weekday);
void RTC_Set_Month(uint8_t Month);
void RTC_Set_Year(uint8_t Year);

//Control
void RTC_CTRL_ClockOut(bool CLK_Active);
void RTC_CTRL_INT_Seconds(bool INT_Active);
void RTC_CTRL_Timer(bool TMR_Active);
void RTC_CTRL_Reset();

void RTC_INIT_Interrupt();
void RTC_INIT_Device();
#endif