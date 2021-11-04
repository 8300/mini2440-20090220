/****************************************************************
 NAME: usbmain.c
 DESC: endpoint interrupt handler
       USB init jobs
 HISTORY:
 Mar.25.2002:purnnamu: ported for S3C2410X.
 Mar.27.2002:purnnamu: DMA is enabled.
 ****************************************************************/
#include <string.h>
#include <stdarg.h>
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "def.h"

#include "2440usb.h"
#include "usbmain.h"
#include "usblib.h"
#include "usbsetup.h"
#include "usbout.h"
#include "usbin.h"

/**************************
    Some PrepareEp1Fifo() should be deleted
 **************************/   

void UsbdMain(void)
{
    int i;
    U8 tmp1;
    U8 oldTmp1=0xff;
    
    //ChangeUPllValue(0x38,2,1);	// UCLK=96Mhz     
   // ChangeUPllValue(0x38,2,2);	// UCLK=48Mhz     
    InitDescriptorTable();
    //ResetUsbd();
    
    rGPGUP  |= 1<<9;		//disable pull-up
	rGPGCON &= ~(3<<18);	//input, hzh
	Delay(2000);
	rGPGDAT |= 1<<9;		//high    
	rGPGCON |= 1<<18;		//output	
    
    ConfigUsbd();
    
    //DetectVbus(); //not used in S3C2400X

    PrepareEp1Fifo(); 
#if 0    
    while(1)
    {
    	if(DbgPrintfLoop())continue;
    	
    	Delay(5000);
    	if((i++%2)==0)Led_Display(0x8);
    	else Led_Display(0x0);
    }
#endif    
}


void __irq IsrUsbd(void)
{
    U8 usbdIntpnd,epIntpnd;
    U8 saveIndexReg=rINDEX_REG;
    usbdIntpnd=rUSB_INT_REG;
    epIntpnd=rEP_INT_REG;
    //DbgPrintf( "[INT:EP_I=%x,USBI=%x]",epIntpnd,usbIntpnd );
    //Uart_SendByte('U');

    if(usbdIntpnd&SUSPEND_INT)
    {
    	rUSB_INT_REG=SUSPEND_INT;
    	DbgPrintf( "<SUS]");
    }
    if(usbdIntpnd&RESUME_INT)
    {
    	rUSB_INT_REG=RESUME_INT;
    	DbgPrintf("<RSM]");
    }
    if(usbdIntpnd&RESET_INT)
    {
    	DbgPrintf( "<RST]");  
    	
    	//ResetUsbd();
    	ReconfigUsbd();

    	rUSB_INT_REG=RESET_INT;  //RESET_INT should be cleared after ResetUsbd().   	

        PrepareEp1Fifo(); 
    }

    if(epIntpnd&EP0_INT)
    {
	rEP_INT_REG=EP0_INT;  
	DbgPrintf("<EP0]"); 
    	Ep0Handler();
    }
    if(epIntpnd&EP1_INT)
    {
    	rEP_INT_REG=EP1_INT;  
    	Ep1Handler();
    }

    if(epIntpnd&EP2_INT)
    {
    	rEP_INT_REG=EP2_INT;  
    	DbgPrintf("<2:TBD]");   //not implemented yet	
    	//Ep2Handler();
    }

    if(epIntpnd&EP3_INT)
    {
    	rEP_INT_REG=EP3_INT;
    	Ep3Handler();
    }

    if(epIntpnd&EP4_INT)
    {
    	rEP_INT_REG=EP4_INT;
    	DbgPrintf("<4:TBD]");   //not implemented yet	
    	//Ep4Handler();
    }

    ClearPending(BIT_USBD);	 
    
    rINDEX_REG=saveIndexReg;
}




/******************* Consol printf for debug *********************/

#define DBGSTR_LENGTH (0x1000)
U8 dbgStrFifo[DBGSTR_LENGTH];
volatile U32 dbgStrRdPt=0;
volatile U32 dbgStrWrPt=0;



void _WrDbgStrFifo(U8 c)
{
    dbgStrFifo[dbgStrWrPt++]=c;
    if(dbgStrWrPt==DBGSTR_LENGTH)dbgStrWrPt=0;

}


int DbgPrintfLoop(void)
{
    if(dbgStrRdPt==dbgStrWrPt)return 0;
    Uart_SendByte(dbgStrFifo[dbgStrRdPt++]);

    if(dbgStrRdPt==DBGSTR_LENGTH)dbgStrRdPt=0;
    return 1;
}


#if 0
void DbgPrintf(char *fmt,...)
{
    int i,slen;
    va_list ap;
    char string[256];

    va_start(ap,fmt);
    vsprintf(string,fmt,ap);
    
    slen=strlen(string);
    
    for(i=0;i<slen;i++)
    	_WrDbgStrFifo(string[i]);
    
    va_end(ap);
}
#else
void DbgPrintf(char *fmt,...)
{
}
#endif


