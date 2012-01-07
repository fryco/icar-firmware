/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ME3000_H
#define __ME3000_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void gsm_bus_init(void);
u8 gsm_connect_tcp(void);
u8 gsm_disconn_tcp(void);
u8 me3000_check_CSQ(void);
bool me3000_check_ip( u8 *rev_at_sbuf );
bool gsm_pwr_off(void);

#endif /* __ME3000_H */
