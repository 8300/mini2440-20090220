#include "def.h"
#include "2440addr.h"
#include "2440lib.h"
#include "2440slib.h"

#define	COLOR_RED_TFT16		0xf800
#define	COLOR_GREEN_TFT16	0x07e0
#define	COLOR_BLUE_TFT16	0x001f

#define LCDFRAMEBUFFER 0x33800000

#define LCD_XSIZE_TFT 	(320)	
#define LCD_YSIZE_TFT 	(240)
#define SCR_XSIZE_TFT 	(LCD_XSIZE_TFT*2)
#define SCR_YSIZE_TFT 	(LCD_YSIZE_TFT*2)

#define CLKVAL_TFT	(7)
#define MVAL		(13)
#define MVAL_USED 	(0)
#define BSWP		(0)		//Byte swap control
#define HWSWP		(1)		//Half word swap control

#define HOZVAL_TFT	(LCD_XSIZE_TFT-1)
#define LINEVAL_TFT	(LCD_YSIZE_TFT-1)

#define VBPD		(4)		//垂直同步信号的后肩
#define VFPD		(4)		//垂直同步信号的前肩
#define VSPW		(4)		//垂直同步信号的脉宽

#define HBPD		(13)		//水平同步信号的后肩
#define HFPD		(4)		//水平同步信号的前肩
#define HSPW		(18)		//水平同步信号的脉宽

#define M5D(n) ((n) & 0x1fffff)	// To get lower 21bits

extern unsigned char uCdragon_logo[];

U16 (*frameBuffer16BitTft)[SCR_XSIZE_TFT];

/*-----------------------------------------------------------------------------
 *  320 x 240 TFT LCD单个象素的显示数据输出
 */
static void PutPixel(U32 x,U32 y,U16 c)
{
    if(x<SCR_XSIZE_TFT && y<SCR_YSIZE_TFT)
        frameBuffer16BitTft[(y)][(x)] = c;
}

/*-----------------------------------------------------------------------------
 *  LCD屏幕显示垂直翻转
 *  LCD display is flipped vertically
 *  But, think the algorithm by mathematics point.
 *    3I2
 *    4 I 1
 *   --+--   <-8 octants  mathematical cordinate
 *    5 I 8
 *    6I7
 */
static void Glib_Line(int x1, int y1, int x2, int y2, U16 color)
{
    int dx, dy, e;
    
    dx = x2 - x1; 
    dy = y2 - y1;
    
    if(dx >= 0)
    {
        if(dy >= 0)        // dy>=0
        {
            if(dx >= dy)   // 1/8 octant
            {
                e = dy-dx/2;
                while(x1 <= x2)
                {
                    PutPixel(x1, y1, color);
                    if(e > 0) {y1+=1; e-=dx;}
                    x1 += 1;
                    e += dy;
                }
            }
            else        // 2/8 octant
            {
                e = dx-dy/2;
                while(y1 <= y2)
                {
                    PutPixel(x1, y1, color);
                    if(e > 0) {x1+=1; e-=dy;}
                    y1 += 1;
                    e += dx;
                }
            }
        }
        else           // dy<0
        {
            dy = -dy;   // dy=abs(dy)
            
            if(dx >= dy) // 8/8 octant
            {
                e = dy-dx/2;
                while(x1 <= x2)
                {
                    PutPixel(x1, y1, color);
                    if(e > 0) {y1-=1; e-=dx;}
                    x1 += 1;
                    e += dy;
                }
            }
            else        // 7/8 octant
            {
                e = dx-dy/2;
                while(y1 >= y2)
                {
                    PutPixel(x1, y1, color);
                    if(e > 0) {x1+=1; e-=dy;}
                    y1 -= 1;
                    e += dx;
                }
            }
        }
    }
    else //dx<0
    {
        dx=-dx;     //dx=abs(dx)
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
            else        // 3/8 octant
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
        else       // dy<0
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
            else        // 6/8 octant
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

/*-----------------------------------------------------------------------------
 *  在LCD屏幕上用颜色填充一个矩形
 */
static void Glib_FilledRectangle(int x1, int y1, int x2, int y2, U16 color)
{
    int i;

    for(i = y1; i <= y2; i++) {    // 用n条直线填满区域!
        Glib_Line(x1, i, x2, i, color);
    }
}

/*-----------------------------------------------------------------------------
 *  在LCD屏幕上指定坐标点画一个指定大小的图片
 */
static void Paint_Bmp(int x0, int y0, int h, int l, unsigned char bmp[])
{
    int x, y;
    U32 c;
    int p = 0;
    
    for( y = 0 ; y < l ; y++ )
    {
        for( x = 0 ; x < h ; x++ )
        {
            c = bmp[p+1] | (bmp[p]<<8) ;
            
            if ( ( (x0+x) < SCR_XSIZE_TFT) && ( (y0+y) < SCR_YSIZE_TFT) )
                frameBuffer16BitTft[y0+y][x0+x] = c ;
            
            p = p + 2 ;
        }
    }
}

/*-----------------------------------------------------------------------------
 *  320 x 240 TFT LCD全屏填充特定颜色单元或清屏
 */
static void Lcd_ClearScr( U16 c)
{
    unsigned int x, y;
    
    for( y = 0 ; y < SCR_YSIZE_TFT ; y++ )
    {
        for( x = 0 ; x < SCR_XSIZE_TFT ; x++ )
        {
            frameBuffer16BitTft[y][x] = c ;
        }
    }
}

//优龙LCD驱动夏普DH01液晶屏
void LcdDisplay(void)
{
	int x, y;

	//PWM, GPB1
	rGPBUP  &= 0xfffd;
	rGPBCON &= 0xfffffff3;	
	rGPBCON |= 0x00000004;
	rGPBDAT |= 0x0002;
	
	frameBuffer16BitTft=(U16 (*)[SCR_XSIZE_TFT])LCDFRAMEBUFFER;
	// rGPCUP=0xffffffff; // Disable Pull-up register
    rGPCUP  = 0x00000000;
	// rGPCCON=0xaaaa56a9; //Initialize VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND 
	 rGPCCON = 0xaaaa02a9; 
	 
	// rGPDUP=0xffffffff; // Disable Pull-up register
     rGPDUP  = 0x00000000;
   rGPDCON=0xaaaaaaaa; //Initialize VD[15:8]

	rLCDCON1=(CLKVAL_TFT<<8)|(MVAL_USED<<7)|(3<<5)|(12<<1)|0;
    // TFT LCD panel,12bpp TFT,ENVID=off
	rLCDCON2=(VBPD<<24)|(LINEVAL_TFT<<14)|(VFPD<<6)|(VSPW);
	rLCDCON3=(HBPD<<19)|(HOZVAL_TFT<<8)|(HFPD);
	rLCDCON4=(MVAL<<8)|(HSPW);
	// rLCDCON5=(1<<11)|(0<<9)|(0<<8)|(0<<6)|(BSWP<<1)|(HWSWP);	//FRM5:6:5,HSYNC and VSYNC are inverted
    rLCDCON5 = (1<<11) | (1<<10) | (1<<9) | (1<<8) | (0<<7) | (0<<6)
             | (1<<3)  |(BSWP<<1) | (HWSWP);

	rLCDSADDR1=(((U32)LCDFRAMEBUFFER>>22)<<21)|M5D((U32)LCDFRAMEBUFFER>>1);
	rLCDSADDR2=M5D( ((U32)LCDFRAMEBUFFER+(SCR_XSIZE_TFT*LCD_YSIZE_TFT*2))>>1 );
	rLCDSADDR3=(((SCR_XSIZE_TFT-LCD_XSIZE_TFT)/1)<<11)|(LCD_XSIZE_TFT/1);
	rLCDINTMSK|=(3); // MASK LCD Sub Interrupt
    rTCONSEL &= (~7) ;     // Disable LPC3480
    // rTCONSEL&=~((1<<4)|1); // Disable LCC3600, LPC3600
	rTPAL=0; // Disable Temp Palette
	
	rLCDCON1|=1; // ENVID=ON
	
    // LOGO

	Glib_FilledRectangle(0, 0, 320, 35, 0x0);
	Glib_FilledRectangle(0, 35,5,   206, 0x0);

	Delay(10);
	Glib_FilledRectangle( 5, 35, 315, 40, 0xf800);	//Red,top
	Glib_FilledRectangle(10, 40, 310, 45, 0x07e0);	//Green,top
	Glib_FilledRectangle(15, 45, 305, 50, 0x001f);	//Blue,top

	Delay(10);
	Glib_FilledRectangle( 5, 37, 9, 200, 0xf800);	//Red,left
	Glib_FilledRectangle(10, 45, 15, 195, 0x07e0);	//Green,left
	Glib_FilledRectangle(16, 50, 20, 190, 0x001f);	//Blue,left

	Delay(10);
	Glib_FilledRectangle(311, 40, 315, 200, 0xf800);	//Red,right
	Glib_FilledRectangle(305, 45, 310, 195, 0x07e0);	//Green,right
	Glib_FilledRectangle(300, 50, 304, 190, 0x001f);	//Blue,right

	Delay(50);
	Paint_Bmp(20, 50, 280, 190, uCdragon_logo);			//picture
	Delay(10);
	
	Glib_FilledRectangle( 5, 200, 315, 206, 0xf800);	//Red,button
	Glib_FilledRectangle( 10,195, 310, 200, 0x07e0);	//Green,button
	Glib_FilledRectangle( 16,190, 304, 195, 0x001f);	//Blue,button

	Glib_FilledRectangle(315,35, 320, 206, 0x0);
	Glib_FilledRectangle(0, 206, 320, 240, 0x0);
}