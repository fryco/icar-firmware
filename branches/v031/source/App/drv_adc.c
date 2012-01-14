#include "main.h"

struct ICAR_ADC adc_temperature;

//ADC,内部温度传感器配置
void ADCTEMP_Configuration(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	DMA_InitTypeDef DMA_InitStructure;

	adc_temperature.completed = false;

	/* 允许 DMA1 */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	/* DMA通道1*/
	DMA_DeInit(DMA1_Channel1);
	//指定DMA外设基地址
	DMA_InitStructure.DMA_PeripheralBaseAddr =(u32)( &(ADC1->DR));		//ADC1数据寄存器
	//设定DMA内存基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)adc_temperature.converted;//获取ADC的数组
	//外设作为数据传输的来源
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;					//片内外设作源头
	//指定DMA通道的DMA缓存大小
	DMA_InitStructure.DMA_BufferSize = ADC_BUF_SIZE;					//每次DMA 个数据
	//外设地址不递增（不变）
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//外设地址不增加
	//内存地址不递增（不变）
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//内存地址增加
	//设定外设数据宽度为16位
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//半字
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//半字
	//设定DMA的工作模式普通模式，还有一种是循环模式
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;								//普通模式
	//设定DMA通道的软件优先级
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	//使能DMA内存到内存的传输，此处没有内存到内存的传输
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;								//非内存到内存
	DMA_Init(DMA1_Channel1, &DMA_InitStructure);
	
	DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);	//DMA通道3传输完成中断

  	/* 允许ADC */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	/* ADC1 configuration ------------------------------------------------------*/
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;						//独立模式
	//Scan (multichannels) or Single (one channel) mode.
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;							//单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;						//连续扫描
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;		//软件启动转换
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;					//数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;									//1个通道
	ADC_Init(ADC1, &ADC_InitStructure);
	
	/* 配置通道16的采样速度,这里因为是测温,不需要很快,配置为最慢*/ 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
	
	/* 使能内部温度传感器和内部的参考电压 */ 
	ADC_TempSensorVrefintCmd(ENABLE); 
	
	/* Enable ADC1 DMA */
	ADC_DMACmd(ADC1, ENABLE);
	
	/* 允许ADC1*/
	ADC_Cmd(ADC1, ENABLE);
	
	/*重置校准寄存器 */   
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	
	/*开始校准状态*/
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	   
	/* 人工打开ADC转换.*/ 
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/**************************************************************
** 函数名:DigitFilter
** 功能:软件滤波
** 注意事项:取NO的2/5作为头尾忽略值,注意N要大于5,否则不会去头尾
***************************************************************/
u16 digit_filter(u16* buf,u8 no)
{
	u8 i,j;
	u16 tmp;
	u8 cut_no=0;

	//排序，将buf[0]到buf[no-1]从大到小排列
	for(i=0;i<no;i++){
		for(j=0;j<no-i-1;j++){	
			if(buf[j]>buf[j+1])	{	
				tmp=buf[j];
				buf[j]=buf[j+1];
				buf[j+1]=tmp;
			}
		}
	}

	if(no>5)//no为整形，此处是将no的前2/5丢掉
	{
		cut_no=no/5;
	}

	//平均
	tmp=0;
	for(i=cut_no;i<no-cut_no;i++)	//只取中间n-2*cut_no个求平均
		tmp+=buf[i];

	return(tmp/(no-2*cut_no));

}
