#include "init.h"
#include "packet.h"
#include "queue.h"
#include "dlc.h"
#include "stdio.h"
#include "debug.h"
#include "util.h"
#include "string.h"
#include "box.h"
#include "prot.h"

#define FC_NORMAL														  0x01
#define FC_2BITS														  0x02
#define FC_KOEO														  	  0x03
#define FLASH_CODE_MAX_BYTE_NUMBER										  0x08

#define QFC_TOYOTA														  0x01

#define EC_OK															0 	//执行成功
#define EC_OVERTIME														-1	//0xFF 通信超时
#define EC_RUN_BREAK													-2	//0xFE 执行被中断
#define EC_ECU_NO_RESPONSION											-3	//0xFD ECU无应答

#define EC_IO_VOLTAGE													-11	// 0xF5 电压不匹配
#define EC_BUFFER_OVER													-12	// 0xF4 缓冲区溢出
#define EC_INVALIDATION_PARAMETER                                       -13 // 0xF3 无效参数
#define EC_FLASH_ERASE_FAILURE											-14 // 0xF2 擦除FLASH失败
#define EC_FLASH_WRITE_PROTECTED										-15 // 0xF1 写保护
#define EC_WRITE_FLASH_FAILURE											-16 // 0xF0 写FLASH 失败
#define EC_RUN_INVALIDATION_PROGRAM_PAGE								-17 // 0xEF 运行无效程序页面
#define EC_CAN_IO_ERROR													-18 // 0xEE CAN OPEN ERROR
#define EC_CAN_SEND_ERROR												-19 // 0xFD CAN SEND ERROR
#define EC_CAN_RECV_LEN_ERROR											-20 // 0xFC 数据长度超长错误。
#define EC_CAN_NOANWSER_ERROR											-21 // 0xFB 没有接受数据
#define EC_CAN_INVALIDATION_BITTIMING									-22 // 0xFA 无效位时间设定
#define EC_DATA_OVER													-23 // 0xF9 数据溢出
#define EC_NO_PULSE_FOUND												-24 // 0xF8 没有期待的脉冲出现
#define EC_NO_IDLESSE													-25 // 0xF7 未找到空闲电平
#define EC_UNDEFINDED_CMD

static char s_chErrorCode=EC_OK;
#define ClearError() 				s_chErrorCode = EC_OK
#define GetErrorCode() 				s_chErrorCode
#define SetErrorCode(iErrorCode)  	s_chErrorCode = (char)iErrorCode


#define ONE_MILLISECOND_CLOCK_TIMES	1
#define ResetTimerCount	   	   ResetCounter
#define GetTimerClick	   	   GetCounter
#define IsTimerCountOut(iMillisecend) 	(GetTimerClick() >= (iMillisecend)*ONE_MILLISECOND_CLOCK_TIMES)

u8 WaitNoPulseUntilTime(unsigned int uiNoPulseTime, unsigned int uiOverTime)
{
	u8 bReceiveLevel = MCU_K_RECEIVE_SIGNAL;
	unsigned long ulExitClick = GetTimerClick()+ONE_MILLISECOND_CLOCK_TIMES*uiNoPulseTime;
	
	ResetTimerCount();
	while(1){
		if(bReceiveLevel != MCU_K_RECEIVE_SIGNAL){
			ulExitClick = GetTimerClick()+ONE_MILLISECOND_CLOCK_TIMES*uiNoPulseTime;
			bReceiveLevel = ~bReceiveLevel;
		}
		else {
			if(GetTimerClick()>ulExitClick)
				break;
			else if(IsNewCommand()){
				SetErrorCode(EC_RUN_BREAK);
				break;
			}
			else if(IsTimerCountOut(uiOverTime)){
				SetErrorCode(EC_NO_IDLESSE);
				break;
			}
			else{
				__nop();
				__nop();
				__nop();
				__nop();
				__nop();
			}
		}
	}
	
	return bReceiveLevel;
}
unsigned long GetPulseTime(unsigned int uiOverTime)
{
	u8 bReceiveLevel = MCU_K_RECEIVE_SIGNAL;

	unsigned long ulLastTimerClock=GetTimerClick();
	ResetTimerCount();
	while(1){
		if(bReceiveLevel != MCU_K_RECEIVE_SIGNAL){
			break;
		}
		else if(IsNewCommand()){
			SetErrorCode(EC_RUN_BREAK);
			break;
		}
		else if(IsTimerCountOut(uiOverTime)){
			SetErrorCode(EC_NO_IDLESSE);
			break;
		}
		else{
			__nop();
			__nop();
			__nop();
			__nop();
			__nop();
		}
	}
	return (GetTimerClick()-ulLastTimerClock)/ONE_MILLISECOND_CLOCK_TIMES;
}
static u8 FilterFlashCodeReturnVoltageLevel(unsigned long dwFilterBuffer)
{
	u8 i,bRet = 0;
	u8 nNumber = 0;
	
	for(i=0; i< sizeof(dwFilterBuffer); i++){
		if(dwFilterBuffer&(0x0001<<i))
			++nNumber;
	}
	if(nNumber>sizeof(unsigned long)/2)
		bRet = 1;
		
	return bRet;
}
static int NormalFlashCode(u8* param)
{
	unsigned int uiBitIntevalMaxTime = (param[0]<<8)|param[1];
	unsigned int uiByteIntevalMaxTime =(param[2]<<8)|param[3];
	unsigned int uiCodeIntevalMaxTime =(param[4]<<8)|param[5];
	unsigned int uiMaxLengthFlashCodeFinishTime =(param[6]<<8)|param[7];

	char nCurrentFlashBytePosition;
	u8 aucFlashBuffer[FLASH_CODE_MAX_BYTE_NUMBER+1];
		
	nCurrentFlashBytePosition = 0;
	memset(aucFlashBuffer, 0, sizeof(aucFlashBuffer));

	
	do {
		unsigned long dwFilterBuffer = 0;
		u8 bitNormalVoltageLevel;

		u8 bAlreadyReadFlag = 1;
		unsigned long ulLastVoltageChangeClock;
		
		
		bitNormalVoltageLevel = WaitNoPulseUntilTime(uiByteIntevalMaxTime, uiCodeIntevalMaxTime+uiMaxLengthFlashCodeFinishTime);
		if(GetErrorCode())break;
		if(bitNormalVoltageLevel)
			dwFilterBuffer = 0xFFFFFFFF;
			
		ulLastVoltageChangeClock = GetTimerClick();
		
		while(1){
			u8 bLastAlreadyReadFlag;
			unsigned int uiContinuedTime;
			
			//Cmd break
			if(IsNewCommand()){
				SetErrorCode(EC_RUN_BREAK);
				nCurrentFlashBytePosition = 0;
				break;
			}
			
			//get flash bit data
			dwFilterBuffer = dwFilterBuffer << 1;
			dwFilterBuffer |= MCU_K_RECEIVE_SIGNAL;	

			//get pass time
			uiContinuedTime = (GetTimerClick()-ulLastVoltageChangeClock)/ONE_MILLISECOND_CLOCK_TIMES;

			//finish one bit
			bLastAlreadyReadFlag = bAlreadyReadFlag;
			if(bitNormalVoltageLevel == FilterFlashCodeReturnVoltageLevel(dwFilterBuffer)){
				if(0 == bAlreadyReadFlag){
					if(++aucFlashBuffer[nCurrentFlashBytePosition] == 0xFF){
						SetErrorCode(EC_DATA_OVER);
						break;
					}
					bAlreadyReadFlag = 1;
				}
			}
			else{
				bAlreadyReadFlag = 0;
			}
			
			if(bLastAlreadyReadFlag != bAlreadyReadFlag){	//voltage level changed
				ulLastVoltageChangeClock = GetTimerClick();
				continue;
			}
			
			if(uiContinuedTime <= uiBitIntevalMaxTime)
				continue;
			
			//finish one byte
			if(0 != aucFlashBuffer[nCurrentFlashBytePosition]){
				if(++nCurrentFlashBytePosition >= FLASH_CODE_MAX_BYTE_NUMBER){
					SetErrorCode(EC_BUFFER_OVER);
					break;
				}
				aucFlashBuffer[nCurrentFlashBytePosition] = 0;
			}
			
			if(uiContinuedTime <= uiByteIntevalMaxTime)
				continue;


			//finish one code
			if(nCurrentFlashBytePosition>0){
				AddFrameToBuffer(aucFlashBuffer, nCurrentFlashBytePosition);
				
				nCurrentFlashBytePosition = 0;
				aucFlashBuffer[nCurrentFlashBytePosition] = 0;
			}
			
			//error
			if(uiContinuedTime>uiCodeIntevalMaxTime){
				if(0==nCurrentFlashBytePosition)
					SetErrorCode(EC_NO_PULSE_FOUND);
				else
					SetErrorCode(EC_OVERTIME);
				break;
			}
		}
		break;
	}while(1);

EXIT:	
	return 2;
}
static int FlashCode2Bits(u8* param)
{
	unsigned int uiBitIntevalMaxTime = (param[0]<<8)|param[1];
	unsigned int uiByteIntevalMaxTime =(param[2]<<8)|param[3];
	unsigned int uiCodeIntevalMaxTime =(param[4]<<8)|param[5];
	unsigned int uiMaxLengthFlashCodeFinishTime =(param[6]<<8)|param[7];
 	unsigned char uiFirstCode=0,uiFlashCode=0,temp[2];
	unsigned long ulIntevalTime;

	while(1){
		ulIntevalTime=GetPulseTime(uiCodeIntevalMaxTime+uiMaxLengthFlashCodeFinishTime);
		if(GetErrorCode())goto EXIT;
		if(ulIntevalTime>uiCodeIntevalMaxTime)break;
	}
	while(1){
		ulIntevalTime=GetPulseTime(uiCodeIntevalMaxTime+uiMaxLengthFlashCodeFinishTime);
		if(GetErrorCode())goto EXIT;
		if(ulIntevalTime>uiByteIntevalMaxTime){
			uiFlashCode+=10;
		}
		else if(ulIntevalTime>uiBitIntevalMaxTime){
			if(uiFlashCode%10==9){//无码
				temp[0]=temp[1]=0;
				AddFrameToBuffer(temp,2);
				goto EXIT;
			}
			uiFlashCode+=1;
		}
		ulIntevalTime=GetPulseTime(uiCodeIntevalMaxTime+uiMaxLengthFlashCodeFinishTime);
		if(GetErrorCode())goto EXIT;		
		if(ulIntevalTime>uiCodeIntevalMaxTime){
			if(uiFlashCode==uiFirstCode)goto EXIT; //完成
			if(uiFirstCode==0)uiFirstCode=uiFlashCode;//记住第一个码
			temp[0]=uiFlashCode/10;
			temp[1]=uiFlashCode%10;
			AddFrameToBuffer(temp,2);
			uiFlashCode=0;
		}
	}
EXIT:		
	return 2;
}
static int FlashCodeKoeo(u8* param)
{
/*	unsigned int uiBitIntevalMaxTime = (param[0]<<8)|param[1];
	unsigned int uiByteIntevalMaxTime =(param[2]<<8)|param[3];
	unsigned int uiCodeIntevalMaxTime =(param[4]<<8)|param[5];
	unsigned int uiMaxLengthFlashCodeFinishTime =(param[6]<<8)|param[7];
*/	return 3;
}

//调用该函数前必须先设定125bps
static int ToyotaQuickFlashCode(u8* param)
{
	#define ONE_BIT_WIDTH                          8
	#define FLASH_CODE_LENGTH                      12
	#define ID_BIT_NUMBER                          12
	
	u8 bRepeat = *param;
	
	u16 wId = 0;
	u8 aucContainBuffer[FLASH_CODE_LENGTH+1];
	
	unsigned long ulTimeClock;
	unsigned long ulExitClock;

	do{
		char i;
		u8 bitNormalVoltageLevel;
	
		if(IsNewCommand()){
			SetErrorCode(EC_RUN_BREAK);
			break;
		}
//加入50bit的冗余时间 11/27/2006		
//		bitNormalVoltageLevel = WaitNoPulseUntilTime(ONE_BIT_WIDTH*11, ONE_BIT_WIDTH*(164+10)*2);
		bitNormalVoltageLevel = WaitNoPulseUntilTime(ONE_BIT_WIDTH*11, ONE_BIT_WIDTH*(164+10+50)*2);
		if(GetErrorCode())break;
		
		if(0==bitNormalVoltageLevel){
			SetErrorCode(EC_IO_VOLTAGE);
			break;
		}
			
		ulTimeClock = GetTimerClick();
//加入50bit的冗余时间 11/27/2006，同上面等待时间。		
//		ulExitClock = ulTimeClock+ONE_MILLISECOND_CLOCK_TIMES*10*8;
		ulExitClock = ulTimeClock+ONE_MILLISECOND_CLOCK_TIMES*(10+50)*8;
		while(MCU_K_RECEIVE_SIGNAL){
			if(GetTimerClick()>ulExitClock){
				SetErrorCode(EC_NO_PULSE_FOUND);
				break;
			}
		}
		if(GetErrorCode())break;	
		//Get ID
		Delay(3+8);	
		ulTimeClock = GetTimerClick();
		for(i=0; i<ID_BIT_NUMBER; i++){
			while( (GetTimerClick()-ulTimeClock) < ONE_BIT_WIDTH*ONE_MILLISECOND_CLOCK_TIMES*i);
			if(MCU_K_RECEIVE_SIGNAL)wId |= (0x8000>>i);
		}		
		//Get flash code
		if(ReceiveFromEcu(aucContainBuffer, FLASH_CODE_LENGTH,g_box.out_b2b)!=FLASH_CODE_LENGTH){
			SetErrorCode(EC_OVERTIME);
			break;
		}	
		AddFrameToBuffer((u8*)&wId, sizeof(wId));
		AddFrameToBuffer(aucContainBuffer, FLASH_CODE_LENGTH);		
		SendPacket(g_box.frame_count,g_box.frame_buff,g_box.frame_length);
		InitFrameBuffer();
		if(!bRepeat)break;
	}while(1);
	
	if(GetErrorCode())return 2;
	return 3;
}
u8 FlashCode(u8* param,u16 len)
{
	u8 ret=3;
	switch(*param){
	case FC_NORMAL:
		ret=NormalFlashCode(param+1);
		break;
	case FC_2BITS:
		ret=FlashCode2Bits(param+1);
		break;
	case FC_KOEO:
		ret=FlashCodeKoeo(param+1);
		break;
	default:
		break;
	}
	return ret;
}
u8 QuickFlashCode(u8 *param,u16 len)
{
	u8 ret=3;
	switch(*param){
	case QFC_TOYOTA:
		ret=ToyotaQuickFlashCode(param+1);
		break;
	default:
		break;
	}
	return ret;
}
