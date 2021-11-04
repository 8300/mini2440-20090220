/**************************************************************
The initial and control for 640×480 16Bpp TFT LCD----VGA
**************************************************************/

#include "def.h"
#include "option.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h" 

#if LCD_TYPE==LCD_TYPE_N35

#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

//NEC35
#define LCD_XSIZE 	(240)	
#define LCD_YSIZE 	(320)

#define SCR_XSIZE 	(240)
#define SCR_YSIZE 	(320)

//NEC35
#define HOZVAL	(LCD_XSIZE-1)
#define LINEVAL	(LCD_YSIZE-1)

#define VBPD					(1)
#define VFPD					(5)
#define VSPW					(1)
#define HBPD					(36)
#define HFPD					(19)
#define HSPW					(5)

#define CLKVAL				(4)

/*************************** LCD 7" ****************************/
#elif LCD_TYPE==LCD_TYPE_A70

#define MVAL		(13)
#define MVAL_USED 	(0)		//0=each frame   1=rate by MVAL
#define INVVDEN		(1)		//0=normal       1=inverted
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

//A70
#define LCD_XSIZE 	(800)	
#define LCD_YSIZE 	(480)

#define SCR_XSIZE 	(800)
#define SCR_YSIZE 	(480)

//A70
#define HOZVAL	(LCD_XSIZE-1)
#define LINEVAL	(LCD_YSIZE-1)

//A70
#define VBPD					((33-1)&0xff)
#define VFPD					((10-1)&0xff)
#define VSPW					((2-1) &0x3f)
#define HBPD					((48-1)&0x7f)
#define HFPD					((16-1)&0xff)
#define HSPW					((96-1)&0xff)

#define CLKVAL	(1) 


#elif LCD_TYPE==LCD_TYPE_VGA1024x768
#define CLKVAL	(2)

#define MVAL		(13)
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control
#define BPP24BL     (1)		//0 = LSB valid   1 = MSB Valid

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

//TFT 1024x768
#define LCD_XSIZE 	(1024)	
#define LCD_YSIZE 	(768)

//TFT 1024x768
#define SCR_XSIZE 	(1024)
#define SCR_YSIZE 	(768)

//TFT800x480
#define HOZVAL	(LCD_XSIZE-1)
#define LINEVAL	(LCD_YSIZE-1)


#define HFPD		(1)		
#define HSPW		(1)		
#define HBPD		(17)		

#define VFPD		(15)		
#define VSPW		(199)		
#define VBPD		(15)	

#endif

extern unsigned char sunflower_240x320[];	//宽1024，高768
extern unsigned char sunflower_800x480[];	//宽1024，高768
extern unsigned char sunflower_1024x768[];	//宽1024，高768

volatile static unsigned short LCD_BUFFER[SCR_YSIZE][SCR_XSIZE];

/**************************************************************
640×480 TFT LCD数据和控制端口初始化
**************************************************************/
static void Lcd_Port_Init( void )
{
    rGPCUP=0xffffffff; // Disable Pull-up register
    rGPCCON=0xaaaa02a8; //Initialize VD[7:0],VM,VFRAME,VLINE,VCLK

    rGPDUP=0xffffffff; // Disable Pull-up register
    rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]
}

/**************************************************************
640×480 TFT LCD功能模块初始化
**************************************************************/
static void LCD_Init(void)
{
	rLCDCON1=(CLKVAL<<8)|(1<<7)|(3<<5)|(12<<1)|0;
    	// TFT LCD panel,16bpp TFT,ENVID=off
	rLCDCON2=(VBPD<<24)|(LINEVAL<<14)|(VFPD<<6)|(VSPW);
	rLCDCON3=(HBPD<<19)|(HOZVAL<<8)|(HFPD);
	rLCDCON4=(MVAL<<8)|(HSPW);
#if LCD_TYPE==LCD_TYPE_VGA1024x768
	rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<3)|(1<<0);		//FRM5:6:5,HSYNC and VSYNC are inverted
#else
    rLCDCON5 = (1<<11) | (1<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6)
             | (1<<3)  |(BSWP<<1) | (HWSWP);
#endif
	rLCDSADDR1=(((U32)LCD_BUFFER>>22)<<21)|M5D((U32)LCD_BUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCD_BUFFER+(SCR_XSIZE*LCD_YSIZE*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE-LCD_XSIZE)/1)<<11)|(LCD_XSIZE/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    rTCONSEL&=~((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
}

/**************************************************************
LCD视频和控制信号输出或者停止，1开启视频输出
**************************************************************/
static void Lcd_EnvidOnOff(int onoff)
{
    if(onoff==1)
	rLCDCON1|=1; // ENVID=ON
    else
	rLCDCON1 =rLCDCON1 & 0x3fffe; // ENVID Off
}

/**************************************************************
320×240 8Bpp TFT LCD 电源控制引脚使能
**************************************************************/
static void Lcd_PowerEnable(int invpwren,int pwren)
{
    //GPG4 is setted as LCD_PWREN
    rGPGUP = rGPGUP|(1<<4); // Pull-up disable
    rGPGCON = rGPGCON|(3<<8); //GPG4=LCD_PWREN
    
    //Enable LCD POWER ENABLE Function
    rLCDCON5 = rLCDCON5&(~(1<<3))|(pwren<<3);   // PWREN
    rLCDCON5 = rLCDCON5&(~(1<<5))|(invpwren<<5);   // INVPWREN
}

/**************************************************************
640×480 TFT LCD移动观察窗口
**************************************************************/
static void Lcd_MoveViewPort(int vx,int vy)
{
    U32 addr;

    SET_IF(); 
	#if (LCD_XSIZE<32)
    	    while((rLCDCON1>>18)<=1); // if x<32
	#else	
    	    while((rLCDCON1>>18)==0); // if x>32
	#endif
        addr=(U32)LCD_BUFFER+(vx*2)+vy*(SCR_XSIZE*2);
	rLCDSADDR1= ( (addr>>22)<<21 ) | M5D(addr>>1);
	rLCDSADDR2= M5D(((addr+(SCR_XSIZE*LCD_YSIZE*2))>>1));
	CLR_IF();
}    

/**************************************************************
640×480 TFT LCD移动观察窗口
**************************************************************/
static void MoveViewPort(void)
{
    int vx=0,vy=0,vd=1;

    Uart_Printf("\n*Move the LCD view windos:\n");
    Uart_Printf(" press 8 is up\n");
    Uart_Printf(" press 2 is down\n");
    Uart_Printf(" press 4 is left\n");
    Uart_Printf(" press 6 is right\n");
    Uart_Printf(" press Enter to exit!\n");

    while(1)
    {
    	switch(Uart_Getch())
    	{
    	case '8':
	    if(vy>=vd)vy-=vd;    	   	
        break;

    	case '4':
    	    if(vx>=vd)vx-=vd;
    	break;

    	case '6':
                if(vx<=(SCR_XSIZE-LCD_XSIZE-vd))vx+=vd;   	    
   	    break;

    	case '2':
                if(vy<=(SCR_YSIZE-LCD_YSIZE-vd))vy+=vd;   	    
   	    break;

    	case '\r':
   	    return;

    	default:
	    break;
		}
	Uart_Printf("vx=%3d,vy=%3d\n",vx,vy);
	Lcd_MoveViewPort(vx,vy);
    }
}

/**************************************************************
640×480 TFT LCD单个象素的显示数据输出
**************************************************************/
static void PutPixel(U32 x,U32 y,U16 c)
{
    if(x<SCR_XSIZE && y<SCR_YSIZE)
		LCD_BUFFER[(y)][(x)] = c;
}

/**************************************************************
640×480 TFT LCD全屏填充特定颜色单元或清屏
**************************************************************/
static void Lcd_ClearScr( U16 c)
{
	unsigned int x,y ;
		
    for( y = 0 ; y < SCR_YSIZE ; y++ )
    {
    	for( x = 0 ; x < SCR_XSIZE ; x++ )
    	{
			LCD_BUFFER[y][x] = c ;
    	}
    }
}

/**************************************************************
LCD屏幕显示垂直翻转
// LCD display is flipped vertically
// But, think the algorithm by mathematics point.
//   3I2
//   4 I 1
//  --+--   <-8 octants  mathematical cordinate
//   5 I 8
//   6I7
**************************************************************/
static void Glib_Line(int x1,int y1,int x2,int y2, U16 color)
{
	int dx,dy,e;
	dx=x2-x1; 
	dy=y2-y1;
    
	if(dx>=0)
	{
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 1/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 2/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 8/8 octant
			{
				e=dy-dx/2;
				while(x1<=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1+=1;
					e+=dy;
				}
			}
			else		// 7/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1+=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
	else //dx<0
	{
		dx=-dx;		//dx=abs(dx)
		if(dy >= 0) // dy>=0
		{
			if(dx>=dy) // 4/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1+=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 3/8 octant
			{
				e=dx-dy/2;
				while(y1<=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1+=1;
					e+=dx;
				}
			}
		}
		else		   // dy<0
		{
			dy=-dy;   // dy=abs(dy)

			if(dx>=dy) // 5/8 octant
			{
				e=dy-dx/2;
				while(x1>=x2)
				{
					PutPixel(x1,y1,color);
					if(e>0){y1-=1;e-=dx;}	
					x1-=1;
					e+=dy;
				}
			}
			else		// 6/8 octant
			{
				e=dx-dy/2;
				while(y1>=y2)
				{
					PutPixel(x1,y1,color);
					if(e>0){x1-=1;e-=dy;}	
					y1-=1;
					e+=dx;
				}
			}
		}	
	}
}

/**************************************************************
在LCD屏幕上画一个矩形
**************************************************************/
static void Glib_Rectangle(int x1,int y1,int x2,int y2, U16 color)
{
    Glib_Line(x1,y1,x2,y1,color);
    Glib_Line(x2,y1,x2,y2,color);
    Glib_Line(x1,y2,x2,y2,color);
    Glib_Line(x1,y1,x1,y2,color);
}

/**************************************************************
在LCD屏幕上用颜色填充一个矩形
**************************************************************/
static void Glib_FilledRectangle(int x1,int y1,int x2,int y2, U16 color)
{
    int i;

    for(i=y1;i<=y2;i++)
	Glib_Line(x1,i,x2,i,color);
}

/**************************************************************
在LCD屏幕上指定坐标点画一个指定大小的图片
**************************************************************/
static void Paint_Bmp(int x0,int y0,int h,int l,unsigned char bmp[])
{
	int x,y;
	U32 c;
	int p = 0;
	
    for( y = 0 ; y < l ; y++ )
    {
    	for( x = 0 ; x < h ; x++ )
    	{
    		c = bmp[p+1] | (bmp[p]<<8) ;

			if ( ( (x0+x) < SCR_XSIZE) && ( (y0+y) < SCR_YSIZE) )
				LCD_BUFFER[y0+y][x0+x] = c ;

    		p = p + 2 ;
    	}
    }
}

/**************************************************************
**************************************************************/
void TFT_LCD_Init(void)
{

    LCD_Init();
	LcdBkLtSet( 70 ) ;
	Lcd_PowerEnable(0, 1);
    Lcd_EnvidOnOff(1);		//turn on vedio

    Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00) );  

#if LCD_TYPE==LCD_TYPE_N35
	Paint_Bmp(0, 0, 240, 320, sunflower_240x320);
#elif LCD_TYPE==LCD_TYPE_A70
	Paint_Bmp(0, 0, 800, 480, sunflower_800x480);
#elif LCD_TYPE==LCD_TYPE_VGA1024x768
    Paint_Bmp(0, 0, 1024, 768, sunflower_1024x768);
#endif    
}

/**************************************************************
**************************************************************/
void TFT_LCD_Test(void)
{
#if LCD_TYPE==LCD_TYPE_N35
	Uart_Printf("\nTest TFT LCD 240x320!\n");
#elif LCD_TYPE==LCD_TYPE_A70
	Uart_Printf("\nTest TFT LCD 800×480!\n");
#elif LCD_TYPE==LCD_TYPE_VGA1024x768
    Uart_Printf("\nTest VGA 1024x768!\n");
#endif    


    Lcd_Port_Init();
    LCD_Init();
    Lcd_EnvidOnOff(1);		//turn on vedio

	Lcd_ClearScr( (0x00<<11) | (0x00<<5) | (0x00)  )  ;		//clear screen
	Uart_Printf( "\nLCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr( (0x1f<<11) | (0x3f<<5) | (0x1f)  )  ;		//clear screen
	Uart_Printf( "LCD clear screen is finished! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

	Lcd_ClearScr(0xffff);		//fill all screen with some color
	#define LCD_BLANK		30
	#define C_UP		( LCD_XSIZE - LCD_BLANK*2 )
	#define C_RIGHT		( LCD_XSIZE - LCD_BLANK*2 )
	#define V_BLACK		( ( LCD_YSIZE - LCD_BLANK*4 ) / 6 )
	Glib_FilledRectangle( LCD_BLANK, LCD_BLANK, ( LCD_XSIZE - LCD_BLANK ), ( LCD_YSIZE - LCD_BLANK ),0x0000);		//fill a Rectangle with some color

	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*0), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*1),0x001f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*1), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*2),0x07e0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*2), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*3),0xf800);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*3), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*4),0xffe0);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*4), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*5),0xf81f);		//fill a Rectangle with some color
	Glib_FilledRectangle( (LCD_BLANK*2), (LCD_BLANK*2 + V_BLACK*5), (C_RIGHT), (LCD_BLANK*2 + V_BLACK*6),0x07ff);		//fill a Rectangle with some color
   	Uart_Printf( "LCD color test, please look! press any key to continue!\n" );
    Uart_Getch() ;		//wait uart input

#if LCD_TYPE==LCD_TYPE_N35
	Paint_Bmp(0, 0, 240, 320, sunflower_240x320);
#elif LCD_TYPE==LCD_TYPE_A70
	Paint_Bmp(0, 0, 800, 480, sunflower_800x480);
#elif LCD_TYPE==LCD_TYPE_VGA1024x768
    Paint_Bmp(0, 0, 1024, 768, sunflower_1024x768);
#endif    
   	Uart_Printf( "LCD paint a bmp, please look! press any key to continue! \n" );
    Uart_Getch() ;		//wait uart input

    Lcd_EnvidOnOff(0);		//turn off vedio
}
//*************************************************************
