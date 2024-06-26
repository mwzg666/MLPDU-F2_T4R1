#include "main.h"
#include "uart.h"
#include "MwPro.h"
#include "cmd.h"
#include "i2c.h"
#include "mcp4725.h"
#include "ads1110.h"


extern const BYTE VERSION[];
extern alt_u8 g_Output[];
extern alt_u8 g_OutChannelLight[];

extern DWORD Cps[];

BYTE g_CrcFlag = 0;

WORD g_CommIdleTime = 0;

//UART_DATA xdata g_UartData[1];



// 51单片机是大端的，通过结构体发送的数据要转换为小端
DWORD DwordToSmall(DWORD dat)
{
    BYTE buf[4];
    BYTE t;
    DWORD ret;
    
    memcpy(buf, &dat, 4);
    t = buf[3];
    buf[3] = buf[0];
    buf[0] = t;
    t = buf[2];
    buf[2] = buf[1];
    buf[1] = t;

    memcpy(&ret, buf, 4);
    return ret;
}


WORD WordToSmall(WORD dat)
{
    BYTE buf[2];
    BYTE t;
    WORD ret;
    
    memcpy(buf, &dat, 2);
    t = buf[1];
    buf[1] = buf[0];
    buf[0] = t;
    
    memcpy(&ret, buf, 2);
    return ret;
}

BYTE ByteToSmall(BYTE dat)
{
    char buf[8];
    BYTE t,i,j;
    BYTE ret;
    
    memcpy(buf, &dat, 8);
    for(i = 0,j = 7;i < 4;i++,j--)
    {
        t = buf[j];
        buf[j] = buf[i];
        buf[i] = t;
    }
    
    memcpy(&ret, buf, 8);
    return ret;
}



float FloatToSmall(float dat)
{
    BYTE buf[4];
    BYTE t;
    float ret;
    
    memcpy(buf, &dat, 4);
    t = buf[3];
    buf[3] = buf[0];
    buf[0] = t;
    t = buf[2];
    buf[2] = buf[1];
    buf[1] = t;

    memcpy(&ret, buf, 4);
    return ret;
}



void ClearRecvData(UART_DATA *pUartData)
{
    memset(pUartData->RecvBuff, 0, MAX_LENGTH);
    pUartData->RecvLength = 0;
    pUartData->Timer = 0;
}

bool ValidFrame(UART_DATA *pUartData)
{
    alt_u8 lcrc;
    alt_u8 tmp[3] = {0},tmp2[3] = {0};
    WORD res = 0;
    FRAME_HEAD FrmHead;
    memcpy(&FrmHead, pUartData->RecvBuff, sizeof(FRAME_HEAD));

    if (FrmHead.Head != HEAD)
    {
        
        //printf("Head_error!\r\n");
        return false;
    }

    if (pUartData->RecvBuff[pUartData->RecvLength-1] != TAIL)
    {
        //printf("TAIL_error!\r\n");
        return false;
    }
    
    if (FrmHead.Len != pUartData->RecvLength)
    {
        //printf("LEN_error!\r\n");
        return false;
    }

    lcrc = CheckSum(pUartData->RecvBuff,pUartData->RecvLength);
    //printf("lcrc = %02X\r\n",lcrc);
    
    sprintf((char *)tmp, "%02X",lcrc);
    //printf("tem = %s\r\n",tmp);

    if ( (memcmp(tmp, &pUartData->RecvBuff[pUartData->RecvLength-3], 2) != 0) )
    {
        return false;
    }
    
    #if 0
    //    if(g_CrcFlag)
    //    {
    //        sprintf((char *)tmp, "%02X",lcrc);
    //        printf("tem = %s\r\n",tmp);
    //        //if ( (memcmp(tmp,tmp2,2) != 0) )
    //        if ( (memcmp(tmp, &pUartData->RecvBuff[pUartData->RecvLength-3], 2) != 0) )
    //        {
    //            printf("CRC_error1!\r\n");
    //            return false;
    //        }
    //    }
    //    else
    //    {
    //        memcpy(&res,&pUartData->RecvBuff[pUartData->RecvLength-3],2);
    //        if ((WORD)lcrc != res)
    //        {
    //            
    //            printf("CRC2_error!\r\n");
    //            return FALSE;
    //        }
    //    }
    #endif

    return true;
}


void MakeFrame(UART_DATA *pUartData, alt_u8 Addr, alt_u8 Cmd, alt_u8 *dat, alt_u8 length)
{
    alt_u8 lcrc;
        
    FRAME_HEAD FrmHead;
    FrmHead.Head = HEAD;
    FrmHead.Len  = length+8;
    FrmHead.Type = 0;
    FrmHead.Addr = Addr;
    FrmHead.Cmd  = Cmd;

    memcpy(pUartData->SendBuff, &FrmHead, sizeof(FRAME_HEAD));
    if (length > 0)
    {
        memcpy(&pUartData->SendBuff[DAT], dat, length);
    }

    lcrc = CheckSum(pUartData->SendBuff,FrmHead.Len);//计算校验和
    //printf("Send_CRC = %x\r\n",lcrc);
    //将校验和转换为两个字节的ASCII
    //memcpy(&pUartData->SendBuff[FrmHead.Len-3],(BYTE*)&lcrc,2);
    sprintf((char *)&pUartData->SendBuff[FrmHead.Len-3],"%02X",lcrc);
    //printf("Send_Buff[5] = %x\r\n",pUartData->SendBuff[FrmHead.Len-3]);
    //printf("Send_Buff[6] = %x\r\n",pUartData->SendBuff[FrmHead.Len-2]);
    pUartData->SendBuff[FrmHead.Len-1] = TAIL;   
    pUartData->SendLength = FrmHead.Len;
}


void SendPcCmd(alt_u8 Addr, alt_u8 Cmd, alt_u8 *dat, alt_u8 length)
{
    g_CrcFlag = 0;
    MakeFrame(&g_UartData[0], Addr, Cmd, dat, length);
    //DebugMsg("PC<:");
    //PrintData(g_UartData[0].SendBuff,g_UartData[0].SendLength);
    Uart1Send(g_UartData[0].SendBuff,(alt_u8)g_UartData[0].SendLength);

    g_CommIdleTime = 0;
}

bool SendSensorCmd(alt_u8 Addr, alt_u8 Cmd, alt_u8 *Data, alt_u8 length)
{

    MakeFrame(&g_UartData[1],Addr, Cmd, Data, length);
    //PrintData(g_UartData[1].SendBuff ,(alt_u8)g_UartData[1].SendLength);
    Uart2Send(g_UartData[1].SendBuff,(u8)g_UartData[1].SendLength);    
    return WaitSensorAck(Addr, Cmd);
}

// 等待探头的应答
bool WaitSensorAck(alt_u8 Addr, alt_u8 Cmd)
{
    alt_u32 to = SENSOR_CMD_TIMEOUT/10;
    while(to--)
    {
        //printf("Uart2timer = %d\r\n",g_UartData[1].Timer);     
        
        if (g_UartData[1].Timer > 50)    //g_UartData[1].Timer//if (g_UartData[1].Timer > UART_DATA_TIMEOUT)
        {
            g_CrcFlag = 1;
            //DebugMsg("Recv Sensor cmd: Addr:%d - Len:%d \r\n", Addr, g_UartData[Addr].RecvLength);
            //DebugMsg("<<");
            PrintData(g_UartData[1].RecvBuff ,(alt_u8)g_UartData[1].RecvLength);
             //g_UartData[1].Timer = 0;
             
             //printf("Cmd = %x\r\n",Cmd);
             //printf("[CMD] = %x\r\n",g_UartData[1].RecvBuff[CMD]);
            if (ValidFrame(&g_UartData[1]))
            {
                if (Cmd == g_UartData[1].RecvBuff[CMD])
                {
//                    printf("成功\r\n");
                    return true; // 成功
                }
            }
        }
        
        Idle(20); // 20ms

        //DebugMsg("Wait : %d \r\n ", to);
    }

    //DebugMsg("Wait timeout, addr = %d \r\n ", Addr);

    return false;
}



bool HndPcFrame()
{
    bool ret = false;
    if (ValidFrame(&g_UartData[0]))
    {
        //printf("CMD_OK!\r\n");
        ret = HndPcCmd();
    }
    ClearRecvData(&g_UartData[0]);
    return ret;
}


bool Out4_20ma(BYTE index, BYTE val)
{
    WORD v = val*100;

    switch(index)
    { 
        case 1:  MCP4725_OutVol(MCP4725_AV_ADDR, v); break;
        case 2:  MCP4725_OutVol(MCP4725_BH_ADDR, v); break; 
        //case 1:  MCP4725_OutVol(MCP4725_BL_ADDR, v); break;
        //case 2:  MCP4725_OutVol(MCP4725_HV_ADDR, v); break; 
    }

    SendPcCmd(0, CMD_OUT_4_20MA, NULL, 0);
    return true;
}


bool Read4_20ma()
{
    BYTE ret = 0;
    int Voltage = 0;
    GetAds1110(I2C_4_20MA_IN, ADS110_4_20mA);

    ret = (BYTE)(Voltage/100);
    SendPcCmd(0,CMD_GET_4_20MA, &ret, 1);
    return true;
}


bool ReadBatVol()
{
    BAT_INFO bi;
    int Voltage = 0;
    GetAds1110(I2C_BAT_VOL, ADS110_BAT_VOL);

    bi.Vol = (WORD)Voltage;
    bi.Charging = (BAT_CHARGE() != 0);
    SendPcCmd(0,CMD_BATTERY, (BYTE *)&bi, 3);
    return true;
}


bool HndPcCmd()
{
    bool ret = false;
    FRAME_HEAD FrmHead;
    memcpy(&FrmHead, g_UartData[0].RecvBuff, sizeof(FRAME_HEAD));
    switch(FrmHead.Cmd)
    {
        // Dev cmd
        case CMD_SOUND:         ret = SoundCtl(g_UartData[0].RecvBuff[DAT]); break;
        case CMD_LED:           ret = LedCtl(&g_UartData[0].RecvBuff[DAT]);    break;
        case CMD_OUT_4_20MA:    ret = Out4_20ma(g_UartData[0].RecvBuff[DAT], g_UartData[0].RecvBuff[DAT+1]);    break;
        case CMD_GET_4_20MA:    ret = Read4_20ma();        break;
        case CMD_VERSION:       ret = DevVer(FrmHead.Addr);  break;
        case CMD_BATTERY:       ret = ReadBatVol();        break;
        case CMD_CHANNEL_ALMLIGHT:  ret =  ChannelAlmLightClt(&g_UartData[0].RecvBuff[DAT]);break;
        
        // Sensor cmd
        case CMD_DEV_CON:        ret = ConnectSensor(FrmHead.Addr);    break;
        case CMD_READ_DOSE:      ret = ReadDoseRate(FrmHead.Addr);     break;
        case CMD_READ_ALARM_PARA:     ret = ReadAlarmParam(FrmHead.Addr);    break;
        case CMD_WRITE_ALARM_PARA_B:  ret = WriteAlarmParam(FrmHead.Addr);    break;
        case CMD_READ_DETER_PARA_R:   ret = ReadSensorParam(FrmHead.Addr);   break;
        case CMD_WRITE_DETER_PARA_W:  ret = WriteSensorParam(FrmHead.Addr);   break;

        default: ret = FrameRevert(&FrmHead);   break;
    }

    return ret;
}

bool FrameRevert(FRAME_HEAD *fres)
{
    bool ret = false;
    if (SendSensorCmd(fres->Addr ,fres->Cmd , &g_UartData[0].RecvBuff[DAT], (alt_u8)(fres->Len-8)))
    {
        SendPcCmd(fres->Addr, fres->Cmd, &g_UartData[1].RecvBuff[DAT], (alt_u8)(g_UartData[1].RecvBuff[LEN]-8 ));
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    return ret; 
}

bool ConnectSensor(alt_u8 Addr)
{
    bool ret = false;
    if (SendSensorCmd(Addr,CMD_DEV_CON , NULL, 0))
    {
        SendPcCmd(Addr, CMD_DEV_CON, NULL, 0);
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    
    return ret;    
}

bool ReadDoseRate(alt_u8 Addr)
{
    bool ret = false;
    
    HOST_COUNT_PULSE HostDose;
    //SENSOR_DOSERATE  SensorDose;
    
    if (SendSensorCmd(Addr,CMD_READ_DOSE , NULL, 0))
    {
        #if 0
        memcpy(&SensorDose, &g_UartData[1].RecvBuff[DAT], sizeof(SENSOR_DOSERATE));
        HostDose.DOSE_RATE = SensorDose.DoseRate;
        HostDose.ACC_DOSE_RATE  = SensorDose.Dose;
        HostDose.ALARM_STATUS.ByteWhole  = SensorDose.State;
        #endif
        memcpy(&HostDose, &g_UartData[1].RecvBuff[DAT], sizeof(HOST_COUNT_PULSE));
        SendPcCmd(Addr, CMD_READ_DOSE, (alt_u8 *)&HostDose, sizeof(HOST_COUNT_PULSE));
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    return ret;    
}

bool ReadAlarmParam(alt_u8 Addr)
{
    bool ret = false;
    
    HOST_ALRAM_PARA HostAlarm;
    SENSOR_ALARM  SensorAlarm;
    
    if (SendSensorCmd(Addr,CMD_READ_ALARM_PARA , NULL, 0))
    {
        memset(&HostAlarm, 0, sizeof(HOST_ALRAM_PARA));
        memcpy(&SensorAlarm, &g_UartData[1].RecvBuff[DAT], sizeof(SENSOR_ALARM));
        HostAlarm.DOSE_RATE_ALARM_1 = SensorAlarm.DoseRatePreAlarm;
        HostAlarm.DOSE_RATE_ALARM_2 = SensorAlarm.DoseRateAlarm;
        HostAlarm.CUM_DOSE_RATE_ALARM_1 = SensorAlarm.DosePreAlarm;
        HostAlarm.CUM_DOSE_RATE_ALARM_2 = SensorAlarm.DoseAlarm;
        SendPcCmd(Addr, CMD_READ_ALARM_PARA, (alt_u8 *)&HostAlarm, sizeof(HOST_ALRAM_PARA));
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    return ret;    
}


bool WriteAlarmParam(alt_u8 Addr)
{
    bool ret = false;
    
    HOST_ALRAM_PARA HostAlarm;
    SENSOR_ALARM  SensorAlarm;

    memset(&SensorAlarm, 0, sizeof(SENSOR_ALARM));
    memcpy(&HostAlarm, &g_UartData[0].RecvBuff[DAT], sizeof(HOST_ALRAM_PARA));
    
    SensorAlarm.DoseRatePreAlarm = HostAlarm.DOSE_RATE_ALARM_1;
    SensorAlarm.DoseRateAlarm    = HostAlarm.DOSE_RATE_ALARM_2;
    SensorAlarm.DosePreAlarm     = HostAlarm.CUM_DOSE_RATE_ALARM_1;
    SensorAlarm.DoseAlarm        = HostAlarm.CUM_DOSE_RATE_ALARM_2;
        
    if (SendSensorCmd(Addr,CMD_WRITE_ALARM_PARA_B , (alt_u8 *)&SensorAlarm, sizeof(SENSOR_ALARM)))
    {
        SendPcCmd(Addr, CMD_WRITE_ALARM_PARA_B, NULL, 0);
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    
    return ret;
}


bool ReadSensorParam(alt_u8 Addr)
{
    bool ret = false;
    
    HOST_SENSOR_PARAM HostParam;
    SENSOR_PARAM      SensorParam;
    char temp[5] = {0};
    
    if (SendSensorCmd(Addr,CMD_READ_DETER_PARA_R , NULL, 0))
    {
        memset(&HostParam, 0, sizeof(HOST_SENSOR_PARAM));
        memcpy(&SensorParam, &g_UartData[1].RecvBuff[DAT], sizeof(SENSOR_PARAM));
        HostParam.LOW_REVISE_COE_A = SensorParam.Canshu1;
        HostParam.LOW_REVISE_COE_B = SensorParam.Canshu2;
        memcpy(temp, SensorParam.yuzhi1, 4);
        HostParam.DET_THR_1 = atoi(temp);
        memcpy(temp, SensorParam.yuzhi2, 4);
        HostParam.DET_THR_2 = atoi(temp);
        memcpy(temp, SensorParam.PingHuaShiJian, 4);
        HostParam.DET_TIME  = atoi(temp);
        SendPcCmd(Addr, CMD_READ_DETER_PARA_R, (alt_u8 *)&HostParam, sizeof(HOST_SENSOR_PARAM));
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    return ret;    
}



bool WriteSensorParam(alt_u8 Addr)
{
    bool ret = false;
    
    HOST_SENSOR_PARAM HostParam;
    SENSOR_PARAM      SensorParam;
    char temp[5] = {0};

    memset(&SensorParam, 0, sizeof(SENSOR_PARAM));
    memcpy(&HostParam, &g_UartData[0].RecvBuff[DAT], sizeof(HOST_SENSOR_PARAM));
    
    SensorParam.Canshu1 = HostParam.LOW_REVISE_COE_A;
    SensorParam.Canshu2 = HostParam.LOW_REVISE_COE_B;
    
    sprintf(temp, "%04u", (unsigned int)HostParam.DET_THR_1);
    memcpy(SensorParam.yuzhi1, temp, 4);

    sprintf(temp, "%04u", (unsigned int)HostParam.DET_THR_2);
    memcpy(SensorParam.yuzhi2, temp, 4);

    sprintf(temp, "%04u", (unsigned int)HostParam.DET_TIME);
    memcpy(SensorParam.PingHuaShiJian, temp, 4);
    
    if (SendSensorCmd(Addr,CMD_WRITE_DETER_PARA_W , (alt_u8 *)&SensorParam, sizeof(SENSOR_PARAM)))
    {
        SendPcCmd(Addr, CMD_WRITE_DETER_PARA_W, NULL, 0);
        ret = true;
    }
    ClearRecvData(&g_UartData[1]);
    return ret;   
}

bool SoundCtl(alt_u8 Ctl)
{
    if (Ctl == 0xAA)
    {
        //LEDM(1);      // 远程报警灯
        g_Output[ALARM_SOUND] = 2;
    }
    else
    {
        //LEDM(0);    
        g_Output[ALARM_SOUND] = 0;
    }
    SendPcCmd(0, CMD_SOUND, NULL, 0);
    return true;
}

bool LedCtl(alt_u8 *led)
{
    memcpy(g_Output, led, OUT_IO_COUNT-1);  // 报警音不在这里控制

    //根据报警灯控制继电器输出
    if (g_Output[LIGHT_YELLOW])
    {
        RELAY_3(1); 
    }
    else
    {
        RELAY_3(0); 

    }

    if (g_Output[LIGHT_RED])
    {
        RELAY_4(1); 
    }
    else
    {
        RELAY_4(0); 
    }

    SendPcCmd(0, CMD_LED, NULL, 0);
    return true;
}

bool ChannelAlmLightClt(alt_u8 *Light)
{
    memcpy(g_OutChannelLight, Light, OUT_Channel_COUNT); 
    if(g_OutChannelLight[LIGHT_OUT1])
    {
        ALMOUT_1(1);
    }
    else
    {
        ALMOUT_1(0);
    }

    if(g_OutChannelLight[LIGHT_OUT2])
    {
        ALMOUT_2(1);
    }
    else
    {
        ALMOUT_2(0);
    }

    if(g_OutChannelLight[LIGHT_OUT3])
    {
        ALMOUT_3(1);
    }
    else
    {
        ALMOUT_3(0);
    }
    if(g_OutChannelLight[LIGHT_OUT4])
    {
        ALMOUT_4(1);
    }
    else
    {
        ALMOUT_4(0);
    }
    
    SendPcCmd(0, CMD_CHANNEL_ALMLIGHT, NULL, 0);
    return true;

    
}

//bool DevVer(alt_u8 Addr)
//{
//    BYTE buf[7] = {0};
//
//    // 
//    memcpy(buf, VERSION, 6);
//    
//    SendPcCmd(Addr, CMD_VERSION, buf, 6);
//    return true;
//}

bool DevVer(alt_u8 Addr)
{
    char buf[7] = {0};

    if (Addr == 0)
    {
        memcpy(buf, VERSION, 6);
    }
    else
    {
        if (SendSensorCmd(Addr,CMD_VERSION , NULL, 0))
        {
            memcpy(buf, &g_UartData[1].RecvBuff[DAT], 6);
            ClearRecvData(&g_UartData[1]);
        }
        else
        {
            return false;
        }
    }
    SendPcCmd(Addr, CMD_VERSION, (alt_u8 *)buf, 6);
    return true;
}


#if 1
/************************************************************
*函数名：CheckSum
*功  能：计算校验和
*参  数：   alt_u8 *dataBuf    缓存接收的字符数组
*参  数：   alt_u16 len   命令长度
*返  回： 校验和
*报文长度、设备类型、通道号、指令、信息体的和，然后上半字节和下半字节各形成一个ASCII码
*作者：yaominggang
***************************************************************/
alt_u8 CheckSum(alt_u8 *dataBuf,alt_u16 len)
{
	 alt_u8 i=0;
	 alt_u8 lchecksum=0;
	 lchecksum = 0;
     for(i=1;i<len-3;i++)//不计算其实标志位，和两个字节校验和，以及结束符
     {
    	 lchecksum += dataBuf[i];
     }
     return lchecksum;
}

#endif

