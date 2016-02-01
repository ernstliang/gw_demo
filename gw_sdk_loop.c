#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "common.h"
#include "gw_sdk_process.h"
#include "gw_sdk_loop.h"
#include "uart_cmd_format.h"
#include "utils.h"
#include "osal.h"
#include "uart_process.h"
#include "gw_device_mgr.h"

extern int g_stop;

static void sig_term(int sig)
{
	if(sig == SIGTERM) {
		//如果是15信号，那么就使进程退出
		printf("receive signal SIGTERM, process will exit now.\n");
		g_stop = TRUE;
	} else if (sig == SIGINT) {
		printf("receive signal SIGINT, process will exit now.\n");
		g_stop = TRUE;
	}
}

int init_signal()
{
	//抓去信号15号信号
	if (signal(SIGTERM, sig_term) == SIG_ERR)
	{
		printf("Capture SIGTERM failure, error = %d\n", errno);
		return 1;
	}

	//抓取信号中断信号
	if (signal(SIGINT, sig_term) == SIG_ERR)
	{
		printf("Capture SIGINT failure, error = %d\n", errno);
		return 1;
	}
	return 0;
}

// osal thread loop
void * osal_thread_func(void *arg)
{
	printf("osal_init_system\n");
	// use osal system
	osal_init_system();

	format_uart_get_device_list();

	printf("osal_start_system\n");
	// start osal system
	osal_start_system();

	return NULL;
}

int do_gw_sdk_loop()
{
	// init gw_sdk
	if (!init_gw_sdk())
		return -1;

	// register signal for process quit
	if (init_signal() != 0)
		return -1;

	// start osal
	start_osal_thread();

	while(!g_stop){
		gw_sleep(1000);
	}

	// wait osal exit
	join_osal_thread();

	printf("device_mgr_uninit\n");
	// release device info
	device_mgr_uninit();

	// release gw_sdk
	uninit_gw_sdk();

	return 0;
}

/************************************************************************/
/* OSAL INIT				                                                                     */
/************************************************************************/

uint8 gw_task_id;

void gw_sdk_init(uint8 taskID)
{
	gw_task_id = taskID;

	set_uart_task_id(gw_task_id);

	device_mgr_init();
}

uint16 gw_event_loop(uint8 task_id, uint16 events)
{
	if (events & SYS_EVENT_MSG)
	{
		event_hdr_t *MSGpkt;

		MSGpkt = (event_hdr_t *)osal_msg_receive(task_id);
		while (MSGpkt)
		{
			switch (MSGpkt->event)
			{
			case SERIAL_MSG:
				parse_data_from_zb(((serialdata_t *)MSGpkt)->msg);
				break;
			case GW_SDK_CMD:
				do_gw_sdk_cmd((gw_sdk_cmd_t *)MSGpkt);
				break;
			case GW_SDK_RM_DEV:
				do_gw_sdk_rm_dev(((gw_sdk_rm_dev_t *)MSGpkt)->device_id);
				break;
			}

			// Release the memory
			osal_msg_deallocate((uint8 *)MSGpkt);

			// Next
			MSGpkt = (event_hdr_t *)osal_msg_receive(task_id);
		}
		return (events ^ SYS_EVENT_MSG);
	}
	return 0;
}

void do_gw_sdk_cmd( gw_sdk_cmd_t * cmd_ptr )
{
	gw_pass_through_data(cmd_ptr->device_id, cmd_ptr->data, cmd_ptr->data_len);
	data_free(cmd_ptr->data);
}

void	do_gw_sdk_rm_dev(unsigned int device_id)
{
	uint16 nwkAddr;
	uint8 ep;
	unsigned long long extAddr;
	if (zb_get_device_info(device_id, &nwkAddr, &ep, &extAddr) != 0)
	{
		format_uart_remove_dev(nwkAddr, extAddr);
	}
}

pthread_t  osal_tid;
void * osal_tret;

int start_osal_thread()
{
	int err;

	err = pthread_create(&osal_tid, NULL, osal_thread_func, NULL);
	if (err != 0)
	{
		perror("can't create thread");
	}
	return err;
}

int join_osal_thread()
{
	int err = pthread_join(osal_tid, &osal_tret);
	if (err != 0)
	{
		perror("can't join with thread");
	}
	printf("thread exit %ld\n", (long)osal_tret);
	return err;
}