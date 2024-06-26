#include "main.h"
#include "uart.h"
#include "cmd.h"

u16 Rx1_Timer  = 0;
u16 Rx2_Timer  = 0;
u16 Rx3_Timer  = 0;
u16 Rx4_Timer  = 0;


u8  TX1_Cnt;    //发送计数
u8  RX1_Cnt;    //接收计数
bit B_TX1_Busy; //发送忙标志

u8  TX2_Cnt;    //发送计数
u8  RX2_Cnt;    //接收计数
bit B_TX2_Busy; //发送忙标志

u8  TX3_Cnt;
u8  RX3_Cnt;
bit B_TX3_Busy;

u8  TX4_Cnt;
u8  RX4_Cnt;
bit B_TX4_Busy;


u8  RX1_Buffer[MAX_LENGTH]; //接收缓冲
u8  RX2_Buffer[MAX_LENGTH]; //接收缓冲
u8  RX3_Buffer[MAX_LENGTH]; //接收缓冲
u8  RX4_Buffer[MAX_LENGTH]; //接收缓冲

UART_DATA xdata g_UartData[2];


void Uart1_Init(void)        //115200bps@11.0592MHz
{

    TR1 = 0;
    AUXR &= ~0x01;      //S1 BRT Use Timer1;
    AUXR |=  (1<<6);    //Timer1 set as 1T mode
    TMOD &= ~(1<<6);    //Timer1 set As Timer
    TMOD &= ~0x30;      //Timer1_16bitAutoReload;
    TH1 = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate) / 256);
    TL1 = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate) % 256);
    ET1 = 0;    //禁止中断
    INTCLKO &= ~0x02;  //不输出时钟
    TR1  = 1;

    /*************************************************/
    //UART1模式, 0x00: 同步移位输出, 0x40: 8位数据,可变波特率, 
    //           0x80: 9位数据,固定波特率, 0xc0: 9位数据,可变波特率 

    SCON = (SCON & 0x3f) | 0x40; 

    PS  = 0;    //中断高优先级
    PSH = 0;
    //PS  = 1;    //高优先级中断
    ES  = 1;    //允许中断
    REN = 1;    //允许接收
    
    //UART1 switch to, 0x00: P3.0 P3.1, 0x40: P3.6 P3.7, 
    //                 0x80: P1.6 P1.7, 0xC0: P4.3 P4.4
    P_SW1 &= 0x3f;
    P_SW1 |= 0x40;  
    
    B_TX1_Busy = 0;
    g_UartData[0].RecvLength  = 0;
    g_UartData[0].SendLength  = 0;


}

void Uart2_Init(void)        //115200bps@11.0592MHz
{
    T2R   = 0;
    T2x12 = 1;
    AUXR &= ~(1<<3);
    T2H = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate3)/ 256);
    T2L = (u8)((65536UL - (MAIN_Fosc / 4) / Baudrate3)% 256);
    ET2 = 0;    //禁止中断
    T2R = 1;

    S2CON = (S2CON & 0x3f) | 0x40;
    
    PS2  = 0;    //中断高优先级
    PS2H = 0;
    
    ES2   = 1;
    S2REN = 1;
    P_SW2 &= ~0x01; 

    B_TX2_Busy = 0;
    g_UartData[1].RecvLength  = 0;
    g_UartData[1].SendLength  = 0;
        
}

//重定向Printf
char putchar(char c )
{
    SBUF = c;
    while(!TI);
    TI = 0;
    return c;
}


void UART1_ISR (void) interrupt 4
{
    if(RI)
    {
        RI = 0;
        g_UartData[0].RecvBuff[g_UartData[0].RecvLength] = SBUF;
        g_UartData[0].Timer = 0;
        
        if(++g_UartData[0].RecvLength >= MAX_LENGTH)   
        {
            g_UartData[0].RecvLength  = 0;
        }
    }

    if(TI)
    {
        TI = 0;
        B_TX1_Busy = 0;
    }
}



void UART2_ISR (void) interrupt 8
{
    if(S2RI)
    {
        S2RI = 0;
        g_UartData[1].RecvBuff[g_UartData[1].RecvLength] = S2BUF;
        g_UartData[1].Timer = 0;
        
        if(++g_UartData[1].RecvLength >= MAX_LENGTH)   
        {
            g_UartData[1].RecvLength  = 0;
        }
    }

    if(S2TI)
    {
        S2TI = 0;
        B_TX2_Busy = 0;
    }
}



//UART1 发送串口数据
void UART1_SendData(char dat)
{
    ES=0;            //关串口中断
    SBUF=dat;            
    while(TI!=1);    //等待发送成功
    TI=0;            //清除发送中断标志
    ES=1;            //开串口中断
}

//UART1 发送字符串
void UART1_SendString(char *s)
{
    while(*s)//检测字符串结束符
    {
        UART1_SendData(*s++);//发送当前字符
    }
}

void Uart1Send(u8 *buf, u8 len)
{
    u8 i;
    for (i=0;i<len;i++)     
    {
        SBUF = buf[i];
        B_TX1_Busy = 1;
        while(B_TX1_Busy);
    }
}

void Uart2Send(u8 *buf, u8 len)
{
    u8 i;
    for (i=0;i<len;i++)     
    {
        S2BUF = buf[i];
        B_TX2_Busy = 1;
        while(B_TX2_Busy);
    }  
}


void Clear_Uart1_Buf()
{
    memset(g_UartData[0].RecvBuff,0,MAX_LENGTH);
    g_UartData[0].RecvLength = 0;
    g_UartData[0].Timer = 0;
}


void ClearUart1Buf()
{
    memset(RX1_Buffer,0,MAX_LENGTH);
    RX1_Cnt = 0;
}


void ClearUart2Buf()
{
    memset(RX2_Buffer,0,MAX_LENGTH);
    RX2_Cnt = 0;
}

void Clear_Uart2_Buf()
{
    memset(g_UartData[1].RecvBuff,0,MAX_LENGTH);
    g_UartData[1].RecvLength = 0;
    g_UartData[1].Timer = 0;
}


void Uart1Hnd()
{
    if (g_UartData[0].Timer > 20)
    {
        //Rx1_Timer = 0;
        //g_UartData[0].RecvLength = RX1_Cnt;
        g_UartData[0].Timer = 0;
        //memcpy(&g_UartData[0].RecvBuff,RX1_Buffer,RX1_Cnt);
        HndPcFrame();
        //ClearUart1Buf();
    }
}


void Uart2Hnd()
{
    u16 i = 0;
    if (g_UartData[1].Timer > 60)
    {
        g_UartData[1].Timer = 0;

        //DumpCmd(RX2_Buffer, RX2_Cnt);
        //printf("进入Uart2\r\n");
        //g_UartData[1].RecvLength = RX2_Cnt;
        
        //printf("g_UartData[1].LEN = %d\r\n",g_UartData[1].RecvLength);
        //PrintData(g_UartData[1].RecvBuff,(u8)g_UartData[1].RecvLength);
        //memcpy(g_UartData[1].RecvBuff,RX2_Buffer,RX2_Cnt);
        //HndPcFrame();
        //ClearUart2Buf();

    }
}


