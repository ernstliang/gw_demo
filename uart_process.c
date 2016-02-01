#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>

#include "osal.h"
#include "uart_api.h"
#include "uart_process.h"
#include "utils.h"
#include "uart_cmd_format.h"
#include "gw_struct_def.h"

#define PRINT_UART_LOG

int g_fd = -1;

extern int g_stop;

#define UART_DEFAULT_BAUDRATE	115200

/************************************************************************/
/* UART INITIALIZE                                                                     */
/************************************************************************/

int init_uart_port(char * port)
{
	if (!port) return FALSE;

	// 打开串口
	g_fd = open_serial_port(port);
	if (g_fd == -1)
	{
		perror("open serial failed!\n");
		return FALSE;
	}

	// 设置串口属性
	set_speed(g_fd, UART_DEFAULT_BAUDRATE);
	if (set_parity(g_fd, 8, 1, 'N') == FALSE)  //8位数据，非两位的停止位，不使用奇偶校验 ，不允许输入奇偶校验
	{
		perror("set parity failed!\n");
		return FALSE;
	}

	return TRUE;
}

void uninit_uart_port()
{
	if (g_fd)
	{
		close(g_fd);
		g_fd = -1;
	}
}

/************************************************************************/
/* UART THREAD																			*/
/************************************************************************/

pthread_t  tid;
void *tret;

void * thread_func(void *arg)
{
	printf("UART loop start\n");

	uart_data_lock_init();

	while (!g_stop)
	{
		uart_rx();
		uart_tx();

		gw_sleep(100);
	}

	uart_data_lock_destroy();

	printf("UART loop finish\n");

	pthread_exit((void*)0);

	return NULL;
}

int start_uart_thread()
{
	int err;
	
	err = pthread_create(&tid, NULL, thread_func, NULL);
	if (err != 0)
	{
		perror("can't create thread");
	}
	return err;
}

int join_uart_thread()
{
	int err = pthread_join(tid, &tret);
	if (err != 0)
	{
		perror("can't join with thread");
	}
	printf("thread exit %ld\n", (long)tret);
	return err;
}

/************************************************************************/
/* UART FUNCTION                                                                     */
/************************************************************************/

#define SOP_STATE			0x00
#define CMD_STATE1		0x01
#define CMD_STATE2		0x02
#define LEN_STATE			0x03
#define DATA_STATE		0x04
#define FCS_STATE			0x05

#define UART_SOF			0xFE

#define RPC_POS_LEN		0
#define RPC_POS_CMD0	1
#define RPC_POS_CMD1	2
#define RPC_POS_DAT0	3


uint8 state = SOP_STATE;

uint8 rx_done;	// does frame read complete

uint8 ch;
uint8 LEN_Token;
uint8 FSC_Token;
uint8 tempDataLen;
serialdata_t* pMsg;


uart_tx_q_t tx_qHead;	// data send to dongle

pthread_mutex_t mutex;	// mutex for tx_qHead

uint8 uart_task_id;

void set_uart_task_id(uint8 task_id)
{
	uart_task_id = task_id;
}

/*
 * read a frame from serial port
 * | SOF | LEN | CMD | DATA | FCS |
 * |   1   |    1   |    2    |   len   |   1   |
 */
void uart_rx()
{
	fd_set rx_fds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500;
	FD_ZERO(&rx_fds);
	FD_SET(g_fd, &rx_fds);

	if (select(g_fd+1, &rx_fds, NULL, NULL, &tv) <= 0)
		return;

	rx_done = FALSE;
	while (FD_ISSET(g_fd, &rx_fds) && g_stop == FALSE && rx_done == FALSE)
	{
		if (read(g_fd, &ch, 1) > 0)//每次读取一个字符
		{
			switch (state)
			{
			case SOP_STATE:
				if (ch == UART_SOF)
				{
					state = LEN_STATE;
//#ifdef PRINT_UART_LOG
//					printf("rx start\n");
//#endif
				}
				break;

			case LEN_STATE:
				LEN_Token = ch;

				tempDataLen = 0;

				pMsg = (serialdata_t*)osal_msg_allocate( sizeof(serialdata_t) + RPC_FRAME_HDR_SZ + LEN_Token );

				if (pMsg)
				{
					pMsg->hdr.event = SERIAL_MSG;
					pMsg->msg = (uint8 *)(pMsg + 1);
					pMsg->msg[RPC_POS_LEN] = LEN_Token;
					state = CMD_STATE1;
				}
				else
				{
					state = SOP_STATE;
				}
				break;

			case CMD_STATE1:
				if (pMsg)
					pMsg->msg[RPC_POS_CMD0] = ch;
				state = CMD_STATE2;
				break;

			case CMD_STATE2:
				if (pMsg)
					pMsg->msg[RPC_POS_CMD1] = ch;
				if (LEN_Token)
				{
					state = DATA_STATE;
				}
				else
				{
					state = FCS_STATE;
				}
				break;

			case DATA_STATE:
				if (pMsg)
					pMsg->msg[RPC_FRAME_HDR_SZ + tempDataLen++] = ch;

				if (tempDataLen == LEN_Token)
					state = FCS_STATE;

				break;

			case FCS_STATE:

				FSC_Token = ch;

				if ((uart_calc_FCS((uint8*)&pMsg->msg[0], RPC_FRAME_HDR_SZ + LEN_Token) == FSC_Token))
				{
					osal_msg_send(uart_task_id, (uint8 *)pMsg);
				}
				else
				{
					free(pMsg);
					pMsg = NULL;
				}

				state = SOP_STATE;
				rx_done = TRUE;
#ifdef PRINT_UART_LOG
				printf("rx %u\n", LEN_Token);
#endif

				break;

			default:
				break;
			}
		}
		else
		{
			if (state == SOP_STATE)
			{
				rx_done = TRUE;
			}
		}
	}
}

/*
 * write data to serial port
 */
void uart_tx()
{
	fd_set tx_fds;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500;
	FD_ZERO(&tx_fds);
	FD_SET(g_fd, &tx_fds);

	if (select(g_fd+1, NULL, &tx_fds, NULL, &tv) <= 0)
		return;

	if (FD_ISSET(g_fd, &tx_fds) && tx_qHead != NULL)
	{
		void * tx_ptr = uart_tx_dequeue();
		if (tx_ptr)
		{
			int n = write(g_fd, UART_TX_DATA(tx_ptr), UART_TX_DATALEN(tx_ptr));

			uart_tx_deallocate((uart_tx_t *)tx_ptr);

#ifdef PRINT_UART_LOG
			printf("tx %d\n", n);
#endif
		}
	}
}

void uart_data_to_send( uint8 *data, uint16 len )
{
	uart_tx_t * tx_ptr = uart_tx_allocate(data, len);
	if (tx_ptr)
	{
		uart_tx_enqueue((void *)tx_ptr);
	}
}

void uart_data_lock_init()
{
	tx_qHead = NULL;
	pthread_mutex_init(&mutex, NULL);
}

void uart_data_lock_destroy()
{
	pthread_mutex_destroy(&mutex);
}

void uart_tx_enqueue( void * tx_ptr )
{
	void * list;
	pthread_mutex_lock(&mutex);

	UART_TX_NEXT(tx_ptr) = NULL;
	if (tx_qHead == NULL)
	{
		tx_qHead = tx_ptr;
	}
	else
	{
		for (list = tx_qHead; UART_TX_NEXT(list) != NULL; list = UART_TX_NEXT(list));

		UART_TX_NEXT(list) = tx_ptr;
	}

	pthread_mutex_unlock(&mutex);
}

void * uart_tx_dequeue()
{
	void * tx_ptr = NULL;
	pthread_mutex_lock(&mutex);

	if (tx_qHead != NULL)
	{
		tx_ptr = tx_qHead;
		tx_qHead = UART_TX_NEXT(tx_ptr);
		UART_TX_NEXT(tx_ptr) = NULL;
	}

	pthread_mutex_unlock(&mutex);
	return tx_ptr;
}

uart_tx_t * uart_tx_allocate( uint8 * data, uint16 data_len )
{
	if (!data || data_len <= 0)
		return NULL;

	uart_tx_t * tx_ptr;
	tx_ptr = (uart_tx_t *)malloc(sizeof(uart_tx_t));
	if (tx_ptr)
	{
		tx_ptr->next = NULL;
		tx_ptr->data = (uint8 *)malloc(data_len * sizeof(uint8));
		memcpy(tx_ptr->data, data, data_len);
		tx_ptr->data_len = data_len;
	}
	return tx_ptr;
}

void uart_tx_deallocate( uart_tx_t * tx_ptr )
{
	if (!tx_ptr) return;

	if (tx_ptr->data)
	{
		free(tx_ptr->data);
	}
	free(tx_ptr);
}