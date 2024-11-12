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



// 51��Ƭ���Ǵ�˵ģ�ͨ���ṹ�巢�͵�����Ҫת��ΪС��
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
    
    //printf("FrmHead.Len = %d\r\n",FrmHead.Len);

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
    
    sprintf((char *)tmp, "%02X",lcrc);

    if ( (memcmp(tmp, &pUartData->RecvBuff[pUartData->RecvLength-3], 2) != 0) )
    {
        return false;
    }
    
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

    lcrc = CheckSum(pUartData->SendBuff,FrmHead.Len);//����У���
    //printf("Send_CRC = %x\r\n",lcrc);
    //��У���ת��Ϊ�����ֽڵ�ASCII
    //memcpy(&pUartData->SendBuff[FrmHead.Len-3],(BYTE*)&lcrc,2);
    sprintf((char *)&pUartData->SendBuff[FrmHead.Len-3],"%02X",lcrc);
    //printf("Send_Buff[5] = %x\r\n",pUartData->SendBuff[FrmHead.Len-3]);
    //printf("Send_Buff[6] = %x\r\n",pUartData->SendBuff[FrmHead.Len-2]);
    pUartData->SendBuff[FrmHead.Len-1] = TAIL;   
    pUartData->SendLength = FrmHead.Len;
}


void SendPcCmd(alt_u8 Addr, alt_u8 Cmd, alt_u8 *dat, alt_u8 length)
{
    //g_CrcFlag = 0;
    MakeFrame(&g_UartData[0], Addr, Cmd, dat, length);
    //DebugMsg("PC<:");
    //PrintData(g_UartData[0].SendBuff,g_UartData[0].SendLength);
    Uart1Send(g_UartData[0].SendBuff,(alt_u8)g_UartData[0].SendLength);

    g_CommIdleTime = 0;
}

bool SendSensorCmd(alt_u8 Addr, alt_u8 Cmd, alt_u8 *Data, alt_u8 length)
{

    MakeFrame(&g_UartData[1],Addr, Cmd, Data, length);
    Uart2Send(g_UartData[1].SendBuff,(u8)g_UartData[1].SendLength);    
    return WaitSensorAck(Addr, Cmd);
}

// �ȴ�̽ͷ��Ӧ��
bool WaitSensorAck(alt_u8 Addr, alt_u8 Cmd)
{
    u8 i = 0;
    alt_u32 to = SENSOR_CMD_TIMEOUT/10;
    while(to--)
    { 
        
        if (g_UartData[1].Timer > UART_DATA_TIMEOUT)
        {
            //DebugMsg("Recv Sensor cmd: Addr:%d - Len:%d \r\n", Addr, g_UartData[Addr].RecvLength);
            //DebugMsg("<<");
            //PrintData(g_UartData[1].RecvBuff ,(alt_u8)g_UartData[1].RecvLength);
             
            if (ValidFrame(&g_UartData[1]))
            {
                if (Cmd == g_UartData[1].RecvBuff[CMD])
                {
                    return true; // �ɹ�
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
    bi.Vol = WordToSmall(bi.Vol);
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
        //case CMD_CHANNEL_ALMLIGHT:  ret =  ChannelAlmLightClt(&g_UartData[0].RecvBuff[DAT]);break;
        
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
        ret = true;
        SendPcCmd(fres->Addr, fres->Cmd, &g_UartData[1].RecvBuff[DAT], (alt_u8)(g_UartData[1].RecvBuff[LEN]-8 ));
    
    }
    ClearRecvData(&g_UartData[1]);
    return ret; 
}

bool ConnectSensor(alt_u8 Addr)
{
    bool ret = false;
    if (SendSensorCmd(Addr,CMD_DEV_CON , NULL, 0))
    {
        ret = true;
        SendPcCmd(Addr, CMD_DEV_CON, NULL, 0);
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

        ret = true;
        SendPcCmd(Addr, CMD_READ_DOSE, (alt_u8 *)&HostDose, sizeof(HOST_COUNT_PULSE));
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

        ret = true;
        SendPcCmd(Addr, CMD_READ_ALARM_PARA, (alt_u8 *)&HostAlarm, sizeof(HOST_ALRAM_PARA));
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
        //HostParam.LOW_REVISE_COE_A = FloatToSmall(HostParam.LOW_REVISE_COE_A);
        HostParam.LOW_REVISE_COE_B = SensorParam.Canshu2;
        //HostParam.LOW_REVISE_COE_B = FloatToSmall(HostParam.LOW_REVISE_COE_B);
        memcpy(temp, SensorParam.yuzhi1, 4);
        HostParam.DET_THR_1 = atoi(temp);
        HostParam.DET_THR_1 = DwordToSmall(HostParam.DET_THR_1);
        memcpy(temp, SensorParam.yuzhi2, 4);
        HostParam.DET_THR_2 = atoi(temp);
        HostParam.DET_THR_2 = DwordToSmall(HostParam.DET_THR_2);
        memcpy(temp, SensorParam.PingHuaShiJian, 4);
        HostParam.DET_TIME  = atoi(temp);
        HostParam.DET_TIME = DwordToSmall(HostParam.DET_TIME);
        //printf("THR_1 = %d\r\n", HostParam.DET_THR_1);
        //printf("THR_2 = %d\r\n", HostParam.DET_THR_2);

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
    HostParam.DET_THR_1 = DwordToSmall(HostParam.DET_THR_1);
    sprintf(temp, "%04u", (unsigned int)HostParam.DET_THR_1);
    memcpy(SensorParam.yuzhi1, temp, 4);
    
    HostParam.DET_THR_2 = DwordToSmall(HostParam.DET_THR_2);
    sprintf(temp, "%04u", (unsigned int)HostParam.DET_THR_2);
    memcpy(SensorParam.yuzhi2, temp, 4);
    
    HostParam.DET_TIME = DwordToSmall(HostParam.DET_TIME);
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
        RELAY_1(1);      // Զ�̱�����
        g_Output[ALARM_SOUND] = 2;
    }
    else
    {
        RELAY_1(0);    
        g_Output[ALARM_SOUND] = 0;
    }
    SendPcCmd(0, CMD_SOUND, NULL, 0);
    return true;
}

bool LedCtl(alt_u8 *led)
{
    memcpy(g_Output, led, OUT_IO_COUNT-1);  // �����������������

    //���ݱ����ƿ��Ƽ̵������
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
*��������CheckSum
*��  �ܣ�����У���
*��  ����   alt_u8 *dataBuf    ������յ��ַ�����
*��  ����   alt_u16 len   �����
*��  �أ� У���
*���ĳ��ȡ��豸���͡�ͨ���š�ָ���Ϣ��ĺͣ�Ȼ���ϰ��ֽں��°��ֽڸ��γ�һ��ASCII��
*���ߣ�yaominggang
***************************************************************/
alt_u8 CheckSum(alt_u8 *dataBuf,alt_u16 len)
{
	 alt_u8 i=0;
	 alt_u8 lchecksum=0;
	 lchecksum = 0;
     for(i=1;i<len-3;i++)//��������ʵ��־λ���������ֽ�У��ͣ��Լ�������
     {
    	 lchecksum += dataBuf[i];
     }
     return lchecksum;
}

#endif

