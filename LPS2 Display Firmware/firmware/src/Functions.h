#include "Values.h"
#include "definitions.h" 
#include "UART_BIOS.h"


#ifndef _FUNCTIONS_H_
	#define _FUNCTIONS_H_


unsigned short CalculateCrc(unsigned char *data, unsigned long len);
void Main_ChangeState(unsigned char State);  
unsigned char *U8ToString(unsigned char ErrorNumber);
unsigned char *SimpleValToString(const Value *Val, char NumberOfDecimals);//, bool ShowPrefix);
//unsigned char *NumberToString(int Number); 
const unsigned char *OperationValToString(const Value *Val);
unsigned char *LanguageValToString(const Value *Val);
unsigned char *SerialValToString(const Value *Val);
unsigned char *PWR_SWVersToString();
unsigned char *VersionValToString(const Value *Val, bool LongVersion);
unsigned char *DateValToString(const Value *Val);
unsigned char *TimeValToString(const Value *Val);
unsigned char *MacValToString(const Value *Val);
unsigned char *PasskeyToString(const Value *Val);


//bool FillString_SimpleVal(const Value *Val, char NumberOfDecimals, char *String, unsigned int *index, int size);
#endif
