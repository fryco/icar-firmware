#include "main.h"

extern struct ICAR_DEVICE my_icar;

CanTxMsg TxMessage;
CanRxMsg RxMessage;
unsigned char rx_msg_cnt0 = 0 ;
unsigned char rx_msg_cnt1 = 0 ;

extern OS_EVENT 	*sem_obd	;

//为了提高任务效率，改用事件驱动方式
//
void  app_task_obd (void *p_arg)
{
	CPU_INT08U	os_err;

	(void)p_arg;

	uart3_init( ); //to OBD, K-BUS

	can_init( ); //to OBD, CAN BUS

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

				prompt("CAN FIFO_0: %d\r\n", CAN_MessagePending(CAN1,CAN_FIFO0));

				if ( CAN_MessagePending(CAN1,CAN_FIFO0)==0 ) { //no data
					CAN_ITConfig(CAN1,CAN_IT_FMP0, ENABLE);//enable FIFO0 FMP int
				}
				else { //still has data
					OSSemPost( sem_obd );//no verify
				}
			}

			//FIFO1 has data, max.: 3 datas
			if ( CAN_MessagePending(CAN1,CAN_FIFO1) ) {

				prompt("CAN FIFO_1: %d\r\n", CAN_MessagePending(CAN1,CAN_FIFO1));

				if ( CAN_MessagePending(CAN1,CAN_FIFO1)==0 ) { //no data
					CAN_ITConfig(CAN1,CAN_IT_FMP1, ENABLE);//enable FIFO1 FMP int
				}
				else { //still has data
					OSSemPost( sem_obd );//no verify
				}
			}

		}
	}
}
