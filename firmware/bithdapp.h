#ifndef _BITHD_APP_
#define _BITHD_APP_

#define speeddisplay 2  //max 8／min 1
#define speeddisplay2 1 //max 8／min 1

extern void shutdowndisplay(void);
extern void blueParingdisplay(unsigned char* buf);
extern void bluenamedisplay(unsigned char* buf);
extern void bithdapp(void);


extern unsigned char Keylogodispone_flag;//key logo just display one times
extern TimProg T_Display; //动态显示定时器
extern TimProg T_Display2; //动态显示定时器
extern unsigned char Timeout1Sec_display_StarFlag;
extern unsigned char Timeout1Sec_display_StarFlag2;
extern volatile unsigned char balance_k;
extern volatile unsigned char balance_j;
#endif