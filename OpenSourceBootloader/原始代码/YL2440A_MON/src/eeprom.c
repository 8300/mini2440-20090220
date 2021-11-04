/*
 create by hzh, to support wr/rd parameters to/from eeprom
*/
#include "def.h"
#include "2440addr.h"
#include "2440lib.h"

#define	SLV_ADDR_24C02	0xa0
#define	SLV_ADDR_CAMERA	0x5a

void mdelay(int ms);

static volatile int iic_ints;
static void __irq IicInt(void)
{
	ClearPending(BIT_IIC);
	iic_ints++;
}

static int iic_wr(U8 data, U8 cnt)
{
	int i, c;

	for(i=0; i<20; i++);
	
	if(cnt)
		rIICDS = data;
		
	rIICCON &= ~0x10;		//Resume IIC operation to send

	i = 3;	//3ms
	//while(!(rIICCON&0x10)&&i--) mdelay(1);
	c = iic_ints;
	while(iic_ints==c&&i--) mdelay(1);
	//rIICCON &= ~0x10;
	
	if(!i)
		return -1;

	return (rIICSTAT&1)?-1:0;
}

static int iic_rd(U8 *pBuf)
{
	int i, c;
	
	for(i=0; i<20; i++);
	
	rIICCON &= ~0x10;		//Resumes IIC operation to receive
	i = 3;	//3ms
	//while(!(rIICCON&0x10)&&i--) mdelay(1);
	c = iic_ints;
	while(iic_ints==c&i--) mdelay(1);
	
	if(i) {
		*pBuf = rIICDS;
		return 0;
	}
	
	return -1;
}

static int seek_iic_device(U8 slvAddr)
{
	int i;
	U8 d;
	
	i = 200;				//200ms
	while(i--) {
		rIICDS   = slvAddr;
		rIICSTAT = 0xb0;	//read mode
		if(!iic_wr(0, 0)) {
			iic_rd(&d);
			break;
		}
		mdelay(1);
	}
	rIICSTAT = 0x90;
	rIICCON &= ~0x10;		//Resume IIC operation to send stop
	mdelay(1);

	return (i>=0)?0:-1;
}

//8 bits register address only
static int write_eeprom(U8 slvAddr, U8 addr, U8 data)
{
	int i;
	U8 ch[3];
	
	ch[1] = addr;
	ch[2] = data;
	
	rIICDS   = slvAddr;
	rIICSTAT = 0xf0;		//MasTx,Start
	for(i=0; i<3; i++)
		if(iic_wr(ch[i], i))
			break;
	rIICSTAT = 0xd0;		//Stop MasTx condition
	rIICCON &= ~0x10;		//Resume IIC operation to send stop
	mdelay(1);
	if(i<3)
		return -1;
		
	return seek_iic_device(slvAddr);
}

//8 bits register address only
static int read_eeprom(U8 slvAddr, U8 addr, U8 *pBuf)
{
	int i;
	U8 ch[2];
	
	ch[1] = addr;
	
	rIICDS   = slvAddr;
	rIICSTAT = 0xf0;		//MasTx,Start
	for(i=0; i<2; i++)
		if(iic_wr(ch[i], i))
			break;
	if(i<2) {
		rIICSTAT = 0xd0;	//Stop MasTx condition
		rIICCON &= ~0x10;	//Resume IIC operation to send stop
		mdelay(1);
		return -1;
	}

	rIICDS   = slvAddr;
	rIICSTAT = 0xb0;		//Master Rx,Start
	if(!iic_wr(0, 0))		//wait ack
		i = iic_rd(pBuf);
	rIICSTAT = 0x90;		//Stop MaxRx condition
	rIICCON &= ~0x10;		//Resume IIC operation to send stop
	mdelay(1);
	
	return i;
}

static void init_iic_op(void)
{
	rCLKCON |= 1<<16;
	
	rGPEUP  |= 0xc000;                  //Pull-up disable
	rGPECON &= ~0xf0000000;
	rGPECON |= 0xa0000000;              //GPE15:IICSDA , GPE14:IICSCL 
	
	//Enable ACK, Prescaler IICCLK=PCLK/16, Enable interrupt, Transmit clock value Tx clock=IICCLK/16
	// If PCLK 50.7MHz, IICCLK = 3.17MHz, Tx Clock = 0.198MHz
	//rIICCON = (1<<7) | (1<<6) | (1<<5) | (0);	//IICCLK=PCLK/512=97K
	//just read one byte, don't enable ack!
	rIICCON = (0<<7) | (1<<6) | (1<<5) | (0);	//IICCLK=PCLK/512=97K
	
	rIICSTAT = 0x10;                    //IIC bus data output enable(Rx/Tx)
	rIICADD  = 0x10;                    //2410 slave address = [7:1]
	
	pISR_IIC = (unsigned)IicInt;
	EnableIrq(BIT_IIC);
}

static void end_iic_op(void)
{
//	rCLKCON &= ~(1<<16);
	DisableIrq(BIT_IIC);
}

int write_24c02(U8 *pBuf)
{
	int i;
	
	init_iic_op();
	
	for(i=0; i<256; i++) {
		//if(seek_iic_device(SLV_ADDR_24C02))
		//	break;
		if(write_eeprom(SLV_ADDR_24C02, i, pBuf[i]))
			break;
	}
	
	end_iic_op();
	
	Uart_Printf("Write EEPROM %s!\n", (i<256)?"Fail":"Success");
	
	return i<256;
}

int read_24c02(U8 *pBuf)
{
	int i;
	
	init_iic_op();
	
	for(i=0; i<256; i++) {
		if(read_eeprom(SLV_ADDR_24C02, i, pBuf+i))
			break;
	}
	
	end_iic_op();
	
	Uart_Printf("Read EEPROM %s!\n", (i<256)?"Fail":"Success");

	return i<256;
}

int find_camera(void)
{
	int ret = -2;
	U8 d[256];
	
	rGPGCON &= ~(3<<24);
	rGPGCON |= 1<<24;
	rGPGUP  &= ~(1<<12);
	rGPGDAT |= 1<<12;
	
	rGPJCON = 0x1aaaaaa;
	rGPJUP  = 0;
	rGPJDAT = 0;
	mdelay(10);
	rGPJDAT = 1<<12;
	mdelay(10);
	
	init_iic_op();
	
	if(!write_eeprom(SLV_ADDR_CAMERA, 0xec, 0)) {
		int i, a;
		
		ret = -1;
		a = 0xb0;
		for(i=0; i<4; i++)
			if(read_eeprom(SLV_ADDR_CAMERA, a+i, d+i))
				break;
		if(i>=4)
			ret = 0;
		
	//	Uart_Printf("i = %d\n", i);
	//	for(i=0; i<256; i++)
	//		Uart_Printf("%c%02x", (i%16)?' ':'\n', d[i]);
	}
	
	end_iic_op();

	switch(ret)	{
	case -2:
		Uart_Printf("Fail to write camera IIC!\n");
		break;
	case -1:
		Uart_Printf("Fail to read camera IIC!\n");
		break;
	case 0:
		Uart_Printf("Read camera ID is 0x%04x, ", (d[0]<<8)|d[1]);
		Uart_Printf("chip version is 0x%04x\n", (d[2]<<8)|d[3]);
		break;
	default:
		break;
	}
		
	return ret;
}