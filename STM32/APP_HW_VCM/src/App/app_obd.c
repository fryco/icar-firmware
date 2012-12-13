#include "main.h"

extern struct ICAR_DEVICE my_icar;

extern OS_EVENT 	*sem_obd_task	;
extern OS_EVENT 	*sem_obd_fifo	;

//warn code define, internal use only, < 255
#define	UNK_CAN_STDID			5	//unknow CAN STD ID
#define	UNK_CAN_ID_250K			10	//unknow CAN ID @ 250K
#define	UNK_CAN_ID_500K			15	//unknow CAN ID @ 500K
#define	UNK_CAN_EXTID_H			20	//Unknow ext-stand CAN ID High
#define	UNK_CAN_EXTID_L			30	//Unknow ext-stand CAN ID low
#define	ERR_CAN_ID_IDX			40	//Error CAN ID index
#define	ERR_CAN_SUP_PID 		50  //Error get CAN  support PID

//RO-data, save in flash
const unsigned int can_snd_id[] = {
0x0,
0x07DF,
0x07E0,
0x18DB33F1,
0x18DA10F1
};

//Parameter ID, check http://en.wikipedia.org/wiki/OBD-II_PIDs

static void obd_report_unknow_canid( u32 );
static void obd_auto_detect( void );

//为了提高任务效率，改用事件驱动方式
//
void  app_task_obd (void *p_arg)
{
	u8 var_uchar;
	//Request Supported PIDs from Vehicle, check sae j1979 2002.pdf page 26
	u8 determine_pid[]="\x02\x01\x00\x00\x00\x00\x00\x00";

	u16 can_id_idx = 0;
	u32 can_id ;
	CanRxMsg can_rx_msg;
	
	CPU_INT08U	os_err;

	(void)p_arg;

//can_enquire_determine_pid( );

	OSSemPend(sem_obd_task, 0, &os_err);//Wait task manger ask to start

	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 
	
	//BKP_DR3, can_snd_id index, check app_obd.c
		
	switch ( obd_read_type ) {
	case OBD_TYPE_CAN1 ://CAN1
		debug_obd("OBD is CAN1 ");
		can_id_idx = obd_read_canid_idx ;
		if ( !can_id_idx ) {//no CAN send id index
			obd_auto_detect( );
			can_id_idx = obd_read_canid_idx ;
		}
		
		switch ( obd_can1_type ) {
		case CAN1_TYPE_STD_250 ://CAN1_TYPE_STD_250
			debug_obd("STD 250\r\n");
			break;
			
		case CAN1_TYPE_EXT_250 ://CAN1_TYPE_EXT_250
			debug_obd("EXT 250\r\n");
			break;

		case CAN1_TYPE_STD_500 ://CAN1_TYPE_STD_500
			debug_obd("STD 500\r\n");
			break;
			
		case CAN1_TYPE_EXT_500 ://CAN1_TYPE_EXT_500
			debug_obd("EXT 500\r\n");
			break;

		case CAN1_TYPE_NO_DEFINE ://CAN1 no define
		default://maybe error, treat as no define
			debug_obd("but type ERR!\r\n");
			BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, prevent err
			BKP_WriteBackupRegister(BKP_DR3, 0);//clean all flag, prevent err
			obd_auto_detect( );
			can_id_idx = obd_read_canid_idx ;
			break;
		}
		break;

	case OBD_TYPE_CAN2 ://CAN2
		debug_obd("OBD is CAN2 bus\r\n");
		if ( !obd_read_canid_idx ) {//no CAN send id index
			obd_auto_detect( );
			can_id_idx = obd_read_canid_idx ;
		}

		break;

	case OBD_TYPE_NO_DFN ://no define
	default://maybe error, treat as no define
		debug_obd("No OBD type\r\n");
		BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, prevent err
		BKP_WriteBackupRegister(BKP_DR3, 0);//clean all flag, prevent err
		obd_auto_detect( );
		can_id_idx = obd_read_canid_idx ;
		
		break;
	}


	while ( !can_id_idx ) { //no CAN ID index, err

		debug_obd("BK2: %04X BK3: %04X\r\n",BKP_ReadBackupRegister(BKP_DR2),BKP_ReadBackupRegister(BKP_DR3));
		debug_obd("can_id_idx = %d\r\n", can_id_idx);
		OSSemPend(sem_obd_task, 0, &os_err);

		debug_obd("CAN ID index ERR\r\n");
		//Report err to server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= ERR_CAN_ID_IDX << 16 ;//Error CAN ID index
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}

		obd_auto_detect( );
		can_id_idx = obd_read_canid_idx ;
	}
	
	//uart3_init( ); //to OBD, K-BUS

	while ( !can_enquire_support_pid( ) ) { //update support table ERR

		debug_obd("Get CAN PID ERR\r\n");
		//Report err to server
		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= ERR_CAN_SUP_PID << 16 ;//Error get CAN  support PID
				my_icar.warn[var_uchar].msg |= __LINE__ ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}
		OSTimeDlyHMSM(0, 0,	30, 0);//wait 30s
	}

	for ( var_uchar = 0 ; var_uchar <= OBD_MAX_PID ; var_uchar++) {
		if ( can_read_pid(var_uchar) ) {//read PID error
			debug_obd("Read PID:%d ERR\r\n",var_uchar);
		}
		else {
			debug_obd("Read PID:%d OK\r\n",var_uchar);
		}
	}
	
	/* Enable Interrupt for receive FIFO 0 and FIFO overflow */
	CAN_ITConfig(CAN1,CAN_IT_FMP0 | CAN_IT_FOV0, ENABLE);

	/* Enable Interrupt for receive FIFO 1 and FIFO overflow */
	CAN_ITConfig(CAN1,CAN_IT_FMP1 | CAN_IT_FOV1, ENABLE);

	while ( 1 ) {
		//debug_obd("OBD task Pend...\r\n");
		OSSemPend(sem_obd_task, 0, &os_err);
		if ( !os_err ) {

			//CAN TX flag, for test only
			if ( my_icar.obd.can_tx_cnt ) { 

				//send CAD ID: 0x18DB33F1
				can_send( can_snd_id[can_id_idx], DAT_FRAME, 8, determine_pid );

				//CAN_Transmit(CAN1, &TxMessage);
				my_icar.obd.can_tx_cnt--;
			}

			//execute CMD from app_taskmanager
			switch ( my_icar.obd.cmd ) {

			case READ_PID ://update support table

				debug_obd("CMD is Read ECU pid:%d\r\n",my_icar.obd.pid);
				my_icar.obd.cmd = NO_CMD ;
				var_uchar = can_read_pid( my_icar.obd.pid );//result in my_icar.obd.rx_buf
				if ( var_uchar ) {//read PID error
					debug_obd("Read PID err: %d\r\n", var_uchar);
				}
				else {//get ECU data
					//debug_obd("Read PID OK: %d\r\n", var_uchar);
					switch ( my_icar.obd.pid ) {

					case OBD_PID_ENGINE_RPM:
						debug_obd("Read RPM OK.\r\n");
						//debug_obd("Rec %d:",rec_len);
						//for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
							//printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
						//}
						printf(", from ID:%08X\r\n", can_id);

						break;

					default://maybe error
						debug_obd("PID err:%d\r\n",my_icar.obd.pid);
						break;
					}
				}
				break;

			case SPEED ://engine speed

				debug_obd("CMD is get engine speed\r\n");
				my_icar.obd.cmd = NO_CMD ;
				can_send( can_snd_id[can_id_idx], DAT_FRAME, 8, determine_pid );
		
				break;
				
			case NO_CMD ://no define
			default://maybe error, treat as NO_CMD
				;//debug_obd("No OBD CMD\r\n");
				break;
			}
			
			//FIFO0 has data, max.: 3 datas
			if ( (CAN1->RF0R)&0x03 ) {

				CAN_Receive(CAN1,CAN_FIFO0, &can_rx_msg);

				if (can_rx_msg.IDE == CAN_ID_STD) {
					debug_obd("CAN FIFO_0 rec %d ID: %X\r\n", (CAN1->RF0R)&0x03,can_rx_msg.StdId);
					debug_obd("DLC %d :", can_rx_msg.DLC);
					//show the data
					for ( var_uchar = 0 ; var_uchar < can_rx_msg.DLC ; var_uchar++ ) {
						printf(" %02X",can_rx_msg.Data[var_uchar]);
					}
					printf("\r\n");

				}
				else {
					debug_obd("CAN FIFO_0 rec %d ID: %X %s,%d\r\n", (CAN1->RF0R)&0x03,can_rx_msg.ExtId,\
						__FILE__,__LINE__);
					debug_obd("DLC %d :", can_rx_msg.DLC);
					//show the data
					for ( var_uchar = 0 ; var_uchar < can_rx_msg.DLC ; var_uchar++ ) {
						printf(" %02X",can_rx_msg.Data[var_uchar]);
					}
					printf("\r\n");
					
				}
				
  				
				if ( (CAN1->RF0R)&0x03==0 ) { //no data
					CAN_ITConfig(CAN1,CAN_IT_FMP0, ENABLE);//enable FIFO0 FMP int
				}
				else { //still has data
					OSSemPost( sem_obd_task );//2012/9/26 14:55:33 verify
				}
			}

			//FIFO1 receive unknow CAN ID, then report to server
			//FIFO0 has data, max.: 3 datas
			if ( (CAN1->RF1R)&0x03 ) {

				CAN_Receive(CAN1,CAN_FIFO1, &can_rx_msg);
				
				if (can_rx_msg.IDE == CAN_ID_STD) { //unknow CAN STD ID, report to server
					can_id = can_rx_msg.StdId ;
				}
				else { //unknow CAN EXT ID, report to server
					can_id = can_rx_msg.ExtId ;
				}
				obd_report_unknow_canid( can_id );
				debug_obd("Unknow CAN ID: %X FIFO:%d\r\n",can_id,(CAN1->RF1R)&0x03);
					  				
				if ( (CAN1->RF1R)&0x03==0 ) { //no data
					CAN_ITConfig(CAN1,CAN_IT_FMP1, ENABLE);//enable FIFO1 FMP int
				}
				else { //still has data
					OSSemPost( sem_obd_task );//no verify
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

static void obd_report_unknow_canid( u32 id )
{
	u8 var_uchar ;
	
	if ( id > 0x7FF ) {//CAN EXT

		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_H << 16 ;//Unknow ext-stand CAN ID high
				my_icar.warn[var_uchar].msg |= ( id >> 16) &0xFFFF ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功 2012/8/28 10:58:10

		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_EXTID_L << 16 ;//Unknow ext-stand CAN ID low
				my_icar.warn[var_uchar].msg |= ( id ) &0xFFFF ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功 2012/8/28 10:58:08

	}
	else {

		for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
			if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
				//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
				my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
				my_icar.warn[var_uchar].msg |= UNK_CAN_STDID << 16 ;//unknow CAN STD ID
				my_icar.warn[var_uchar].msg |= id ;
				var_uchar = MAX_WARN_MSG ;//end the loop
			}
		}// 验证成功

	}

	return ;
}
static void obd_auto_detect( void )
{
	//Request Supported PIDs from Vehicle, check sae j1979 2002.pdf page 26
	u8 determine_pid[]="\x02\x01\x00\x00\x00\x00\x00\x00";

	u8 obd_typ = 0 , var_uchar;
	u32 can_id ;
	
	CanRxMsg can_rx_msg;

	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 
	//BKP_DR3, can_snd_id index, check app_obd.c
	
	prompt("Auto detect OBD ...\r\n");
	
	//1, Try CAN1_TYPE_STD_250

	debug_obd("Try CAN1,STD_250 ...\r\n");
	can_init( CAN_250K, CAN_STD );
	
	var_uchar = can_add_filter( CAN_STD , STD_RCV_CAN_ID );//0x07E8,save to fifo0
	if ( var_uchar ) { 
		debug_obd("Set filter %d OK\r\n", var_uchar-1);
	}
	else {
		debug_obd("ERR: no free filter\r\n");
	}

	can_rec_all_id( true ); //save to fifo1

	//send CAD ID: 0x07DF
	can_send( can_snd_id[1], DAT_FRAME, 8, determine_pid );
	OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
	if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
		debug_obd("CAN1,STD_250 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);

		obd_typ = OBD_TYPE_CAN1 ;
		BKP_WriteBackupRegister(BKP_DR3, 1);//0x07DF,save CAN ID index
	}
	else { //try CAD ID: 0x07E0 again
		can_send( can_snd_id[2], DAT_FRAME, 8, determine_pid );
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
			debug_obd("CAN1,STD_250 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);

			obd_typ = OBD_TYPE_CAN1 ;
			BKP_WriteBackupRegister(BKP_DR3, 2);//0x07E0, save CAN ID index
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

	if ( (CAN1->RF1R)&0x03 ) { //have data in fifo1, maybe SND ID err or EXT ID
		debug_obd("\r\n");
		debug_obd("FIFO1: %d. %d\r\n",(CAN1->RF1R)&0x03,__LINE__);

		//Try to CAN1_TYPE_EXT_250
		debug_obd("CAN1,STD_250 failure, try CAN1,EXT_250 ...\r\n");
		can_init( CAN_250K, CAN_EXT );

		var_uchar = can_add_filter( CAN_EXT , EXT_RCV_CAN_ID );//0x18DAF111, save to fifo0
		if ( var_uchar ) { 
			debug_obd("Set filter %d OK\r\n", var_uchar-1);
		}
		else {
			debug_obd("ERR: no free filter\r\n");
		}
	
		can_rec_all_id( true ); //save to fifo1

		//send CAD ID: 0x18DB33F1
		can_send( can_snd_id[3], DAT_FRAME, 8, determine_pid );//0x18DB33F1
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
			debug_obd("CAN1,EXT_250 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);
	
			obd_typ = OBD_TYPE_CAN1 ;
			BKP_WriteBackupRegister(BKP_DR3, 3);//0x18DB33F1, save CAN ID index
		}
		else { //try CAD ID: 0x18DA10F1 again
			can_send( can_snd_id[4], DAT_FRAME, 8, determine_pid );//0x18DA10F1
			OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
			if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
				debug_obd("CAN1,EXT_250 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);
	
				obd_typ = OBD_TYPE_CAN1 ;
				BKP_WriteBackupRegister(BKP_DR3, 4);//0x18DA10F1, save CAN ID index
			}
		}

		if ( obd_typ ) { //detect CAN1_TYPE_EXT_250 successful
			//save to flag
			obd_set_type( OBD_TYPE_CAN1 ) ;
			can_set_speed( CAN_1, CAN1_TYPE_EXT_250 );
			//Disable filter for all ID
			can_rec_all_id( false );
	
			return ;
		}
		else {//report this err to server, RCV ID err
			
			CAN_Receive(CAN1,CAN_FIFO1, &can_rx_msg);
			
			if (can_rx_msg.IDE == CAN_ID_STD) { //unknow CAN STD ID, report to server
				can_id = can_rx_msg.StdId ;
			}
			else { //unknow CAN EXT ID, report to server
				can_id = can_rx_msg.ExtId ;
			}
			obd_report_unknow_canid( can_id );
			debug_obd("Unknow CAN ID: %X FIFO:%d\r\n",can_id,(CAN1->RF1R)&0x03);

			//report CAN speed and unknow CAN_ID to cloud server
			for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
				if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
					//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
					my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
					my_icar.warn[var_uchar].msg |= UNK_CAN_ID_250K << 16 ;//unknow CAN_ID_250K
					my_icar.warn[var_uchar].msg |= __LINE__ ;
					var_uchar = MAX_WARN_MSG ;//end the loop
				}
			}// 验证成功
	
			debug_obd("Rep err:UNK_CAN_ID_500K. %d\r\n",__LINE__);
	
			//Disable filter for all ID
			can_rec_all_id( false );
	
			return ;		
		}
	}

	//2, Try CAN1_TYPE_STD_500
	debug_obd("\r\n");
	debug_obd("CAN1,STD_250 failure, try CAN1,STD_500 ...\r\n");
	can_init( CAN_500K, CAN_STD );


	var_uchar = can_add_filter( CAN_STD , STD_RCV_CAN_ID );//0x07E8, save to fifo0
	if ( var_uchar ) { 
		debug_obd("Set filter %d OK\r\n", var_uchar-1);
	}
	else {
		debug_obd("ERR: no free filter\r\n");
	}

	can_rec_all_id( true ); //save to fifo1

	//send CAD ID: 0x07DF
	can_send( can_snd_id[1], DAT_FRAME, 8, determine_pid );
	OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
	if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
		debug_obd("CAN1,STD_500 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);

		obd_typ = OBD_TYPE_CAN1 ;
		BKP_WriteBackupRegister(BKP_DR3, 1);//0x07DF,save CAN ID index
	}
	else { //try CAD ID: 0x07E0 again
		can_send( can_snd_id[2], DAT_FRAME, 8, determine_pid );
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
			debug_obd("CAN1,STD_500 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);

			obd_typ = OBD_TYPE_CAN1 ;
			BKP_WriteBackupRegister(BKP_DR3, 2);//0x07E0, save CAN ID index
		}
	}
	
	if ( obd_typ ) { //detect CAN1_TYPE_STD_500 successful
		//save to flag
		obd_set_type( OBD_TYPE_CAN1 ) ;
		can_set_speed( CAN_1, CAN1_TYPE_STD_500 );
		//Disable filter for all ID
		can_rec_all_id( false );

		return ;
	}

	if ( (CAN1->RF1R)&0x03 ) { //have data in fifo1, maybe SND ID err or EXT ID
		debug_obd("\r\n");
		debug_obd("FIFO1: %d. %d\r\n",(CAN1->RF1R)&0x03,__LINE__);

		//Try to CAN1_TYPE_EXT_500
		debug_obd("CAN1,STD_500 failure, try CAN1,EXT_500 ...\r\n");
		can_init( CAN_500K, CAN_EXT );

		var_uchar = can_add_filter( CAN_EXT , EXT_RCV_CAN_ID );//0x18DAF111, save to fifo0
		if ( var_uchar ) { 
			debug_obd("Set filter %d OK\r\n", var_uchar-1);
		}
		else {
			debug_obd("ERR: no free filter\r\n");
		}
	
		can_rec_all_id( true ); //save to fifo1

		//send CAD ID: 0x18DB33F1
		can_send( can_snd_id[3], DAT_FRAME, 8, determine_pid );//0x18DB33F1
		OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
		if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
			debug_obd("CAN1,EXT_500 OK, ID is: 0x18DB33F1, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);
	
			obd_typ = OBD_TYPE_CAN1 ;
			BKP_WriteBackupRegister(BKP_DR3, 3);//0x18DB33F1, save CAN ID index
		}
		else { //try CAD ID: 0x18DA10F1 again
			can_send( can_snd_id[4], DAT_FRAME, 8, determine_pid );//0x18DA10F1
			OSTimeDlyHMSM(0, 0,	0, 50);	//wait 50 ms
			if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
				debug_obd("CAN1,EXT_500 OK, ID is: 0x18DA10F1, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);
	
				obd_typ = OBD_TYPE_CAN1 ;
				BKP_WriteBackupRegister(BKP_DR3, 4);//0x18DA10F1, save CAN ID index
			}
		}

		if ( obd_typ ) { //detect CAN1_TYPE_EXT_500 successful
			//save to flag
			obd_set_type( OBD_TYPE_CAN1 ) ;
			can_set_speed( CAN_1, CAN1_TYPE_EXT_500 );
			//Disable filter for all ID
			can_rec_all_id( false );
	
			return ;
		}
		else {//report this err to server, RCV ID err
			
			CAN_Receive(CAN1,CAN_FIFO1, &can_rx_msg);
			
			if (can_rx_msg.IDE == CAN_ID_STD) { //unknow CAN STD ID, report to server
				can_id = can_rx_msg.StdId ;
			}
			else { //unknow CAN EXT ID, report to server
				can_id = can_rx_msg.ExtId ;
			}
			obd_report_unknow_canid( can_id );
			debug_obd("Unknow CAN ID: %X FIFO:%d\r\n",can_id,(CAN1->RF1R)&0x03);

			//report CAN speed and unknow CAN_ID to cloud server
			for ( var_uchar = 0 ; var_uchar < MAX_WARN_MSG ; var_uchar++) {
				if ( !my_icar.warn[var_uchar].msg ) { //empty msg	
					//unsigned int msg;//file name(1 Byte), msg(1 Byte), line(2 B)
					my_icar.warn[var_uchar].msg = (F_APP_OBD) << 24 ;
					my_icar.warn[var_uchar].msg |= UNK_CAN_ID_500K << 16 ;//unknow CAN_ID_500K
					my_icar.warn[var_uchar].msg |= __LINE__ ;
					var_uchar = MAX_WARN_MSG ;//end the loop
				}
			}// 验证成功
	
			debug_obd("Rep err:UNK_CAN_ID_500K. %d\r\n",__LINE__);
	
			//Disable filter for all ID
			can_rec_all_id( false );
	
			return ;		
		}
	}
}

bool obd_check_support_pid( u16 id )
{
	int bytepos;              
	int bitpos;               

	if( (id < OBD_MIN_PID) || (id > OBD_MAX_PID )){

		return false;
	}

	if ( !(id%0x20)) {//0x20,0x40,0x60,0x80: PIDs supported list
		return false;
	}
	
	id--;

	bytepos=id/8;
	bitpos=id%8;

	//bit7 ~ bit 0 代表 数据流ID 1~8 
	return 0 != (my_icar.obd.support_pid[bytepos] & (0x80>>bitpos));
}
