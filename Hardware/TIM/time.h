#ifndef _TIME_H_
#define _TIME_H_

#include "main.h"

//===================================定义=================================

#define SysTick         9216                        // 10ms,系统滴答频率, 在4000~16000之间
#define Timer0_Reload   (65536UL - SysTick)         //Timer 0 中断频率


//==================================变量声明=============================
extern u16  Timer0Cnt;



//==================================函数声明=============================

void Timer0_Init(void);
void Timer1_Init(void);
void Delay(WORD ms);

void delay_ms(unsigned int ms);
void delay_us(BYTE us);    
void TimerTask();



#endif

