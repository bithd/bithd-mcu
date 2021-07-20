#include "bithd_device.h"
#include "trezor.h"
#include "oled.h"
#include "bitmaps.h"
#include "util.h"
#include "usb.h"
#include "setup.h"
#include "storage.h"
#include "layout.h"
#include "layout2.h"
#include "rng.h"
#include "timer.h"
#include "buttons.h"
#include "gettext.h"
#include "fastflash.h"


#include <libopencm3/stm32/usart.h>
#include <timerbitpie.h>
#include "../uart.h"
#include "bithdapp.h"
#include <libopencm3/stm32/gpio.h>

unsigned char Keylogodispone_flag=0;//key logo just display one times
TimProg T_Display;                  //动态显示定时器
TimProg T_Display2;                 //动态显示定时器
unsigned char Timeout1Sec_display_StarFlag=0;
unsigned char Timeout1Sec_display_StarFlag2=0;
volatile unsigned char balance_k=0;
volatile unsigned char balance_j=0;

const BITMAP *BitpieDigits715[12]={
&bitpie7_15_digit0, &bitpie7_15_digit1, &bitpie7_15_digit2, &bitpie7_15_digit3, &bitpie7_15_digit4,
&bitpie7_15_digit5, &bitpie7_15_digit6, &bitpie7_15_digit7, &bitpie7_15_digit8, &bitpie7_15_digit9,
&bitpie7_15_digitdi, &bitpie8_16_digit_
};
const BITMAP *BitpieDigits816[12]={
&bitpie8_16_digit0, &bitpie8_16_digit1, &bitpie8_16_digit2, &bitpie8_16_digit3, &bitpie8_16_digit4,
&bitpie8_16_digit5, &bitpie8_16_digit6, &bitpie8_16_digit7, &bitpie8_16_digit8, &bitpie8_16_digit9,
&bitpie8_16_dian,&bitpie8_16_clear
};

const BITMAP *BitpieDigits1632[12]={
&bitpie16_32_digit0, &bitpie16_32_digit1, &bitpie16_32_digit2, &bitpie16_32_digit3, &bitpie16_32_digit4,
&bitpie16_32_digit5, &bitpie16_32_digit6, &bitpie16_32_digit7, &bitpie16_32_digit8, &bitpie16_32_digit9,
&bitpie16_32_digit_,&bitpie16_32_digit_no
};
const BITMAP *BitpieASIIC_ABC[26]={
&bitpieAcii_A,&bitpieAcii_B,&bitpieAcii_C,&bitpieAcii_D,&bitpieAcii_E,
&bitpieAcii_F,&bitpieAcii_G,&bitpieAcii_H,&bitpieAcii_I,&bitpieAcii_J,&bitpieAcii_K,
&bitpieAcii_L,&bitpieAcii_M,&bitpieAcii_N,&bitpieAcii_O,&bitpieAcii_P,&bitpieAcii_Q,
&bitpieAcii_R,&bitpieAcii_S,&bitpieAcii_T,&bitpieAcii_U,&bitpieAcii_V,&bitpieAcii_W,
&bitpieAcii_X,&bitpieAcii_Y,&bitpieAcii_Z,
};
const BITMAP *BitpieASIIC_abc[26]={
&bitpieAcii_a,&bitpieAcii_b,&bitpieAcii_c,&bitpieAcii_d,&bitpieAcii_e,
&bitpieAcii_f,&bitpieAcii_g,&bitpieAcii_h,&bitpieAcii_i,&bitpieAcii_j,
&bitpieAcii_k,&bitpieAcii_l,&bitpieAcii_m,&bitpieAcii_n,&bitpieAcii_o,
&bitpieAcii_p,&bitpieAcii_q,&bitpieAcii_r,&bitpieAcii_s,&bitpieAcii_t,
&bitpieAcii_u,&bitpieAcii_v,&bitpieAcii_w,&bitpieAcii_x,&bitpieAcii_y,
&bitpieAcii_z,
};

void display_str_oled(unsigned char x,unsigned char y,unsigned char* strp,unsigned char length)
{
	unsigned char i;
	unsigned char xy=x;
    for(i=0;i<length;i++)
	{
        if(strp[i]>0x5B){oledDrawBitmap(xy,y, BitpieASIIC_abc[(strp[i]-0x61)]);}
        else{oledDrawBitmap(xy,y, BitpieASIIC_ABC[(strp[i]-0x41)]);}
        xy=xy+8;
	}
}


#define d_x 4
void bluenamedisplay(unsigned char* buf)
{
    unsigned char xy=0;
    unsigned char i;
    unsigned char buf_bluetooth[]="B";
    unsigned char buf_version[]="v";
    unsigned char buf_name[]="F";
    unsigned char buf_No[]="No";

    oledClear();
    layoutQR(buf,12);

    display_str_oled(52,d_x,buf_bluetooth,sizeof(buf_bluetooth)-1);
    xy=(sizeof(buf_bluetooth)-1)*8+52;
    oledDrawBitmap(xy,d_x, &bitpie1616Asciimaohao);
    xy=xy+8+8;
    display_str_oled(xy,d_x,buf_version,sizeof(buf_version)-1);
    xy=xy+8;
    oledDrawBitmap(xy,d_x,BitpieDigits715[(buf[0+12]&0x0f)]);
    xy=xy+8;
    oledDrawBitmap(xy,d_x, &bitpie7_15_digitdi);
    xy=xy+8;
    oledDrawBitmap(xy,d_x,BitpieDigits715[(buf[1+12]&0x0f)]);
    xy=xy+8;
    oledDrawBitmap(xy,d_x, &bitpie7_15_digitdi);
    xy=xy+8;
    oledDrawBitmap(xy,d_x,BitpieDigits715[(buf[2+12]&0x0f)]);


    display_str_oled(52,d_x+16,buf_name,sizeof(buf_name)-1);
    xy=(sizeof(buf_bluetooth)-1)*8+52;
    oledDrawBitmap(xy,d_x+16, &bitpie1616Asciimaohao);
    xy=xy+8+8;
    display_str_oled(xy,d_x+16,buf_version,sizeof(buf_version)-1);
    xy=xy+8;
    oledDrawBitmap(xy,d_x+16,BitpieDigits715[VERSION_MAJOR]);
    xy=xy+8;
    oledDrawBitmap(xy,d_x+16, &bitpie7_15_digitdi);
    xy=xy+8;
    oledDrawBitmap(xy,d_x+16,BitpieDigits715[VERSION_MINOR]);
    xy=xy+8;
    oledDrawBitmap(xy,d_x+16, &bitpie7_15_digitdi);
    xy=xy+8;
    oledDrawBitmap(xy,d_x+16,BitpieDigits715[VERSION_PATCH]);
 


    display_str_oled(0,48,buf_No,sizeof(buf_No)-1);
    xy=(sizeof(buf_No)-1)*8;
    oledDrawBitmap(xy,48, &bitpie8_16_dian);
    xy=xy+16;

    for(i=0;i<12;i++)
    {
        oledDrawBitmap(xy,48,BitpieDigits715[(buf[i]&0x0f)]);
        xy=xy+8;
    }
    oledFrame(0,0,127,63);
	oledRefresh();

}

void shutdowndisplay(void)
{    const char *action;

    oledClear();
    switch (storage_getLang()) {
        case CHINESE : 
            oledDrawZh(42,24,"关机#?#");
            oledDrawZh(104,0,"取消");
            oledInvert(104, 0,128,12);
            oledDrawZh(104,52,"确认");
            oledInvert(104,52,128,64);
            break;
        default:
            action = _("Power off?");
            oledDrawString(38, 24, action);
            action = _("Cancel");
            oledDrawString(94,1, action);
            oledInvert(93, 0,128,8);
            action = _("Ok");
            oledDrawString(115,55,action);
            oledInvert(114,54,128,64);
            break;
    }    
    oledRefresh();
}

void blueParingdisplay(unsigned char* buf)
{
    unsigned char xy=0;
    unsigned char i;
    unsigned char pair[]="Pair";
    unsigned char password[]="Password";	

    oledClear();
    display_str_oled(12,0,pair,sizeof(pair)-1); 
    xy=(sizeof(pair))*8;
    display_str_oled(12,0,password,sizeof(password)-1); 

    xy=16;
    for(i=0;i<6;i++)
    {
        oledDrawBitmap(xy,24,BitpieDigits1632[(buf[i]&0x0f)]);
        xy=xy+16;
    }

	oledRefresh();

}

/*
judge balance
return 1 have balance
       0 no balance
*/
unsigned char JudgeBalance(void)
{
    unsigned char buf[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    if(memcmp(&(Balancedisplaybuf.coin_name),buf,10))//((memcmp(&(Balancedisplaybuf.balance),buf,10))||(memcmp(&(Balancedisplaybuf.balance_after_di),buf,10)))
    {return 1;}
    return 0;
}

void Display_balance(unsigned char* balance,unsigned char Y)
{
    unsigned char buf[41];
    volatile	unsigned short length=0;;
    volatile	unsigned short i,j;
    volatile	unsigned short xy=0;
    volatile unsigned short L=0;
    unsigned char* p1;
    unsigned char* p2;

	for(i=0;i<10;i++)                                    //小数点前数据整理
	{
		if(((balance[i]>>4)&0x0f)!=0)
		{
            for(j=0;j<10-i;j++)
			{
				buf[2*j]=(balance[i+j]>>4)&0x0f;
				buf[2*j+1]=(balance[i+j])&0x0f;
				length=length+2;
			}
			break;
		}
		else
		{
    	    if((balance[i]&0x0f)!=0)
			{
				buf[0]=balance[i]&0x0f;
				length=1;
				for(j=1;j<10-i;j++)
				{
					buf[2*j-1]=(balance[i+j]>>4)&0x0f;
					buf[2*j]=(balance[i+j])&0x0f;
					length=length+2;
				}
				break;
			}		
		}
	}

	if(length==0)
	{
		buf[0]=0;
		buf[1]=10;
		length=2;		
	}
	else
	{
		buf[length]=10;
		length++;
	}
	L=length;


    p1=&balance[10];                                     //小数点后数据整理
	p2=&buf[length];
	for(i=10;i>0;i--)
	{
		if((p1[i-1]&0x0f)!=0)
		{
            for(j=0;j<i;j++)
			{
				p2[2*j]=(p1[j]>>4)&0x0f;
				p2[2*j+1]=(p1[j])&0x0f;
				length=length+2;
			}
			break;
		}
		else
		{
    	    if(((p1[i-1]>>4)&0x0f)!=0)
			{
				for(j=0;j<i-1;j++)
				{
					p2[2*j]=(p1[j]>>4)&0x0f;
					p2[2*j+1]=(p1[j])&0x0f;
					length=length+2;
				}
				p2[2*j]=(p1[i-1]>>4)&0x0f;
				length++;
				break;
			}		
		}
	}

    if(length==L)
    {
	    length--;
    }

	if(OLED_WIDTH<length*7)
	{
	    if(balance_j/1==0)
	    {
            if(Timeout1Sec_display_StarFlag2==TimeClose)
			{
				TP_START(&T_Display2, 10);               //开启定时器
				Timeout1Sec_display_StarFlag2=TimeOpen;
			}
		    else
			{
                if(CheckTP(&T_Display2)==0)             //判断是否超时
                {	                                    //超时
                    TP_CLOSE(&T_Display2);              //关闭计时器，准备从新计时//关闭1秒超时定时器
                    Timeout1Sec_display_StarFlag2=TimeClose;
                    if(balance_j<((1+length)*7-(OLED_WIDTH-7)))
                    {balance_j=balance_j+speeddisplay2;}
                    else
                    {balance_j=0;}
                }
                else
                {}
			}
	    }
	    else
	    {
            if(balance_j<((1+length)*7-(OLED_WIDTH-7))){balance_j=balance_j+speeddisplay2;}
            else
            {
                if(Timeout1Sec_display_StarFlag2==TimeClose)
                {
                    TP_START(&T_Display2, 10); 
                    Timeout1Sec_display_StarFlag2=TimeOpen;
                }
                if(CheckTP(&T_Display2)==0)
                {	
                    TP_CLOSE(&T_Display2);    
                    Timeout1Sec_display_StarFlag2=TimeClose;
                    balance_j=0;
                }					
			}		 
	    }

        xy=7-(balance_j%7);
		for(i=balance_j/7;i<(OLED_WIDTH/7-1)+(balance_j/7);i++)
		{
            oledDrawBitmap(xy,Y,BitpieDigits715[buf[i]]);
            xy=xy+7;
		}

		oledDrawBitmap(0,Y,&bitpie8_16_clear);
		oledDrawBitmap(121,Y,&bitpie8_16_clear);
	}
	else
	{
	    xy=(128-length*7)/2;
		for(i=0;i<length;i++)
		{
            oledDrawBitmap(xy,Y,BitpieDigits715[buf[i]]);
            xy=xy+7;		
		}
	}
}

#define MAX_COIN_NAME_LENGTH 20
void balancedispaly(void)
{
    unsigned char namelength;
    static char BALANCE[]="Balance :";
    int x;
    uint8_t has_balance = JudgeBalance();

    Keylogodispone_flag=0;//clear flag

    oledClear();

    namelength = Balancedisplaybuf.coin_name[31] != 0 ? 32 : strlen((const char*)Balancedisplaybuf.coin_name); // 计算name字符串个数
    if (namelength > MAX_COIN_NAME_LENGTH) {
        Balancedisplaybuf.coin_name[MAX_COIN_NAME_LENGTH] = 0;
    }

    switch (storage_getLang()) {
        case CHINESE :
            x = (OLED_WIDTH - (12 + 12 + 8)) / 2;
            oledDrawZh(x, 8, "余额#:#");
            break;
        default:
            oledDrawStringCenter(14, BALANCE);
            break;
    }

    if (has_balance) {
        Display_balance((unsigned char *) (&(Balancedisplaybuf.balance)), 24);
        oledDrawStringCenter(44, (const char *) Balancedisplaybuf.coin_name);
    } else {
        oledDrawStringCenter(28, "-");
    }

    oledRefresh();
}

void displaytimer(void)
{
    unsigned char xy=0;
    unsigned char i;
IF_BITHD(
    unsigned short year;
)
    Keylogodispone_flag=0;//clear flag
IF_RAZOR(
    if(1)//(Timerdisplaybuf.Year!=0)
)
IF_BITHD(
    if(Timerdisplaybuf.Year!=0)
)
    {
	    oledClear();

        oledDrawBitmap(xy, 0,&bitpiebatfull);
        xy=16;
        for(i=0;i<4-(Timerdisplaybuf.bat_vol/20);i++)
        {
            oledBox(xy,4,xy+2,10,false);
            xy=xy-4;
        }
        
        xy = 23;
        if(Timerdisplaybuf.chargingstatus==1)
        {
             oledDrawBitmap(xy, 0,&bitpiebatcharg);
        }

	    xy=112;
	    if(Timerdisplaybuf.bluetoothconnetstatus==0)
	    {
	        oledDrawBitmap(xy, 0,&bitpieblueopen);
	    }
	    else
	    {
	        oledDrawBitmap(xy, 0,&bitpieblueconect);
	    }

IF_RAZOR(
         //TEXT
        oledDrawBitmap(0, 16,&bmp_logomain);
)
IF_BITHD(
        xy=24;
	    oledDrawBitmap(xy, 16,BitpieDigits1632[Timerdisplaybuf.hours/10]);
	    xy=xy+16;
        oledDrawBitmap(xy, 16,BitpieDigits1632[Timerdisplaybuf.hours%10]);
        xy=xy+16;
        if(Timerdisplaybuf.second%2)
        {
            oledDrawBitmap(xy, 16,BitpieDigits1632[11]);	//:
        }
        else
        {
            oledDrawBitmap(xy, 16,BitpieDigits1632[10]);	//:
        }

        xy=xy+16;
        oledDrawBitmap(xy, 16,BitpieDigits1632[Timerdisplaybuf.minute/10]);
        xy=xy+16;
        oledDrawBitmap(xy, 16,BitpieDigits1632[Timerdisplaybuf.minute%10]);
        year=Timerdisplaybuf.Year;
        xy=16+8+4;
        oledDrawBitmap(xy,48,BitpieDigits816[year/1000]);
        xy=xy+8;
        year=year%1000;
        oledDrawBitmap(xy,48,BitpieDigits816[year/100]);
        xy=xy+8;
        year=year%100;
        oledDrawBitmap(xy,48,BitpieDigits816[year/10]);
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits816[year%10]);
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits715[11]);//------
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits816[Timerdisplaybuf.Month/10]);
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits816[Timerdisplaybuf.Month%10]);
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits715[11]);//------
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits816[Timerdisplaybuf.Day/10]);
        xy=xy+8;
        oledDrawBitmap(xy,48,BitpieDigits816[Timerdisplaybuf.Day%10]);
)
	
        oledRefresh();
    }
}


void bithdapp(void)
{
    //just for dispay
    switch(stm32workstatus)
    {
        case Stm32_KeyMode:
                if(Keylogodispone_flag==0)
                {
                    Timerdisplaybuf.Year=0;
                    oledClear();
                    oledDrawBitmap(40, 8,&bmp_logo64);
                    oledRefresh();
                    Keylogodispone_flag=1;
                }
        break;
        case Stm32_TimerMode:
                displaytimer();

        break;
        case Stm32_LowpowerMode:
                Timerdisplaybuf.Year=0;
                Keylogodispone_flag=0;
                oledClear();
                oledDrawBitmap(32, 16,&bitpieneedcharging);
                oledRefresh();
        break;
        case Stm32_BalanceMode:
                if(SaveNewbalanceflag==1)
                {
                    balancedispaly();
                    SaveNewbalanceflag=0;
                }
        break;
    }
}