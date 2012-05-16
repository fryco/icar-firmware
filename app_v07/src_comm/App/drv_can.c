#include "main.h"
#include "stm32f10x_can.h"

CAN_InitTypeDef        CAN_InitStructure;
CAN_FilterInitTypeDef  CAN_FilterInitStructure;
extern CanTxMsg TxMessage;
extern CanRxMsg RxMessage;
extern unsigned int rx_msg_cnt0, rx_msg_cnt1 ;

void can_init( )
{
	GPIO_InitTypeDef  GPIO_InitStructure;

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
	CAN_InitStructure.CAN_ABOM = ENABLE; //EN:���ߺ��Զ������ָ�����
	CAN_InitStructure.CAN_AWUM = DISABLE; //automatic wake-up mode
	//CAN_InitStructure.CAN_NART = DISABLE; //no-automatic retransmission mode
	CAN_InitStructure.CAN_NART = ENABLE; //EN:ֻ��һ�Σ����ܽ����DIS: �Զ��ش���ֱ���ɹ�
	//CAN_InitStructure.CAN_RFLM = DISABLE; //Receive FIFO Locked mode
	CAN_InitStructure.CAN_RFLM = ENABLE; //EN: ���ʱ�����±���, DIS: �����±���
	//CAN_InitStructure.CAN_TXFP = DISABLE; //transmit FIFO priority
	CAN_InitStructure.CAN_TXFP = ENABLE; //EN: ���ȼ��ɷ���˳�����, DIS�����ȼ��ɱ���ID����
	// CAN_Mode_Normal             ((uint8_t)0x00)  /*!< normal mode */
	// CAN_Mode_LoopBack           ((uint8_t)0x01)  /*!< loopback mode */
	// CAN_Mode_Silent             ((uint8_t)0x02)  /*!< silent mode */
	// CAN_Mode_Silent_LoopBack    ((uint8_t)0x03)  /*!< loopback combined with silent mode */
	CAN_InitStructure.CAN_Mode = CAN_Mode_Silent_LoopBack; //CAN_Mode_Normal;

	
	//Baud = 24M(APB1) / (Prescaler) / 8(TqCount)	 								
	CAN_InitStructure.CAN_SJW=CAN_SJW_2tq; //��ͬ����Ԫ
	CAN_InitStructure.CAN_BS1=CAN_BS1_3tq;//������ǰ
	CAN_InitStructure.CAN_BS2=CAN_BS2_4tq;//���õ��
	CAN_InitStructure.CAN_Prescaler = 12 ;//250K
	//CAN_InitStructure.CAN_Prescaler = 6  ;//500K

	CAN_Init(CAN1, &CAN_InitStructure);

	/* CAN filter init */
	//Filter number 0
	CAN_FilterInitStructure.CAN_FilterNumber = 0;
	
	//CAN_FilterMode: CAN_FilterMode_IdMask or CAN_FilterMode_IdList
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;

	//CAN_FilterScale: CAN_FilterScale_16bit or CAN_FilterScale_32bit
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;

	//0x0000 ~ 0xFFFF
	CAN_FilterInitStructure.CAN_FilterIdHigh = (0x7FD)<<5;//11 bits,�����
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
	TxMessage.StdId = 0x7FD;//11bit ���ٲ��򣬼���ʶ����Խ�����ȼ�Խ��, 0 to 0x7FF
	TxMessage.ExtId = 0x01; //��չ֡
	TxMessage.RTR = CAN_RTR_DATA;//Զ�̷�������λ��������֡������֡�����λΪ0��
								 //�����Զ��֡����Ϊ1��
	TxMessage.IDE = CAN_ID_STD;  //0 ��ʾ�����׼֡��IDE=1 ��ʾ����չ֡
	TxMessage.DLC = 1;//����֡���ֽ�����0~8��������Data Field���ĳ���

	/* Enable Interrupt for receive FIFO 0 message pending */
	CAN_ITConfig(CAN1,CAN_IT_FMP0, ENABLE);

	/* Enable Interrupt for receive FIFO 1 message pending */
	CAN_ITConfig(CAN1,CAN_IT_FMP1, ENABLE);	

}

/********************************************************************************
* CAN1 Transmit mailbox empty Interrupt function
* params: void 
* return: void
********************************************************************************/
void USB_HP_CAN1_TX_IRQHandler(void)
{
	prompt("TX\r\n");
}

/********************************************************************************
* CAN1 receive FIFO 0 message pending Interrupt
* params: void 
* return: void
********************************************************************************/
void USB_LP_CAN1_RX0_IRQHandler(void)
{	
	rx_msg_cnt0++;
	CAN_Receive(CAN1,CAN_FIFO0, &RxMessage);
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
}


/********************************************************************************
* CAN1 receive FIFO 1 message pending Interrupt
* params: void 
* return: void
********************************************************************************/
void CAN1_RX1_IRQHandler(void)
{
	rx_msg_cnt1++;
	CAN_Receive(CAN1,CAN_FIFO1, &RxMessage);
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP1);
}
