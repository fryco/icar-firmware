#include "main.h"

extern u8 *u1_tx_buf;//set to tx buffer before enable interrupt
extern u8 u1_rx_buf1[RX_BUF_SIZE];
extern u8 u1_rx_buf2[RX_BUF_SIZE];
extern u8 u1_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u1_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u1_rx_lost_data;//1: lost data
extern u8 u1_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u1_tx_cnt, u1_rx_cnt1, u1_rx_cnt2 ;

extern u8 *u2_tx_buf;//set to tx buffer before enable interrupt
extern u8 u2_rx_buf1[RX_BUF_SIZE];
extern u8 u2_rx_buf2[RX_BUF_SIZE];
extern u8 u2_rx_buf1_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf2_ava;//0: unavailable, 1: available
extern u8 u2_rx_buf_flag;//1: use buf1, 0: use buf2
extern u8 u2_rx_lost_data;//1: lost data
extern u8 u2_rec_binary; //0:receive bin, 1:char, default is char
extern u32 u2_tx_cnt, u2_rx_cnt1, u2_rx_cnt2 ;

void  App_TaskGsm (void *p_arg)
{
	u8 gsm_cmd[AT_CMD_LENGTH];
	u32  gsm_time;
	(void)p_arg;

	gsm_bus_init();
	prompt("Start GSM atfer 3 second. %s: %d\r\n",__FILE__, __LINE__);
	led_off(LED1); 
	OSTimeDlyHMSM(0, 0, 3, 0);
	
	while ( 1 )
	{
		u8 err ;
	
		err = gsm_connect_tcp() ;
		if (  err == 0 ) { //connected
			led_on(LED1);

			USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
			//send cmd: ZIPSEND	
			snprintf((char *)gsm_cmd, AT_CMD_LENGTH-1, "AT+ZIPSEND=1,11\r\n");

#ifdef DEBUG_GSM
			//send the gsm cmd char for debug
			u1_tx_buf = gsm_cmd ;
			u1_tx_cnt = strlen((char*)gsm_cmd);
			USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
			while ( u1_tx_cnt ) ;//wait send complete
#endif

			//clean u2 rec buffer
			if ( u2_rx_buf1_ava ) {//send buf1
				//send the receive char
				u1_tx_buf = u2_rx_buf1 ;
				u1_tx_cnt = u2_rx_cnt1;
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				while ( u1_tx_cnt ) ;//wait send complete
				u2_rx_cnt1 = 0 ;
				u2_rx_buf1_ava = 0 ;
			}
	
			if ( u2_rx_buf2_ava ) {//send buf2
				//send the receive char
				u1_tx_buf = u2_rx_buf2 ;
				u1_tx_cnt = u2_rx_cnt2;
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				while ( u1_tx_cnt ) ;//wait send complete
				u2_rx_cnt2 = 0 ;
				u2_rx_buf2_ava = 0 ;
			}

			//send the gsm cmd to gsm module
			u2_tx_buf = gsm_cmd ;
			u2_tx_cnt = strlen((char*)gsm_cmd);
			USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
			while ( u2_tx_cnt ) ;//wait send complete

			//prevent gsm module no respond for long time
			gsm_time = OSTime ;
			while( OSTime - gsm_time < GSM_TIMEOUT) {

				if ( u2_rx_lost_data ) {//error! lost data
					prompt("Lost data, check: %s: %d\r\n",__FILE__, __LINE__);
					while ( 1 ) ;
				}

				//except ">"
				if ( u2_rx_buf1_ava ) { //using buf1 rec
					if ( u2_rx_buf1[0] == '>' ) {//correct char
						//gsm ready, send data...
						snprintf((char *)gsm_cmd, AT_CMD_LENGTH-1,"L%010d\r\n",OSTime);
	
#ifdef DEBUG_GSM
						//send the gsm cmd char for debug
						u1_tx_buf = gsm_cmd ;
						u1_tx_cnt = strlen((char*)gsm_cmd);
						USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
						while ( u1_tx_cnt ) ;//wait send complete
#endif
	
						//send the gsm cmd to gsm module
						u2_tx_buf = gsm_cmd ;
						u2_tx_cnt = strlen((char*)gsm_cmd);
						USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
						while ( u2_tx_cnt ) ;//wait send complete
						u2_rx_cnt1 = 0 ;
						u2_rx_buf1_ava = 0 ;
						break ;
					}
					else { //un-expect data, just show it
						//send the gsm cmd response
						u1_tx_buf = u2_rx_buf1 ;
						u1_tx_cnt = u2_rx_cnt1;
						USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
						while ( u1_tx_cnt ) ;//wait send complete
						u2_rx_cnt1 = 0 ;
						u2_rx_buf1_ava = 0 ;
					}
				}

				if ( u2_rx_buf2_ava ) { //using buf2 rec
					if ( u2_rx_buf2[0] == '>' ) {//correct char
						//gsm ready, send data...
						snprintf((char *)gsm_cmd, AT_CMD_LENGTH-1,"L%010d\r\n",OSTime);

#ifdef DEBUG_GSM
						//send the gsm cmd char for debug
						u1_tx_buf = gsm_cmd ;
						u1_tx_cnt = strlen((char*)gsm_cmd);
						USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
						while ( u1_tx_cnt ) ;//wait send complete
#endif

						//send the gsm cmd to gsm module
						u2_tx_buf = gsm_cmd ;
						u2_tx_cnt = strlen((char*)gsm_cmd);
						USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
						while ( u2_tx_cnt ) ;//wait send complete
						u2_rx_cnt2 = 0 ;
						u2_rx_buf2_ava = 0 ;
						break ;
					}
					else { //un-expect data, just show it
						//send the gsm cmd response
						u1_tx_buf = u2_rx_buf2 ;
						u1_tx_cnt = u2_rx_cnt2;
						USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
						while ( u1_tx_cnt ) ;//wait send complete
						u2_rx_cnt2 = 0 ;
						u2_rx_buf2_ava = 0 ;
					}
				}
			} //end of while( OSTime - gsm_time < 600)

			USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
			//clean u2 rec buffer
			if ( u2_rx_buf1_ava ) {//send buf1
				//send the receive char
				u1_tx_buf = u2_rx_buf1 ;
				u1_tx_cnt = u2_rx_cnt1;
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				while ( u1_tx_cnt ) ;//wait send complete
				u2_rx_cnt1 = 0 ;
				u2_rx_buf1_ava = 0 ;
			}
	
			if ( u2_rx_buf2_ava ) {//send buf2
				//send the receive char
				u1_tx_buf = u2_rx_buf2 ;
				u1_tx_cnt = u2_rx_cnt2;
				USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
				while ( u1_tx_cnt ) ;//wait send complete
				u2_rx_cnt2 = 0 ;
				u2_rx_buf2_ava = 0 ;
			}

			if ( u2_rx_lost_data ) {//error! lost data
				prompt("Lost data, check: %s: %d\r\n",__FILE__, __LINE__);
				while ( 1 ) ;
			}

			prompt("Online, will be close after 1 second...\r\n"); 
			OSTimeDlyHMSM(0, 0, 1, 0);

			//disconnect gsm
			err = gsm_disconn_tcp();
			if (  err == 0 ) { //disconnect ok
				prompt("Disconnect ok, will be shutdown GSM after 1 second...\r\n");
				OSTimeDlyHMSM(0, 0, 1, 0);
			}
			else { //disconnect failure
				prompt("gsm_disconn_tcp failure. %s :%d\r\n",__FILE__, __LINE__);
				prompt("Error code is :%d\r\n\r\n",err);
			}
	
			led_off(LED1); 
			//power off module
			prompt("ME3000 power off ...\r\n");
		    if ( gsm_pwr_off( ) ) { 
			  prompt("ME3000 power off ok.\r\n");
			}
			else {
			  prompt("ME3000 power off failure!\r\n");
			}
		
			prompt("GSM power off, will be re-start after 10 second...\r\n");
			OSTimeDlyHMSM(0, 0, 10, 0);

		}
		else { //connect tcp failure

			if ( err == 4 ) {//module no respond, had beed reset
				prompt("Module shutdown completed, continue...\r\n");
			}
			else { //others error
				prompt("gsm_connect_tcp failure. %s :%d\r\n",__FILE__, __LINE__);
				prompt("Error code is :%d\r\n\r\n",err);

				led_off(LED1); 
				//power off module
				prompt("ME3000 power off ...\r\n");
			    if ( gsm_pwr_off( ) ) { 
				  prompt("ME3000 power off ok.\r\n");
				}
				else {
				  prompt("ME3000 power off failure!\r\n");
				}
			
				prompt("GSM power off, will be re-start after 5 second...\r\n");
				OSTimeDlyHMSM(0, 0, 5, 0);
			}
		}
	}
}
