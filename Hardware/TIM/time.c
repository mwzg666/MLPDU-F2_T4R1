#include "time.h"
#include "cmd.h"
#include "uart.h"

u16  Timer0Cnt = 0;
u16  Timer1Cnt = 0;
u16  Time1s = 0;

void Timer0_Init(void)        //10毫秒@11.0592MHz
{
    AUXR = 0x00;    //Timer0 set as 12T, 16 bits timer auto-reload, 
    TH0 = (u8)(Timer0_Reload / 256);
    TL0 = (u8)(Timer0_Reload % 256);
    ET0 = 1;    //Timer0 interrupt enable
    TR0 = 1;    //Tiner0 run
    
    // 中断优先级3
    PT0  = 0;
    PT0H = 0;


}

// 10ms 中断一下
void Timer0ISR (void) interrupt 1
{
    Timer0Cnt ++;
}


void Timer1_Init(void)        //10毫秒@11.0592MHz
{
    AUXR &= 0xBF;            //定时器时钟12T模式
    TMOD &= 0x0F;            //设置定时器模式
    TL1 = 0x00;                //设置定时初始值
    TH1 = 0xDC;                //设置定时初始值
    //TF1 = 0;                //清除TF1标志
    TR1 = 1;                //定时器1开始计时
    PT0  = 1;               //中断优先级
    PT0H = 0;
}


// 10ms 中断一下
void Timer1ISR (void) interrupt 3
{
    Timer1Cnt ++;
}

void delay_us(BYTE us)
{
    while(us--)
    {
        ;
    }
}

void delay_ms(unsigned int ms)
{
    unsigned int i;
    do{
        i = MAIN_Fosc / 6030;
        while(--i);
    }while(--ms);
}

void Delay(WORD ms)
{
    WORD t = 1000;
    while(ms--)
    {
        for (t=0;t<1000;t++) ;
    }
}

void TimerTask()
{
    u16 delta = 0;
    static u16 Time1s = 0;
    BYTE i;
    if (Timer0Cnt)
    {
        delta = Timer0Cnt * 10;
        Timer0Cnt = 0;

        if (RX1_Cnt > 0)
        {
            Rx1_Timer += delta;
        }
        if (RX2_Cnt > 0)
        {
            Rx2_Timer += delta;
        }

        gIdleTime += delta;
        
        if (gRunTime < 5000)
        {
            gRunTime += delta;
        }
        
        for (i=0;i<2;i++)
        {
            if (g_UartData[i].RecvLength > 0)
            {
                g_UartData[i].Timer += delta;
            }
        }
        
        Time1s += delta;
        if (Time1s >= 1000)
        {
            CLR_WDT = 1;  // 喂狗
            Time1s = 0;
            Task_1s();
        }
        g_CommIdleTime += delta;

        RunLed(delta);
        IoCtlTask();
    }
}



