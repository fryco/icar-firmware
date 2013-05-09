//$URL$ 
//$Rev$, $Date$
//fprintf(stderr, "rokko: %X\tbuf: %X\t%s:%d\n",rokko,rec_buf,__FILE__,__LINE__);

#include "config.h"
#include "rokkod.h"

extern unsigned int foreground;


/****************** Below for server respond console command *********/
//return 0: ok, others: err
int console_list_all( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf, \
				struct rokko_data *rokko_all, unsigned int conn_amount )
{//case CONSOLE_CMD_LIST : //list all or special active client

	unsigned char *console_buf, log_str[EMAIL+1];
	unsigned char var_u8;
	unsigned short sn_cnt = 0, var_u16, crc16;
	unsigned int data_len ;
	
	console_buf = NULL;

	if ( conn_amount > MAXCLIENT ) { //err
		return 1;
	}

	if ( foreground ) {
		fprintf(stderr, "List all active client:\n");
	}
	
	//estimate buffer size
	for (var_u16 = 0; var_u16 < MAXCLIENT; var_u16++) {
		if ((rokko_all[var_u16].client_socket != 0) && rokko_all[var_u16].login_cnt ) {
			//exist connection and login
			sn_cnt++;
		}
	}
	data_len = (sn_cnt*8)+20; //buffer size for console
	console_buf = (unsigned char*)malloc(data_len);
	if(console_buf==NULL){
		exit(1);
	}
	fprintf(stderr, "Apply %d buf ok\n",data_len);
	
	bzero( console_buf, data_len);//send all SN to console

	console_buf[0] = GSM_HEAD ;
	console_buf[1] = cmd->seq ;
	console_buf[2] = cmd->pcb | 0x80 ;

	console_buf[5] =  ERR_RETURN_NONE; //ok

	//format: buf[6] total client SN in this frame
	console_buf[6] =  (sn_cnt>>8)&0xFF; //SN count high
	console_buf[7] =  (sn_cnt)&0xFF;; //SN count low

	sn_cnt = 0 ;
	//buf[8]~[15], SN1, 8 Bytes
	for (var_u16 = 0; var_u16 < MAXCLIENT; var_u16++) {
		if ((rokko_all[var_u16].client_socket != 0) && rokko_all[var_u16].login_cnt ) {
			//exist connection and login, copy SN to console buf
			sn_cnt++;
			for ( var_u8 = 0 ; var_u8 < PRODUCT_SN_LEN ; var_u8++ ) {
				console_buf[((sn_cnt<<3)+var_u8)] = rokko_all[var_u16].sn_short[var_u8];
				//fprintf(stderr, "buf[%02d]=%02X\n",(sn_cnt<<3)+var_u8,console_buf[((sn_cnt<<3)+var_u8)]);
			}
		}
	}			

	//update buf length
	data_len = (sn_cnt<<3) + 3 ;
	//maybe overflow, ingore this value in console
	console_buf[3] =  data_len>>8 & 0xFF;//len high
	console_buf[4] =  data_len & 0xFF;//len low

	//Calc CRC16
	crc16 = 0xFFFF & (crc16tablefast(console_buf , data_len+5));

	console_buf[data_len+5] = (crc16)>>8 ;
	console_buf[data_len+6] = (crc16)&0xFF ;

	if ( foreground ) {
		fprintf(stderr, "Console CMD: L, %d Bytes to %s\n",\
				data_len+7,rokko->sn_long);
		for ( crc16 = 0 ; crc16 < 16 ; crc16++ ) {
			fprintf(stderr, "%02X ",console_buf[crc16]);
		}
		fprintf(stderr, "\n");
	}


	//buf[0] = GSM_HEAD
	//buf[1] = SEQ
	//buf[2] = CMD
	//buf[5] = ERR_RETURN_NONE; //ok
	
	if ( (console_buf[0] != GSM_HEAD) || console_buf[5] != 0 ) {
		char log_str[EMAIL+1];
		
		bzero(log_str, sizeof(log_str));
		snprintf(log_str,sizeof(log_str),"!!!ERR!!! %02X,%02X,%02X, Len:%d @ %s:%d\n",\
				console_buf[0],console_buf[1],console_buf[5],data_len+7,__FILE__,__LINE__);
		log_err(log_str);		
		if ( foreground ) fprintf(stderr,"%s",log_str);
		return 1 ;
	}
	else {
		write(rokko->client_socket,console_buf,data_len+7);

		//save transmit count
		rokko->tx_cnt += data_len+7 ;
		
		if(console_buf != NULL) free(console_buf);
		console_buf = NULL;

		return 0 ;
	}
}
/*Below is for Fork
{//case CONSOLE_CMD_LIST : //list all or special active client

	pid_t pid;

	if ( conn_amount > MAXCLIENT ) { //err
		return 1;
	}
	
	pid=fork();

	if (pid < 0) { //fork error
		fprintf(stderr, "Fork err @ %s:%d\n",__FILE__,__LINE__);
		return 1;
	}
	else { 
		if (pid == 0) {//In child process
			if ( foreground ) {
				fprintf(stderr, "List all active client:\n");
			}
			
			unsigned char *console_buf, log_str[EMAIL+1];
			unsigned char var_u8;
			unsigned short sn_cnt = 0, var_u16, crc16, data_len;
			
			console_buf = NULL;
			//estimate buffer size
			for (var_u16 = 0; var_u16 < MAXCLIENT; var_u16++) {
				if ((rokko_all[var_u16].client_socket != 0) && rokko_all[var_u16].login_cnt ) {
					//exist connection and login
					sn_cnt++;
				}
			}
			data_len = (sn_cnt*8)+20; //buffer size for console
			console_buf = (unsigned char*)malloc(data_len);
			if(console_buf==NULL){
				exit(1);
			}
			fprintf(stderr, "Apply %d buf ok\n",data_len);
			
			bzero( console_buf, data_len);//send all SN to console

			console_buf[0] = GSM_HEAD ;
			console_buf[1] = cmd->seq ;
			console_buf[2] = cmd->pcb | 0x80 ;

			console_buf[5] =  ERR_RETURN_NONE; //ok

			//format: buf[6] total client SN in this frame
			console_buf[6] =  (sn_cnt>>8)&0xFF; //SN count high
			console_buf[7] =  (sn_cnt)&0xFF;; //SN count low

			sn_cnt = 0 ;
			//buf[8]~[15], SN1, 8 Bytes
			for (var_u16 = 0; var_u16 < MAXCLIENT; var_u16++) {
				if ((rokko_all[var_u16].client_socket != 0) && rokko_all[var_u16].login_cnt ) {
					//exist connection and login, copy SN to console buf
					sn_cnt++;
					for ( var_u8 = 0 ; var_u8 < PRODUCT_SN_LEN ; var_u8++ ) {
						console_buf[((sn_cnt<<3)+var_u8)] = rokko_all[var_u16].sn_short[var_u8];
						//fprintf(stderr, "buf[%02d]=%02X\n",(sn_cnt<<3)+var_u8,console_buf[((sn_cnt<<3)+var_u8)]);
					}
				}
			}			

			//update buf length
			data_len = (sn_cnt<<3) + 3 ;
			//maybe overflow, ingore this value in console
			console_buf[3] =  data_len>>8 & 0xFF;//len high
			console_buf[4] =  data_len & 0xFF;//len low
	
			//Calc CRC16
			crc16 = 0xFFFF & (crc16tablefast(console_buf , data_len+5));
	
			console_buf[data_len+5] = (crc16)>>8 ;
			console_buf[data_len+6] = (crc16)&0xFF ;

			if ( foreground ) {
				fprintf(stderr, "Console CMD: L, %d Bytes to %s\n",\
						data_len+7,rokko->sn_long);
				for ( crc16 = 0 ; crc16 < 16 ; crc16++ ) {
					fprintf(stderr, "%02X ",console_buf[crc16]);
				}
				fprintf(stderr, "\n");
			}
		
		
			//buf[0] = GSM_HEAD
			//buf[1] = SEQ
			//buf[2] = CMD
			//buf[3] = len_h
			//buf[4] = len_l
			
			if ( (console_buf[0] != GSM_HEAD) || console_buf[5] != 0 ) {
				char log_str[EMAIL+1];
				
				bzero(log_str, sizeof(log_str));
				snprintf(log_str,sizeof(log_str),"!!!ERR!!! %02X,%02X,%02X, Len:%d @ %s:%d\n",\
						console_buf[0],console_buf[1],console_buf[5],data_len+7,__FILE__,__LINE__);
				log_save(log_str, FORCE_SAVE_FILE );
				if ( foreground ) fprintf(stderr,"%s",log_str);
				//exit(1);
			}

			write(rokko->client_socket,console_buf,data_len+7);

			//save transmit count, ignore, can not feedback to parent 
			//rokko->tx_cnt += data_len+7 ;
			if(console_buf != NULL) free(console_buf);
			console_buf = NULL;

			exit( 0 );
		}
		else { //in parent
			return 0 ;
		} 
	}
}*/

//return 0: ok, others: err
int console_list_spe( struct rokko_data *rokko, struct rokko_command * cmd,\
				unsigned char *rec_buf, unsigned char *snd_buf, \
				struct rokko_data *rokko_all, unsigned int conn_amount )
{//case CONSOLE_CMD_SPE : //list special active client

	unsigned char log_str[EMAIL+1];
	unsigned char *p1, *p2;
	unsigned short var_u16, crc16, data_len;

	if ( conn_amount > MAXCLIENT ) { //err
		return 1;
	}

	//Found the client by SN	
	for (var_u16 = 0; var_u16 < MAXCLIENT; var_u16++) {
		if ((rokko_all[var_u16].client_socket != 0) && rokko_all[var_u16].login_cnt ) {
			//exist connection and login
			if ( cmpmem( &rec_buf[6], rokko_all[var_u16].sn_short, PRODUCT_SN_LEN) ) {//found
				fprintf(stderr, "Found %s, %d\n", rokko_all[var_u16].sn_long,var_u16);

				//send detail to console
				bzero( snd_buf, BUFSIZE);
			
				snd_buf[0] = GSM_HEAD ;
				snd_buf[1] = cmd->seq ;
				snd_buf[2] = cmd->pcb | 0x80 ;
			
				snd_buf[5] =  ERR_RETURN_NONE; //ok
				
				//copy from rokko.hw_rev to rokko.gps
				p1 = (unsigned char *)&rokko_all[var_u16].hw_rev;
				//p2 = (unsigned char *)&rokko_all[var_u16].adc1 ;
				//data_len = p2 - p1 + sizeof(rokko_all[var_u16].adc1);
				p2 = (unsigned char *)&rokko_all[var_u16].gps ;
				data_len = p2 - p1 + sizeof(rokko_all[var_u16].gps);
				//fprintf(stderr, "GPS.sat val:%X\n",rokko_all[var_u16].gps.sat_cnt);
				
				for ( crc16 = 0 ; crc16 < data_len ; crc16++ ) {
					snd_buf[6+crc16] = *(p1+crc16);
				}
				//update len
				data_len = data_len + 2;
				snd_buf[3] = (data_len >> 8)&0xFF; //len high
				snd_buf[4] = (data_len)&0xFF; //len low
				
				/*
				//If change rokko_data struct sequence in rokkod.h, 
				//need double check below content is same as above!
				//hw_rev, fw_rev
				*(unsigned short *)&snd_buf[6] = rokko_all[var_u16].hw_rev;
				*(unsigned short *)&snd_buf[8] = rokko_all[var_u16].fw_rev;
				//tx rx cnt
				*(unsigned int *)&snd_buf[10] = rokko_all[var_u16].rx_cnt;
				*(unsigned int *)&snd_buf[14] = rokko_all[var_u16].tx_cnt;
				//con_time
				*(unsigned int *)&snd_buf[18] = rokko_all[var_u16].con_time;
				*/
				
				//Calc CRC16
				crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));
		
				snd_buf[data_len+5] = (crc16)>>8 ;
				snd_buf[data_len+6] = (crc16)&0xFF ;

				if ( (snd_buf[0] != GSM_HEAD) || snd_buf[5] != 0 ) {					
					bzero(log_str, sizeof(log_str));
					snprintf(log_str,sizeof(log_str),"!!!ERR!!! %02X,%02X,%02X, Len:%d @ %s:%d\n",\
							snd_buf[0],snd_buf[1],snd_buf[5],data_len+7,__FILE__,__LINE__);
					log_err(log_str);		
					if ( foreground ) fprintf(stderr,"%s",log_str);
					return 1 ;
				}
				else {
					if ( foreground ) {
						fprintf(stderr, "Console CMD: l reply %d Bytes to %s\n",\
							data_len+7,rokko->sn_long);
						for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
							fprintf(stderr, "%02X ",snd_buf[crc16]);
						}
						fprintf(stderr, "\n");
					}

					write(rokko->client_socket,snd_buf,data_len+7);
					//save transmit count
					rokko->tx_cnt += data_len+7 ;
		
					return 0;
				}
			}
		}
	}
	//No found SN
	bzero( snd_buf, BUFSIZE); //send failure to console
	snd_buf[0] = GSM_HEAD ;
	snd_buf[1] = cmd->seq ;
	snd_buf[2] = cmd->pcb | 0x80 ;
	snd_buf[3] =  00;//len high
	snd_buf[4] =  01;//len low
	snd_buf[5] =  ERR_RETURN_NO_SN ;

	data_len = ((snd_buf[3])<<8) | snd_buf[4] ;

	//Calc CRC16
	crc16 = 0xFFFF & (crc16tablefast(snd_buf , data_len+5));

	snd_buf[data_len+5] = (crc16)>>8 ;
	snd_buf[data_len+6] = (crc16)&0xFF ;

	if ( foreground ) {
		fprintf(stderr, "CMD: %c(0x%02X), reply: ",cmd->pcb,cmd->pcb);
		for ( crc16 = 0 ; crc16 < data_len+7 ; crc16++ ) {
			fprintf(stderr, "%02X ",snd_buf[crc16]);
		}
		fprintf(stderr, "to %s\n",rokko->sn_long);
	}

	check_sndbuf( snd_buf );
	write(rokko->client_socket,snd_buf,data_len+7);
	//save transmit count
	rokko->tx_cnt += data_len+7 ;
	
	return 0 ;
}
