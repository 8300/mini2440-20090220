#include "def.h"
#include "2440addr.h"
#include "2440lib.h"

#define	puts	Uart_Printf
#define	printf	Uart_Printf
#define	getch	Uart_Getch

#ifndef DEBUG_VERSION
#define	USE_UART_INT
#endif

extern U32 downloadAddress, downloadFileSize;
static U8 *temp;

#ifdef	USE_UART_INT
static void __irq Uart0RxInt(void)
{
    ClearSubPending(BIT_SUB_RXD0); //rSUBSRCPND = BIT_SUB_RXD0;          //Clear pending bit (Requested)
    ClearPending(BIT_UART0);

    *temp ++= RdURXH0(); 
}

static void __irq Uart1RxInt(void)
{
    ClearSubPending(BIT_SUB_RXD1); //rSUBSRCPND = BIT_SUB_RXD1;          //Clear pending bit (Requested)
    ClearPending(BIT_UART1);

    *temp ++= RdURXH1(); 
}
#endif

void call_linux(U32 a0, U32 a1, U32 a2);

void comdownload(void)
{
	U32 size;
	U8 *buf;
	U16 checksum;
	int up;
	
	puts("\nNow download file from uart0...\n");
	downloadAddress = _NONCACHE_STARTADDRESS;
	buf  = (U8 *)downloadAddress;
	temp = buf-4;
	
	Uart_GetKey();

#ifdef	USE_UART_INT
	up = Uart_Select(-1); 					//get the console port
	if(!up) {
		pISR_UART0 = (U32)Uart0RxInt;		//串口接收数据中断
		ClearSubPending(BIT_SUB_RXD0);
		ClearPending(BIT_UART0);
		EnableSubIrq(BIT_SUB_RXD0);
		EnableIrq(BIT_UART0);
	} else {
		pISR_UART1 = (U32)Uart1RxInt;		//串口接收数据中断
		ClearSubPending(BIT_SUB_RXD1);
		ClearPending(BIT_UART1);
		EnableSubIrq(BIT_SUB_RXD1);
		EnableIrq(BIT_UART1);
	}
#endif

	while((U32)temp<(U32)buf)		
    {
 #ifdef	USE_UART_INT
        Led_Display(0);
        Delay(1000);
        Led_Display(15);
        Delay(1000);
#else
		*temp++ = Uart_Getch();
#endif
    }							//接收文件长度,4 bytes	
	
	size  = *(U32 *)(buf-4);
	downloadFileSize = size-6;

#ifdef	USE_UART_INT
    printf("Download File Size = %d\n", size);
#endif
	
	while(((U32)temp-(U32)buf)<(size-4))
	{
#ifdef	USE_UART_INT	
		Led_Display(0);
        Delay(1000);
        Led_Display(15);
        Delay(1000);
#else
		*temp++ = Uart_Getch();
#endif		
	}

#ifdef	USE_UART_INT
	if(!up) {
		DisableSubIrq(BIT_SUB_RXD0);
		DisableIrq(BIT_UART0);
	} else {
		DisableSubIrq(BIT_SUB_RXD1);
		DisableIrq(BIT_UART1);
	}
#endif

#ifndef	USE_UART_INT
	printf("Download File Size = %d\n", size);
#endif
	
	checksum = 0;
	for(size=0; size<downloadFileSize; size++)
		checksum += buf[size];
	if(checksum!=(buf[size]|(buf[size+1]<<8))) {
		puts("Checksum fail!\n");
		return;
	}
	
	puts("Are you sure to run? [y/n]\n");
	while(1)
	{
		U8 key = getch();
		if(key=='n')
			return;
		if(key=='y')
			break;	
	}
	
	call_linux(0, 193, downloadAddress);
}