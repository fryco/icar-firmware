#ifndef __PROT_H__
#define __PROT_H__

//Ð­Òé
#define PT_NORMAL		0x00
#define PT_KWP			0x01
#define PT_ISO			0x02
#define PT_VPW			0x03
#define PT_PWM			0x04
#define PT_BOSCH		0x05
#define PT_CCD			0x06
#define PT_CAN			0x07
#define PT_VOLVO_CAN	0x08
#define PT_TP20_CAN		0x09
#define PT_NISSAN_OLD	0x10
#define PT_SINGEL_CAN	0x11
#define PT_J1708		0x12
#define PT_J1587		0x13
#define PT_WABCOABS		0x14
#define PT_BMW_CAN		0x15
#define PT_BMW_MODE2	0x16
#define PT_BMW_MODE3	0x17
#define PT_VOLVO_KWP	0x18
//¹ýÂË
#define FILTER_NORMAL	0x00
#define FILTER_TIMEOUT	0x01
#define FILTER_KEYWORD	0x02
#define FILTER_FIXLEN 	0x03
#define FILTER_SPCLEN 	0x04
#define FILTER_J1708 	0x05

int SendToEcu(u8* pBuffer, int iLength);
int ReceiveFromEcu(u8 *pBuffer, int iLength,int iTimeout);

int TP20_RecvFrame(u8* pRecvFrame);

int VPW_SendFrame (unsigned char* pSendFrame);
int VPW_RecvFrame(unsigned char* pRecvFrame);

int PWM_SendFrame (unsigned char* pSendFrame);
int PWM_RecvFrame(unsigned char* pRecvFrame);

//int CAN_SendFrame (u8* pSendFrame);
//int CAN_RecvFrame(u8* pRecvFrame);
//u8 CAN_Polling(void);
//void CAN_Configuration(u32 bps);
//u8 CAN_SetFilter(u8 *param,u16 len);

int KWP_SendFrame(u8 *pSendFrame);
int KWP_RecvFrame(u8 *pRecvFrame);
//int VOLVO_KWP_RecvFrame(u8 *pRecvFrame);

//int BOSCH_SendFrame(u8 *pSendFrame);
//int BOSCH_RecvFrame(u8 *pRecvFrame);

int Normal_SendFrame (u8* pSendFrame);
int Normal_RecvFrame(u8* pRecvFrame,u16 timeout);

//int WABCO_SendFrame (u8* pSendFrame);
//int WABCO_RecvFrame(u8* pRecvFrame);

//int SendFrame(u8 *pSendFrame);
//int RecvFrame(u8 *pRecvFrame);
int SendFrame(u8 type,u8 *pSendFrame);
int RecvFrame(u8 type,u8 *pRecvFrame);
bool KWPTryQuickInit(void);
bool ISOSendSlowInit(void);

u8 IsBusyFrame(u8 *pRecvFrame,int iLength);

#endif
