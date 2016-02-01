#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "uart_api.h"
#include "uart_process.h"
#include "gw_sdk_loop.h"

int g_stop = FALSE;

int main(int argc, char **argv)
{
	// 还未实现串口监听线程，暂时先使用参数指定串口
	if (argc < 2)
	{
		printf("error params\n");
		return -1;
	}
	
	do 
	{

		if (!init_uart_port(argv[1]))
			break;

		if (start_uart_thread() != 0)
			break;

		// init gw_sdk loop
		do_gw_sdk_loop();
		
		join_uart_thread();
		
	} while (FALSE);

	uninit_uart_port();

	return 0;
}
