#include "main.h"

extern struct ICAR_DEVICE my_icar;

CanTxMsg TxMessage;
CanRxMsg RxMessage;

extern OS_EVENT 	*sem_obd	;

static void auto_detect_obd( void );

//为了提高任务效率，改用事件驱动方式
//
void  app_task_obd (void *p_arg)
{
	CPU_INT08U	os_err;

	(void)p_arg;

	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 
	
	//BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);//For test

	switch ( obd_type ) {
	case OBD_TYPE_CAN1 ://CAN1
		debug_obd("OBD is CAN bus...\r\n");
		break;

	case OBD_TYPE_NO_DFN ://no define
	default://maybe error, treat as no define
		debug_obd("No OBD type\t");
		BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, prevent err
		auto_detect_obd( );
		
		break;
	}
			
	uart3_init( ); //to OBD, K-BUS

	//can_init( ); //to OBD, CAN BUS

	while ( 1 ) {

		OSSemPend(sem_obd, 0, &os_err);
		if ( !os_err ) {

			//CAN TX flag, for test only
			if ( my_icar.obd.can_tx_cnt ) { 
				CAN_Transmit(CAN1, &TxMessage);
				my_icar.obd.can_tx_cnt--;
			}

			//FIFO0 has data, max.: 3 datas
			if ( CAN_MessagePending(CAN1,CAN_FIFO0) ) {

				CAN_Receive(CAN1,CAN_FIFO0, &RxMessage);

				if (RxMessage.IDE == CAN_ID_STD) {
					prompt("CAN FIFO_0: %d ID: %X\r\n", CAN_MessagePending(CAN1,CAN_FIFO0),RxMessage.StdId);
				}
				else {
					prompt("CAN FIFO_0: %d ID: %X\r\n", CAN_MessagePending(CAN1,CAN_FIFO0),RxMessage.ExtId);
				}
				
  				
				if ( CAN_MessagePending(CAN1,CAN_FIFO0)==0 ) { //no data
					CAN_ITConfig(CAN1,CAN_IT_FMP0, ENABLE);//enable FIFO0 FMP int
				}
				else { //still has data
					OSSemPost( sem_obd );//no verify
				}
			}

			//FIFO1 receive unknow CAN ID, then report to server
			//check CAN1_RX1_IRQHandler(void) in drv_can.c
		}
	}
}

static void auto_detect_obd( void )
{
	u8 stream_support[]="\x02\x01\x00\x00\x00\x00\x00\x00";
	
	//1, Try CAN1_TYPE_STD_250
	//debug_obd("Try CAN1,STD_250\r\n");
	//can_init( CAN_250K, CAN_STD );
	//can_send( CAN_STD, DAT_FRAME, 0x07df, 8, stream_support );
	//save to flag
	
	//return ;
	
	//2, Try CAN1_TYPE_STD_500
	debug_obd("Try CAN1,STD_500\r\n");
	can_init( CAN_500K, CAN_STD );
	can_send( CAN_STD, DAT_FRAME, 0x07E0, 8, stream_support );
	
	//save to flag
	return ;

}
