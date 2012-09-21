#include "main.h"

extern struct ICAR_DEVICE my_icar;

CanTxMsg TxMessage;
CanRxMsg RxMessage;

extern OS_EVENT 	*sem_obd	;

//warn code define, internal use only
#define	UNK_CAN_STDID			5	//unknow CAN STD ID
#define	UNK_CAN_STDID_250K		10	//unknow CAN STD ID
#define	UNK_CAN_STDID_500K		15	//unknow CAN STD ID
#define	UNK_CAN_EXTID_H			20	//Unknow ext-stand CAN ID High
#define	UNK_CAN_EXTID_L			30	//Unknow ext-stand CAN ID low

static void auto_detect_obd( void );

//为了提高任务效率，改用事件驱动方式
//
void  app_task_obd (void *p_arg)
{
	u8 var_uchar;
	
	CPU_INT08U	os_err;

	(void)p_arg;

	OSSemPend(sem_obd, 0, &os_err);//Wait task manger ask to start

	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 
	
	//BKP_WriteBackupRegister(BKP_DR2, 0xFFFF);//For test

	switch ( obd_read_type ) {
	case OBD_TYPE_CAN1 ://CAN1
		debug_obd("OBD is CAN bus\r\n");
		break;

	case OBD_TYPE_NO_DFN ://no define
	default://maybe error, treat as no define
		debug_obd("No OBD type\r\n");
		BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, prevent err
		auto_detect_obd( );
		
		break;
	}

	debug_obd("BK2: %04X\r\n",BKP_ReadBackupRegister(BKP_DR2));
	//uart3_init( ); //to OBD, K-BUS

	//can_init( ); //to OBD, CAN BUS

	/* Enable Interrupt for receive FIFO 0 and FIFO overflow */
	//CAN_ITConfig(CAN1,CAN_IT_FMP0 | CAN_IT_FOV0, ENABLE);

	/* Enable Interrupt for receive FIFO 1 and FIFO overflow */
	//CAN_ITConfig(CAN1,CAN_IT_FMP1 | CAN_IT_FOV1, ENABLE);	

	while ( 1 ) {
		debug_obd("OBD task Pend...\r\n");
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
			//FIFO0 has data, max.: 3 datas
			if ( CAN_MessagePending(CAN1,CAN_FIFO1) ) {

				CAN_Receive(CAN1,CAN_FIFO1, &RxMessage);
				
				if (RxMessage.IDE == CAN_ID_STD) { //unknow CAN STD ID, report to server
					//prompt("CAN FIFO_1: %d ID: %X\r\n", CAN_MessagePending(CAN1,CAN_FIFO1),RxMessage.StdId);
					debug_obd("Unknow CAN STD: %X FIFO:%d\r\n",RxMessage.StdId,CAN_MessagePending(CAN1,CAN_FIFO1));
					
					for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
						if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
							//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
							my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
							my_icar.warn[var_uchar].msg |= UNK_CAN_STDID << 16 ;//unknow CAN STD ID
							my_icar.warn[var_uchar].msg |= RxMessage.StdId ;
							var_uchar = MAX_WARN_MSG ;//end the loop
						}
					}// 验证成功
				}
				else { //unknow CAN EXT ID, report to server
					//prompt("CAN FIFO_1: %d ID: %X\r\n", CAN_MessagePending(CAN1,CAN_FIFO1),RxMessage.ExtId);
					debug_obd("Unknow CAN EXT: %X FIFO:%d\r\n",RxMessage.ExtId,CAN_MessagePending(CAN1,CAN_FIFO1));

					for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
						if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
							//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
							my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
							my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_H << 16 ;//Unknow ext-stand CAN ID high
							my_icar.warn[var_uchar].msg |= ( RxMessage.ExtId >> 16) &0xFFFF ;
							var_uchar = MAX_WARN_MSG ;//end the loop
						}
					}// 验证成功 2012/8/28 10:58:10
			
					for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
						if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
							//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
							my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
							my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_L << 16 ;//Unknow ext-stand CAN ID low
							my_icar.warn[var_uchar].msg |= ( RxMessage.ExtId ) &0xFFFF ;
							var_uchar = MAX_WARN_MSG ;//end the loop
						}
					}// 验证成功 2012/8/28 10:58:08
				}
  				
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

void obd_set_type( u8 type )
{
	u16 bk_reg ;

	//BKP_DR2, OBD Flag:	3~0: OBD_TYPEDEF
	
	bk_reg = BKP_ReadBackupRegister(BKP_DR2) & 0xFFF0 ;
	bk_reg = bk_reg | (type&0x0F) ;
	BKP_WriteBackupRegister(BKP_DR2, bk_reg);
}

void can_set_speed( can_pin_typedef can_pin, u8 speed )
{
	u16 bk_reg ;
	
	//BKP_DR2, OBD Flag:	11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF

	if ( can_pin == CAN_1 ) {
		bk_reg = BKP_ReadBackupRegister(BKP_DR2) & 0xFF0F ;
		bk_reg = bk_reg | ((speed<<4)&0xF0) ;
		BKP_WriteBackupRegister(BKP_DR2, bk_reg);
		return ;
	}
	
	if ( can_pin == CAN_2 ) {
		bk_reg = BKP_ReadBackupRegister(BKP_DR2) & 0xF0FF ;
		bk_reg = bk_reg | ((speed<<8)&0xF00) ;
		BKP_WriteBackupRegister(BKP_DR2, bk_reg);
		return ;
	}
}

static void auto_detect_obd( void )
{
	u8 stream_support[]="\x02\x01\x00\x00\x00\x00\x00\x00";
	u8 obd_typ = 0 , var_uchar;
	
	//流程：设波特率==>开所有ID接收==>查看CAN_MessagePending有无数据
	//方法不好，改发ID，然后收2012/9/7 17:23:33

	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 

	//1, Try CAN1_TYPE_STD_250

	debug_obd("Try CAN1,STD_250\r\n");
	can_init( CAN_250K, CAN_STD );
	can_add_filter( CAN_STD , 0x07DF );//save to fifo0
	can_rec_all_id( true ); //save to fifo1

	//send CAD ID: 0x07DF
	can_send( CAN_STD, DAT_FRAME, 0x07DF, 8, stream_support );
	OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
	if ( CAN_MessagePending(CAN1,CAN_FIFO0) ) { //have data, setting correct
		debug_obd("FIFO0: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO0));

		obd_typ = OBD_TYPE_CAN1 ;
		my_icar.obd.can_id = 0x07DF ;
	}
	else { //try CAD ID: 0x07E0 again
		can_send( CAN_STD, DAT_FRAME, 0x07E0, 8, stream_support );
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( CAN_MessagePending(CAN1,CAN_FIFO0) ) { //have data, setting correct
			debug_obd("FIFO0: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO0));

			obd_typ = OBD_TYPE_CAN1 ;
			my_icar.obd.can_id = 0x07E0 ;
		}
	}
	
	if ( obd_typ ) { //detect successful
		//save to flag
		obd_set_type( OBD_TYPE_CAN1 ) ;
		can_set_speed( CAN_1, CAN1_TYPE_STD_250 );
		//Disable filter for all ID
		can_rec_all_id( false );

		return ;
	}

	if ( CAN_MessagePending(CAN1,CAN_FIFO1) ) { //have data in fifo1: unknow CAN ID
		debug_obd("FIFO1: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO1));

		//report CAN speed and unknow CAN_ID to cloud server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_STDID_250K << 16 ;//unknow CAN_STDID_250K
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功

		//Disable filter for all ID
		can_rec_all_id( false );

		return ;
	}
		
	//2, Try CAN1_TYPE_STD_500
	debug_obd("Try CAN1,STD_500\r\n");
	can_init( CAN_500K, CAN_STD );
	can_add_filter( CAN_STD , 0x07DF );//save to fifo0
	can_rec_all_id( true ); //save to fifo1

	//send CAD ID: 0x07DF
	can_send( CAN_STD, DAT_FRAME, 0x07DF, 8, stream_support );
	OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
	if ( CAN_MessagePending(CAN1,CAN_FIFO0) ) { //have data, setting correct
		debug_obd("FIFO0: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO0));

		obd_typ = OBD_TYPE_CAN1 ;
		my_icar.obd.can_id = 0x07DF ;
	}
	else { //try CAD ID: 0x07E0 again
		can_send( CAN_STD, DAT_FRAME, 0x07E0, 8, stream_support );
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( CAN_MessagePending(CAN1,CAN_FIFO0) ) { //have data, setting correct
			debug_obd("FIFO0: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO0));

			obd_typ = OBD_TYPE_CAN1 ;
			my_icar.obd.can_id = 0x07E0 ;
		}
	}
	
	if ( obd_typ ) { //detect successful
		//save to flag
		obd_set_type( OBD_TYPE_CAN1 ) ;
		can_set_speed( CAN_1, CAN1_TYPE_STD_500 );
		//Disable filter for all ID
		can_rec_all_id( false );

		return ;
	}

	if ( CAN_MessagePending(CAN1,CAN_FIFO1) ) { //have data in fifo1: unknow CAN ID
		debug_obd("FIFO1: %d\r\n",CAN_MessagePending(CAN1,CAN_FIFO1));

		//report CAN speed and unknow CAN_ID to cloud server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_STDID_500K << 16 ;//unknow CAN_STDID_250K
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功

		//Disable filter for all ID
		can_rec_all_id( false );

		return ;
	}
}
