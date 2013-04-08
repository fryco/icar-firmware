#include "config.h"
#include "main.h"

extern struct ICAR_DEVICE my_icar;
extern struct OBD_FRM_BUFF obd_fbuff;

extern OS_EVENT 	*sem_obd_task	;
extern OS_EVENT 	*sem_obd_prot	;


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

u8 ENTER_VPW[]		= {0x02,0x01,0x00};
u8 ENTER_ISO9141[]	= {0x02,0x01,0x00};
u8 ENTER_PWM[]		= {0x02,0x01,0x00};
u8 ENTER_KWP2000[]	= {0x01,0x81};
u8 EXIT_KWP2000[]	= {0x01,0x82};

u8 VPW_HEAD[] 		= {0x68,0x6A,0xF1};
u8 ISO9141_HEAD[] 	= {0x68,0x6A,0xF1};
u8 KWP2000_HEAD[] 	= {0xC0,0x33,0xF1};
u8 EKWP2000_HEAD[] = {0x80,0x11,0xF1};
u8 PWM_HEAD[] 		= {0x61,0x6A,0xF1};

//Parameter ID, check http://en.wikipedia.org/wiki/OBD-II_PIDs

static void obd_report_unknow_canid(u32);
void obd_auto_detect(void);
bool Enquire_support_pid(u8 type);
bool Obd_Enter_VPW(void);
bool Obd_Enter_PWM(void);
bool Obd_Enter_KWP(void);
u8 Obd_Enter_ISO(void);
u8 Icar_SendRead(u8 type,u8* Strp);

//为了提高任务效率，改用事件驱动方式
//
void  app_task_obd (void *p_arg)
{
	u8 var_uchar;
	u8 cmd[3];
	//Request Supported PIDs from Vehicle, check sae j1979 2002.pdf page 26
	u8 DTCcmd[]={0x01,0x03};

	u16 can_id_idx = 0;
	
	CPU_INT08U	os_err;

	(void)p_arg;

	my_icar.event = 0;
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
			obd_auto_detect();
			can_id_idx = obd_read_canid_idx ;
		}
		break;

	case OBD_TYPE_VPW ://no define
		if(Obd_Enter_VPW()){	debug_obd("OBD is VPW\r\n");break;}
		obd_auto_detect();
		break;
	case OBD_TYPE_PWM ://no define
		if(Obd_Enter_PWM()){debug_obd("OBD is PWM\r\n");break;}
		obd_auto_detect();
		break;
	case OBD_TYPE_KWP ://no define
		if(Obd_Enter_KWP())	{debug_obd("OBD is KWP\r\n");break;}
		obd_auto_detect();
		break;
	case OBD_TYPE_ISO ://no define
		if(Obd_Enter_ISO()){debug_obd("OBD is ISO\r\n");break;}
		obd_auto_detect();
   		break;
	case OBD_TYPE_NO_DFN ://no define
	default://maybe error, treat as no define
		debug_obd("No OBD type\r\n");
		BKP_WriteBackupRegister(BKP_DR2, 0);//clean all flag, prevent err
		BKP_WriteBackupRegister(BKP_DR3, 0);//clean all flag, prevent err
		obd_auto_detect();
		can_id_idx = obd_read_canid_idx ;
		break;
	}

/*	while ( (!can_id_idx) &&((obd_read_type == OBD_TYPE_CAN1)||(obd_read_type == OBD_TYPE_CAN2) ) ) { //no CAN ID index, err
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

		obd_auto_detect();
		can_id_idx = obd_read_canid_idx ;
	} */

	OSTimeDlyHMSM(0, 0, 0, 500);//wait 500ms
  /*
	while ( !Enquire_support_pid(obd_read_type ) ) { //update support table ERR

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
	//	if ( can_read_pid(var_uchar) ) {//read PID error
	//		debug_obd("Read PID:%d ERR\r\n",var_uchar);
	//	}
	//	else {
	//		debug_obd("Read PID:%d OK\r\n",var_uchar);
	//	}
	}
	*/

	while ( 1 ) {
		//debug_obd("OBD task Pend...\r\n");
		OSSemPend(sem_obd_task, 0, &os_err);
		memcpy(cmd,ENTER_VPW,3);
		if ( !os_err ) {
			//execute CMD from app_taskmanager
			switch ( my_icar.obd.cmd ) {

			case READ_PID ://update support table

				debug_obd("CMD is Read ECU pid:%d\r\n",my_icar.obd.pid);
				my_icar.obd.cmd = NO_CMD ;
				cmd[2]= my_icar.obd.pid;
				var_uchar=Icar_SendRead(obd_read_type,cmd);
				if ( var_uchar ) {//read PID error
					debug_obd("Read PID err: %d\r\n", var_uchar);
				}
				else {//get ECU data
					//debug_obd("Read PID OK: %d\r\n", var_uchar);
					cmd[2]=my_icar.obd.pid;

					switch ( my_icar.obd.pid ) {

					case OBD_PID_ENGINE_RPM:
						debug_obd("Read RPM OK.\r\n");
						break;
						
					case OBD_PID_VECHIVE_SPEED:
						debug_obd("VECHIVE_SPEED OK.\r\n");
						break;
					case OBD_PID_CLT:
						
						debug_obd("CLT OK.\r\n");
						break;
						//debug_obd("Rec %d:",rec_len);
					/*	for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
							printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
						}
						printf(", from ID:%08X\r\n", can_id);
					  */
					default://maybe error
						debug_obd("PID err:%d\r\n",my_icar.obd.pid);
						break;
					}
				}

				break;

			case SPEED ://engine speed

				debug_obd("CMD is get engine speed\r\n");
				my_icar.obd.cmd = NO_CMD ;
				cmd[2]= 0x0d;
				Icar_SendRead(obd_read_type,cmd);
				break;
				
			case READ_DTC ://no define

				debug_obd("CMD is get Dtcs\r\n");
				my_icar.obd.cmd = NO_CMD ;
				memcpy(cmd,DTCcmd,2);
				Icar_SendRead(obd_read_type,cmd);
				
				//can_send( can_snd_id[can_id_idx], DAT_FRAME, 8, DTCcmd );
		/*		if ( can_receive( my_icar.obd.rx_buf, OBD_BUF_SIZE, &rec_len, &can_id) ) {//Rec Data
				  	
					debug_obd("Rec %d:",rec_len);
					for ( var_uchar = 0 ; var_uchar < rec_len ; var_uchar++ ) {
						printf(" %02X",my_icar.obd.rx_buf[var_uchar]);
					}
					printf(", from ID:%08X\r\n", can_id);


					debug_obd("Rec %d:",my_icar.obd.rx_buf1[0]);
					for ( var_uchar = 1 ; var_uchar < my_icar.obd.rx_buf1[0]; var_uchar++ ) {
						printf(" %02X",my_icar.obd.rx_buf1[var_uchar]);
					}
					printf(", from ID:%08X\r\n", can_id);

		
				}
				else {

				}
		  		*/
				break;
			case NO_CMD ://no define

				break;
			default://maybe error, treat as NO_CMD
				;//debug_obd("No OBD CMD\r\n");
				break;
			}
			
			//FIFO0 has data, max.: 3 datas
	/*		if ( (CAN1->RF0R)&0x03 ) {

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
			}*/			
		}
	}
}

#define CAN_SendFrame can_send
#define CAN_RecvFrame can_receive	

int SendFrame(u8 type,u8 *pSendFrame)
{
	int iLength=0;
	switch(type){

	case OBD_TYPE_ISO:
	case OBD_TYPE_KWP:
		iLength=KWP_SendFrame(pSendFrame);
		break;
	case OBD_TYPE_VPW:
		iLength=VPW_SendFrame(pSendFrame);
		break;
	case OBD_TYPE_PWM:
		iLength=PWM_SendFrame(pSendFrame);
		break;
	case OBD_TYPE_CAN1:
	case OBD_TYPE_CAN2:
		//can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, determine_pid );
		//iLength=CAN_SendFrame(pSendFrame);
		  iLength=CAN_SendFrame( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, pSendFrame+1 );
		break;
	}
	return iLength;
}

int RecvFrame(u8 type,u8 *pRecvFrame)
{
	u8 os_err;
	int iLength=0;//系统时间 有问题
	
	while(1){
		
		switch(type){
		case OBD_TYPE_ISO:
			iLength=Normal_RecvFrame(pRecvFrame,50);
			break;
		case OBD_TYPE_KWP:
			iLength=KWP_RecvFrame(pRecvFrame);
			break;
		case OBD_TYPE_VPW:
			iLength=VPW_RecvFrame(pRecvFrame);
			break;
		case OBD_TYPE_PWM:
			iLength=PWM_RecvFrame(pRecvFrame);
			break;
		case OBD_TYPE_CAN1:
		case OBD_TYPE_CAN2:
			CAN_ITConfig(CAN1,CAN_IT_FMP0 | CAN_IT_FOV0, ENABLE);
			OSSemPend(sem_obd_prot, 1*OS_TICKS_PER_SEC, &os_err);//timeout: 1 seconds
			if(os_err)break;
			iLength=CAN_RecvFrame(pRecvFrame);
			break;
		}
		if(iLength==0)continue;
		else break;
	}
	my_icar.event = 0;
	return iLength;
}

void InitFrameBuffer(void)
{
	obd_fbuff.frame_count=0;
	obd_fbuff.frame_length=0;
}

void AddFrameToBuffer(u8 *buff,u16 len)
{
	if(obd_fbuff.frame_length+2+len>OBD_BUF_SIZE)return;
	obd_fbuff.frame_count++;
	obd_fbuff.frame_buff[obd_fbuff.frame_length++]=(u8)(len>>8);
	obd_fbuff.frame_buff[obd_fbuff.frame_length++]=(u8)(len>>0);
	memcpy(obd_fbuff.frame_buff+obd_fbuff.frame_length,buff,len);
	obd_fbuff.frame_length+=len;
}


void Icar_TranToSendBuf(u8 type,u8* Strp,u8* Sendbuf)
{

	u8* p;

	memset(&Sendbuf,0xAA,TX_BUF_SIZE);

	switch(type)
	{
		case  OBD_TYPE_CAN1		:
		case  OBD_TYPE_CAN2		:
		case  OBD_TYPE_CAN1_2	:
			break;
		case  OBD_TYPE_PWM		:	p = PWM_HEAD;		break;
		case  OBD_TYPE_VPW		:	p = ISO9141_HEAD;	break;
		case  OBD_TYPE_KWP		:	p = KWP2000_HEAD;	break;
		case  OBD_TYPE_ISO		:	p = KWP2000_HEAD;	break;

	}
	Sendbuf[0] = 0;
	if(type!=OBD_TYPE_CAN1){ memcpy(&Sendbuf[1],p,3);Sendbuf[0]= 3;}
    memcpy(&Sendbuf[1+Sendbuf[0]],Strp+1,* Strp);
	Sendbuf[0] +=* Strp;

	if(type==OBD_TYPE_KWP)Sendbuf[1] +=Sendbuf[0];	//KWP2000 的第一个指令值为 CO+有效数据长度
														//                    或者 80+有效数据长度														
 	switch(type)
	{
		case  OBD_TYPE_VPW		:	AddCRC8(Sendbuf);break;
		case  OBD_TYPE_PWM		:	AddCRC8(Sendbuf);break;
		case  OBD_TYPE_ISO	    :	AddLJH(Sendbuf);break;
		case  OBD_TYPE_KWP      :	AddLJH(Sendbuf);break;
	}
}


#define g_box_out_time 3*OS_TICKS_PER_SEC //3s ?
#define F_MUT 0xfc



u8 	Icar_SendRead(u8 type,u8* Strp)
{
	u8 buffer[16],CANflag=1;
	u16 len;			 
	u8 *pFrm=buffer;
 	u32 iClick=OSTime;
	u8 FRAMES[]="\x30\x00\x00\x00\x00\x00\x00\x00";

	Icar_TranToSendBuf(type,Strp,buffer);
	len=buffer[0];
	InitFrameBuffer();
	SendFrame(type,buffer);
	pFrm+=1+len;

	while(1){
		len=(u16)RecvFrame(type,buffer);
		if(len==0)break;
		if(((type==OBD_TYPE_CAN1) ||(type==OBD_TYPE_CAN2))&&len>13){AddFrameToBuffer(buffer,13);AddFrameToBuffer(buffer+13,13);}
		else AddFrameToBuffer(buffer,len);
		if(*pFrm!=(u8)F_MUT){  //
			if(OSTime-iClick>g_box_out_time)break;
		}
		if(((type==OBD_TYPE_CAN1) ||(type==OBD_TYPE_CAN2))&&((buffer[7]==0x10)||(buffer[7+13]==0x10))&&(CANflag)){ can_send( can_snd_id[obd_read_canid_idx], DAT_FRAME, 8, FRAMES);CANflag=0;}
		else break;
	}
	return obd_fbuff.frame_count;
}





bool Enquire_support_pid(u8 type)
{
	if(Icar_SendRead(type, ENTER_VPW)>3)return true;
	return false;
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


bool can_enter( can_speed_typedef can_spd,  can_std_typedef can_typ ,u8 CANIDTABLE){
	
	u8 var_uchar;
	CanRxMsg can_rx_msg;
	u8 determine_pid[]="\x02\x01\x00\x00\x00\x00\x00\x00";

	
	can_init( can_spd, can_typ );
	if(can_typ ==CAN_STD) var_uchar = can_add_filter( CAN_STD , STD_RCV_CAN_ID );//0x07E8,save to fifo0
	else var_uchar = can_add_filter( CAN_EXT, EXT_RCV_CAN_ID );//0x07E8,save to fifo0
	if ( var_uchar ==0) debug_obd("ERR: no free filter\r\n");
	
	//send CAN ID: 0x07DF
	
	OSTimeDlyHMSM(0, 0, 0, 50); //wait 50 ms
	can_send( can_snd_id[CANIDTABLE], DAT_FRAME, 8, determine_pid );
	OSTimeDlyHMSM(0, 0, 0, 50); //wait 50 ms
	
	if ( (CAN1->RF0R)&0x03 ) { //have data, setting correct
		CAN_Receive(CAN1,CAN_FIFO0, &can_rx_msg);
		if (can_rx_msg.Data[1]==0x41) { //have data, setting correct

			//debug_obd("CAN1,EXT_250 OK, FIFO0: %d\r\n",(CAN1->RF0R)&0x03);
			BKP_WriteBackupRegister(BKP_DR3, CANIDTABLE);//0x18DB10F1, save CAN ID index
			obd_set_type( OBD_TYPE_CAN1 ) ;
			can_set_speed(CAN_1, can_spd);
			return true;
		 }
		return false;
	}

	return false;
}

bool Obd_Enter_CAN(void)
{
	//	u8 obd_typ = 0 , var_uchar;
	//	u32 can_id ;

	//	debug_obd("CAN1,STD_250 1..\r\n");
	if(can_enter(CAN_250K, CAN_STD,1))return true;
	if(can_enter(CAN_250K, CAN_STD,2))return true;
	if(can_enter(CAN_250K, CAN_EXT,3))return true;
	if(can_enter(CAN_250K, CAN_EXT,4))return true;
	if(can_enter(CAN_500K, CAN_STD,1))return true;
	if(can_enter(CAN_500K, CAN_STD,2))return true;
	if(can_enter(CAN_500K, CAN_EXT,3))return true;
	if(can_enter(CAN_500K, CAN_EXT,4))return true;
	return false;

}
	
bool Obd_Enter_VPW(void)
{
   	if(Icar_SendRead(OBD_TYPE_VPW,ENTER_VPW)<3)return false;
	if(obd_fbuff.frame_buff[3]!=0x41)return false;
	obd_set_type(OBD_TYPE_VPW); 
	return true;
}

bool Obd_Enter_PWM(void)
{
	if(Icar_SendRead(OBD_TYPE_PWM,ENTER_VPW)<3)return false;
	if(obd_fbuff.frame_buff[3]!=0x41)return false;
	obd_set_type(OBD_TYPE_VPW); 
	return true;
}

bool Obd_Enter_KWP(void)
{

	kline_init();
	if (!KWPTryQuickInit())return false;
	obd_set_type(OBD_TYPE_KWP);
	return true;

}

u8 Obd_Enter_ISO(void)
{
	return ISOSendSlowInit();
}




void obd_auto_detect(void)
{
	//Request Supported PIDs from Vehicle, check sae j1979 2002.pdf page 26
	

	//u8 obd_typ = 0 , var_uchar;
	//u32 can_id ;
	
	//BKP_DR2, OBD Flag:	15~12:KWP_TYPEDEF
	//						11~8:CAN2_TYPEDEF
	//						7~4: CAN1_TYPEDEF
	//						3~0: OBD_TYPEDEF 
	//BKP_DR3, can_snd_id index, check app_obd.c
	
	prompt("Auto detect OBD ...\r\n");

	//if(Obd_Enter_CAN())return;
	//if(Obd_Enter_VPW())return;
	//if(Obd_Enter_PWM())return;
	prompt("Auto detect kwp ...\r\n");
	//if(Obd_Enter_KWP())return;
	prompt("Auto detect iso ...\r\n");
	if(Obd_Enter_ISO())return;
	prompt("Auto detect error ...\r\n");

	
}
/*
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
	return 0; //!= (my_icar.obd.support_pid[bytepos] & (0x80>>bitpos));
}
*/
