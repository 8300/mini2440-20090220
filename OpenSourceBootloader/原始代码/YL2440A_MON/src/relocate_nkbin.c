#include <string.h>

#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"

void call_linux(U32 a0, U32 a1, U32 a2);

#define	ULONG	U32
#define	USHORT	U16
#define	PVOID	void *

#define	getch	Uart_Getch
#define	printf	Uart_Printf
#define	PHY_START	0x30000000
#define	VIR_START	0x8c000000
#define	mem_convert(addr)	((addr)- VIR_START + PHY_START)

//in romldr.h
#define ROM_SIGNATURE_OFFSET 64
#define ROM_SIGNATURE 0x43454345

typedef struct ROMHDR {
    ULONG   dllfirst;               // first DLL address
    ULONG   dlllast;                // last DLL address
    ULONG   physfirst;              // first physical address
    ULONG   physlast;               // highest physical address
    ULONG   nummods;                // number of TOCentry's
    ULONG   ulRAMStart;             // start of RAM
    ULONG   ulRAMFree;              // start of RAM free space
    ULONG   ulRAMEnd;               // end of RAM
    ULONG   ulCopyEntries;          // number of copy section entries
    ULONG   ulCopyOffset;           // offset to copy section
    ULONG   ulProfileLen;           // length of PROFentries RAM 
    ULONG   ulProfileOffset;        // offset to PROFentries
    ULONG   numfiles;               // number of FILES
    ULONG   ulKernelFlags;          // optional kernel flags from ROMFLAGS .bib config option
    ULONG   ulFSRamPercent;         // Percentage of RAM used for filesystem 
                                        // from FSRAMPERCENT .bib config option
                                        // byte 0 = #4K chunks/Mbyte of RAM for filesystem 0-2Mbytes 0-255
                                        // byte 1 = #4K chunks/Mbyte of RAM for filesystem 2-4Mbytes 0-255
                                        // byte 2 = #4K chunks/Mbyte of RAM for filesystem 4-6Mbytes 0-255
                                        // byte 3 = #4K chunks/Mbyte of RAM for filesystem > 6Mbytes 0-255

    ULONG   ulDrivglobStart;        // device driver global starting address
    ULONG   ulDrivglobLen;          // device driver global length
    USHORT  usCPUType;              // CPU (machine) Type
    USHORT  usMiscFlags;            // Miscellaneous flags
    PVOID   pExtensions;            // pointer to ROM Header extensions
    ULONG   ulTrackingStart;        // tracking memory starting address
    ULONG   ulTrackingLen;          // tracking memory ending address
} ROMHDR;

typedef struct {
	U32 start;
	U32 length;
	U32 chksum;
} CtxRecord;

static U32 GetU32Num(char *src)
{
	return (src[3]<<24)|(src[2]<<16)|(src[1]<<8)|src[0];
}

static int CalChkSum(U8 *pDst, U32 len, U32 chksum)
{
	U32 i, tmp;
	
	tmp = 0;
	for(i=0; i<len; i++)
		tmp += pDst[i];
	
	if(tmp!=chksum)
		return -1;
	
	return 0;
}

int RelocateNKBIN(U32 img_src, U32 *pStart, U32 *pLength, U32 *pLaunch)
{
	U32 img_start, img_length;
	CtxRecord *pCtxRecord;
	
	if(strncmp("B000FF\x0a", (char *)img_src, 7)) {
		char *c = (char *)img_src;
		printf("Invalid BIN file signature!\n");
		printf("%x,%x,%x,%x,%x,%x,%x\n", c[0],c[1],c[2],c[3],c[4],c[5],c[6]);
		return -1;
	}
	
	printf("Found single BIN signature.\n");
	img_src += 7;
	img_start = GetU32Num((char *)img_src);
	img_src += sizeof(U32);
	img_length = GetU32Num((char *)img_src);
	printf("Image start is 0x%08x, length is 0x%08x\n", img_start, img_length);
	
	memset((char *)mem_convert(img_start), 0, 0x1000);	//clear 4K
	
	img_src += sizeof(U32);
	while(1) {
		U32 start, length, chksum;

		pCtxRecord = (CtxRecord *)img_src;
		start  = GetU32Num((char *)&pCtxRecord->start);
		length = GetU32Num((char *)&pCtxRecord->length);
		chksum = GetU32Num((char *)&pCtxRecord->chksum);
		img_src += sizeof(CtxRecord);
		
		if(!start&&!chksum) {
			*pLaunch = mem_convert(length);
			printf("real entry is 0x%08x, physical address is 0x%08x\n", length, *pLaunch);
			break;
		}
		printf("REC start 0x%08x, length 0x%x\n", start, length);
		
		memcpy((char *)mem_convert(start), (char *)img_src, length);
		img_src += length;
		
		if(CalChkSum((U8 *)mem_convert(start), length, chksum)) {
			printf("Checksum fail!\n");
			return -1;
		}
		
		if(length==sizeof(ROMHDR)&&
			*(U32 *)mem_convert(img_start+ROM_SIGNATURE_OFFSET)==ROM_SIGNATURE&&
			*(U32 *)mem_convert(img_start+ROM_SIGNATURE_OFFSET+4)==start) {
			ROMHDR *pRomHdr = (ROMHDR *)mem_convert(start);
			printf("    ROMHDR 0x%08x, 0x%08x\n", start, *(U32 *)mem_convert(img_start+ROM_SIGNATURE_OFFSET+4));
			printf("    dllfirst  0x%08x, dlllast  0x%08x\n", pRomHdr->dllfirst, pRomHdr->dlllast);
			printf("    physfirst 0x%08x, physlast 0x%08x\n", pRomHdr->physfirst, pRomHdr->physlast);
		}
	}
	
	printf("do you want to run it? [y/n]\n");
	while(1) {
		U8 c = getch();
		if(c=='y'||c=='Y')
			call_linux(0, 0, *pLaunch);
		if(c=='n'||c=='N')
			break;
	}
	*pStart  = mem_convert(img_start);
	*pLength = img_length;

	return 0;
}