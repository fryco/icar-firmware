/**
 *      rokkod - Copyright (c) 2011-2012 cn0086 <cn0086.info@gmail.com>
 *
 *      This is NOT a freeware, use is subject to license terms
 *
 *      $URL$
 *      $Rev$, $Date$
 */

#ifndef _ROKKOD_H 
#define _ROKKOD_H

struct record_struct {//check below for record index define
	unsigned int time;//UTC
	unsigned int val;//low  8  bits: record item index, 
					 //high 24 bits: value, max: 0xFF FF FF = 16777216
};

struct gps_struct { //don't change sequence
	unsigned int time;//UTC, from GPS, 4B
	unsigned int lat; //4B
	unsigned int lon; //4B

	unsigned char sat_cnt; //satellite count, 1B
	unsigned char speed; //GPS speed, 1B
	unsigned short status;//check above, 2B	
	//status(2B)
	//bit6~7, 定位方式: 1: 未定位，2：2D定位，3：3D定位
	//bit5: 0, bit4: rsv
	//bit 3: 东西经：0：东经，1：西经
	//bit 2: 南北纬：0：南纬，1：北纬
	//bit0~1: Track angle in degrees True, high
};

struct rokko_db {
	//database
	MYSQL mysql;
	unsigned char * db_host;
	unsigned char * db_name;
	unsigned char * db_user;
	unsigned char * db_pwd;
};

struct rokko_command {
	unsigned char seq;//sequence
	unsigned char pcb;//protocol control byte
	unsigned short len;//length;
	unsigned char *inf;//Information Field
	unsigned short crc16;//crc16 result
};


//记录已发命令，如果收到响应，就清除记录
//否则超时(CMD_TIMEOUT), 重发，详见config.h
struct rokko_send_queue {
	unsigned int timer;//cancel CMD if time > 5*CMD_TIMEOUT
	unsigned char confirm; 
	unsigned char seq;
	unsigned char pcb;
};

struct rokko_data { //as minimum as possible
	//Don't change below sequence, need align 4 Bytes
	//Check console_list_spe( ) in console.c
	unsigned short hw_rev; //2 Bytes
	unsigned short fw_rev; //2 Bytes
	unsigned int rx_cnt;//receive total bytes, 4 Bytes
	unsigned int tx_cnt;//transmit total bytes, 4 Bytes
	unsigned int con_time;//connect time, 4 Bytes

	struct record_struct adc1;	//8 Bytes
	struct record_struct adc2;	//8 Bytes
	struct record_struct adc3;	//8 Bytes
	struct record_struct adc4;	//8 Bytes
	struct record_struct adc5;	//8 Bytes
	struct record_struct v_tp1;	//8 Bytes
	struct record_struct v_tp2;	//8 Bytes
	struct record_struct v_tp3;	//8 Bytes
	struct record_struct v_tp4;	//8 Bytes
	struct record_struct v_tp5;	//8 Bytes
	struct record_struct v_tp6;	//8 Bytes
	struct record_struct v_tp7;	//8 Bytes
	struct record_struct v_tp8;	//8 Bytes
	struct record_struct v_tp9;	//8 Bytes
	struct record_struct v_tp10;//8 Bytes
	struct record_struct mcu; 	//8 Bytes, for mcu temperature
	struct record_struct gsm; 	//8 Bytes, for gsm signal
	struct gps_struct gps;		//16 Bytes

	//socket
	struct sockaddr_in client_addr;
	int client_socket ;

	
	unsigned int idle_timer;//close socket if time > 2*CMD_TIMEOUT
	//unsigned char active_time; //close socket if timeout
	unsigned char cmd_err_cnt;//command error count
	
	unsigned char sn_short[PRODUCT_SN_LEN];//product serial number, IMEI(15),1234=>0x12 0x34
	unsigned char sn_long[16];//product serial number, IMEI(15),1234=>0x31 0x32 0x33 0x34
	unsigned char login_cnt;//login count, preven always login //1 Bytes
	
	//int  err_code;
	//char err_msg[BUFSIZE+1];
	
	struct rokko_send_queue send_q[MAX_CMD_QUEUE];
};


//记录已发命令，如果收到响应，就清除记录
//否则超时(CMD_TIMEOUT), 重发，详见config.h
struct SENT_QUEUE {
	unsigned int send_timer;//cancel CMD if time > 5*CMD_TIMEOUT, 
							//can be improved to char, reduce size, TBD
	unsigned char confirm; 
	unsigned char send_seq;
	unsigned char send_pcb;
};

int main(int, char *[]);
void bg(void);
void scan_args(int, char *[]);
void print_help(char *[]);
void print_version(void);
void period_check( struct rokko_data *, unsigned int);
unsigned char sock_init( unsigned int );
unsigned char daemon_server(struct rokko_data *, unsigned char *, unsigned short, struct rokko_data *, unsigned int);

#endif /* _ROKKOD_H */
