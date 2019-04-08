#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/f2/rng.h>

#include "timerbitpie.h"
#include "uart.h"
#include "./firmware/usb.h"
#include <libopencm3/cm3/nvic.h>
#include "./firmware/bithdapp.h"


unsigned char cmd_uart[5];                         //命令存储缓冲区
unsigned char data_array[1];                       //数据接收缓冲区，串口中断，每次接收一个字节
unsigned char Data_64Bytes[1024];                  //recive bufer
unsigned char uartnum=0;                           //串口流号
unsigned char uart_enable_flag=0;                  //串口使能标志位 0 串口未开启工作，1 串口开启工作，为了降低功耗，在不需要串口时关闭
unsigned char uart_send_flag=0;                    //串口数据发送标志位 0 无数据发送，1 有数据发送
unsigned char uart_recive_All=0;                   //串口接收完成标志 0 没有全部接收完成，1 全部接收完成
unsigned char olddata=0;                           //记录上一个接收数据
unsigned char ACK_recive_satus=0;                  //ack返回值值状态 0 ack报错  ；1 ack成功
unsigned char Ack_recive_enable=0;                 //应答接收使能标志 0 禁止接收使能 ；1开启接收使能;2进入重收功能，上行出错
unsigned char ACC_resend=0;                        //重发次数累加器
static unsigned short uart_recive_l=0;             //已经接收数据长度缓存
unsigned char stm32workstatus=0;                  //stm32 work status remeber
TimProg T_Connect;                                //communicate timeout timer
uart_communication_dmwz uart_communicate_buf={0,0,0,Data_64Bytes,0};//数据buf缓冲区
unsigned char needsuccessack_flag;
unsigned char Timeout1Sec_Uart_StarFlag=0;
unsigned char sendsuccessflag=0;
/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
uint16_t const crc16_table[256] =
    {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };
//////////////////////////////////////

static inline uint16_t crc16_byte(uint16_t crc, const uint8_t data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}
/**
 * crc16 - compute the CRC-16 for the data buffer
 * @crc: previous CRC value
 * @buffer: data pointer
 * @len: number of bytes in the buffer
 *
 * Returns the updated CRC value.
 */
uint16_t bd_crc16(uint16_t crc, uint8_t const *buffer, uint16_t len)
{
    while (len--)
        crc = crc16_byte(crc, *buffer++);
    return crc;
}

///////////////////////////////////////
 void usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART2);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2 | GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO0 | GPIO1 | GPIO2 | GPIO3);


	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2,USART_FLOWCONTROL_RTS_CTS);//USART_FLOWCONTROL_NONE);//
	usart_set_mode(USART2, USART_MODE_TX_RX);

	nvic_enable_irq(38);                                     //enable sum interrupt
        usart_enable_rx_interrupt(USART2);                   //enable usart2 interrupt

	usart_enable(USART2);
}

 ///////////////////////////////////////////////////////////////////////////////////////////////

/*****************************************
函数名称：串口发送函数
入参：buf 需要发送数据首地址，len数据长度
*****************************************/
void uart_send_Bty(unsigned char* buf,unsigned short len)
{
	unsigned short i;
	for (i = 0; i < len; i++)
	{
		usart_send_blocking(USART2, buf[i]);
	}
}



/*****************************************
函数名称：串口接收中断函数
入参:串口事件状态
*****************************************/
//串口中断接收，每次中断只接收1字节数据
void usart2_isr(void)
{                                                                           //judge whether or not recive the uart data          
	if(usart_get_interrupt_source(USART2,USART_SR_RXNE))
		{
			data_array[0]= usart_recv_blocking(USART2);                     //read uart data
			switch(olddata)
			{
				case uart_0xa5:                                             //已经收到数据头0xa5,
								if(data_array[0]==0x5a){olddata=uart_0x5a;} //接收数据头完成准备，接收长度字节
								else{olddata=0;}                            //接收字节错误，从新等待接收数据头
				break;

				case uart_0x5a:                                             //已经收到数据头0x5a,数据头已经全部接收完成且正确
								uart_communicate_buf.length=data_array[0];  //存储长度字节高8位
								uart_communicate_buf.length=uart_communicate_buf.length<<8;
								olddata=uart_length_H;
				break;

				case uart_length_H:                                         //已经收到长度字节高8位,
								uart_communicate_buf.length=(uart_communicate_buf.length)|data_array[0];//存储长度字节高8位
								olddata=uart_length_L;
				break;

				case uart_length_L:                                         //长度字节接收完成
								uart_communicate_buf.cmd = data_array[0];   //存储命令字节
								olddata=uart_cmd;
				break;

				case uart_cmd:                                              //命令字节接收完成
								uart_communicate_buf.no_use = data_array[0];//存储保留字节
								uart_recive_l=0;                            //已接收的数据长度清零
								olddata=uart_nouseful;
				break;

				case uart_nouseful:                                       //保留字节接收完成
								uart_communicate_buf.data[uart_recive_l]=data_array[0];//接收数据区数据
								uart_recive_l++;
								if(uart_recive_l<uart_communicate_buf.length-2){}
								else
								{                                                     //接收完成
									uart_recive_All=1;                                //接收完成标志位置 olddata与uart_recive_All的清零都要在接收数据处理完以后进行
									olddata=uart_databytes;
									uart_communicate_buf.crc16=uart_communicate_buf.data[uart_recive_l-2];
									uart_communicate_buf.crc16=(uart_communicate_buf.crc16)<<8;
									uart_communicate_buf.crc16=uart_communicate_buf.crc16|uart_communicate_buf.data[uart_recive_l-1];
								}
				break;

				case uart_databytes:                                               //数据已经接收完成，若没有处理完再有数据接收的化，不进行存储
				break;

				case uart_first_0x5a:                                              //已经接收到ack数据头5a
								if(data_array[0]==0xa5){olddata=uart_secend_0xa5;} //接收数据头完成准备，接收状态字节
								else{olddata=0;}                                   //接收字节错误，从新等待接收数据头
				break;

				case uart_secend_0xa5:                                             //ACK数据头接收完成
								if(data_array[0]==0x00){ACK_recive_satus=SuccsACK;}//ACK成功接收
								else{ACK_recive_satus=FailACK;}
				break;

				default:                                                              //等待接收数据头字节
								if(data_array[0]==0xa5&&olddata==0){olddata=uart_0xa5;}//接收到数据头，准备接收下一字节数据头//正常数据
								if(data_array[0]==0x5a&&olddata==0){olddata=uart_first_0x5a;}//接收到数据头，准备接收下一字节数据头//ACK
				break;
			}
		}
	USART_SR(USART2)=0;                                                             // Clear the interupt flag for the processed IRQs
}
/**@snippet [Handling the data received over UART] */

//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////APP 协议层///////////////////////////////////////////////////


/***********************
//uart发送函数
***********************/
void uart_send(void)
{
  unsigned char len[2]={0,0};

  len[0]=0x00ff&((uart_communicate_buf.length)>>8);
	len[1]=0x00ff&(uart_communicate_buf.length);

  //数据头发送
	usart_send_blocking(USART2, 0xa5);
	usart_send_blocking(USART2, 0x5a);
  //数据长度发送
	usart_send_blocking(USART2, len[0]);
	usart_send_blocking(USART2, len[1]);
	//命令字节发送
	usart_send_blocking(USART2, uart_communicate_buf.cmd);
	//预留字节发送
	usart_send_blocking(USART2, uart_communicate_buf.no_use);
	//数据区发送
	uart_send_Bty(uart_communicate_buf.data,((uart_communicate_buf.length)-2));

	uart_communicate_buf.data=Data_64Bytes;
}

/******************************
函数名称：crc校验
出参: 0 校验成功 ， 1 校验失败
****************************/
unsigned char CRC16_check(void)
{
	unsigned short crc16=0;
	unsigned char data[4];

	data[0]=0x00ff&((uart_communicate_buf.length)>>8);
	data[1]=0x00ff&((uart_communicate_buf.length));
	data[2]=uart_communicate_buf.cmd;
	data[3]=uart_communicate_buf.no_use;

  crc16=bd_crc16(0,data,4);               //计算校验值
	crc16=bd_crc16(crc16,uart_communicate_buf.data,uart_communicate_buf.length-4);
	if(crc16==uart_communicate_buf.crc16)	{return 0;}
  else{}
  return 1;

}

unsigned short CRC16_Uart_send(void)
{
  unsigned short crc16=0;
	unsigned char data[4];

	data[0]=0x00ff&((uart_communicate_buf.length)>>8);
	data[1]=0x00ff&((uart_communicate_buf.length));
	data[2]=uart_communicate_buf.cmd;
	data[3]=uart_communicate_buf.no_use;

  crc16=bd_crc16(0,data,4);               //计算校验值
	crc16=bd_crc16(crc16,uart_communicate_buf.data,uart_communicate_buf.length-4);

	return crc16;
}

/**********************************
 function name:mode change
 **********************************/
void ModeSelect(unsigned char* buf)
{
  switch(buf[0])
  {
    case 1://timer mode
      stm32workstatus=Stm32_TimerMode;
      break;
    case 0://Key mode
      stm32workstatus=Stm32_KeyMode;
      break;
    case 2://lowpower
      stm32workstatus=Stm32_LowpowerMode;
      needsuccessack_flag=1;                   //do not need successACK
      break;
    case 3://balance mode
      stm32workstatus=Stm32_BalanceMode;
	  balance_k=0;
	  balance_j=0;
	  TP_CLOSE(&T_Display);                    //关闭计时器，准备从新计时//关闭1秒超时定时器
	  Timeout1Sec_display_StarFlag=TimeClose;	  
	  TP_CLOSE(&T_Display2);                   //关闭计时器，准备从新计时//关闭1秒超时定时器
	  Timeout1Sec_display_StarFlag2=TimeClose;	
      break;
    case 4://broadcast name
      stm32workstatus=Stm32_Broadcastname;
      break;
    default:
      break;
  }

}

unsigned char SaveNewbalanceflag=0;
void SaveBalanceData(unsigned char* buf)
{
      memcpy(&Balancedisplaybuf,&buf[0],balance_size);
      SaveNewbalanceflag=1;
}
/*
 * Save timer display Datas from bluetoothIC
 */
void SaveTimData(unsigned char* buf)
{
  Timerdisplaybuf.Year=buf[0];
  Timerdisplaybuf.Year=((Timerdisplaybuf.Year)<<8)|buf[1];
  Timerdisplaybuf.Month=buf[2];
  Timerdisplaybuf.Day=buf[3];
  Timerdisplaybuf.hours=buf[4];
  Timerdisplaybuf.minute=buf[5];
  Timerdisplaybuf.second=buf[6];
  Timerdisplaybuf.chargingstatus=0x0f&(buf[7]>>4);
  Timerdisplaybuf.bluetoothconnetstatus=0x0f&buf[7];
  Timerdisplaybuf.bat_vol=buf[8];
}

/*************************
函数名称:指令发送
入参：
*************************/
void CmdSendUart(unsigned char Cmd_uart,unsigned char* apdubuf,unsigned short apdulength)
{
		 uart_communicate_buf.length=1+1+apdulength+2;
		 uart_communicate_buf.cmd=Cmd_uart;              //命令

		 uart_communicate_buf.no_use=uartnum;        //数据流号
		 uart_communicate_buf.data=apdubuf;          //APDU数据区

		 uart_communicate_buf.crc16=CRC16_Uart_send();//计算校验位

		 uart_communicate_buf.data[apdulength]=0x00ff&(uart_communicate_buf.crc16>>8);//校验位存放
		 uart_communicate_buf.data[apdulength+1]=0x00ff&(uart_communicate_buf.crc16);

		 uart_send_flag=1;//串口发送标志置位，串口发送数据
			needsuccessack_flag=1;//No need to send Success
}
//void _rx_callback(unsigned char* buf);
//void ManyBlock64bytes_callback(unsigned char* buf,unsigned short length);
/************************************
函数名称：数据指令处理
***********************************/
void Uart_cmdprogam(void)
{
	switch(uart_communicate_buf.cmd)
	{
		case 0x01://APDU值
					if(stm32workstatus==Stm32_KeyMode)
					{ManyBlock64bytes_callback(uart_communicate_buf.data,uart_communicate_buf.length-4);}
		break;

		case 0x02://mode change
					ModeSelect(uart_communicate_buf.data);
		break;

		case 0x03://timer display//for save timer
					if(stm32workstatus==Stm32_TimerMode)
					{SaveTimData(uart_communicate_buf.data);}
		break;

		case 0x06://balance display//for save balance
					if(stm32workstatus==Stm32_BalanceMode)
					{
						SaveBalanceData(uart_communicate_buf.data);
					}
		break;

		case 0x07://balance display//for save balance
					stm32workstatus=Stm32_idleMode;
					blueParingdisplay(uart_communicate_buf.data);
	    break;

		case 0x08://bluetooth name display
					stm32workstatus=Stm32_idleMode;
					bluenamedisplay(uart_communicate_buf.data);
		break;
		case 0x0A://shutdown display
					stm32workstatus=Stm32_idleMode;
					shutdowndisplay();
        break;

	}
}
extern uint32_t msg_out_start;
extern uint32_t msg_out_end;
/*************************************
函数名称：数据发送接收函数
**************************************/
void UartDataSendrecive(void)
{
	unsigned char ack_erro[3]={0x5a,0xa5,0x01};
	unsigned char ack_succs[3]={0x5a,0xa5,0x00};

	if(uart_send_flag==1)                                 //有数据需要发送
	{
		uart_send();                                      //发送数据
		uart_send_flag=0;                                 //发送标志清零
		sendsuccessflag=1;
	}

	if((uart_recive_All==1)||(ACK_recive_satus!=NOACK))  //判断是否有应答数据//数据接收已完成
	{                                                    //有应答数据
		TP_CLOSE(&T_Connect);                            //关闭计时器，准备从新计时//关闭1秒超时定时器
		Timeout1Sec_Uart_StarFlag=TimeClose;
		if(uart_recive_All==1)                           //成功返回数据，数据接收完成
		{
			ACC_resend=0;                                //重发累计清零
		    if(CRC16_check()==0)                         //接收数据校验是否正确
			{
				sendsuccessflag=0;               
				needsuccessack_flag=0;
				if(uart_communicate_buf.no_use!=uartnum)  //判断recive again
				{
					uartnum=uart_communicate_buf.no_use;
					Uart_cmdprogam();                     //根据指令进行处理
				}

				if(((loopuart==0)||((loopuart==1)&&(protectbuttonflag==1)))&&(needsuccessack_flag==0)&& (msg_out_start == msg_out_end) )
				{
					uart_send_Bty(ack_succs,3);          //串口发送 成功ACK
					sendsuccessflag=1;
				}
			}
			else
			{
				uart_send_Bty(ack_erro,3);               //返回错误ACK
				sendsuccessflag=1;
			}
		}
		else                                             //接收到ACK包
		{
			if(ACK_recive_satus==FailACK)
			{                                            //重发数据
				ACC_resend++;                            //重发次数累加
				if(ACC_resend<MAX_resend)                //判断是否达到最大次数
				{                                        //没有达到最大次数//重发数据
					uart_send_flag=1;                    //发送标志位置1，从新发送原有数据
				}
				else
				{
					ACC_resend=0;                        //重发累计清零
				}
			}

			if(SuccsACK==ACK_recive_satus)
			{
				sendsuccessflag=0;                      //send success!!!!!!!!
				ACC_resend=0;                           //重发累计清零
			}
		}

		ACK_recive_satus=0;                             //清ACK接收状态
		uart_recive_All=0;                              //清接收完成标志
		olddata=0;				                        //清接收流程状态
	}
	else
	{                                                  //无应答数据 wait send ack
		if(sendsuccessflag==1)                         //判断超时1秒定时器是否开启
		{
			if(Timeout1Sec_Uart_StarFlag==TimeClose)
			{
				TP_START(&T_Connect, ONE_SECOND_INTERVAL); //开启定时器
				Timeout1Sec_Uart_StarFlag=TimeOpen;
			}
			else
			{	                                          //已经打开
				if(CheckTP(&T_Connect)==0)                //判断是否超时
				{	                                      //超时
					TP_CLOSE(&T_Connect);                 //关闭计时器，准备从新计时//关闭1秒超时定时器
					Timeout1Sec_Uart_StarFlag=TimeClose;
					ACC_resend++;                         //重发次数累加
					if(ACC_resend<MAX_resend)             //判断是否达到最大次数
					{}                                    //没有达到最大次数//重发数据
					else
					{
						ACC_resend=0;                     //重发累计清零
						sendsuccessflag=3;                //Resend fail
					}
				}
				else
				{}//无超时，继续等待ACK或应答数据
			}
		}
	}

}


#define USART_ISR_IDLE			(1 << 4)
#define USART_ISR(usart_base)		MMIO32((usart_base) + 0x1c)
#define USART_ISR_TXE			(1 << 7)
#define USART_ISR_ORE			(1 << 3)


bool usart_get_interrupt_source(uint32_t usart, uint32_t flag)
{
	uint32_t flag_set = (USART_SR(usart) & flag);
	/* IDLE, RXNE, TC, TXE interrupts */
	if ((flag >= USART_ISR_IDLE) && (flag <= USART_ISR_TXE)) {
		return ((flag_set & USART_CR1(usart)) != 0);
	/* Overrun error */
	} else if (flag == USART_ISR_ORE) {
		return flag_set && (USART_CR3(usart) & USART_CR3_CTSIE);
	}

	return false;
}











