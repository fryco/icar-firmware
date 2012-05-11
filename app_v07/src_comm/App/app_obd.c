#include "main.h"

CanTxMsg TxMessage;
CanRxMsg RxMessage;
unsigned int rx_msg_cnt = 0 ;
void  app_task_obd (void *p_arg)
{

	(void)p_arg;

	uart3_init( ); //to OBD, K-BUS

	can_init( ); //to OBD, CAN BUS

	while ( 1 ) {
		prompt("OBD task @ %d, rx msg count: %d\r\n", __LINE__, rx_msg_cnt);
		CAN_Transmit(CAN1, &TxMessage);	
		//1, release CPU
		//2, 
		OSTimeDlyHMSM(0, 0, 0, 800);
	}
}
