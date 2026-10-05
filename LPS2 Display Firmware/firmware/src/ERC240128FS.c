#include "ERC240128FS.h"
#include "Timer.h"
#include <string.h>

//Display Buffer
volatile unsigned char LCD_Buffer [3840] = {0};

// Function to wait for a specified number of milliseconds
void waitms(unsigned int milliseconds) {
    // Calculate the number of cycles required for the given milliseconds
    unsigned int cycles = (milliseconds * 1100);

    // Use a loop to wait for the calculated number of cycles
    while (cycles > 0) {
        cycles--;
        // You can add some NOP (No Operation) instructions here if needed
        // This might be necessary to fine-tune the timing based on your specific requirements
		//asm("nop");
        WDTCONbits.WDTCLR = 1;
    }
}

//Write data to display
static inline void ERC240_Write(unsigned char ucDATA)
{
    //Store data in DATAbits_t type
    DATAbits_t DATA = {.BYTE=ucDATA};
    
    //Set Port Values
    #ifdef PROCESSOR_PIC32MX170F256D_HW0130
        LATBCLR = 0b1111111100000;
        LATBSET = (DATA.D7<<5)|(DATA.D6<<6)|(DATA.D5<<7)|(DATA.D4<<8)|(DATA.D3<<9)|(DATA.D2<<10)|(DATA.D1<<11)|(DATA.D0<<12); //3.2uS
    #endif

    //Set Port Values
    #if defined(PROCESSOR_PIC32MX130F256D_HW0150_HW0160) || defined(PROCESSOR_PIC32MX170F256D_HW0160_HW0170) || defined(PROCESSOR_PIC32MX230F256D_HW0160_HW0210)  
        LATBCLR = 0b1110001111;
        LATACLR = 0b11;
        LATBSET = (DATA.D5<<7)|(DATA.D4<<8)|(DATA.D3<<9)|(DATA.D2<<2)|(DATA.D1<<1)|(DATA.D0); //3.2uS
        LATASET = (DATA.D7)|(DATA.D6<<1);
    #endif

    //Perform a write
    LCD_WR_Clear();
    LCD_WR_Set();
}

void ERC240_EnterSleep() {
    //LCD_COMMAND;
    ERC240_Write(0b10101100);
}

void ERC240_ExitSleep() {
    //LCD_COMMAND;
    ERC240_Write(0b10101101);
    waitms(15); //Timeout for supply to stabilize when charge pump starts
}

void ERC240_ResetMTP() {
    LCD_COMMAND;
    ERC240_EnterSleep();        //Set DC2 = 0 to allow MTP on
    ERC240_Write(0b10111000);   //MTP Operation Control Command
    ERC240_Write(0b00011001);   //MTP Enable in Read Mode
    ERC240_Write(0b11110111);   //Set MTP Read Timer Command
    ERC240_Write(0b00001000);   //Read timer set to 10 ms (perform read)
    
    waitms(200);
    
    ERC240_Init(false);
}

// This is a short version of the display initialization used for periodical re-initialization
// Make sure the values are consistent with those in ERC240_Init()
void ERC240_ReInit() {
    ERC240_Write(0b11101011);	//Bias Ratio:1/12 bias
	ERC240_Write(0b00101011);	//power control set as internal power, 13-22nF LCD
	ERC240_Write(0b00100110);	//Set temperaturee compensation as -0.15% / Deg
    ERC240_Write(0b10100100);   //All Pixels On disabled
    ERC240_Write(0b10000101);	//12,set partial display control:on
	ERC240_Write(0b11110001);	//COM scanning end (last COM with full line cycle)
	ERC240_Write(159);          //160 lines minus 1 (0-based index)
	ERC240_Write(0b11110010);	//Display start (first COM with active scan pulse)
	ERC240_Write(32);           
	ERC240_Write(0b11110011);	//Display end (last COM with active scan pulse)
	ERC240_Write(159);                  
	ERC240_Set_Contrast(&Block_3_ID_5);
}

//Initialization of ERC240128FS display
void ERC240_Init(bool LongDelay)
{
    // Wait for startup of controller
    if(LongDelay) waitms(200);
    
    LCD_COMMAND;
    ERC240_EnterSleep();
    ERC240_Write(0b11100010); //(23) Set system reset
    waitms(10);
    
    /*Power Control*/
    ERC240_Write(0b11101011);	//Bias Ratio:1/12 bias
	ERC240_Write(0b00101011);	//power control set as internal power, 13-22nF LCD
	ERC240_Write(0b00100110);	//Set Temperate compensation as -0.15% / Deg
	ERC240_Write(0b10000001);	//electronic potentionmeter
    unsigned char Contrast = (unsigned char)(Block_3_ID_5.NumValue/655.36);
    if(Contrast > 100)
        Contrast = 100;
    else if(Contrast < 30)
        Contrast = 30;
    ERC240_Write(Contrast);		//Contrast level
    
    /*display control*/
	ERC240_Write(0b10100100);			//all pixel off
	ERC240_Write(0b10100110);			//inverse display off
    
    /*lcd control*/
	ERC240_Write(0b11000100);			//Set LCD Maping Control (MY=1, MX=0)
	ERC240_Write(0b10100001);			//line rate 10.4klps
	ERC240_Write(0b11010001);			//rgb-rgb
	ERC240_Write(0b11010101);			//4k color mode
	ERC240_Write(0b10000100);			//12:partial display control disable
    
    /*n-line inversion*/
	ERC240_Write(0b11001000);
	ERC240_Write(0b00010000);			//enable NIV
    
    /*com scan fuction*/
	ERC240_Write(0b11011010);			//enable FRC, dither directly (no PWM), LRM AEBCD-AEBCD
    
    /*window*/
	ERC240_Write(0b11110100);			//wpc0:column
	ERC240_Write(0b00000000);			//start from 0
	ERC240_Write(0b11110110);			//wpc1
	ERC240_Write(79);                   //end:3*80=240

	ERC240_Write(0b11110101);			//wpp0:row
	ERC240_Write(0b00000000);			//start from 0
	ERC240_Write(0b11110111);			//wpp1
	ERC240_Write(127);                  //end 128

	ERC240_Write(0b11111000);			//inside mode

	ERC240_Write(0b10001001);			//RAM control
    
    
    /*scroll line*/
	ERC240_Write(0b01000000);			//low bit of scroll line
	ERC240_Write(0b01010000);			//high bit of scroll line

	ERC240_Write(0b10010000);			//14:FLT,FLB set
	ERC240_Write(0b00000000);

    // TODO: Test with partial display off?
	/*partial display*/
	ERC240_Write(0b10000101);			//12,set partial display control:on
	ERC240_Write(0b11110001);			//COM scanning end (last COM with full line cycle)
	ERC240_Write(159);                  //160 lines minus 1 (0-based index)
	ERC240_Write(0b11110010);			//Display start (first COM with active scan pulse)
	ERC240_Write(32);                   //160-32 = 128 (screen size)
	ERC240_Write(0b11110011);			//Display end (last COM with active scan pulse)
	ERC240_Write(159);                  
    
    //Clear Screen
    ERC240_Clear_MemoryBuffer();
    ERC240_Show_MemoryBuffer();
    LCD_COMMAND;
    
    //Read calibration values in case controller failed to do so. For some reason, this must be done after initialization, but requires re-initialization.
    if(LongDelay) ERC240_ResetMTP();
    
    ERC240_ExitSleep();
    ERC240_ExitSleep();

    LCD_DATA;
}

//Set address to zero (Pixel address)
void ERC240_Display_Address(void)
{  
    LCD_COMMAND;
    ERC240_Write(0x60);			//Row address LSB -
    ERC240_Write(0x70);			//Row address MSB
    ERC240_Write(0x10);			//Column address MSB
    ERC240_Write(0x00);			//Column address LSB
    LCD_DATA;
}

//Clear Buffer
void ERC240_Clear_MemoryBuffer()
{
    memset((unsigned char*)LCD_Buffer, 0, 3840);
}

//Display a buffer from memory
volatile unsigned char LUT_0Deg[4] = {0x00,0x0F,0xF0,0xFF};
volatile unsigned char LUT_0Deg_Invert[4] = {0xFF,0xF0,0x0F,0x00};
volatile unsigned char LUT_180Deg[4] = {0x00,0xF0,0x0F,0xFF};
volatile unsigned char LUT_180Deg_Invert[4] = {0x00,0xF0,0x0F,0xFF};

//Image rotation is performed in this function
void ERC240_Show_MemoryBuffer()
{ 
    //Set addres to zero;
    ERC240_Display_Address();
    int BufferByte;
    unsigned char MSG;
    for(BufferByte=0;BufferByte<3840;BufferByte++)  //180DEG
    //for(BufferByte=3839;BufferByte>=0;BufferByte--)   //0DEG
    {      
        //Pixels: 00000011
        MSG=(LCD_Buffer[BufferByte]>>0) & 0b00000011; //0DEG = 6 ; 180DEG = 0
        ERC240_Write(LUT_180Deg[MSG]);

        //Pixels: 00001100
        MSG=(LCD_Buffer[BufferByte]>>2) & 0b00000011; //0DEG = 4 ; 180DEG = 2
        ERC240_Write(LUT_180Deg[MSG]);

        //Pixels: 00110000
        MSG=(LCD_Buffer[BufferByte]>>4) & 0b00000011; //0DEG = 2 ; 180DEG = 4
        ERC240_Write(LUT_180Deg[MSG]);

        //Pixels: 11000000
        MSG= (LCD_Buffer[BufferByte]>>6) & 0b00000011; //0DEG = 0 ; 180DEG = 6
        ERC240_Write(LUT_180Deg[MSG]); 
    }
}

//Set display contrast level (0-100%)
void ERC240_Set_Contrast(Value *VAL_Contrast)
{
    unsigned char Contrast = (unsigned char)(VAL_Contrast->NumValue/655.36);
    if(Contrast > 100)
        Contrast = 100;
    
    LCD_COMMAND;
    ERC240_Write(0b10000001);	//Electronic potentiometer (next comand should be the contrast)
    ERC240_Write(Contrast);		//Contrast level
    LCD_DATA;
}

//Calculates the length of string
unsigned char ERC240_LengthOfCharecter(unsigned char Charecter, const unsigned char *Font)
{
    //Return variable - Used pixels in x (vertical) direction
    int offset = ((int)Font[3]<<8)|(int)Font[2];
    return Font[8+(Charecter-offset)*4];
}

//Calculates the Hight of Font
unsigned char ERC240_HightOfCharecter(const unsigned char *Font)
{
    //Hight of char in pixels
    return Font[6];
}

//Writes a byte to display buffer. Can shift the byte.
void ERC240_Display_Byte(unsigned char x, unsigned char y, unsigned char byte)
{
    unsigned short buffer = byte<<(x%8); //10101010 to 1010101000000000

    //X position given in bytes and bits
    unsigned short XByte = (short)x/8;

    //Spitting the short out into two after shifting to achive the right pixel position
    //and putting it out to memory
    unsigned char MSB = buffer>>8;
    unsigned char LSB = buffer;
    LCD_Buffer[30*y+XByte] = LCD_Buffer[30*y+XByte]|LSB;
    LCD_Buffer[30*y+XByte+1] = LCD_Buffer[30*y+XByte+1]|MSB;
}

void ERC240_Display_Pixel(unsigned char x, unsigned char y)
{
    
    //Full 3840 bytes
    //30 bytes per line
    //128 lines
    int byte = y*30+x/8;
    LCD_Buffer[byte] |= 1<<(x%8);
}


//Display a character from a given front returns used pixels in x direction. NOTE: Front must be made in accordance to MikroElektronica note.
//Returns length of the
unsigned char ERC240_Display_Character(unsigned char x, unsigned char y, unsigned char Charecter, const unsigned char *Font )
{
    //Get information about character
    int bytesize;
    unsigned char hight;
    unsigned char width;

    //Checking in the lookup part of Font
    //Finding the offset
    int offset = ((int)Font[3]<<8)|(int)Font[2];
    
    int startindex = 10+((Charecter-offset)*4);
    int endindex = 10+((Charecter-offset+1)*4);
    
    //Finding the start address of character
    int startaddress =((int)Font[startindex]<<8)|(int)Font[startindex-1]; // 10 //9

    //Finding the start address of next character
    int endaddress = ((int)Font[endindex]<<8)|(int)Font[endindex-1]; //14 13

    //Reading from Font data
    bytesize = endaddress-startaddress;	//Memory size of the character in bytes
    hight = Font[6]; 			//Hight of char in pixels
    width = bytesize/hight; 		//Width of character in bytes

    //Print out the character into Buffer
    int h,w,k;
    k=startaddress;
    for(h=0;h<hight;h++)
    {
            for(w=0;w<width;w++)
            {
                    int transX = ((w*8)+x);
                    int transY = (h+y);
                    ERC240_Display_Byte(transX,transY,Font[k]);
                    k++;
            }
    }
    return ERC240_LengthOfCharecter(Charecter,Font);
}

unsigned char ERC240_Display_String(int x, int y, const unsigned char *String, unsigned char CharSpacing, unsigned char adjust, const unsigned char *Font)
{
    unsigned char LengthOfString = String[0];
	unsigned char UsedPixelsX = 0;

        //Right adjust
        if(adjust == 'L')
        {
            unsigned char i;
            for(i=0;i<LengthOfString;i++)
            {
                    int Spacing = i*CharSpacing;
                    int xnew = (x+UsedPixelsX+Spacing);
                    UsedPixelsX += ERC240_Display_Character(xnew, y, String[i+1], Font);

            }
        }

        //Center adjust
        if(adjust == 'C')
        {
            int length = 0;
            unsigned char i;
            for(i=0;i<LengthOfString;i++)
            {
                    length = length + ERC240_LengthOfCharecter(String[i+1], Font) + CharSpacing;
            }

            for(i=0;i<LengthOfString;i++)
            {
                    int Spacing = i*CharSpacing;
                    int xnew = (x-(length/2)+UsedPixelsX+Spacing);
                    UsedPixelsX += ERC240_Display_Character(xnew, y, String[i+1], Font);
            }
        }

        //Left adjust
        if(adjust == 'R')
        {
            unsigned char i;
            int length = 0;
            for(i=0;i<LengthOfString;i++)
            {
                    length = length + ERC240_LengthOfCharecter(String[i+1], Font)+CharSpacing;
            }
            
            for(i=0;i<LengthOfString;i++)
            {
                    int Spacing = i*CharSpacing;
                    int xnew = (x-length+UsedPixelsX+Spacing);
                    UsedPixelsX += ERC240_Display_Character(xnew, y, String[i+1], Font);
            }
        }
    
    return UsedPixelsX;
}

//Adjust is 'L' for left; 'R' for right; 'C' for center
//Boundarylengt is the dedicated area in x direction to write
void ERC240_Display_StringTextBox(int x, int y, int BoundaryLength, const unsigned char *String, const unsigned char *Font, unsigned char LineSpace, unsigned Adjust)
{
    //LenghtOfSting is the complete length of the text that needs to be split
    unsigned char LengthOfString = String[0];
    unsigned char StringCharIndex = 1;                      //StringCharIndex is a pointer to the charecter inside the complete string

	//Get higt of font to determinite line shift
    unsigned char CharHight = ERC240_HightOfCharecter(Font);

    unsigned char StringToWrite[100] = {0}; 
    unsigned char StringLen = 0;
    unsigned char StartLocation = 1;
    unsigned char LastFoundStopLocation = 0;
    bool StringFound = false;

    int LinePixelLength = 0;
    int LineCount = 0;

    while (StringCharIndex <= LengthOfString)                                   //Go throug all charactes in string
    { 
        StringLen++;
        StringToWrite[StringLen] = String[StringCharIndex];                     //Gem Næste Char i String array. (Det array der skal udskrives)
        LinePixelLength += ERC240_LengthOfCharecter(String[StringCharIndex], Font);  //Add the next charlength to the complete length og line

        if (LinePixelLength > BoundaryLength) {                                 //Check if string Pixel length is out of boundary
            StringFound = true;
            if (LastFoundStopLocation == 0) {                                   //Check if Stop location has been found. Otherwise use this location
                LastFoundStopLocation = StringCharIndex;                        //Use This location As LastFoundStopLocation
            } 
        } else if (StringCharIndex == LengthOfString) {                         //Check if This is EOS
            StringFound = true;
            LastFoundStopLocation = StringCharIndex + 1;                        //Update last found stop location to this char.
        } else if (String[StringCharIndex] == ' ') {                            //Check for space character
            LastFoundStopLocation = StringCharIndex;                          
        }    
        
        if (StringFound == true) {
            StringFound = false;
            
            StringLen = LastFoundStopLocation - StartLocation;                  //Move StringLen back to last found Stop Location
            StringToWrite[0] = StringLen;                                       //Update StringToWrite with lenght of string.
            ERC240_Display_String(x, y+((CharHight+LineSpace)*LineCount), StringToWrite, 0, Adjust, Font);

            //Prepare for next line.
            LineCount++;
            LinePixelLength = 0;
            StringLen = 0;
            if (StringCharIndex < LengthOfString) {
                StartLocation = LastFoundStopLocation;
                if (String[LastFoundStopLocation] == ' ') StartLocation++;      //If last Found Stop location is ' ' use next as start instead.
                StringCharIndex = StartLocation - 1;                            //Move String Pointer back to new start location
                LastFoundStopLocation = 0;
            }
        }
        StringCharIndex++;
    }
}

void ERC240_Display_XLine(unsigned char x, unsigned char y, unsigned char Length)
{
	unsigned short StartBit  = (short)x%8;
	unsigned short StartByte = (short)((30*y)+(x/8));
	
	//Print out bytes
	unsigned char i;
	for(i=0;i<Length;i++)
	{
		LCD_Buffer[StartByte] |= 0b00000001<<StartBit;
		StartBit++;
		if(StartBit > 7)
		{
			StartBit = 0;
			StartByte++;
		}
	}
}

void ERC240_Display_YLine(unsigned char x, unsigned char y, unsigned char Length)
{
    unsigned short ByteToWrite;
    unsigned char Content = 0b00000001<<(x%8);

    unsigned char i;
    for(i=0;i<Length;i++)
    {
        ByteToWrite = (((i+y)*240)+x)/8;
        LCD_Buffer[ByteToWrite] |= Content;
    }
}

void ERC240_Display_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
    //Draw a square
    ERC240_Display_XLine(x0,y0,(x1-x0));
    ERC240_Display_XLine(x0,y1,(x1-x0)+1); //TODO Find fejl
    ERC240_Display_YLine(x0,y0,(y1-y0));
    ERC240_Display_YLine(x1,y0,(y1-y0));
}

void ERC240_Display_EraseArea_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
    unsigned short XStartBit  = (short)x0%8;
	unsigned short XStartByte = (short)((30*y0)+(x0/8));
    unsigned short XEndBit = (short)x1%8; //
    unsigned short XEndByte = (short)((30*y0)+(x1/8));
   
    unsigned short YLines = y1-y0; 
    
    //Loop variables
    unsigned short a,i;
    
    //Execute code for every line in Y direction
    for (a=0; a<=YLines; a++)
    {
        unsigned short YByte = a*30;
        
        if(XStartByte == XEndByte)
        {
            LCD_Buffer[XStartByte+YByte] &= ~(unsigned char)((0b11111111<<XStartBit)&(0b11111111>>(7-XEndBit)));
        }
        else
        {
            //Execute code for every line in Y direction
            for(i=XStartByte;i<=XEndByte;i++)
            {
                //Make left shadow
                if(i==XStartByte)
                {
                    LCD_Buffer[i+YByte]&= ~(unsigned char)(0b11111111<<XStartBit);
                }

                //Make right shadow
                if(i==XEndByte)
                {
                    LCD_Buffer[i+YByte]&= ~(unsigned char)((0b0000000011111111>>(7-XEndBit)) & 0xFF);
                }

                //Make area in the middle
                if((i>XStartByte)&&(i<XEndByte))
                {
                    LCD_Buffer[i+YByte]=0x00;
                }
            } 
        }
    }
}

//NEW FUNCITON //TODO - Correct area of invertion
void ERC240_Display_InvertArea_Square(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
    //Check if coordinates are specified correct
    if((y1 > y0) && (x1 > x0))
    {
        unsigned short XStartBit  = (short)x0%8;
        unsigned short XStartByte = (short)((30*y0)+(x0/8));
        unsigned short XEndBit = (short)x1%8; //
        unsigned short XEndByte = (short)((30*y0)+(x1/8));

        unsigned short YLines = y1-y0; 

        //Loop variables
        unsigned short a,i;

        //Execute code for every line in Y direction
        for (a=0; a<=YLines; a++)
        {
            unsigned short YByte = a*30;
            if(XStartByte == XEndByte)
                LCD_Buffer[XStartByte+YByte]^=(unsigned char)((0b11111111<<XStartBit)&(0b11111111>>(7-XEndBit))); //Går i overflow og trigger traps
            else
            {
                //Execute code for every line in Y direction
                for(i=XStartByte; i<=XEndByte; i++)
                {
                    //Make left shadow
                    if(i==XStartByte)
                        LCD_Buffer[i+YByte]^=(unsigned char)(0b11111111<<XStartBit);

                    //Make right shadow
                    if(i==XEndByte)
                        LCD_Buffer[i+YByte]^=(unsigned char)((0b0000000011111111>>(7-XEndBit)) & 0xFF);

                    //Make area in the middle
                    if((i>XStartByte)&&(i<XEndByte))
                        LCD_Buffer[i+YByte]^=0xFF;
                } 
            }
        }
    }
}

void ERC240_Display_Draw_Button (unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
    //Draw a square
    ERC240_Display_XLine(x0+1,y0,(x1-x0-1));
    ERC240_Display_XLine(x0+2,y1,(x1-x0)+1);
    ERC240_Display_XLine(x0+1,y1+1,(x1-x0)+1);
    ERC240_Display_YLine(x0,y0,(y1-y0));
    ERC240_Display_YLine(x1,y0,(y1-y0)); 
}


void ERC240_Display_SplashScreen(unsigned char Splash)
{
    switch(Splash)
    {
        case SPLASH_LPS_V1:
            ERC240_Display_Character(2,37,0,(unsigned char*)SplashScreen240x54);  
        break;
        case SPLASH_LPS_V2:
            ERC240_Display_Character(2,37,2,(unsigned char*)SplashScreen240x54);  
            ERC240_Display_Character(2,55,3,(unsigned char*)SplashScreen240x54);
        break;
        case SPLASH_ZEBRA:
            ERC240_Display_Character(2,37,1,(unsigned char*)SplashScreen240x54);
        break;
        default:
            ERC240_Display_Character(2,37,2,(unsigned char*)SplashScreen240x54);  
            ERC240_Display_Character(2,55,3,(unsigned char*)SplashScreen240x54);
            break;
    }
    
}

void ERC240_Display_Diagnostic(unsigned char Logo)
{
    ERC240_Clear_MemoryBuffer();
    ERC240_Display_Character(2,37,Logo,(unsigned char*)SplashScreen240x54);
    if(Logo == 2) //Combined LPS 2 LOGO
        ERC240_Display_Character(2,55,3,(unsigned char*)SplashScreen240x54);
    //ERC240_Show_MemoryBuffer();
}