#ifndef __UART_H__
#define __UART_H__

//===================================定义=================================
#include "main.h"

#define Baudrate        115200UL
#define Baudrate3       9600UL

#define UART_BUFF_LENGTH 128

#define MAX_LENGTH      200

#define USER_MAIN_DEBUG




typedef struct
{
    alt_u32   Timer;
    alt_u16   RecvLength;                  //接收的数据长度
    alt_u16   SendLength;                  //要发送数据长度
    alt_u8    RecvBuff[MAX_LENGTH];	       //串口接收缓存
    alt_u8    SendBuff[MAX_LENGTH];	       //串口接收缓存
}UART_DATA;


//==================================变量声明=============================

extern u8  TX1_Cnt;    //发送计数
extern u8  RX1_Cnt;    //接收计数
extern bit B_TX1_Busy; //发送忙标志
extern u16 Rx1_Timer;

extern u8  TX2_Cnt;    //发送计数
extern u8  RX2_Cnt;    //接收计数
extern bit B_TX2_Busy; //发送忙标志
extern u16 Rx2_Timer;

extern u8  RX3_Cnt;    //接收计数
extern u8  TX3_Cnt;    //发送计数
extern bit B_TX3_Busy; //发送忙标志
extern u16 Rx3_Timer;

extern u8  RX4_Cnt;    //接收计数
extern u8  TX4_Cnt;    //发送计数
extern bit B_TX4_Busy; //发送忙标志
extern u16 Rx4_Timer;


extern u8  xdata RX2_Buffer[]; //接收缓冲
extern u8  xdata RX3_Buffer[]; //接收缓冲

extern UART_DATA xdata g_UartData[2];

//==================================函数声明=============================

void Uart1_Init(void);
void Uart2_Init(void);


void Uart1Send(u8 *buf, u8 len);
void Uart2Send(u8 *buf, u8 len);


void ClearUart1Buf();
void ClearUart2Buf();


void Clear_Uart1_Buf();
void Clear_Uart2_Buf();


void Uart1Hnd();
bool Uart2Hnd();

#endif



