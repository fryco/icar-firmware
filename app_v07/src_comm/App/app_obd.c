#include "main.h"

CanTxMsg TxMessage;
CanRxMsg RxMessage;
unsigned int rx_msg_cnt0 = 0 ;
unsigned int rx_msg_cnt1 = 0 ;

extern OS_EVENT 	*obd_can_tx	;

void  app_task_obd (void *p_arg)
{
	CPU_INT08U	os_err;

	(void)p_arg;

	uart3_init( ); //to OBD, K-BUS

	can_init( ); //to OBD, CAN BUS

	while ( 1 ) {

		OSSemPend(obd_can_tx, 1, &os_err);
		if ( !os_err ) {
			CAN_Transmit(CAN1, &TxMessage);
		}

		prompt("OBD task @ %d, FIFO_0: %d FIFO_1: %d\r\n", __LINE__, rx_msg_cnt0,rx_msg_cnt1);
		//	
		//1, release CPU
		//2, 
		OSTimeDlyHMSM(0, 0, 0, 800);

	}
}
