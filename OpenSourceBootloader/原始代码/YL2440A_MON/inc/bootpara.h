#ifndef BOOTPARAMS_H
#define	BOOTPARAMS_H

typedef struct {
	char flags[12];
	unsigned int val;
} ParamItem;

typedef struct {
	ParamItem start;
	ParamItem cpu_clk;
	ParamItem boot_delay;
	ParamItem serial_sel;
	ParamItem serial_baud;
	ParamItem machine;
	ParamItem run_addr;
	ParamItem root_sel;
	ParamItem tty_sel;
	ParamItem initrd_addr;
	ParamItem initrd_len;
	ParamItem mem_cfg;
	ParamItem devfs_sel;
	ParamItem osstor;
	ParamItem user_params;
	char string[128];
} BootParams;

typedef struct {
	ParamItem vid;
	ParamItem pid;
	ParamItem ser_l;
	ParamItem ser_h;
	ParamItem user_params;
	char string[128];
} VenderParams;

int search_params(void);
int save_params(void);
int set_params(void);

#define	DEFAULT_USER_PARAMS	"display=shp240"

#ifdef GLOBAL_PARAMS

//小于等于512个字节,最多保存24个ITEM和128字节用户定义的字符串
BootParams boot_params = {
	{"bootpara", 1},	//0=boot without parameters,1=boot with parameters
	{"cpuclk",   2},	//0=200M, 1=300M, 2=400M, 3=440M
	{"rundelay", 0},	//0 seconds
	{"serial",   1},	//0=serial port 0, 1=serial port 1
	{"baudrate", 115200},
	{"machine",  193},
	{"runAddr",  0x30400000},
	{"rootfs",   0},
	{"tty",      1},
	{"initrdA",  0},
	{"initrdL",  0},
	{"memsize",  0x04000000},
	{"devfs",    1},
	{"ostore",   0},	//0=nand, 1=nor
	{"userpara", sizeof(DEFAULT_USER_PARAMS)},
	DEFAULT_USER_PARAMS
};

//小于等于256字节
VenderParams vend_params = {
	{"VendID",   0x76543210},
	{"ProdID",   0xfedcba98},
	{"Serial_L", 0x01234567},
	{"Serial_H", 0x89abcdef},
	{"userpara", 1},	//0=data, 1=string
	"www.ucdragon.com"
};

#else

extern BootParams boot_params;
extern VenderParams vend_params;

#endif

#endif