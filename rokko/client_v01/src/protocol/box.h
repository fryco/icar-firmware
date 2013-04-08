#ifndef __SETUP_H__
#define __SETUP_H__

#define CA_IO_Parameter                 0x6001
#define CA_ECU_COMMUNICATION_BAUD_RATE  0x6002
#define CA_ECU_COMM_TIME_INTERVAL       0x6003
#define CA_ECU_LINK_KEEP                0x6004
#define CA_ECU_commnication_model       0x6005
#define CA_LINK_KEEP_ENABLE_AND_DISABLE 0x6006
#define CA_IO_HI_LOW_Voltage            0x6007
#define CA_SET_CAN_Filter              	0x6008
#define CA_SET_CAN_FLOW_CTRL           	0x6009
#define CA_RESPOSE_STA           		0x600A
#define CA_RESPOSE_IO_STA           	0x600B
#define CA_AFC_ENABLE_AND_DISABLE      	0x600E
#define CA_SET_BUSY_MODE	           	0x600C

#define CA_RESPOSE_VERSION           	0x60C0
#define CA_RESPOSE_SN           		0x60C1
#define CA_RESPOSE_TIME_DATA           	0x60C2	
#define CA_RESPOSE_IO_ADC           	0x60C3	
#define CA_RESPOSE_BPS           		0x60C4	
#define CA_RESPOSE_5CONVERT8V			0x60E0
#define CA_RESPOSE_SELTEST_485			0x60E1
#define CA_RESPOSE_SELFTEST_CAN			0x60E2
#define CA_RESPOSE_TEST_RXD				0x60E3
#define CA_RESPOSE_TEST_TXDRXD3			0x60E4
#define CA_READ_FLASH_CODE              0x6101
#define CA_RECEIVE_DATA_FROM_ECU        0x6102
#define CA_DELAY_TIM_MS                 0x6103
#define CA_SEND_M_FRAME_DATA            0x6104
#define CA_SEND_5BPS_DATA               0x6105
#define CA_SET_COMM_FILTER              0x6106
#define CA_DELAY_RTC_MS                 0x6107
#define CA_DELAY_TIM_US                 0x6108
#define CA_QUICK_FLASHCODE              0x6109
#define CA_DELAY_TICK_MS                0x6110
#define CA_JMP_BOOT                     0x60C8
#define CA_JMP_MCU                      0x60C9
#define CA_RESPOSE_CPU_ID               0x60C5
#define CA_RESET_COMMBOX                0x60FF
#define CA_SET_WORRMODE                 0x60FE
#define CA_SEEK_BPS	 	                0x60FD

#define ResetIdleTime()  	    g_box.idle_tick=Clock()
#define IsExecKeepLink()	    (Clock()-g_box.idle_tick>g_box.link_time)
#define IsNewCommand()	   		queue_size(SQ_HOST)

#define SetLevel(io,pin,level)	 ((level)?(io->BSRR=(pin)):(io->BRR=(pin)))
#define GetLevel(io,pin)  		 (((io->IDR)&(pin))?1:0)
#define MCU_K_RECEIVE_SIGNAL    GetLevel(GPIOA,GPIO_Pin_3)

#define PB_NONE			0
#define PB_ODD			1
#define PB_EVEN			2

#define MODE_COMMBOX		0x00
#define MODE_EMULATOR		0x01
#define MODE_COLLECT		0x02

#define MAX_FRAME_SIZE		2048
typedef struct{ 
//通讯接口
	USART_TypeDef * usart;//当前通讯的USART
	u8 protocol;//协议
	u32 baudrate;//USART设置
	u8 databits;
//通讯时间
	u16 out_time;//P2
	u16 out_b2b;//P1
	u16 f2f_time;//P3
	u16 b2b_time;//P4
//链路保持
	u16 link_time;
	u8 link_flag;
	u8 link_buff[128];
//过滤参数
	u8 filter_mode;
	u8 filter_buff[128];
//自动流控
	u8 afc_flag;
	u8 afc_buff[32];
//发送数据前是否等待
	u8 IsWaitBeforeSendFrame;
//链路空闲开始时间
	u32 idle_tick;
//ReceiveOnly标志
	u8 recv_only;
//ECU负相应处理
	u16 busy_count;
	u8 busy_flag;
//数据接收缓存
	u8 frame_count;
	u16 frame_length;
	u8 frame_buff[MAX_FRAME_SIZE];
//下位机运行模式
	u8 run_mode;
//VWP IO定义
	GPIO_TypeDef *vpw_tx_io;
	u16 vpw_tx_pin;
	GPIO_TypeDef *vpw_rx_io;
	u16 vpw_rx_pin;
//PWM IO定义
	GPIO_TypeDef *pwm_tx_io;
	u16 pwm_tx_pin;
	GPIO_TypeDef *pwm_rx_io;
	u16 pwm_rx_pin;
//接口函数
	void (*fnInitCommPort)(void);
	void (*fnDisableLLine)(void);
	void (*fnEnableLLine)(void);
	u8 (*fnSetIoPort)(u8*,u16);						
}ST_BOX;

extern ST_BOX g_box;

void InitBox(void);
void InitCommPort(void);
u8 ResetCommbox(u8 *param,u16 len);

u8 SetProtocol(u8 *param,u16 len);
u8 SetCommPort(u8 *param,u16 len);
u8 SetBaudrate(u8 *param,u16 len);
u8 SetCanFilter(u8 *param,u16 len);
u8 SetCommFilter(u8 *param,u16 len);
u8 SetCommTime(u8 *param,u16 len);
u8 SetFlowControl(u8 *param,u16 len);
u8 SetBusyCount(u8 *param,u16 len);
u8 SetKeepLink(u8 *param,u16 len);
u8 FlashCode(u8 *param,u16 len);
u8 QuickFlashCode(u8 *param,u16 len);

u8 SetCommLevel(u8 *param,u16 len);
u8 SendReceive(u8 *param,u16 length);
u8 AddrCodeEnter(u8 *param,u16 len);
u8 AutoBpsBy55(u16 nTimeout);
u8 AutoSeekBps(u8 *param,u16 len);
u8 IsFilterFrame(u8 *pRecvFrame,int iLength);
u8 WaitDataFromEcu(void);

u8 SetIoPort(u8 *param,u16 len);
void DisableLLine(void);

u8 IsCanbus(void);
void ExecKeepLink(void);

void InitFrameBuffer(void);
void AddFrameToBuffer(u8 *buff,u16 len);

void SetVpwIoPin(
	GPIO_TypeDef *vpw_tx_io,
	u16 vpw_tx_pin,
	GPIO_TypeDef *vpw_rx_io,
	u16 vpw_rx_pin
);
void SetPwmIoPin(
	GPIO_TypeDef *pwm_tx_io,
	u16 pwm_tx_pin,
	GPIO_TypeDef *pwm_rx_io,
	u16 pwm_rx_pin
);
#endif
