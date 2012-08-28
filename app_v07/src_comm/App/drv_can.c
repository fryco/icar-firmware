#include "main.h"
#include "stm32f10x_can.h"

extern struct ICAR_DEVICE my_icar;
extern OS_EVENT 	*sem_obd	;

extern CanTxMsg TxMessage;
//extern CanRxMsg RxMessage;
//extern unsigned int rx_msg_cnt0, rx_msg_cnt1 ;

//warn code define, internal use only
#define	FIFO0_OF			01	//FIFO0 over flow
#define	UNK_CAN_STDID		02	//unknow CAN STD ID
#define	UNK_CAN_EXTID_H		03	//Unknow ext-stand CAN ID High
#define	UNK_CAN_EXTID_L		04	//Unknow ext-stand CAN ID low

void can_init( )
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	// GPIO clock enable, had been init in uart1_init
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA , ENABLE);
	
	/* Configure CAN pin: RX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* Configure CAN pin: TX */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	/* CANx Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

	/* CAN register init */
	CAN_DeInit(CAN1);
	CAN_StructInit(&CAN_InitStructure);

	/* CAN cell init */
	CAN_InitStructure.CAN_TTCM = DISABLE; //time triggered communication mode
	//CAN_InitStructure.CAN_ABOM = DISABLE; //automatic bus-off management
	CAN_InitStructure.CAN_ABOM = ENABLE; //EN:离线后，自动开启恢复过程
	CAN_InitStructure.CAN_AWUM = DISABLE; //automatic wake-up mode
	//CAN_InitStructure.CAN_NART = DISABLE; //no-automatic retransmission mode
	CAN_InitStructure.CAN_NART = ENABLE; //EN:只发一次，不管结果；DIS: 自动重传，直到成功
	//CAN_InitStructure.CAN_RFLM = DISABLE; //Receive FIFO Locked mode
	CAN_InitStructure.CAN_RFLM = ENABLE; //EN: 溢出时丢弃新报文, DIS: 保留新报文
	//CAN_InitStructure.CAN_TXFP = DISABLE; //transmit FIFO priority
	CAN_InitStructure.CAN_TXFP = ENABLE; //EN: 优先级由发送顺序决定, DIS：优先级由报文ID决定
	// CAN_Mode_Normal             ((uint8_t)0x00)  /*!< normal mode */
	// CAN_Mode_LoopBack           ((uint8_t)0x01)  /*!< loopback mode */
	// CAN_Mode_Silent             ((uint8_t)0x02)  /*!< silent mode */
	// CAN_Mode_Silent_LoopBack    ((uint8_t)0x03)  /*!< loopback combined with silent mode */
	//CAN_InitStructure.CAN_Mode = CAN_Mode_Silent_LoopBack; //CAN work mode
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal; //CAN work mode

	
	//Baud = 24M(APB1) / (Prescaler) / 8(TqCount)	 								
	CAN_InitStructure.CAN_SJW=CAN_SJW_2tq; //重同步单元
	CAN_InitStructure.CAN_BS1=CAN_BS1_3tq;//采样点前
	CAN_InitStructure.CAN_BS2=CAN_BS2_4tq;//采用点后
	CAN_InitStructure.CAN_Prescaler = 12 ;//250K
	//CAN_InitStructure.CAN_Prescaler = 6  ;//500K

	CAN_Init(CAN1, &CAN_InitStructure);

	/* CAN filter init */
	//1, 共有14组过滤器(0~13),每组有2个32位寄存器：CAN_FxR0,CAN_FxR1
	//2, CAN_FMR的FBMx位，设置过滤器工作在屏蔽模式(0)或列表模式(1)
	//3, CAN_FilterFIFOAssignment 决定报文存去FIFO0 或 FIFO1
	//4, 目前策略：已知的ID，通过屏蔽模式列出，保存到FIFO0
	//   未知的ID，全部保存到 FIFO1，将来上传到服务器上供分析

	//Filter number 0
	CAN_FilterInitStructure.CAN_FilterNumber = 0;
	
	//CAN_FilterMode: CAN_FilterMode_IdMask or CAN_FilterMode_IdList
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;

	//CAN_FilterScale: CAN_FilterScale_16bit or CAN_FilterScale_32bit
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;

	//0x0000 ~ 0xFFFF
	CAN_FilterInitStructure.CAN_FilterIdHigh = (0x7FD)<<5;//11 bits,左对齐
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x7FF0;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;

	//FIFOAssignment: 0 or 1
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

	//Filter number 1
	CAN_FilterInitStructure.CAN_FilterNumber = 1;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 1;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

	/* Transmit */
	TxMessage.StdId = 0x7FE;//11bit 的仲裁域，即标识符，越低优先级越高, 0 to 0x7FF
	TxMessage.ExtId = 0x19ABCDEF; //扩展帧
	TxMessage.RTR = CAN_RTR_DATA;//远程发送请求位。如果这个帧是数据帧，则该位为0，
								 //如果是远程帧，则为1。
	TxMessage.IDE = CAN_ID_STD;  //0 表示这个标准帧；IDE=1 表示是扩展帧
	//TxMessage.IDE = CAN_ID_EXT;  //0 表示这个标准帧；IDE=1 表示是扩展帧
	
	TxMessage.DLC = 1;//数据帧的字节数，0~8，数据域（Data Field）的长度

	/* Enable Interrupt for receive FIFO 0 and FIFO overflow */
	CAN_ITConfig(CAN1,CAN_IT_FMP0 | CAN_IT_FOV0, ENABLE);

	/* Enable Interrupt for receive FIFO 1 and FIFO overflow */
	CAN_ITConfig(CAN1,CAN_IT_FMP1 | CAN_IT_FOV1, ENABLE);	

	//Enable CAN transceiver power
	CAN_PM_ON;
}

/********************************************************************************
* CAN1 Transmit mailbox empty Interrupt function
* params: void 
* return: void
********************************************************************************/
void USB_HP_CAN1_TX_IRQHandler(void)
{
	;//HW bug, 不会产生中断，但可正常发送
}

/********************************************************************************
* CAN1 receive FIFO 0 message pending Interrupt
* params: void 
* return: void
********************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void) //for know CAN ID
{	
	unsigned char var_uchar;

	if( CAN_GetITStatus(CAN1, CAN_IT_FOV0 ) != RESET) { //FIFO0 overflow
		//Release FIFO0, will lose data
		CAN1->RF0R |= CAN_RF0R_RFOM0;

		//report this error:  1,//find a empty record
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= FIFO0_OF << 16 ;//FIFO0 overflow
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}//8/20/2012 7:11:32 PM 验证成功
	}
	else { //receive a data from FIFO0
		//不应在此次接收FIFO，因为有3个buffer可用，
		//改在任务里接收，可充分利用buffer
		//rx_msg_cnt0++;
		CAN_ITConfig(CAN1,CAN_IT_FMP0, DISABLE);//disable FIFO0 FMP int

		//Send semaphore: 在任务中接收FIFO
		OSIntEnter(); OSSemPost( sem_obd ); OSIntExit();
		//CAN_Receive(CAN1,CAN_FIFO0, &RxMessage);
		//CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	}
}


/********************************************************************************
* CAN1 receive FIFO 1 message pending Interrupt
* params: void 
* return: void
********************************************************************************/
void CAN1_RX1_IRQHandler(void) //for unknow CAN ID
{	
	unsigned char var_uchar;
	CanRxMsg rx_msg;

	CAN_Receive(CAN1,CAN_FIFO1, &rx_msg);

	if (rx_msg.IDE == CAN_ID_STD) { 		
		//unknow CAN STD ID, report to server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_STDID << 16 ;//unknow CAN STD ID
				my_icar.warn[var_uchar].msg |= rx_msg.StdId ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功
	}
	else {
		//unknow CAN EXT ID, report to server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_H << 16 ;//Unknow ext-stand CAN ID high
				my_icar.warn[var_uchar].msg |= ( rx_msg.ExtId >> 16) &0xFFFF ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功 2012/8/28 10:58:10

		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_L << 16 ;//Unknow ext-stand CAN ID low
				my_icar.warn[var_uchar].msg |= ( rx_msg.ExtId ) &0xFFFF ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功 2012/8/28 10:58:08
	}
}

