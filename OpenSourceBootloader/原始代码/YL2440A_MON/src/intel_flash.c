#include "def.h"
#include "2440lib.h"
#define	printf	Uart_Printf
#define	puts	Uart_SendString


#include "norflash.h"

//#define	out_l(v, a)	(*(volatile U32 *)(a) = (v))
//#define	in_l(a)		(*(volatile U32 *)(a))
#define	out_w(v, a)	(*(volatile U16 *)(a) = (v))
#define	in_w(a)		(*(volatile U16 *)(a))

#define	ROM_BASE		0x04000000

static U32 FlashSize;

static void ReadArray(void)
{
	out_w(0x00ff, ROM_BASE);
}

static void ClrStatus(void)
{
	out_w(0x0050, ROM_BASE);
}

static int ReadStatus(void)
{
	int ret;
	
	out_w(0x0070, ROM_BASE);
	ret = in_w(ROM_BASE);
	ReadArray();
	return ret;
}
/*
static int GetBlkProtStat(U32 addr)
{
	addr &= ~0x3ffff;
	out_l(0x00900090, ROM_BASE+addr+8);
	return in_l(ROM_BASE+addr+8)&0x10001;
}

static int LockBlk(U32 addr)
{
	int ret;
	
	addr &= ~0x3ffff;
	out_l(0x00600060, ROM_BASE);
	out_l(0x10001, ROM_BASE+addr);
	do {
		ret = ReadStatus();
	} while((ret&0x00800080)!=0x00800080);
	ClrStatus();
	
	printf("Lock Block 0x%x ", addr);
	if(ret&0x1a) {
		printf("Fail, status=%x\n", ret);
		return -1;
	}
	puts("OK\n");
	return 0;
}
*/
int UnlockAllBlks(void)
{
	int ret;
	
	out_w(0x0060, ROM_BASE);
	out_w(0x00d0, ROM_BASE);
	do {
		ret = ReadStatus();
	} while((ret&0x0080)!=0x0080);
	ClrStatus();
	
	puts("Unprotect All Blocks ");
	if(ret&0x002a) {
		printf("Fail, status=%x\n", ret);
	}
	puts("OK\n");
	return 0;
}

static int EraseBlk(U32 addr)
{
	int ret;
	
	addr &= ~0x1ffff;
	addr += ROM_BASE;
	out_w(0x0020, addr);
	out_w(0x00d0, addr);
	do {
		ret = ReadStatus();
	} while((ret&0x0080)!=0x0080);
	ClrStatus();
	
	printf("Erase Block 0x%x ", addr);
	if(ret&0x006a) {
		printf("Fail, status=%x\n", ret);
		return -1;	
	}
	puts("OK\n");
	return 0;
}

static int ProgramBlk(U32 addr, U32 src, U32 len)
{
	int ret;
	U32 i;
	
	addr += ROM_BASE;
	
/*	for(i=0; i<len; i+=4) {
		out_l(0x00400040, addr+i);
		out_l(*(U32 *)(src+i), addr+i);
		do {
			ret = ReadStatus();
		} while((ret&0x00800080)!=0x00800080);
		ClrStatus();
		
		if(ret&0x1e) {
			printf("Program addr 0x%x Fail, status=%x\n", addr, ret);
			return -1;
		}
	}
*/	
	for(i=0; i<len; i+=32) {
		int j;
		do {
			out_w(0x00e8, addr);
			ret = in_w(ROM_BASE);//ReadStatus();
		} while((ret&0x0080)!=0x0080);
		out_w(0x000f, addr);
		for(j=0; j<16; j++)
			out_w(*(U16 *)(src+i+j*2), addr+i+j*2);
		out_w(0x00d0, addr);
		do {
			ret = ReadStatus();
		} while((ret&0x0080)!=0x0080);
		ClrStatus();
		
		if(ret&0x001e) {
			printf("Program addr 0x%x Fail, status=%x\n", addr, ret);
			return -1;
		}		
	}
		

	ReadArray();
	for(i=0; i<len; i+=2)
		if(in_w(addr+i)!=*(U16 *)(src+i)) {
			printf("Program addr 0x%x Fail, wr=0x%x, rd=0x%x\n", addr+i, *(U16 *)(src+i), in_w(addr+i));
			return -1;
		}
	
	ret = 0;//LockBlk(addr);
	
	return ret;
}

void ProgNorFlash(U32 addr, U32 src, U32 len)
{
	U32 prog_len;
	
	if((addr&0x1ffff)||(src&1)) {
		puts("Flash Address Must Be 128KBytes And Source Address Must Be 2Bytes aligned\n");
		return;
	}
	if(!len) {
		puts("Write 0 Bytes!\n");
		return;
	}
	if(UnlockAllBlks())
		return;
//	ReadArray();
	for(; len; ) {
		prog_len = (len>0x20000)?0x20000:len;
		EraseBlk(addr);
//		ReadArray();
		//printf("data at 0x%x = 0x%x\n", addr, in_w(ROM_BASE+addr));
		//printf("data at 0x%x = 0x%x\n", src,  *(U16 *)(src));
		if(ProgramBlk(addr, src, prog_len))
			break;//return;
//		ReadArray();
		//for(i=0; i<prog_len; i+=2)
		//	printf("%x\n", in_w(addr+i));
		addr += prog_len;
		src  += prog_len;
		len  -= prog_len;
	}
}

int ChkNorFlash(void)
{
	U32 id;
	
	out_w(0x0090, ROM_BASE);
	id = in_w(ROM_BASE);
	printf("NOR Flash Man. ID is 0x%x\n", id&0xffff);
	if(id==0x0089) {
		printf("Intel Flash Found With 16 Bits Bus Width\n");
	} else {
		printf("Unsupported Flash Type!\n");
		return -1;
	}
	
	out_w(0x0090, ROM_BASE+2);
	id = in_w(ROM_BASE+2);
	
	ReadArray();
	
	printf("NOR Flash Dev. ID is 0x%x\n", id&0xffff);
	id &= 0xffff;
	if(id==0x16) {
		FlashSize = 4;	//4MBytes
	} else if(id==0x17) {
		FlashSize = 8;	//8MBytes
	} else if(id==0x18) {
		FlashSize = 16;	//16MBytes		
	} else {
		puts("Unsupported Intel Flash Type!\n");
		return -1;
	}
	printf("Total FlashSize = %dMBytes\n", FlashSize);
	return 0;
}
