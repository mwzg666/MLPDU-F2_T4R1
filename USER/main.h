#ifndef __MAIN_H__
#define __MAIN_H__

#include "STC32G.h"
#include "stdio.h"
#include "intrins.h"
#include "string.h"
#include <stdlib.h>


//===================================定义=================================

//#define  FOR_DEBUG
#define     TRUE    1
#define     FALSE   0

typedef     unsigned char    BOOL;

typedef     unsigned char    BYTE;
typedef     unsigned short    WORD;
typedef     unsigned long    DWORD;

typedef     unsigned char    u8;
typedef     unsigned short    u16;
typedef     unsigned long    u32;

typedef     unsigned int    uint;

#define alt_u8      BYTE
#define alt_u16     WORD
#define alt_u32     DWORD  


#define bool        BYTE
#define true        1
#define false       0

#define BOOL        BYTE
#define TRUE        1
#define FALSE       0


#define MAIN_Fosc        11059200UL    // 11.0592M
//#define MAIN_Fosc        35000000UL    // 35M
#if 1
#define OUT_IO_COUNT     6

#define LED_GREEN        0
#define LED_YELLOW       1
#define LED_RED          2
#define LIGHT_YELLOW     3
#define LIGHT_RED        4
#define ALARM_SOUND      5
#endif

#if 1
#define OUT_Channel_COUNT     4

#define LIGHT_OUT1       0
#define LIGHT_OUT2       1
#define LIGHT_OUT3       2
#define LIGHT_OUT4       3
#endif



#if 0
#define RELAY_PRE         6   // 预警
#define RELAY_ALM         7   // 报警
#define RELAY_OL          8   // 过载
#define RELAY_FLT         9   // 故障
#endif

#define KEY_ALM_CFM      3

#define LED_FLASH_TIME    500       // ms

#define RUN_LED(x) (x)?(P0 |= (1<<7)):(P0 &= ~(1<<7))    // 板载LED

// 三色LED
#define GRE_LED(x) (x)?(P4 |= (1<<3)):(P4 &= ~(1<<3))     // 绿
#define YEL_LED(x) (x)?(P4 |= (1<<4)):(P4 &= ~(1<<4))     // 黄
#define RED_LED(x) (x)?(P2 |= (1<<0)):(P2 &= ~(1<<0))     // 红

// 指示灯
#define RED_LIGHT(x) (x)?(P3 |= (1<<5)):(P3 &= ~(1<<5))      // 红灯
#define YEL_LIGHT(x) (x)?(P5 |= (1<<1)):(P5 &= ~(1<<1))      // 黄灯
//#define BLU_LIGHT(x) (x)?(P3 |= (1<<2)):(P3 &= ~(1<<2))      // 蓝灯

//#define LEDF(x)   (x)?(P2 |= (1<<4)):(P2 &= ~(1<<4))   //故障报警灯
//#define LEDM(x)   (x)?(P2 |= (1<<3)):(P2 &= ~(1<<3))    //远程报警灯
#define ALARM(x)  (x)?(P4 |= (1<<2)):(P4 &= ~(1<<2))      // 报警音

#define PW_MAIN(x)      (x)?(P0 |= (1<<6)):(P0 &= ~(1<<6))         // 主电源控制
#define IPC_PWR_ON(x)   (x)?(P0 |= (1<<3)):(P0 &= ~(1<<3))         // PC上电开关
//#define SEN_POWER(x)    (x)?(P5 |= (1<<2)):(P5 &= ~(1<<2))         // 探测器电源控制
#define CONFIRM_BUTTON()      (P4 & (1<<1))        //J5 确认键

#define PC_STAUTUS()          (P1 & (1<<3))        //PC运行状态信号
#define BAT_CHARGE()          (P5 & (1<<2))        //电池充电状态

#define ALMOUT_1(x) (x)?(P0 |= (1<<0)):(P0 &= ~(1<<0))    // Aout1
#define ALMOUT_2(x) (x)?(P0 |= (1<<1)):(P0 &= ~(1<<1))    // Aout2 
#define ALMOUT_3(x) (x)?(P0 |= (1<<2)):(P0 &= ~(1<<2))    // Aout3
#define ALMOUT_4(x) (x)?(P0 |= (1<<4)):(P0 &= ~(1<<4))    // Aout4

#define RELAY_1(x) (x)?(P2 |= (1<<3)):(P2 &= ~(1<<3))    // 远程
#define RELAY_2(x) (x)?(P2 |= (1<<4)):(P2 &= ~(1<<4))    // 故障 
#define RELAY_3(x) (x)?(P2 |= (1<<5)):(P2 &= ~(1<<5))    // 一级
#define RELAY_4(x) (x)?(P2 |= (1<<6)):(P2 &= ~(1<<6))    // 二级

#define POWER_LOCK()    (P5 & 0x01)                      // 开关机锁
#define INPUT1_M()      (P2 & 0x04)                          
#define INPUT2_M()      (P2 & 0x02)                           

#define POWER_ON         0xAA
#define POWER_OFF        0x55


//==================================变量声明=============================
extern alt_u32 gIdleTime;
extern alt_u32 gTask1sTime;
extern alt_u16 gRunTime;

//==================================函数声明=============================

void Error();
void Idle(alt_u32 time);
alt_u8 GetKey();
BYTE GetStatus();
void HndInput();
void Task_1s();
void RunLed(u16 dt);
void IoCtlTask();

void PrintData(alt_u8 *pata, alt_u8 len);


#endif






