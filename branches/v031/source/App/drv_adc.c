#include "main.h"

struct ICAR_ADC adc_temperature;

//ADC,�ڲ��¶ȴ���������
void ADCTEMP_Configuration(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	adc_temperature.completed = false;

	/* ���� DMA1 */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* DMAͨ��1*/
	DMA_DeInit(DMA1_Channel1);
	//ָ��DMA�������ַ
	DMA_InitStructure.DMA_PeripheralBaseAddr =(u32)( &(ADC1->DR));		//ADC1���ݼĴ���
	//�趨DMA�ڴ����ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)adc_temperature.converted;//��ȡADC������
	//������Ϊ���ݴ������Դ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;					//Ƭ��������Դͷ
	//ָ��DMAͨ����DMA�����С
	DMA_InitStructure.DMA_BufferSize = ADC_BUF_SIZE;					//ÿ��DMA ������
	//�����ַ�����������䣩
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ������
	//�ڴ��ַ�����������䣩
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ����
	//�趨�������ݿ��Ϊ16λ
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//����
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//����
	//�趨DMA�Ĺ���ģʽ��ͨģʽ������һ����ѭ��ģʽ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//��ͨģʽ
	//�趨DMAͨ����������ȼ�
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	//ʹ��DMA�ڴ浽�ڴ�Ĵ��䣬�˴�û���ڴ浽�ڴ�Ĵ���
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;								//���ڴ浽�ڴ�
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);	//DMAͨ��3��������ж�

  	/* ����ADC */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	/* ADC1 configuration ------------------------------------------------------*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;						//����ģʽ
	//Scan (multichannels) or Single (one channel) mode.
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;							//��ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;						//����ɨ��
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;		//�������ת��
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;					//�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;									//1��ͨ��
	ADC_Init(ADC1, &ADC_InitStructure);
	
	/* ����ͨ��16�Ĳ����ٶ�,������Ϊ�ǲ���,����Ҫ�ܿ�,����Ϊ����*/ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
	
	/* ʹ���ڲ��¶ȴ��������ڲ��Ĳο���ѹ */ 
	ADC_TempSensorVrefintCmd(ENABLE); 
	
	/* Enable ADC1 DMA */
	ADC_DMACmd(ADC1, ENABLE);
	
	/* ����ADC1*/
	ADC_Cmd(ADC1, ENABLE);
	
	/*����У׼�Ĵ��� */   
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	
	/*��ʼУ׼״̬*/
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	   
	/* �˹���ADCת��.*/ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/**************************************************************
** ������:DigitFilter
** ����:����˲�
** ע������:ȡNO��2/5��Ϊͷβ����ֵ,ע��NҪ����5,���򲻻�ȥͷβ
***************************************************************/
u16 digit_filter(u16* buf,u8 no)
{
	u8 i,j;
	u16 tmp;
	u8 cut_no=0;

	//���򣬽�buf[0]��buf[no-1]�Ӵ�С����
	for(i=0;i<no;i++){
		for(j=0;j<no-i-1;j++){	
			if(buf[j]>buf[j+1])	{	
				tmp=buf[j];
				buf[j]=buf[j+1];
				buf[j+1]=tmp;
			}
		}
	}

	if(no>5)//noΪ���Σ��˴��ǽ�no��ǰ2/5����
	{
		cut_no=no/5;
	}

	//ƽ��
	tmp=0;
	for(i=cut_no;i<no-cut_no;i++)	//ֻȡ�м�n-2*cut_no����ƽ��
		tmp+=buf[i];

	return(tmp/(no-2*cut_no));

}
