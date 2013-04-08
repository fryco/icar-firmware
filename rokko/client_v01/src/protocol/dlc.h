#ifndef __DLC_H__
#define __DLC_H__

//-----------------------------------------------------------------
#define CA_LINE_IO0     0x9
#define CA_LINE_IO1     0xf
#define CA_LINE_IO2     0xe
#define CA_LINE_IO3		0x4
#define CA_LINE_IO4     0x5
#define CA_LINE_IO5     0x7
#define CA_LINE_IO6     0x6
#define CA_LINE_IO7     0x2
#define CA_LINE_IO8     0x3
#define CA_LINE_IO9     0x1
#define CA_LINE_IO10	0x8
#define CA_LINE_IO11	0x0B
#define CA_LINE_IO12	0x0C
#define CA_LINE_IO13	0x0A
//------------------------------------------------------------------
#define OP_510		0x0000
#define OP_4K7		0x0002
#define OP_1K		0x0001
#define OP_10K		0x0003
#define Receive_P	0x0000
#define Receive_N	0x0004
#define POSITIVE    0x0008
#define NEGATIVE    0x0000

#define L_ENABLE    0x0000
#define L_DISABLE   0x0010
#define L_PULL_NONE	0x0000
#define L_PULL_DOWN	0x0020


#define RECEIVE_K    0x0000
#define RECEIVE_L    0x0040

#define VOLT_5      0x0000
#define VOLT_12     0x0100
#define VOLT_24     0x0200


#define CLK_ENABLE	0x0000
#define CLK_DISABLE	0x0400
#define ENABLE_K_LLINE	0x0000
#define ENABLE_CANBUS 0x0800

#define SEND_K_DATA_N 0x0000
#define SEND_K_DATA_P 0x2000

#define SEND_L_DATA_PK 0x4000
#define SEND_L_DATA_NK 0x0000

#define ENABLE_IO89  0x8000
#define DISABLE_IO89  0x0000

#define DTS_ENABLE	 0x0080
#define DTS_DISABLE	 0x0000

//---------------------------------------------------------------
#define HW_VCI2		0x66
#define HW_PS100	0x00
#define HW_VAG401	0x00
#define HW_PS150	0x01
#define HW_PS701	0x01
#define HW_MPS2		0x02
#define HW_PS201	0x03
#define HW_T605		0x04
#define HW_X100		0x05
#define HW_X200		0x05

//---------------------------------------------------------------
void VCI2_ConfigGpio(void);
void VCI2_InitCommPort(void);
void VCI2_DisableLLine(void);
void VCI2_EnableLLine(void);
u8 VCI2_SetIoPort(u8 *param,u16 len);

void PS100_ConfigGpio(void);
void PS100_InitCommPort(void);
void PS100_DisableLLine(void);
void PS100_EnableLLine(void);
u8 PS100_SetIoPort(u8 *param,u16 len);

void PS150_ConfigGpio(void);
void PS150_InitCommPort(void);
void PS150_DisableLLine(void);
void PS150_EnableLLine(void);
u8 PS150_SetIoPort(u8 *param,u16 len);

void MPS2_ConfigGpio(void);
void MPS2_InitCommPort(void);
void MPS2_DisableLLine(void);
void MPS2_EnableLLine(void);
u8 MPS2_SetIoPort(u8 *param,u16 len);

void PS201_ConfigGpio(void);
void PS201_InitCommPort(void);
void PS201_DisableLLine(void);
void VCI2_EnableLLine(void);
u8 PS201_SetIoPort(u8 *param,u16 len);

void T605_ConfigGpio(void);
void T605_InitCommPort(void);
void T605_DisableLLine(void);
void T605_EnableLLine(void);
u8 T605_SetIoPort(u8 *param,u16 len);

#endif

