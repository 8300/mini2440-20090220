/****************************************************************
 NAME: u2440mon.c
 DESC: u2440mon entry point,menu,download
 HISTORY:
 Mar.25.2002:purnnamu: S3C2400X profile.c is ported for S3C2410X.
 Mar.27.2002:purnnamu: DMA is enabled.
 Apr.01.2002:purnnamu: isDownloadReady flag is added.
 Apr.10.2002:purnnamu: - Selecting menu is available in the waiting loop. 
                         So, isDownloadReady flag gets not needed
                       - UART ch.1 can be selected for the console.
 Aug.20.2002:purnnamu: revision number change 0.2 -> R1.1       
 Sep.03.2002:purnnamu: To remove the power noise in the USB signal, the unused CLKOUT0,1 is disabled.
 ****************************************************************/
#define	GLOBAL_CLK		1//hzh

#include <stdlib.h>
#include <string.h>
#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"
#include "mmu.h"
#include "profile.h"
#include "memtest.h"

#include "usbmain.h"
#include "usbout.h"
#include "usblib.h"
#include "2440usb.h"

#include "norflash.h"

void Isr_Init(void);
void HaltUndef(void);
void HaltSwi(void);
void HaltPabort(void);
void HaltDabort(void);
void Lcd_Off(void);
void WaitDownload(void);
void Menu(void);
void ClearMemory(void);


void Clk0_Enable(int clock_sel);	
void Clk1_Enable(int clock_sel);
void Clk0_Disable(void);
void Clk1_Disable(void);

//#define DOWNLOAD_ADDRESS _RAM_STARTADDRESS
volatile U32 downloadAddress;

void (*restart)(void)=(void (*)(void))0x0;
//void (*run)(void);	//don't use gloable variable, hzh!!!


volatile unsigned char *downPt;
volatile U32 downloadFileSize;
volatile U16 checkSum;
volatile unsigned int err=0;
volatile U32 totalDmaCount;

volatile int isUsbdSetConfiguration;

int download_run=0;
U32 tempDownloadAddress;
int menuUsed=0;

extern char Image$$RW$$Limit[];
U32 *pMagicNum=(U32 *)Image$$RW$$Limit;
int consoleNum;

/*************************************************************/
#include "bootpara.h"

void LcdDisplay(void);
int write_24c02(U8 *pBuf);
int read_24c02(U8 *pBuf);
int find_camera(void);
void Led_Test(void);
void comdownload(void);
U32 GetFlashID(void);
int SectorProg(U32 begin, U16 *data, U32 size);
int RelocateNKBIN(U32 img_src, U32 *pStart, U32 *pLength, U32 *pLaunch);

void NandErase(void);
void NandWrite(void);
void NandLoadRun(void);

#define	printf	Uart_Printf

#define	DM9000_BASE			0x20000300
#define	DM9000_DATA_OFFSET	4

void mdelay(int ms)
{
	U32 val = (PCLK>>3)/1000-1;
	
	rTCFG0 &= ~(0xff<<8);
	rTCFG0 |= 3<<8;			//prescaler = 3+1
	rTCFG1 &= ~(0xf<<12);
	rTCFG1 |= 0<<12;		//mux = 1/2

/*	while(ms--) {
		rTCNTB3 = val;
		rTCMPB3 = val>>1;		// 50%
		rTCON &= ~(0xf<<16);
		rTCON |= 3<<16;			//one shot, inv-off, update TCNTB3&TCMPB3, start timer 3
		rTCON &= ~(2<<16);		//clear manual update bit
		while(rTCNTO3);
	}*/
	rTCNTB3 = val;
	rTCMPB3 = val>>1;		// 50%
	rTCON &= ~(0xf<<16);
	rTCON |= 0xb<<16;		//interval, inv-off, update TCNTB3&TCMPB3, start timer 3
	rTCON &= ~(2<<16);		//clear manual update bit
	while(ms--) {
		while(rTCNTO3>=val>>1);
		while(rTCNTO3<val>>1);
	};

}

static U8 dm9000_ior(int reg)
{
	*(volatile U8 *)DM9000_BASE = reg;
	return *(volatile U8 *)(DM9000_BASE+DM9000_DATA_OFFSET);
}

static void rd_dm9000_id(void)
{
	U16 id;
	
	id = dm9000_ior(0x28) | (dm9000_ior(0x29)<<8);
	printf("read dm9000 vid = 0x%x\n", id);
	
	id = dm9000_ior(0x2a) | (dm9000_ior(0x2b)<<8);
	printf("read dm9000 pid = 0x%x\n", id);
	
	id = dm9000_ior(0x8) | (dm9000_ior(0x9)<<8);
	printf("read dm9000 reg(0x09,0x08) = 0x%x\n", id);
	
	printf("dm9000 isr = 0x%x\n", dm9000_ior(0xfe));
}

static void buzzer(int freq, int ms)
{
	rGPBCON &= ~3;			//set GPB0 as tout0, pwm output
	rGPBCON |= 2;
		
	rTCFG0 &= ~0xff;
	rTCFG0 |= 15;			//prescaler = 15+1
	rTCFG1 &= ~0xf;
	rTCFG1 |= 2;			//mux = 1/8
	rTCNTB0 = (PCLK>>7)/freq;
	rTCMPB0 = rTCNTB0>>1;	// 50%
	rTCON &= ~0x1f;
	rTCON |= 0xb;			//disable deadzone, auto-reload, inv-off, update TCNTB0&TCMPB0, start timer 0
	rTCON &= ~2;			//clear manual update bit
	
	mdelay(ms);
	
	
	rGPBCON &= ~3;			//set GPB0 as output
	rGPBCON |= 1;
	rGPBDAT &= ~1;
}

static U32 autorun_10ms;
static U16 autorun_ds;
static U16 autorun_trig;

static __irq void autorun_proc(void)
{
	ClearPending(BIT_TIMER4);

	if(autorun_ds)
		DisableIrq(BIT_TIMER4);
	
	autorun_10ms--;
	if(!autorun_10ms) {
		DisableIrq(BIT_TIMER4);
		//CLR_IF();	//in irq service routine, irq is disabled
		autorun_trig = 1;
		//NandLoadRun();
	}
}

static void init_autorun_timer(int sec)
{
	U32 val = (PCLK>>4)/100-1;
	
	autorun_10ms = sec*100;
	
	pISR_TIMER4 = (U32)autorun_proc;
	
	rTCFG0 &= ~(0xff<<8);
	rTCFG0 |= 3<<8;			//prescaler = 3+1
	rTCFG1 &= ~(0xf<<16);
	rTCFG1 |= 1<<16;		//mux = 1/4

	rTCNTB4 = val;
	rTCON &= ~(0xf<<20);
	rTCON |= 7<<20;			//interval, inv-off, update TCNTB4&TCMPB4, start timer 4
	rTCON &= ~(2<<20);		//clear manual update bit
	EnableIrq(BIT_TIMER4);
}

static U32 cpu_freq;
static U32 UPLL;
static void cal_cpu_bus_clk(void)
{
	U32 val;
	U8 m, p, s;
	
	val = rMPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;

	//(m+8)*FIN*2 不要超出32位数!
	FCLK = ((m+8)*(FIN/100)*2)/((p+2)*(1<<s))*100;
	
	val = rCLKDIVN;
	m = (val>>1)&3;
	p = val&1;	
	val = rCAMDIVN;
	s = val>>8;
	
	switch (m) {
	case 0:
		HCLK = FCLK;
		break;
	case 1:
		HCLK = FCLK>>1;
		break;
	case 2:
		if(s&2)
			HCLK = FCLK>>3;
		else
			HCLK = FCLK>>2;
		break;
	case 3:
		if(s&1)
			HCLK = FCLK/6;
		else
			HCLK = FCLK/3;
		break;
	}
	
	if(p)
		PCLK = HCLK>>1;
	else
		PCLK = HCLK;
	
	if(s&0x10)
		cpu_freq = HCLK;
	else
		cpu_freq = FCLK;
		
	val = rUPLLCON;
	m = (val>>12)&0xff;
	p = (val>>4)&0x3f;
	s = val&3;
	UPLL = ((m+8)*FIN)/((p+2)*(1<<s));
	if(UPLL==96*MEGA)
		rCLKDIVN |= 8;	//UCLK=UPLL/2
	UCLK = (rCLKDIVN&8)?(UPLL>>1):UPLL;
}

void IIC_test(void);
int search_vend_params(void)
{
	U8 dat[256];
	VenderParams *pVP = (VenderParams *)dat;
	
	//IIC_test();
	if(!read_24c02(dat)) {
		int i;
		
		//for(i=0; i<256; i++)
		//	Uart_Printf("%c0x%02x", (i%16)?' ':'\n', dat[i]);
		
		i = 0;
		if(strncmp(vend_params.vid.flags, pVP->vid.flags, sizeof(vend_params.vid.flags))==0)
			vend_params.vid.val = pVP->vid.val;
		else
			i = -1;

		if(strncmp(vend_params.pid.flags, pVP->pid.flags, sizeof(vend_params.pid.flags))==0)
			vend_params.pid.val = pVP->pid.val;
		else
			i = -1;
		if(strncmp(vend_params.ser_l.flags, pVP->ser_l.flags, sizeof(vend_params.ser_l.flags))==0)
			vend_params.ser_l.val = pVP->ser_l.val;
		else
			i = -1;
		if(strncmp(vend_params.ser_h.flags, pVP->ser_h.flags, sizeof(vend_params.ser_h.flags))==0)
			vend_params.ser_h.val = pVP->ser_h.val;
		else
			i = -1;
		if(strncmp(vend_params.user_params.flags, pVP->user_params.flags, sizeof(vend_params.user_params.flags))==0) {
			vend_params.user_params.val = pVP->user_params.val;
			memcpy(vend_params.string, pVP->string, sizeof(vend_params.string));
		} else
			i = -1;
		
		//if it's string, make sure the last char is 0
		if(vend_params.user_params.val)
			vend_params.string[sizeof(vend_params.string)-1] = 0;
	
		return i;
	}

	return -2;
}

int save_vend_params(void)
{
	return write_24c02((U8 *)&vend_params);
}

/*************************************************************/

void Main(void)
{
	char *mode;
	int i;
	U8 key;
	U32 mpll_val, divn_upll=0;
    
	#if ADS10   
	__rt_lib_init(); //for ADS 1.0
	#endif

	Port_Init();
	rGPACON &= ~(1<<11);
	rGPADAT |= 1<<11;
	// USB device detection control
/*	rGPGCON &= ~(3<<24);
	rGPGCON |=  (1<<24); // output
	rGPGUP  |=  (1<<12); // pullup disable
	rGPGDAT |=  (1<<12); // output	
*/	//masked by hzh

	//ChangeUPllValue(60,4,2);		// 48MHz
	//for(i=0; i<7; i++);
	//ChangeClockDivider(13,12);
	//ChangeMPllValue(97,1,2);		//296Mhz
	
/*
#if (FCLK==271500000)
	ChangeClockDivider(13,12);	//hzh
	ChangeMPllValue(173,2,2);	//271.5Mhz,2440A!
#elif (FCLK==304800000)
	ChangeClockDivider(13,12);	//hzh
	ChangeMPllValue(68,1,1);	//304.8MHz,2440A!
#elif (FCLK==200000000)
	ChangeMPllValue(92,4,1);	//200MHz,2440A!
	ChangeClockDivider(12,12);	//hzh
#elif (FCLK==240000000)
	ChangeClockDivider(13,12);	//hzh
	ChangeMPllValue(52,1,1);	//240MHz,2440A!
#elif (FCLK==300000000)
	ChangeClockDivider(14,11);	//hzh
	ChangeMPllValue(67,1,1);	//304.8MHz,2440A!
#elif (FCLK==320000000)
	ChangeClockDivider(14,11);	//hzh
	ChangeMPllValue(72,1,1);	//320MHz,2440A!
#elif (FCLK==330000000)
	ChangeClockDivider(14,11);	//hzh
	ChangeMPllValue(157,4,1);	//330MHz,2440A!
#elif (FCLK==340000000)
	ChangeClockDivider(14,11);	//hzh
	ChangeMPllValue(77,1,1);	//340MHz,2440A!
#elif (FCLK==350000000)
	ChangeClockDivider(14,11);	//hzh
	ChangeMPllValue(167,4,1);	//350MHz,2440A!
#elif (FCLK==360000000)
	ChangeClockDivider(14,12);	//hzh
	ChangeMPllValue(82,1,1);	//360MHz,2440A!
#elif (FCLK==380000000)
	ChangeClockDivider(14,12);	//hzh
	ChangeMPllValue(87,1,1);	//380MHz,2440A!
#elif (FCLK==400000000)
	ChangeClockDivider(14,12);	//hzh
	ChangeMPllValue(92,1,1);	//400MHz,2440A!
#endif
*/
	
	Isr_Init();
	
	//Led_Test();

	i = search_params();	//hzh, don't use 100M!
	switch (boot_params.cpu_clk.val) {
//	switch(1) {
	case 0:	//240
		key = 14;
		mpll_val = (112<<12)|(4<<4)|(1);
		break;
	case 1:	//320
		key = 14;
		mpll_val = (72<<12)|(1<<4)|(1);
		break;
	case 2:	//400
		key = 14;
		mpll_val = (92<<12)|(1<<4)|(1);
		break;
	case 3:	//440!!!
		key = 14;
		mpll_val = (102<<12)|(1<<4)|(1);
		break;
	default:
		key = 14;
		mpll_val = (92<<12)|(1<<4)|(1);
		break;
	}
#if 1
	//init FCLK=400M, so change MPLL first
	ChangeMPllValue((mpll_val>>12)&0xff, (mpll_val>>4)&0x3f, mpll_val&3);
	ChangeClockDivider(key, 12);
	cal_cpu_bus_clk();
	if(PCLK<(40*MEGA)) {
		ChangeClockDivider(key, 11);
		cal_cpu_bus_clk();
	}
#else
	cal_cpu_bus_clk();
#endif

	consoleNum=boot_params.serial_sel.val&1;	// Uart 1 select for debug.
	Uart_Init(0,boot_params.serial_baud.val);
	Uart_Select(consoleNum);
	//Uart_Select(0);
	
	LcdDisplay();
	
	//buzzer(2100, 200);
	while(0) {
		Uart_SendByte('a');
	//	rGPFDAT |= 1<<6;
	//	mdelay(500);
	//	rGPFDAT &= ~(1<<6);
	//	mdelay(500);
	}
	Uart_SendByte('\n');
	Uart_Printf("<*************************************************************>\n");
	Uart_Printf("          S3C2440 Bootloader V2006\n");
/*	switch (search_vend_params()) {
	case 0:
		Uart_Printf("Found vender params.\n");
		break;
	case -1:	
		Uart_Printf("Can't found vender params!\n");
		save_vend_params();
		break;
	case -2:
		Uart_Printf("Can't found EEPROM device!\n");
		break;
	default:
		break;
	}
	if(vend_params.user_params.val)
		Uart_Printf("        %s\n", vend_params.string);
	Uart_Printf("VID is 0x%08x, PID is 0x%08x\n", vend_params.vid.val, vend_params.pid.val);
	Uart_Printf("Serial NO. is %08x%08x\n", vend_params.ser_h.val, vend_params.ser_l.val);
*/
	//Uart_Printf("         www.ucdragon.com\n");
	//Uart_Printf("BWSCON = 0x%08x\n", rBWSCON);
	Uart_Printf("CPU ID is 0x%08x\n", rGSTATUS1);
	
	if(!i)
		Uart_Printf("Found boot params\n"); 
	else if(i==-1) {
		Uart_Printf("Fail to found boot params!\n");
		save_params();
	} else if(i==-2)
			Uart_Printf("Fail to read EEPROM!\n");
		
	Uart_Printf("FCLK=%dMHz, HCLK=%dMHz, PCLK=%dMHz, CPU is running at %dMHz\n",
					FCLK/MEGA, HCLK/MEGA, PCLK/MEGA, cpu_freq/MEGA);
	Uart_Printf("UPLL=%dMHz, UCLK=%dMHz\n", UPLL/MEGA, UCLK/MEGA);
	Uart_Printf("Serial port %d, Baud rate is %d.\n", boot_params.serial_sel.val, boot_params.serial_baud.val);
	Uart_Printf("OS image stored in %s Flash.\n", boot_params.osstor.val?"NOR":"NAND");
	Uart_Printf("Autoboot delay is %d seconds.\n", boot_params.boot_delay.val);
	if(boot_params.boot_delay.val)
		init_autorun_timer(boot_params.boot_delay.val);
	Uart_Printf("<*************************************************************>\n");

	rMISCCR=rMISCCR&~(1<<3); // USBD is selected instead of USBH1 
	rMISCCR=rMISCCR&~(1<<13); // USB port 1 is enabled.


//
//  USBD should be initialized first of all.
//
	isUsbdSetConfiguration=0;
	
//	rd_dm9000_id();			//hzh
//	rGPBCON &= ~(3<<20);	//CF_CARD Power
//	rGPBCON |= 1<<20;
//	rGPBDAT |= 1<<10;
//	rDSC0 = 0x155;
//	rDSC1 = 0x15555555;
	rDSC0 = 0x2aa;
	rDSC1 = 0x2aaaaaaa;
//	rDSC0 = 0x3ff;
//	rDSC1 = 0x3fffffff;
	//Enable NAND, USBD, PWM TImer, UART0,1 and GPIO clock,
	//the others must be enabled in OS!!!
	//rCLKCON = (1<<4)|(3<<7)|(3<<10)|(1<<13);
	if(0) {
		int i;
		volatile U16 *p = (volatile U16 *)0x08000000;
		
		p[3] = 0xbf;
		p[2] = 0;
		p[3] = 0;
		p[2] = 1;
		printf("dr2=0x%04x\n", p[2]);
		
		for(i=0; i<8; i++)
			printf("0x%08x\n", p[i]);
		
	}

#if 0
#define	CS8900A_BASE	0x19000300
		//0xa = address port, 0xc=data port
		//Chip ID
		*(volatile U16 *)(CS8900A_BASE+0xa) = 0;
		i = *(volatile U16 *)(CS8900A_BASE+0xc);
		printf("0x%04x\n", i);
		//Product ID
		*(volatile U16 *)(CS8900A_BASE+0xa) = 2;
		i = *(volatile U16 *)(CS8900A_BASE+0xc);
		printf("0x%04x\n", i);
		*(volatile U16 *)(CS8900A_BASE+0xa) = 0x136;
		i = *(volatile U16 *)(CS8900A_BASE+0xc);
		//if(i&0x80)	//SelfST. INITD bit
		//	break;
#endif

#if 0
	UsbdMain(); 
	MMU_Init(); //MMU should be reconfigured or turned off for the debugger, 
	//After downloading, MMU should be turned off for the MMU based program,such as WinCE.	
#else
	//MMU_EnableICache();
		MMU_Init();	//hzh
		Delay(0);	//calibrate Delay() first, hzh
		Uart_Printf("Check SST39VF160 Flash ID is 0x%08x\n", GetFlashID());
		ChkNorFlash();	//28F128
  #ifdef DEBUG_VERSION
		comdownload();	//hzh
	//	SectorProg(0, (U16 *)downloadAddress, downloadFileSize);
		NandWrite();
  #endif
	UsbdMain(); 
#endif
//	Delay(0);  //calibrate Delay()

	find_camera();	//hzh
	
	pISR_SWI=(_ISR_STARTADDRESS+0xf0);	//for pSOS

	Led_Display(0x6);

#if USBDMA
	mode="DMA";
#else
	mode="Int";
#endif

	// CLKOUT0/1 select.
	//Uart_Printf("CLKOUT0:MPLL in, CLKOUT1:RTC clock.\n");
	//Clk0_Enable(0);	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	//Clk1_Enable(2);	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	Clk0_Disable();
	Clk1_Disable();
	
	mpll_val = rMPLLCON;
	Uart_Printf("DIVN_UPLL%x\n", divn_upll);
	Uart_Printf("MPLLVal [M:%xh,P:%xh,S:%xh]\n", (mpll_val&(0xff<<12))>>12,(mpll_val&(0x3f<<4))>>4,(mpll_val&0x3));
	Uart_Printf("CLKDIVN:%xh\n", rCLKDIVN);

	Uart_Printf("\n\n");
	Uart_Printf("+---------------------------------------------+\n");
	Uart_Printf("| S3C2440A USB Downloader ver R0.03 2004 Jan  |\n");
	Uart_Printf("+---------------------------------------------+\n");
	Uart_Printf("FCLK=%4.1fMHz,%s mode\n",FCLK/1000000.,mode); 
	Uart_Printf("USB: IN_ENDPOINT:1 OUT_ENDPOINT:3\n"); 
	Uart_Printf("FORMAT: <ADDR(DATA):4>+<SIZE(n+10):4>+<DATA:n>+<CS:2>\n");
	Uart_Printf("NOTE: 1. Power off/on or press the reset button for 1 sec\n");
	Uart_Printf("		 in order to get a valid USB device address.\n");
	Uart_Printf("	  2. For additional menu, Press any key. \n");
	Uart_Printf("\n");

	download_run=1; //The default menu is the Download & Run mode.

	while(1)
	{
		if(menuUsed==1)Menu();
		WaitDownload();	
	}

}



void Menu(void)
{
	int i;
	U8 key;
	menuUsed=1;
	while(1)
	{
		Uart_Printf("\n###### Select Menu ######\n");
		Uart_Printf(" [0] Download & Run\n");
		Uart_Printf(" [1] Download Only\n");
	//	Uart_Printf(" [2] Test SDRAM \n");
	//	Uart_Printf(" [3] Change The Console UART Ch.\n");
	//	Uart_Printf(" [4] Clear unused area in SDRAM \n");
		Uart_Printf(" [2] Download From UART\n");
		Uart_Printf(" [3] Write File to SST39VF160\n");
	//	Uart_Printf(" [5] Write File to TE28F128\n");
		Uart_Printf(" [4] Write File to NAND Flash\n");
		Uart_Printf(" [5] Boot OS\n");	
		Uart_Printf(" [6] Erase NAND Flash Partition\n");
		Uart_Printf(" [7] Config parameters\n");
		Uart_Printf(" [8] Relocate NK.bin\n");
		key=Uart_Getch();
		
		switch(key)
		{
		case '0':
			Uart_Printf("\nDownload&Run is selected.\n\n");
			download_run=1;
			return;
		case '1':
			Uart_Printf("\nDownload Only is selected.\n");
			Uart_Printf("Enter a new temporary download address(0x3...):");
			tempDownloadAddress=Uart_GetIntNum();
			download_run=0;
			Uart_Printf("The temporary download address is 0x%x.\n\n",tempDownloadAddress);
			return;
		//case '2':
		//	Uart_Printf("\nMemory Test is selected.\n");
		//MemoryTest();
		//Menu();
		//return;
		//	break;
/*		case '3':
			Uart_Printf("\nWhich UART channel do you want to use for the console?[0/1]\n");
			if(Uart_Getch()!='1')
			{
			*pMagicNum=0x0;
		Uart_Printf("UART ch.0 will be used for console at next boot.\n");					
		}
		else
		{
			*pMagicNum=0x12345678;
 		Uart_Printf("UART ch.1 will be used for console at next boot.\n");		
				Uart_Printf("UART ch.0 will be used after long power-off.\n");
		}
			Uart_Printf("System is waiting for a reset. Please, Reboot!!!\n");
			while(1);
			break;
		case '4':
			Uart_Printf("\nMemory clear is selected.\n");
			ClearMemory();
		break;
*/		case '2':
			comdownload();
		break;
		case '3':
			if(downloadFileSize)
				SectorProg(0, (U16 *)downloadAddress, downloadFileSize);
		break;
		/*case '5':
		{
			int c;
			Uart_Printf("Where to program?\n1 : 0x00000000\n2 : 0x00020000\n3 : 0x00200000\nEsc to abort\n");
			while(1) {
				c = Uart_Getch();
				if(c==0x1b||c=='1'||c=='2'||c=='3')
					break;
			}
			if(c>='1'&&c<='3') {
				U32 addr = 0;
				if(c=='2')
					addr = 0x00020000;
				if(c=='3')
					addr = 0x00200000;
				ProgNorFlash(addr, downloadAddress, downloadFileSize);
			}
			break;
		}*/
		case '4':
			if(downloadFileSize)
				NandWrite();
		break;
		case '5':
			NandLoadRun();
		break;
		case '6':
			NandErase();
		break;
		case '7':
			set_params();
			break;
		case '8':
		//case 'A':
		{
			U32 launch;
			if(!RelocateNKBIN(downloadAddress, (U32 *)&downloadAddress, (U32 *)&downloadFileSize, &launch)) {
				boot_params.run_addr.val    = launch;
				boot_params.initrd_addr.val = downloadAddress;
				boot_params.initrd_len.val  = downloadFileSize;
				save_params();	//save initrd_len.val
			}
		}
			break;
	/*	case 'c':
		case 'C':
		{
			int i;
			U32 *p1 = (U32 *)downloadAddress;
			U32 *p2 = (void *)0x32000000;
			for(i=0; i<(boot_params.initrd_len.val/4); i++)
				if(p1[i]!=p2[i])
					printf("0x%08x, p1 0x%08x, p2 0x%08x\n", i*4, p1[i], p2[i]);
		}
			break;	
	*/	default:
			break;
	}	
	}		

}



void WaitDownload(void)
{
	U32 i;
	U32 j;
	U16 cs;
	U32 temp;
	U16 dnCS;
	int first=1;
	float time;
	U8 tempMem[16];
	U8 key;
	
	checkSum=0;
	downloadAddress=(U32)tempMem; //_RAM_STARTADDRESS; 
	downPt=(unsigned char *)downloadAddress;
	//This address is used for receiving first 8 byte.
	downloadFileSize=0;
	
#if 0
	MMU_DisableICache(); 
		//For multi-ICE. 
		//If ICache is not turned-off, debugging is started with ICache-on.
#endif

	/*******************************/
	/*	Test program download	*/
	/*******************************/
	j=0;

	if(isUsbdSetConfiguration==0)
	{
	Uart_Printf("USB host is not connected yet.\n");
	}

	while(downloadFileSize==0)
	{
		if(first==1 && isUsbdSetConfiguration!=0)
		{
			Uart_Printf("USB host is connected. Waiting a download.\n");
			first=0;
		}

	if(j%0x50000==0)Led_Display(0x6);
	if(j%0x50000==0x28000)Led_Display(0x9);
	j++;

	key=Uart_GetKey();
	if(autorun_trig)
		NandLoadRun();	//run it in svc mode
	if(key!=0)
	{
		autorun_ds = 1;
		//printf("disable autorun\n");
		Menu();
			first=1; //To display the message,"USB host ...."
//在串口下载返回后downloadFileSize不为0,因此不能再执行USB下载! hzh
	}
	}
	
	autorun_ds = 1;
	//printf("disable autorun\n");
	
	Timer_InitEx();	  
	Timer_StartEx();  

#if USBDMA	

	rINTMSK&=~(BIT_DMA2);  

	ClearEp3OutPktReady(); 
		// indicate the first packit is processed.
		// has been delayed for DMA2 cofiguration.

	if(downloadFileSize>EP3_PKT_SIZE)
	{
		if(downloadFileSize<=(0x80000))
		{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,downloadFileSize-EP3_PKT_SIZE);
 
	  		//will not be used.
/*	   rDIDST2=(downloadAddress+downloadFileSize-EP3_PKT_SIZE);  
		   rDIDSTC2=(0<<1)|(0<<0);  
		rDCON2=rDCON2&~(0xfffff)|(0);				
*/
		}
	  	else
	  	{
	  		ConfigEp3DmaMode(downloadAddress+EP3_PKT_SIZE-8,0x80000-EP3_PKT_SIZE);
	  		//2440比2410的DIDSTCx寄存器多了中断产生条件的控制位,USB的DMA传输为字节计数
	  		//防止高频开cache运行时下载大于0x80000字节文件时IsrDma2出错!!! hzh
	  		//while((rDSTAT2&0xfffff)==(0x80000-EP3_PKT_SIZE));
	  		while(!(rDSTAT2&(1<<20)));	//防止DMA传输尚未开始就写入下一次重装值!!! hzh
			if(downloadFileSize>(0x80000*2))//for 1st autoreload
			{
				rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
			 rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
				rDCON2=rDCON2&~(0xfffff)|(0x80000);			  

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
			}	
 		else
 		{
 			rDIDST2=(downloadAddress+0x80000-8);  //for 1st autoreload.
	  			rDIDSTC2=(1<<2)|(0<<1)|(0<<0);  
 			rDCON2=rDCON2&~(0xfffff)|(downloadFileSize-0x80000); 		

  		while(rEP3_DMA_TTC<0xfffff)
  		{
  			rEP3_DMA_TTC_L=0xff; 
  			rEP3_DMA_TTC_M=0xff;
  			rEP3_DMA_TTC_H=0xf;
  		}
		}
	}
 	totalDmaCount=0;
	}
	else
	{
	totalDmaCount=downloadFileSize;
	}
#endif

	Uart_Printf("\nNow, Downloading [ADDRESS:%xh,TOTAL:%d]\n",
			downloadAddress,downloadFileSize);
	Uart_Printf("RECEIVED FILE SIZE:%8d",0);
   
#if USBDMA	
	j=0x10000;

	while(1)
	{
		if( (rDCDST2-(U32)downloadAddress+8)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
		}
	if(totalDmaCount>=downloadFileSize)break;
	}

#else
	j=0x10000;

	while(((U32)downPt-downloadAddress)<(downloadFileSize-8))
	{
	if( ((U32)downPt-downloadAddress)>=j)
	{
		Uart_Printf("\b\b\b\b\b\b\b\b%8d",j);
   		j+=0x10000;
	}
	}
#endif

	time=Timer_StopEx();
	
	Uart_Printf("\b\b\b\b\b\b\b\b%8d",downloadFileSize);	
	Uart_Printf("\n(%5.1fKB/S,%3.1fS)\n",(float)(downloadFileSize/time/1000.),time);
	
#if USBDMA	
	/*******************************/
	/*	 Verify check sum		*/
	/*******************************/

	Uart_Printf("Now, Checksum calculation\n");

	cs=0;	
	i=(downloadAddress);
	j=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	while(i<j)
	{
		temp=*((U32 *)i);
		i+=4;
		cs+=(U16)(temp&0xff);
		cs+=(U16)((temp&0xff00)>>8);
		cs+=(U16)((temp&0xff0000)>>16);
		cs+=(U16)((temp&0xff000000)>>24);
	}

	i=(downloadAddress+downloadFileSize-10)&0xfffffffc;
	j=(downloadAddress+downloadFileSize-10);
	while(i<j)
	{
  	cs+=*((U8 *)i++);
	}
	
	checkSum=cs;
#else
	//checkSum was calculated including dnCS. So, dnCS should be subtracted.
	checkSum=checkSum - *((unsigned char *)(downloadAddress+downloadFileSize-8-2))
		 - *( (unsigned char *)(downloadAddress+downloadFileSize-8-1) );	
#endif	  

	dnCS=*((unsigned char *)(downloadAddress+downloadFileSize-8-2))+
	(*( (unsigned char *)(downloadAddress+downloadFileSize-8-1) )<<8);

	if(checkSum!=dnCS)
	{
	Uart_Printf("Checksum Error!!! MEM:%x DN:%x\n",checkSum,dnCS);
	return;
	}

	Uart_Printf("Download O.K.\n\n");
	Uart_TxEmpty(consoleNum);


	if(download_run==1)
	{
		register void(*run)(void);	//hzh,使用寄存器变量以防止禁止DCACHE后数据不一致!!!
		rINTMSK=BIT_ALLMSK;
		run=(void (*)(void))downloadAddress;	//使用DCACHE且RW区也在CACHE区间downloadAddress会在cache中
		{	//hzh
			MMU_DisableDCache();	//download program must be in 
    		MMU_DisableICache();	//non-cache area
    		MMU_InvalidateDCache();	//使所有DCACHE失效,本程序的MMU_Init中将会刷新DCACHE到存储器,
    								//为使应用此MMU_Init方式的程序能被正确运行必须先使DCACHE失效!!!
    		MMU_DisableMMU();
    		//call_linux(0, 193, downloadAddress);	//或不用上面3个函数而直接使用call_linux
		}
	run();
	}
}




void Isr_Init(void)
{
	pISR_UNDEF=(unsigned)HaltUndef;
	pISR_SWI  =(unsigned)HaltSwi;
	pISR_PABORT=(unsigned)HaltPabort;
	pISR_DABORT=(unsigned)HaltDabort;
	rINTMOD=0x0;	  // All=IRQ mode
	rINTMSK=BIT_ALLMSK;	  // All interrupt is masked.

	//pISR_URXD0=(unsigned)Uart0_RxInt;	
	//rINTMSK=~(BIT_URXD0);   //enable UART0 RX Default value=0xffffffff

#if 1
	pISR_USBD =(unsigned)IsrUsbd;
	pISR_DMA2 =(unsigned)IsrDma2;
#else
	pISR_IRQ =(unsigned)IsrUsbd;	
		//Why doesn't it receive the big file if use this. (???)
		//It always stops when 327680 bytes are received.
#endif	
	ClearPending(BIT_DMA2);
	ClearPending(BIT_USBD);
	//rINTMSK&=~(BIT_USBD);  
   
	//pISR_FIQ,pISR_IRQ must be initialized
}


void HaltUndef(void)
{
	Uart_Printf("Undefined instruction exception!!!\n");
	while(1);
}

void HaltSwi(void)
{
	Uart_Printf("SWI exception!!!\n");
	while(1);
}

void HaltPabort(void)
{
	Uart_Printf("Pabort exception!!!\n");
	while(1);
}

void HaltDabort(void)
{
	Uart_Printf("Dabort exception!!!\n");
	while(1);
}


void ClearMemory(void)
{
	int i;
	U32 data;
	int memError=0;
	U32 *pt;
	
	//
	// memory clear
	//
	Uart_Printf("Clear Memory (%xh-%xh):WR",_RAM_STARTADDRESS,HEAPEND);

	pt=(U32 *)_RAM_STARTADDRESS;
	while((U32)pt < HEAPEND)
	{
		*pt=(U32)0x0;
		pt++;
	}
	
	if(memError==0)Uart_Printf("\b\bO.K.\n");
}

void Clk0_Enable(int clock_sel)	
{	// 0:MPLLin, 1:UPLL, 2:FCLK, 3:HCLK, 4:PCLK, 5:DCLK0
	rMISCCR = rMISCCR&~(7<<4) | (clock_sel<<4);
	rGPHCON = rGPHCON&~(3<<18) | (2<<18);
}
void Clk1_Enable(int clock_sel)
{	// 0:MPLLout, 1:UPLL, 2:RTC, 3:HCLK, 4:PCLK, 5:DCLK1	
	rMISCCR = rMISCCR&~(7<<8) | (clock_sel<<8);
	rGPHCON = rGPHCON&~(3<<20) | (2<<20);
}
void Clk0_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<18);	// GPH9 Input
}
void Clk1_Disable(void)
{
	rGPHCON = rGPHCON&~(3<<20);	// GPH10 Input
}

