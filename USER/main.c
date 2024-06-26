#include "main.h"
#include "time.h"
#include "uart.h"
#include "i2c.h"
#include "Ads1110.h"
#include "Cmd.h"
#include "mcp4725.h"
#include "MwPro.h"

const BYTE VERSION[8] = "V1.0.0";
BYTE xdata StrTmp[64] = {0};

alt_u32 gIdleTime;
alt_u32 gTask1sTime = 0;
#if 0
alt_u8 g_Output[OUT_IO_COUNT]      = {1,0,0,0,0,0};   // 上电绿灯亮 // 
alt_u8 g_OutStatus[OUT_IO_COUNT]   = {0,0,0,0,0,0};
#else
alt_u8 g_Output[OUT_IO_COUNT]      = {1,0,0,0,0,0};   // 上电绿灯亮 // 
alt_u8 g_OutStatus[OUT_IO_COUNT]   = {0,0,0,0,0,0};

alt_u8 g_OutChannelLight[OUT_Channel_COUNT]      = {0,0,0,0};   // 各探头报警灯 // 
alt_u8 g_OutChannelStatus[OUT_Channel_COUNT]   = {0,0,0,0};

#endif

alt_u16 gRunTime = 0;

alt_u8 g_Key_Confrom  = 0; 
alt_u8 g_Key_Power  = 0; 
alt_u8 g_Key_Input  = 0; 
BYTE Input_Status = 0;


void DebugMsg(char *msg)
{
    BYTE len = (BYTE)strlen(msg);
    //Uart1Send((BYTE *)msg,len);
}

void DebugInt(int msg)
{
    memset(StrTmp,0,64);
    sprintf(StrTmp,"%x\r\n",msg);
    DebugMsg(StrTmp);
}

void DumpCmd(BYTE *dat, BYTE len)
{
    BYTE i;
    memset(StrTmp,0,64);
    for (i=0;i<len;i++)
    {
        if (strlen(StrTmp) >= 60)
        {
            break;
        }
        sprintf(&StrTmp[i*3], "%02X ", dat[i]);
    }
    sprintf(&StrTmp[i*3], "\r\n");
    DebugMsg(StrTmp);
}


void Error()
{
    while(1)
    {
        RUN_LED(1);
        delay_ms(50);
        RUN_LED(0);
        delay_ms(50);
    }
    
}

// 板载指示灯
void RunLed(u16 dt)
{   
    static u16 tm = 0;
    u16 to = 3000;
    tm += dt;

    if (tm > to)
    {
        tm = 0;
        RUN_LED(0);
    }
    else if (tm > (to-100))
    {
        RUN_LED(1);
    }
}

void Idle(alt_u32 time)
{
    gIdleTime = 0;
    
    while(gIdleTime < time)
    {
        TimerTask();
        HndInput();
    }
}

void SysInit()
{
    HIRCCR = 0x80;           // 启动内部高速IRC
    while(!(HIRCCR & 1));
    CLKSEL = 0;              
}

void IoInit()
{
    EAXFR = 1;
    WTST = 0;                                           //设置程序指令延时参数，赋值为0可将CPU执行指令的速度设置为最快

    P0M1 = 0x00;   P0M0 = 0xDF;                         // 推挽输出
    P1M1 = 0x08;   P1M0 = 0x00;                         //设置为准双向口//(1<<3)
    P2M1 = 0x06;   P2M0 = 0x79;                        // P2.2 推挽输出
    P3M1 = 0x00;   P3M0 |= (1<<5);                      //设置为准双向口
    P4M1 = 0x02;   P4M0 = 0x1C;                        //设置为准双向口  
    P5M1 = 0x05;   P5M0 |= (1<<1);                      //设置为准双向口
}


void MainTask()
{
    // 等待PC命令
    if (g_UartData[0].Timer > UART_DATA_TIMEOUT)
    {
        //DebugMsg("Recv pc frame Len: %d \r\n", g_UartData[0].RecvLength);
        DebugMsg("PC>:\n");
        PrintData((alt_u8 *)g_UartData[0].RecvBuff, (alt_u8)(g_UartData[0].RecvLength));
        HndPcFrame();
    }
}

void TestRs485()
{
    if (g_UartData[1].Timer > UART_DATA_TIMEOUT)
    {
        //DebugMsg("Recv pc frame Len: %d \r\n", g_UartData[0].RecvLength);
        DebugMsg("PC>:");
        PrintData(g_UartData[1].RecvBuff, (alt_u8)(g_UartData[1].RecvLength));
        Clear_Uart2_Buf();
        //HndPcFrame();
    }
}


void OutCtl(alt_u8 id, alt_u8 st)
{
    if (g_OutStatus[id] == st)
    {
        return;
    }

    g_OutStatus[id] = st;
    
    switch(id)
    {
        case LIGHT_YELLOW: 
        {
            (st)? YEL_LIGHT(1):YEL_LIGHT(0);   
            break;
        }
        
        case LIGHT_RED:    
        {
            (st)? RED_LIGHT(1):RED_LIGHT(0);    
            break;
        }
        
        case LED_RED:      (st)? RED_LED(1):RED_LED(0);        break;
        case LED_GREEN:    (st)? GRE_LED(1):GRE_LED(0);        break;
        
        case LED_YELLOW:    // 故障
        {
            (st)? YEL_LED(1):YEL_LED(0); 
            //(st)? LEDF(1):LEDF(0); 
            //RELAY_2(st);
            break;
        }
        
        case ALARM_SOUND:  (st)? ALARM(1):ALARM(0);       break;
    }

    
}

void OutChannelLightCtl(alt_u8 id, alt_u8 st)
{
     if (g_OutChannelStatus[id] == st)
    {
        return;
    }

    g_OutChannelStatus[id] = st;
    
    switch(id)
    {
        case LIGHT_OUT1: 
        {
            (st)? ALMOUT_1(1):ALMOUT_1(0);   
            break;
        }
        
        case LIGHT_OUT2: 
        {
            (st)? ALMOUT_2(1):ALMOUT_2(0);   
            break;
        }
        case LIGHT_OUT3: 
        {
            (st)? ALMOUT_3(1):ALMOUT_3(0);   
            break;
        }
        
        case LIGHT_OUT4: 
        {
            (st)? ALMOUT_4(1):ALMOUT_4(0);   
            break;
        }
    }
}


void OutFlash(alt_u8 id)
{
    static alt_u16 OutTimer[OUT_IO_COUNT] = {0,0,0,0,0,0};
    //static alt_u16 OutChannelTimer[OUT_Channel_COUNT] = {0,0,0,0};
    if (OutTimer[id] ++ > LED_FLASH_TIME/20)
    {
        OutTimer[id] = 0;
        if (g_OutStatus[id] == 1)
        {
            OutCtl(id, 0);
        }
        else
        {
            OutCtl(id, 1);
        }
    }
    #if 0
    if (OutChannelTimer[id] ++ > LED_FLASH_TIME/20)
    {
        OutChannelTimer[id] = 0;
        if (g_OutChannelStatus[id] == 1)
        {
            OutChannelLightCtl(id, 0);
        }
        else
        {
            OutChannelLightCtl(id, 1);
        }
    }
    #endif
}


void IoCtlTask()
{
    alt_u8 i;
    for (i=0;i<OUT_IO_COUNT;i++)
    {
        if (g_Output[i] == 2)
        {
            OutFlash(i);
        }
        else
        {
            OutCtl(i, g_Output[i]);
        }
    }
    #if 0
    for (i=0;i<OUT_Channel_COUNT;i++)
    {
        if (g_OutChannelLight[i] == 2)
        {
            OutFlash(i);
        }
        else
        {
            OutChannelLightCtl(i, g_OutChannelLight[i]);
        }
    }
    #endif
}

void PowerOff()
{
    GRE_LED(0);
    YEL_LED(0);
    RED_LED(0);
    RED_LIGHT(0);
    YEL_LIGHT(0);
    ALARM(0);

    IPC_PWR_ON(0);
    PW_MAIN(0);    //关闭控制板电源

    while(1)
    {
        ;
    }
}


void WaitCommIdle()
{
    while (g_CommIdleTime < 200)
    {
        Idle(10);
    }
}

void LedInit()
{
    // 初始状态都为0

    // 指示灯
    GRE_LED(0);
    YEL_LED(0);
    RED_LED(0);
    
    YEL_LIGHT(0);   // 黄灯
    RED_LIGHT(0);   // 红灯
    
    //LEDF(0);   
    //LEDM(0);      
    ALARM(0);       

    ALMOUT_1(0);
    ALMOUT_2(0);
    ALMOUT_3(0);
    ALMOUT_4(0);
    
}



void HndInput()
{
    static alt_u8 PwrHis = POWER_OFF;   
    static BYTE  StHis = 0x01;
    
    BYTE s;

    // 报警确认
    alt_u8 key = GetKey();
    if (key == KEY_ALM_CFM)
    {
        DebugMsg("KEY_ALM_CFM \r\n");
        //WaitCommIdle();
        //SendPcCmd(0, CMD_CERTAINKEY, NULL, 0);
        g_Key_Confrom = 1;
    }


    Input_Status = GetStatus();
    if (Input_Status != 0xFF)
    {
        s = (Input_Status ^ StHis);
        if (s && 1)   // 开关机
        {
            if (Input_Status && 1)
            {
                PwrHis = POWER_OFF;
                //WaitCommIdle();
                //SendPcCmd(0, CMD_POWER, &PwrHis, 1);
                g_Key_Power = 1;
            }
            else
            {
                IPC_PWR_ON(1);
                PwrHis = POWER_ON;
                gRunTime = 0;
            }
        }

//        if (s && 4) // 输入1
//        {
//            //WaitCommIdle();
//            //SendPcCmd(0, CMD_INPUT, &st, 1);
//            g_Key_Input = 1;
//        }
//
//        if (s && 2) // 输入2
//        {
//            //WaitCommIdle();
//            //SendPcCmd(0, CMD_INPUT, &st, 1);
//            g_Key_Input = 1;
//        }
            
        StHis = Input_Status;
    }


     //延时关机
    if (PwrHis == POWER_OFF)
    {
        if ((PC_STAUTUS() == 0) && (gRunTime >= 5000))
        {
            gRunTime = 0;
            //DebugMsg("Power off \r\n");
            PowerOff();
        }
    }
}

alt_u8 GetKey()
{
    static alt_u8 key_pressed = 0;
    
    // 不能调用Idle
    if (CONFIRM_BUTTON() == 0)
    {
        if (key_pressed == 1)
        {
            return 0;
        }
        
        delay_ms(20);
        if (CONFIRM_BUTTON() == 0)
        {
            key_pressed = 1;
            return KEY_ALM_CFM;
        }
    }
    else
    {
        key_pressed = 0;
    }
    
    return 0;
}


BYTE GetStatus()
{
    //static BYTE his = 0x07;
    static BYTE his = 0x01;
    BYTE st = POWER_LOCK(),st2 = 0;
    //st |= INPUT1_M();
    //st |= INPUT2_M();

    if (st != his)
    {
        delay_ms(50);
        //st2 = POWER_LOCK();
        //st2 |= INPUT1_M();
        //st2 |= INPUT2_M(); 
        if ( st == POWER_LOCK() )
        {
            his = st;
            return st;
        }
    }

    return 0xFF;
}

void PrintData(alt_u8 *pata, alt_u8 len)
{
    alt_u8 i=0;
    for (i=0; i<len; i++)
    {
        printf("%02X ", pata[i]);
    }
    printf("\r\n");
}


void Task_1s()
{
    //Rs485Send("123456", 6);
    //ReadBatVol();
    //Read4_20ma();
}

void ReportInput()
{
    BYTE PwOff = POWER_OFF;
    
    if (g_CommIdleTime > 300)
    {
        if (g_Key_Confrom)
        {
            g_Key_Confrom = 0;
            SendPcCmd(0, CMD_CERTAINKEY, NULL, 0);
            return;
        }

        if (g_Key_Power)
        {
            g_Key_Power = 0;
            SendPcCmd(0, CMD_POWER, &PwOff, 1);
            return;
        }

        if (g_Key_Input)
        {
            g_Key_Input = 0;
            SendPcCmd(0, CMD_INPUT, &Input_Status, 1);
        }
    }
}


void main(void)
{
    SysInit();
    IoInit();
    PW_MAIN(1);                 // 主电源
    LedInit();
    RUN_LED(1);
   
    delay_ms(200);
    
    Timer0_Init();
    delay_ms(200);
    
    Uart1_Init();
    delay_ms(200);
    
    Uart2_Init();    
    delay_ms(200);

    // 待CPU稳定了再读参数
    delay_ms(500);

    memset((BYTE *)&g_UartData,0,sizeof(UART_DATA)*2); 
    delay_ms(200);

    RUN_LED(0);
    EA = 1;  //打开总中断
    
    WDT_CONTR |= (1<<5) |  7;   // 启动开门狗，约8秒
    delay_ms(200);
    Out4_20ma(1,0);
    Out4_20ma(2,0);

    while(1)
    {
        TimerTask();
        HndInput();
        ReportInput();
        Uart1Hnd();
      
    }
}


