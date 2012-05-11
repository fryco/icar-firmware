/*
*********************************************************************************************************
*
*                                      APPLICATION CONFIGURATION
*
*                                     ST Microelectronics STM32
*                                       with the TRM V1 board
*
* Filename      : app_cfg.h
* Version       : V1.00
* Programmer(s) : BAN
*********************************************************************************************************
*/

#ifndef  __APP_CFG_H__
#define  __APP_CFG_H__

/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/

//#define  APP_OS_PROBE_EN                         DEF_ENABLED
//#define  APP_PROBE_COM_EN                        DEF_ENABLED

#define  APP_OS_PROBE_EN                         DEF_DISABLED
#define  APP_PROBE_COM_EN                        DEF_DISABLED

/*
*********************************************************************************************************
*                                              TASKS NAMES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            TASK PRIORITIES
*********************************************************************************************************
*/

#define  APP_TASK_MANAGER_PRIO                              3
#define  APP_TASK_OBD_PRIO                                  6
#define  APP_TASK_GSM_PRIO                                  9

#define  OS_PROBE_TASK_PRIO              (OS_LOWEST_PRIO - 3)
#define  OS_TASK_TMR_PRIO                (OS_LOWEST_PRIO - 2)

/*
*********************************************************************************************************
*                                            TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_TASK_MANAGER_STK_SIZE                    512	
#define  APP_TASK_GSM_STK_SIZE                        512
#define  APP_TASK_OBD_STK_SIZE                        512
//#define  OS_PROBE_TASK_STK_SIZE                          128

/*
*********************************************************************************************************
*                                                  LIB
*********************************************************************************************************
*/

#define  uC_CFG_OPTIMIZE_ASM_EN                 DEF_ENABLED
#define  LIB_STR_CFG_FP_EN                      DEF_DISABLED

/*
*********************************************************************************************************
*                                                 PROBE
*********************************************************************************************************
*/

#define  OS_PROBE_TASK                                     1    /* Task will be created for uC/Probe OS Plug-In.        */
#define  OS_PROBE_TMR_32_BITS                              0    /* uC/Probe OS Plugin timer is a 16-bit timer.          */
#define  OS_PROBE_TIMER_SEL                                2    /* Select timer 2.                                      */
#define  OS_PROBE_HOOKS_EN                                 1    /* Hooks to update OS_TCB profiling members included.   */
#define  OS_PROBE_USE_FP                                   1


#endif
