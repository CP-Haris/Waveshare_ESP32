#include "Functions.h"
#include "Text.h"
#include <string.h>

unsigned char *U8ToString(unsigned char Number)
{
    static unsigned char String[5];
    unsigned char StringIndex = 4;
    
    do                            
    {
       String[StringIndex] = (Number % 10) + '0';    
       Number /= 10;
       StringIndex--;
    } while (Number);
    
    String[StringIndex]=4-StringIndex;
    
    return &String[StringIndex];
}

unsigned char *SimpleValToString(const Value *Val, char NumberOfDecimals)//, bool QR_Format) 
{
    //Result Buffer
    static unsigned char buffer[15];
    //Init buffer with 0 in all places
    memset(buffer, 0, sizeof(buffer)); 
    //Set buffer pointer
    unsigned char bufferIndex = 14;    
    

    //Get absolute value
    long long llValue = Val->NumValue;
    if (Val->NumValue < 0) 
        llValue = 0 - llValue;
     
    switch (Val->Prefix)
    {
        case TEMPERATURE:
           buffer[bufferIndex] = 'C';
           bufferIndex--;
           buffer[bufferIndex] = 176; //Degree
           bufferIndex--;
        break;

        case VOLTAGE:
           buffer[bufferIndex] = 'V';
           bufferIndex--;
        break;

        case CURRENT:
           buffer[bufferIndex] = 'A';
           bufferIndex--;
        break;

        case PROCENT:
           llValue *= 100;
           buffer[bufferIndex] = '%';
           bufferIndex--;
        break;

        case POWER:
           buffer[bufferIndex] = 'W';
           bufferIndex--;
        break;

        case OHM:
           buffer[bufferIndex] = 'm';
           bufferIndex--;
           buffer[bufferIndex] = 'h';
           bufferIndex--;
           buffer[bufferIndex] = 'O';
           bufferIndex--;
        break;

        case KWH:
           buffer[bufferIndex] = 'h';
           bufferIndex--;
           buffer[bufferIndex] = 'W';
           bufferIndex--;
           buffer[bufferIndex] = 'k';
           bufferIndex--;
        break;

       default:  
       break;
    }

    //Format fixed point value with number of decimals and round up if necessary
    char i;
    for (i=0; i < NumberOfDecimals; i++)
        llValue *= 10;

    long FormattedValue = llValue >> 16;
    if ((llValue & 0x0000FFFF) > 0x8000) {          //Check if value should be rounded up.
            FormattedValue++;
    }

    //Extract numbers and place comma
    do {
        buffer[bufferIndex] = (FormattedValue % 10) + '0';
        bufferIndex--;
        FormattedValue /= 10;
        
        NumberOfDecimals--;
        if (NumberOfDecimals == 0) {
            buffer[bufferIndex] = ',';
            bufferIndex--;
        }
    }while ((FormattedValue>0) || (NumberOfDecimals >= 0));         // NumberOfDecimals bliver -1 når første cifer foran kommat vises.
    
    
    //Add Sign
    if (Val->NumValue < 0) { //Check if value is negative
        buffer[bufferIndex] = '-';
        bufferIndex--;
    }

    //Add length of data
    buffer[bufferIndex] = 14-bufferIndex;
    
    return &buffer[bufferIndex];
}

const unsigned char *OperationValToString(const Value *Val)
{
    switch(Val->Prefix)
    {
        case STATE_OPERATION:
        {
            switch(Val->NumValue)
            {
                case OS_OFF:
                    return ENG_OS_Off;
                break;

                case OS_WAKEUP:
                    return ENG_OS_Standby;
                break;

                case OS_READY_TO_START:
                    return ENG_OS_Standby;
                break;

                case OS_STARTING:
                    return ENG_OS_Starting;
                break;

                case OS_STOPPING:
                    return ENG_OS_Stopping;
                break;

                case OS_DISABLED:
                    return ENG_OS_Disabled;
                break;
                
                case OS_ON:
                    return ENG_OS_On;
                break;
                
                default:
                    return ENG_Empty;
                break;
            }
        }
        break;
        
        case STATE_FAILURE:
        {
            switch(Val->NumValue)
            {
                case FL_OK:
                    return ENG_FL_Okay;
                break;

                case FL_WARNING:
                    return ENG_FL_Warning;
                break;

                case FL_SIMPLE_FAILURE:
                    return ENG_FL_Simple;
                break;

                case FL_EMPTY:
                    return ENG_FL_Empty;
                break;

                case FL_CRITICAL_FAILURE:
                    return ENG_FL_Critical;
                break;

                default:
                    return ENG_Empty;
                break;
            }
        }
        break;
        
        case STATE_BATTERY:
        {
            switch(Val->NumValue)
            {
                case BS_IDLE:
                    return ENG_OS_Standby;
                break;

                case BS_DISCHARGE:
                    return ENG_BS_Discharging;
                break;

                case BS_CHARGE:
                    return ENG_BS_Charging;
                break;
                
                case BS_BALANCING:
                    return ENG_BS_Balancing;
                break;

                case BS_FULL:
                    return ENG_BS_Full;
                break;
                
                default:
                    return ENG_BS_Empty;
                break;
            }
        }
        break;
        
        case STATE_TESTMODE:
        {
            switch(Val->NumValue)
            {
                case TM_NORMAL:
                    return ENG_None;
                break;

                case TM_BIOS:
                    return ENG_Testmode_Bios;
                break;

                case TM_APPLICATION:
                    return ENG_Testmode_App;
                break;
                
                case TM_NOPROTECTION:
                    return ENG_Testmode_NoProtection;
                break;

                default:
                    return ENG_Empty;
                break;
            }
        }
        break;
        
        default:
            return ENG_Empty;
        break;
    }
}

unsigned char *PasskeyToString(const Value *Val)
{
     //Result Buffer
    static unsigned char String[7] = {6, '0','0','0','0','0'};
    unsigned char StringIndex=6;    
    //memset(buffer, 0, sizeof(buffer)); 
    
    unsigned long Value = Val->NumValue; 
    //Make Integer Part
    do
    {
        String[StringIndex] = (Value % 10) + '0';    
        Value /= 10;
        StringIndex--;
    } while (StringIndex>0);
    
    return String;
}

unsigned char *SerialValToString(const Value *Val)
{
     //Result Buffer
    static unsigned char SerialString[12];
    unsigned char SerialStringIndex=11;    
    //memset(buffer, 0, sizeof(buffer)); 
    
    unsigned long SerialValue = Val->NumValue; 
    //Make Integer Part
    do
    {
        if(SerialStringIndex==7)
        {
            SerialString[SerialStringIndex]='-';
            SerialStringIndex--;
        }
        SerialString[SerialStringIndex] = (SerialValue % 10) + '0';    
        SerialValue /= 10;
        SerialStringIndex--;
    } while (SerialStringIndex>0);
    
    SerialString[SerialStringIndex]=11;
    
    return &SerialString[SerialStringIndex];
}

static unsigned char VersionString[9];
unsigned char *VersionValToString(const Value *Val, bool LongVersion)
{
    //Value    
    long VersionValue = Val->NumValue; 
    
    //If no version is defined
    if ((VersionValue == 0x00) || (VersionValue == 0xFFFFFFFF))
    {
        //Show text None
        //return (unsigned char*)&ENG_None[0];
        if(LongVersion)
            return (unsigned char*)&ENG_NoVersionLong[0];
        else
            return (unsigned char*)&ENG_NoVersionShort[0];
    }
    //If version is defined
    else
    {
        //Result Buffer
        unsigned char NumberOfChars=5;
        if(LongVersion)
            NumberOfChars=8;

        //Write Index
        unsigned char VersionStringIndex = NumberOfChars;

        //Make Integer Part
        do
        {
            if(VersionStringIndex==6 || VersionStringIndex==3)
            {
                VersionString[VersionStringIndex]=':';
                VersionStringIndex--;
            }
            VersionString[VersionStringIndex] = (VersionValue % 10) + '0';    
            VersionValue /= 10;
            VersionStringIndex--;
        } while (VersionStringIndex>0);
        
        VersionString[VersionStringIndex] = NumberOfChars;

        return &VersionString[VersionStringIndex];
    }
}


void byteToHexString(unsigned char byte, unsigned char *hexString) {
    const char hexDigits[] = "0123456789ABCDEF";

    // Convert the upper nibble (4 bits)
    hexString[0] = hexDigits[(byte >> 4) & 0x0F];
    // Convert the lower nibble (4 bits)
    hexString[1] = hexDigits[byte & 0x0F];
}

#ifdef BLUETOOTH_CODE
static unsigned char MacString[18] = {17,'X','X',':','X','X',':','X','X',':','X','X',':','X','X',':','X','X'};
unsigned char *MacValToString(const Value *Val)
{
    switch(Val->ID)
    {
        case 110:
        case 111:
            byteToHexString(Block_200_ID_110.NumValue_Bytes[0], &MacString[1]);
            byteToHexString(Block_200_ID_110.NumValue_Bytes[1], &MacString[4]);
            byteToHexString(Block_200_ID_110.NumValue_Bytes[2], &MacString[7]);
            byteToHexString(Block_200_ID_111.NumValue_Bytes[0], &MacString[10]);
            byteToHexString(Block_200_ID_111.NumValue_Bytes[1], &MacString[13]);
            byteToHexString(Block_200_ID_111.NumValue_Bytes[2], &MacString[16]);
        break;
        
        case 112:
        case 113:
            byteToHexString(Block_200_ID_112.NumValue_Bytes[0], &MacString[1]);
            byteToHexString(Block_200_ID_112.NumValue_Bytes[1], &MacString[4]);
            byteToHexString(Block_200_ID_112.NumValue_Bytes[2], &MacString[7]);
            byteToHexString(Block_200_ID_113.NumValue_Bytes[0], &MacString[10]);
            byteToHexString(Block_200_ID_113.NumValue_Bytes[1], &MacString[13]);
            byteToHexString(Block_200_ID_113.NumValue_Bytes[2], &MacString[16]);
        break;
        
        case 114:
        case 115:
            byteToHexString(Block_200_ID_114.NumValue_Bytes[0], &MacString[1]);
            byteToHexString(Block_200_ID_114.NumValue_Bytes[1], &MacString[4]);
            byteToHexString(Block_200_ID_114.NumValue_Bytes[2], &MacString[7]);
            byteToHexString(Block_200_ID_115.NumValue_Bytes[0], &MacString[10]);
            byteToHexString(Block_200_ID_115.NumValue_Bytes[1], &MacString[13]);
            byteToHexString(Block_200_ID_115.NumValue_Bytes[2], &MacString[16]);
        break;
        
        case 116:
        case 117:
            byteToHexString(Block_200_ID_116.NumValue_Bytes[0], &MacString[1]);
            byteToHexString(Block_200_ID_116.NumValue_Bytes[1], &MacString[4]);
            byteToHexString(Block_200_ID_116.NumValue_Bytes[2], &MacString[7]);
            byteToHexString(Block_200_ID_117.NumValue_Bytes[0], &MacString[10]);
            byteToHexString(Block_200_ID_117.NumValue_Bytes[1], &MacString[13]);
            byteToHexString(Block_200_ID_117.NumValue_Bytes[2], &MacString[16]);
        break;
        
        case 118:
        case 119:
            byteToHexString(Block_200_ID_118.NumValue_Bytes[0], &MacString[1]);
            byteToHexString(Block_200_ID_118.NumValue_Bytes[1], &MacString[4]);
            byteToHexString(Block_200_ID_118.NumValue_Bytes[2], &MacString[7]);
            byteToHexString(Block_200_ID_119.NumValue_Bytes[0], &MacString[10]);
            byteToHexString(Block_200_ID_119.NumValue_Bytes[1], &MacString[13]);
            byteToHexString(Block_200_ID_119.NumValue_Bytes[2], &MacString[16]);
        break;
    }  
    return MacString;
}
#endif

unsigned char *DateValToString(const Value *Val)
{
    //Result Buffer
    static unsigned char DateString[11];
    unsigned char DateStringIndex=10;    

    
    long DateValue = Val->NumValue; 
    //Make Integer Part
    do
    {
        if(DateStringIndex==8 || DateStringIndex==5)
        {
            DateString[DateStringIndex]='-';
            DateStringIndex--;
        }
        DateString[DateStringIndex] = (DateValue % 10) + '0';    
        DateValue /= 10;
        DateStringIndex--;
    } while (DateStringIndex>0);
    
    DateString[DateStringIndex]=10;
    
    return &DateString[DateStringIndex];
}


unsigned char *TimeValToString(const Value *Val)
{   
    short Sec;
    short Min;
    short Hour;
    long TotalTime;
    signed long long ullValue;
    static unsigned char TimeString[15];
    
    //Calculate Time
    ullValue = (signed long long)labs(Val->NumValue) * 3600;
    TotalTime = ullValue >> 16;

    if ((ullValue & 0x0000FFFF) > 0x8000) 
        TotalTime++;

    Sec = TotalTime % 60;
    Min = (TotalTime % 3600) / 60;
    Hour = TotalTime / 3600;
    if(Hour > 99)
        Hour = 99;
      
    if(Val->Prefix == TIME_HEAD)
    {
        //Convert Time into Text
        TimeString[0] = 14;   
        TimeString[1] = (Hour / 10) + '0';   
        TimeString[2] = (Hour % 10) + '0';    
        TimeString[3] = ' ';
        TimeString[4] = 'h';
        TimeString[5] = 'r';
        TimeString[6] = ' ';
        TimeString[7] = ':';
        TimeString[8] = ' ';
        TimeString[9] = (Min / 10) + '0'; ;
        TimeString[10]= (Min % 10) + '0'; ;
        TimeString[11]= ' ';
        TimeString[12]= 'm';
        TimeString[13]= 'i';
        TimeString[14]= 'n';
    }
    else if(Val->Prefix == TIME_HHMM) {               
        //Convert Time into Text
        if(Val->NumValue<0) {
            TimeString[0] = 6;   
            TimeString[1] = '-';
            TimeString[2] = (Hour / 10) + '0';   
            TimeString[3] = (Hour % 10) + '0';    
            TimeString[4] = ':';
            TimeString[5] = (Min / 10) + '0';
            TimeString[6] = (Min % 10) + '0';
        }
        else {
            TimeString[0] = 5;   
            TimeString[1] = (Hour / 10) + '0';   
            TimeString[2] = (Hour % 10) + '0';    
            TimeString[3] = ':';
            TimeString[4] = (Min / 10) + '0';
            TimeString[5] = (Min % 10) + '0';
        }
    }
    
    else if(Val->Prefix == TIME_HHMMSS) {             
        //Convert Time into Text
        if(Val->NumValue<0) {
            TimeString[0] = 9;   
            TimeString[1] = '-';
            TimeString[2] = (Hour / 10) + '0';   
            TimeString[3] = (Hour % 10) + '0';    
            TimeString[4] = ':';
            TimeString[5] = (Min / 10) + '0';
            TimeString[6] = (Min % 10) + '0';
            TimeString[7] = ':';
            TimeString[8] = (Sec / 10) + '0';
            TimeString[9] = (Sec % 10) + '0';
        }
        else {
            TimeString[0] = 8;   
            TimeString[1] = (Hour / 10) + '0';   
            TimeString[2] = (Hour % 10) + '0';    
            TimeString[3] = ':';
            TimeString[4] = (Min / 10) + '0';
            TimeString[5] = (Min % 10) + '0';
            TimeString[6] = ':';
            TimeString[7] = (Sec / 10) + '0';
            TimeString[8] = (Sec % 10) + '0';
        }
    }
    return &TimeString[0];
}

