#include <p32xxxx.h>
#include <stdbool.h>

#ifndef _ERROR_H_
	#define _ERROR_H_

    #define HIDE        2
    #define KEEP        1
    #define AUTO        0

    typedef struct Error_Constants{	
        const long                ErrorLevel;
        const unsigned char       *Title;  
        const unsigned char       *Description;  
        const unsigned char       ErrorNumber;
        const unsigned char       PopLevel;
    }Error_Constants;	
    extern const Error_Constants ErrorList[];
    
    typedef struct Error_Variables{	
        unsigned char Active : 1;
        unsigned char Minimized : 1;
    }Error_Variables;
    extern Error_Variables ErrorFlags[];
    
    //Graphic Functions
    void Error_ShowPopup();
    void Error_ClearPopup(bool RemoveLatch);
    
    //Logic Functions
    void Error_ClearAllMinimized();
    bool Error_CheckCleared();
    bool Error_CheckForErrorInBuf(unsigned char Error);
    bool Error_CheckForPopup();
    void Error_Language(long Language);

    extern unsigned char LPSErrorBuffer[];
    
#endif