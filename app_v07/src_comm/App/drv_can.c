#include "main.h"
#include "stm32f10x_can.h"

extern struct ICAR_DEVICE my_icar;
extern const unsigned int can_snd_id[];

extern u8 obd_support_pid[OBD_SUPPORT_PID_CNT];

extern OS_EVENT 	*sem_obd	;

//warn code define, internal use only
#define	FIFO0_OF			10	//FIFO0 over flow
#define	FIFO1_OF			20	//FIFO0 over flow
#define	ENQUIRE_PID_SEM		30	//enquire can support PID error, OSSem
#define	ENQUIRE_PID_DAT		40	//enquire can support PID error, no rec data

/********************************************************************************
* CAN1 Transmit mailbox empty Interrupt function
* params: void 
* return: void
********************************************************************************/
void USB_HP_CAN1_TX_IRQHandler(void)
{
	;//HW bug, ��������жϣ�������������
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
		}//8/20/2012 7:11:32 PM ��֤�ɹ�
	}
	else { //receive a data from FIFO0
		//��Ӧ�ڴ˴ν���FIFO����Ϊ��3��buffer���ã�
		//������������գ��ɳ������buffer
		//rx_msg_cnt0++;
		CAN_ITConfig(CAN1,CAN_IT_FMP0, DISABLE);//disable FIFO0 FMP int

		//Send semaphore: �������н���FIFO
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

	if( CAN_GetITStatus(CAN1, CAN_IT_FOV1 ) != RESET) { //FIFO0 overflow
		//Release FIFO0, will lose data
		CAN1->RF1R |= CAN_RF1R_RFOM1;

		//report this error:  1,//find a empty record
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= FIFO1_OF << 16 ;//FIFO1 overflow
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}//8/20/2012 7:11:32 PM ��֤�ɹ�
	}
	else { //receive a data from FIFO0
		//��Ӧ�ڴ˴ν���FIFO����Ϊ��3��buffer���ã�
		//������������գ��ɳ������buffer
		//rx_msg_cnt0++;
		CAN_ITConfig(CAN1,CAN_IT_FMP1, DISABLE);//disable FIFO1 FMP int

		//Send semaphore: �������н���FIFO
		OSIntEnter(); OSSemPost( sem_obd ); OSIntExit();
		//CAN_Receive(CAN1,CAN_FIFO0, &RxMessage);
		//CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	}
}

//Add rule to empty filter, return filter_id + 1, err: return 0
u8 can_add_filter( can_std_typedef can_typ, u32 can_id )
{
	u8 i;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	
	// Try to find a new idle filter
	for(i = 0; i < 13; i++){ //filter 13 is for unknow CAN ID
		//debug_obd("Filter:%d\t%04X\r\n",i,CAN1->FA1R);
		if((CAN1->FA1R & (0x00000001<<i)) == 0){	//found a idle filter

			/* CAN filter init  */
			//1, ����14�������(0~13),ÿ����2��32λ�Ĵ�����CAN_FxR0,CAN_FxR1
			//2, CAN_FMR��FBMxλ�����ù���������������ģʽ(0)���б�ģʽ(1)
			//3, CAN_FilterFIFOAssignment �������Ĵ�ȥFIFO0 �� FIFO1
			//4, Ŀǰ���ԣ���֪��ID��ͨ������ģʽ�г������浽FIFO0
			//   δ֪��ID��ȫ�����浽 FIFO1�������ϴ����������Ϲ�����
		
			//Filter number
			CAN_FilterInitStructure.CAN_FilterNumber = i;
			
			//CAN_FilterMode: CAN_FilterMode_IdMask or CAN_FilterMode_IdList
			CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
		
			//CAN_FilterScale: CAN_FilterScale_16bit or CAN_FilterScale_32bit
			CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
		
			//���Filter=0xF0�� Mask=0x00����ô�����յ�0xF0-0xFF��16��ID����Ϣ��
			//����Filter=0xF0�� Mask=0xFF����ôֻ���յ�0xF0��һ��ID��Ϣ��
			//����Filter=0xF0�� Mask=0xF8����ô�����յ�0xF8-0xFF��8��ID����Ϣ��
			//����Filter=0xF0�� Mask=0xFC����ô�����յ�0xFC-0xFF��4��ID����Ϣ��
			//�� Mask��Ӧλ=1ʱ�����յ�����ϢID��Ӧλ����=Filter�Ķ�Ӧλ����ȷ�ϴ���
		    //   Mask��Ӧλ=0ʱ��Filter��Ӧλ=1ʱ�����յ�����ϢID��Ӧλ����=1����ȷ�ϴ���
		    //   Mask��Ӧλ=0ʱ��Filter��Ӧλ=0ʱ�����յ�����ϢID��Ӧλ=0��=1����ȷ�ϴ���
			if ( can_typ == CAN_EXT ) { //2012/10/8 15:33:42 ��֤

				can_id = (can_id & 0x1FFFFFFF)  ;
				debug_obd("F_ID: %08X\r\n",can_id);

				CAN_FilterInitStructure.CAN_FilterIdHigh = (can_id>>13)&0xFFFF;//29 bits,�����
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFFF;//����λ����ƥ��

				CAN_FilterInitStructure.CAN_FilterIdLow = ((can_id << 3) & 0xFFFF) | 0x04;//In EXT, IDE=1, bit 2
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFFD;//Don't care bit 1: RTR

			}
			else {//2012/9/10 17:15:49 ����֤, 0x07E8
				//In EXT, IDE=0

				can_id = can_id & 0x07FF ;
				CAN_FilterInitStructure.CAN_FilterIdHigh = (can_id)<<5;//11 bits,�����
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFFFF;

				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0xFFFD;//Don't care bit 1: RTR
			}
			
			//FIFOAssignment: 0 or 1
			CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
			CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
			CAN_FilterInit(&CAN_FilterInitStructure);
			
			return i+1;
		}
	}
	return 0 ;//no idle filter
}

void can_rec_all_id( bool en )
{
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	
	//Last Filter number ==> save all unknow ID to FIFO 1
	CAN_FilterInitStructure.CAN_FilterNumber = 13;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 1;
	if ( en ) { CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;}
	else {	CAN_FilterInitStructure.CAN_FilterActivation = DISABLE; }
	CAN_FilterInit(&CAN_FilterInitStructure);
}

void can_init( can_speed_typedef can_spd,  can_std_typedef can_typ )
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	CAN_InitTypeDef        CAN_InitStructure;

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

	CAN_InitStructure.CAN_NART = DISABLE; //no-automatic retransmission mode
	//���ֻ��һ�Σ������߳�ͻʱ���ᷢ��ʧ�ܣ����ڽ��ճ�����������
	//CAN_InitStructure.CAN_NART = ENABLE; //EN:ֻ��һ�Σ����ܽ����DIS: �Զ��ش���ֱ���ɹ�

	//CAN_InitStructure.CAN_RFLM = DISABLE; //Receive FIFO Locked mode
	CAN_InitStructure.CAN_RFLM = ENABLE; //EN: ���ʱ�����±���, DIS: �����±���
	//CAN_InitStructure.CAN_TXFP = DISABLE; //transmit FIFO priority
	CAN_InitStructure.CAN_TXFP = ENABLE; //EN: ���ȼ��ɷ���˳�����, DIS�����ȼ��ɱ���ID����
	// CAN_Mode_Normal             ((uint8_t)0x00)  /*!< normal mode */
	// CAN_Mode_LoopBack           ((uint8_t)0x01)  /*!< loopback mode */
	// CAN_Mode_Silent             ((uint8_t)0x02)  /*!< silent mode */
	// CAN_Mode_Silent_LoopBack    ((uint8_t)0x03)  /*!< loopback combined with silent mode */
	//CAN_InitStructure.CAN_Mode = CAN_Mode_Silent_LoopBack; //����ʱ��
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal; //CAN work mode

	
	//Baud = 24M(APB1) / (Prescaler) / 8(TqCount)	 								
	CAN_InitStructure.CAN_SJW=CAN_SJW_2tq; //��ͬ����Ԫ
	CAN_InitStructure.CAN_BS1=CAN_BS1_3tq;//������ǰ
	CAN_InitStructure.CAN_BS2=CAN_BS2_4tq;//���õ��

	if ( can_spd == CAN_500K ) {
		CAN_InitStructure.CAN_Prescaler = 6  ;//500K
	}
	else {
		CAN_InitStructure.CAN_Prescaler = 12 ;//250K
	}

	CAN_Init(CAN1, &CAN_InitStructure);

	//Enable CAN transceiver power
	CAN_PM_ON;
}

bool can_send( u32 can_id, frame_typedef frame_typ, u8 dat_len, u8 * dat )
{
	u8 i , var_uchar ;
	CanTxMsg can_tx_msg;

	if ( can_id > 0x7FF ) {//CAN EXT

		can_tx_msg.ExtId = can_id; //��չ֡
		can_tx_msg.IDE = CAN_ID_EXT;  //0 ��ʾ�����׼֡��IDE=1 ��ʾ����չ֡
	}
	else {
		can_tx_msg.StdId = can_id; //11bit ���ٲ��򣬼���ʶ����Խ�����ȼ�Խ��, 0 to 0x7FF
		can_tx_msg.IDE = CAN_ID_STD;  //0 ��ʾ�����׼֡��IDE=1 ��ʾ����չ֡
	}

	if ( frame_typ == DAT_FRAME ) {
		can_tx_msg.RTR = CAN_RTR_DATA;//������֡������֡��((uint32_t)0x00000000)  /*!< Data frame */
	}
	else {
		can_tx_msg.RTR = CAN_RTR_REMOTE;//((uint32_t)0x00000002)  /*!< Remote frame */
	}
	
	can_tx_msg.DLC = dat_len;//����֡���ֽ�����0~8��������Data Field���ĳ���

	/* fill data to ring buffer */
	for(i = 0; i < dat_len; i++){
		can_tx_msg.Data[i]  =dat[i];
	}

	/* fill 0x00 for free bytes*/
	for(i = dat_len; i < 8; i++){
		can_tx_msg.Data[i]=0;
	}

	i = 5 ; //retry 5
	while ( i ) {
		//return transmit_mailbox; if transmit_mailbox == CAN_NO_MB ==>ERR
		if ( CAN_Transmit(CAN1, &can_tx_msg) == CAN_NO_MB ) { //no mail box
			OSTimeDlyHMSM(0, 0,	0, 100);//wait 100 ms
			i--;
		}
		else {//end this loop
			i = 0 ;
			debug_obd("CAN send:");
			for ( var_uchar = 0 ; var_uchar < dat_len ; var_uchar++ ) {
				printf(" %02X", can_tx_msg.Data[var_uchar]);
			}
			printf(" to %X OK\r\n", can_id);
			return true;
		}
	}
	debug_obd("CAN send to %X ERR\r\n", can_id);
	return false;
}

bool can_receive( u8 * buffer, u16 bufsize, u16 * rec_len, u32 * rec_id )
{

	u32 cur_time = OSTime ;
	CanRxMsg can_rx_msg;
	
	if ( bufsize < 8 ) return false;
		
	while ( OSTime - my_icar.mg323.try_online_time < 3*OS_TICKS_PER_SEC ) {
		if ( (CAN1->RF0R)&0x03 ) { //Rec ECU data 
			debug_obd("CAN FIFO_0: %d\r\n", (CAN1->RF0R)&0x03);
			
			CAN_Receive(CAN1,CAN_FIFO0, &can_rx_msg);

			*rec_len = can_rx_msg.Data[0]&0x0F ;
			memcpy(buffer, &can_rx_msg.Data[1],*rec_len);
			if (can_rx_msg.IDE == CAN_ID_STD) { //CAN STD ID
				*rec_id = can_rx_msg.StdId;
			}
			else { //CAN EXT ID
				*rec_id = can_rx_msg.ExtId;
			}
			
			return true;
		}
		else {
			OSTimeDlyHMSM(0, 0, 0, 50);
		}
	}
	debug_obd("Timeout!%s,%d\r\n",__FILE__,__LINE__);
	return false;
}

bool can_enquire_support_pid( void )
{
	//Request Supported PIDs from Vehicle, check sae j1979 2002.pdf page 26
	u8 determine_pid[]="\x02\x01\x00\x00\x00\x00\x00\x00";

	u8 var_uchar ;
	u16 rec_len;
	u32 can_id ;

	debug_obd("Start %s \r\n",__FUNCTION__);

	//Clean all FIFO_0	
	while ( (CAN1->RF0R)&0x03 ) CAN1->RF0R |= CAN_RF0R_RFOM0;
	//Clean all FIFO_1
	while ( (CAN1->RF1R)&0x03 ) CAN1->RF1R |= CAN_RF1R_RFOM1;

	
	//Clean support table
	memset(my_icar.obd.support_pid, 0x0, OBD_SUPPORT_PID_CNT);

	//Trying round 1
	can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );

	if ( can_receive( my_icar.obd.rx_buf, OBD_BUF_SIZE, &rec_len, &can_id) ) {//Rec Data
		
		debug_obd("Rec %d:",rec_len);
		for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
			printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
		}
		printf(", %08X\r\n", can_id);
	}
	else { 
		
		return false;
	}
	
	memcpy(my_icar.obd.support_pid, &my_icar.obd.rx_buf[2],4);

	debug_obd("OBD sup table:");
	for ( var_uchar = 0 ; var_uchar < 4 ; var_uchar++ ) {
		printf(" %02X",my_icar.obd.support_pid[var_uchar]);
	}
	printf("\r\n");

	if ( !my_icar.obd.rx_buf[5]&0x01 ){//Data byte D,it does not support PID $20
		return true;	//Detect End
	}
	
	//Trying round 2
	determine_pid[2]=0x20;
	can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );

	if ( can_receive( my_icar.obd.rx_buf, OBD_BUF_SIZE, &rec_len, &can_id) ) {//Rec Data
		
		debug_obd("Rec %d:",rec_len);
		for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
			printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
		}
		printf(", %08X\r\n", can_id);
	}
	else { 
		
		return false;
	}

	memcpy(&my_icar.obd.support_pid[4], &my_icar.obd.rx_buf[2],4);

	debug_obd("OBD sup table:");
	for ( var_uchar = 0 ; var_uchar < 8 ; var_uchar++ ) {
		printf(" %02X",my_icar.obd.support_pid[var_uchar]);
	}
	printf("\r\n");

	if ( !my_icar.obd.rx_buf[5]&0x01 ){//Data byte D,it does not support PID $20
		return true;	//Detect End
	}


	//Trying round 3
	determine_pid[2]=0x40;
	can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );

	if ( can_receive( my_icar.obd.rx_buf, OBD_BUF_SIZE, &rec_len, &can_id) ) {//Rec Data
		
		debug_obd("Rec %d:",rec_len);
		for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
			printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
		}
		printf(", %08X\r\n", can_id);
	}
	else { 
		
		return false;
	}

	memcpy(&my_icar.obd.support_pid[8], &my_icar.obd.rx_buf[2],4);

	debug_obd("OBD sup table:");
	for ( var_uchar = 0 ; var_uchar < 12 ; var_uchar++ ) {
		printf(" %02X",my_icar.obd.support_pid[var_uchar]);
	}
	printf("\r\n");

	if ( !my_icar.obd.rx_buf[5]&0x01 ){//Data byte D,it does not support PID $20
		return true;	//Detect End
	}

	//Trying round 4
	determine_pid[2]=0x60;
	can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );

	if ( can_receive( my_icar.obd.rx_buf, OBD_BUF_SIZE, &rec_len, &can_id) ) {//Rec Data
		
		debug_obd("Rec %d:",rec_len);
		for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
			printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
		}
		printf(", %08X\r\n", can_id);
	}
	else { 
		
		return false;
	}

	memcpy(&my_icar.obd.support_pid[12], &my_icar.obd.rx_buf[2],4);

	debug_obd("OBD sup table:");
	for ( var_uchar = 0 ; var_uchar < 16 ; var_uchar++ ) {
		printf(" %02X",my_icar.obd.support_pid[var_uchar]);
	}
	printf("\r\n");

	if ( !my_icar.obd.rx_buf[5]&0x01 ){//Data byte D,it does not support PID $20
		return true;	//Detect End
	}

	return true;

	//OSSemPend(sem_obd, 1*OS_TICKS_PER_SEC, &os_err);//wait 1 second
	/*
	if ( os_err ) { //failure, timeout or others err
		debug_obd("OSSemPend err %d\r\n", os_err);
		//report this error: 
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
				my_icar.warn[var_uchar].msg |= ENQUIRE_PID_SEM << 16 ;//enquire can support PID error
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}

		return false;
	}
	else { //receive data from ECU
		if ( (CAN1->RF0R)&0x03 ) { //Rec ECU data
	
			CAN_Receive(CAN1,CAN_FIFO0, &can_rx_msg);
	
			if (can_rx_msg.IDE == CAN_ID_STD) { //CAN STD ID
	
				debug_obd("CAN FIFO_0: %d STD_ID: %X\r\n", (CAN1->RF0R)&0x03,can_rx_msg.StdId);
				debug_obd("DLC %d : by %d rule", can_rx_msg.DLC,can_rx_msg.FMI);
				//show the data
				for ( var_uchar = 0 ; var_uchar < can_rx_msg.DLC ; var_uchar++ ) {
					printf(" %02X",can_rx_msg.Data[var_uchar]);
				}
				printf("\r\n");
				
				if ( can_rx_msg.StdId == STD_RCV_CAN_ID ) { //correct CAN ID
					;
				}
				else {//err rcv can id
					debug_obd("CAN STD ID Err!\r\n");
					
					//return false;	
				}
				
				return false;
			}
			else { //CAN EXT ID
	
				debug_obd("CAN FIFO_0: %d EXT_ID: %X\r\n", (CAN1->RF0R)&0x03,can_rx_msg.ExtId);
				debug_obd("DLC %d : by %d rule", can_rx_msg.DLC,can_rx_msg.FMI);
				//show the data
				for ( var_uchar = 0 ; var_uchar < can_rx_msg.DLC ; var_uchar++ ) {
					printf(" %02X",can_rx_msg.Data[var_uchar]);
				}
				printf("\r\n");
					
				if ( can_rx_msg.ExtId == EXT_RCV_CAN_ID ) { //correct CAN ID

					memcpy(my_icar.obd.support_pid,&can_rx_msg.Data[3],4);

					debug_obd("OBD sup table:");
					for ( var_uchar = 0 ; var_uchar < can_rx_msg.DLC ; var_uchar++ ) {
						printf(" %02X",my_icar.obd.support_pid[var_uchar]);
					}
					printf("\r\n");


					if ( can_rx_msg.Data[6]&0x01 == 0x00 ){//Data byte D,it does not support PID $20
						return true;	//Detect End
					}
					
					//Trying round 2
					determine_pid[2]=0x20;
					can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );
					
					return true;
				}
				else  {//err rcv can id
					debug_obd("CAN EXT ID Err!\r\n");
					return false;	
				}
			}
		}

		else { //can not get ECU data
			debug_obd("No ECU DAT: %d\r\n", (CAN1->RF0R)&0x03);
			//report this error: 
			for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
				if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
					//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
					my_icar.warn[var_uchar].msg = (F_DRV_can) << 24 ;
					my_icar.warn[var_uchar].msg |= ENQUIRE_PID_DAT << 16 ;//enquire can support PID error, no rec data
					my_icar.warn[var_uchar].msg |= __LINE__ ;
					var_uchar = MAX_WARN_MSG ;//end the loop
				}
			}
			
			return false;
		}
	}
	*/
}
