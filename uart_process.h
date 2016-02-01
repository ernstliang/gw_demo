#ifndef _UART_PROCESS_H_
#define _UART_PROCESS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

#define RPC_FRAME_HDR_SZ	3
#define RPC_STANDARD_FRAME_HDR_SZ		6

	typedef struct
	{
		void * next;
		uint8 * data;
		uint16 data_len;
	} uart_tx_t;

	typedef void * uart_tx_q_t;

#define UART_TX_NEXT(tx_ptr)	((uart_tx_t *)tx_ptr)->next

#define UART_TX_DATA(tx_ptr)	((uart_tx_t *)tx_ptr)->data

#define UART_TX_DATALEN(tx_ptr)	((uart_tx_t *)tx_ptr)->data_len

	// init/uninit uart
	int init_uart_port(char * port);
	void uninit_uart_port();

	// start uart rx/tx thread
	int start_uart_thread();
	int join_uart_thread();

	// uart read/write
	void set_uart_task_id(uint8 task_id);
	void uart_rx();
	void uart_tx();
	void uart_data_to_send(uint8 *data, uint16 len);
	void uart_data_lock_init();
	void uart_data_lock_destroy();

	uart_tx_t * uart_tx_allocate(uint8 * data, uint16 data_len);
	void		uart_tx_deallocate(uart_tx_t * tx_ptr);
	void		uart_tx_enqueue(void * tx_ptr);
	void *	uart_tx_dequeue();

#ifdef __cplusplus
}
#endif

#endif /* _UART_PROCESS_H_ */