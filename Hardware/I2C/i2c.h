#ifndef I2C_H_
#define I2C_H_

#include "time.h"

//extern BYTE P2DIR;

#define I2C_SEN_THRES      1 
#define I2C_4_20MA_IN    2 
#define I2C_BAT_VOL      2 

#define I2C_4_20MA_OUT   2

#define I2C1_DIRM1   P3M1
#define I2C1_DIRM0   P3M0

#define I2C1_DIR   P3
#define I2C1_OUT   P3
#define I2C1_IN    P3
#define I2C1_SDA   (1<<3) 
#define I2C1_SCL   (1<<2) 

#define I2C2_DIRM1   P4M1
#define I2C2_DIRM0   P4M0
#define I2C2_DIR   P4
#define I2C2_OUT   P4
#define I2C2_IN    P4
#define I2C2_SDA  (1<<5)
#define I2C2_SCL  (1<<6)


#define I2C_HIGH 1
#define I2C_LOW  0

#define I2C_ACK 0
#define I2C_NACK 1

#define I2C_DELAY 50


#define IODireOut(io_dir,io_mask)\
    io_dir##M1 &= ~(io_mask);\
    io_dir##M0 |= (io_mask)

         
#define IOWrite(io_out,io_mask,data_mask)\
    io_out = (io_out & ~(io_mask)) | ((data_mask) & (io_mask))
	

#define IODireIn(io_dir,io_mask)\
    io_dir##M1 |= (io_mask);\
    io_dir##M0 &= ~(io_mask)

    
#define IORead(io_in) (io_in)



#define I2C_Init(id) \
{   \
    I2C##id##_DIR |= (I2C##id##_SDA | I2C##id##_SCL);\
}

#define I2C_Start(id)  \
{ \
    IODireOut(I2C##id##_DIR, I2C##id##_SDA); \
	IOWrite(I2C##id##_OUT,I2C##id##_SDA,I2C##id##_SDA); \
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL); \
	delay_us(I2C_DELAY); \
	IOWrite(I2C##id##_OUT,I2C##id##_SDA,0);\
	delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
	delay_us(I2C_DELAY);\
} 

#define I2C_Stop(id)\
{\
    IODireOut(I2C##id##_DIR, I2C##id##_SDA); \
	IOWrite(I2C##id##_OUT,I2C##id##_SDA,0);	\
	delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
	delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SDA,I2C##id##_SDA);\
	delay_us(I2C_DELAY);\
}

#define I2C_Ack(id)\
{\
    IODireOut(I2C##id##_DIR, I2C##id##_SDA); \
    IOWrite(I2C##id##_OUT,I2C##id##_SDA,0);	\
    delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
    delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
    IOWrite(I2C##id##_OUT,I2C##id##_SDA,I2C##id##_SDA);\
}


#define I2C_NAck(id)\
{\
    IODireOut(I2C##id##_DIR, I2C##id##_SDA); \
    IOWrite(I2C##id##_OUT,I2C##id##_SDA,I2C##id##_SDA);	\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
    delay_us(I2C_DELAY);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
    IOWrite(I2C##id##_OUT,I2C##id##_SDA,0);\
}

#define I2C_ReadAck(id)\
{\
    WORD i = 0;\
    BYTE d = 0;\
    IODireIn(I2C##id##_DIR, I2C##id##_SDA);\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
    delay_us(I2C_DELAY);\
    while(++i<1000)\
    {\
        d = IORead(I2C##id##_IN);\
        if ((d & (I2C##id##_SDA)) == 0)\
        {\
			break;\
        }\
        delay_us(2);\
    }\
	IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
}

// 返回的数据存放在 BYTE  rd_data;
// 条用前还需要定义并赋值 char ack = I2C_ACK //or I2C_NACK 
#define I2C_Read_Byte(id)\
{\
    BYTE cnt=0;\
	BYTE io_data=0;\
	IODireIn(I2C##id##_DIR, I2C##id##_SDA);\
	for(cnt=0;cnt<8;cnt++)\
    {\
		IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
		delay_us(I2C_DELAY);\
		io_data=IORead(I2C##id##_IN);\
		rd_data<<=1;        \
		if(io_data & (I2C##id##_SDA))\
	   	    rd_data|=1;\
        IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
		delay_us(I2C_DELAY);\
    }\
    if (ack == I2C_ACK)\
    {\
        I2C_Ack(id);\
    }\
    else\
    {\
        I2C_NAck(id);\
    }\
}

// 输出的数据存放在：BYTE out_data;
#define I2C_Write_Byte(id)\
{\
    unsigned char cnt;\
    IODireOut(I2C##id##_DIR, I2C##id##_SDA); \
	for(cnt=0;cnt<8;cnt++)\
	{\
		if(out_data&(0x80>>cnt))\
			IOWrite(I2C##id##_OUT,I2C##id##_SDA,I2C##id##_SDA);\
		else\
			IOWrite(I2C##id##_OUT,I2C##id##_SDA,0);\
		IOWrite(I2C##id##_OUT,I2C##id##_SCL,I2C##id##_SCL);\
  	  	delay_us(I2C_DELAY);\
  	  	IOWrite(I2C##id##_OUT,I2C##id##_SCL,0);\
		delay_us(I2C_DELAY);\
	}\
}



#endif
