
#ifndef GW_UART_CMD_H
#define GW_UART_CMD_H

#ifdef __cplusplus
extern "C"
{
#endif

/************************************************************************/
/* CONSTANTS                                                                     */
/************************************************************************/
// gateway uart command

#define GW_UART_NONE	0x00
#define GW_UART_HEX		0x01

// cmd1
#define GW_UART_LOG	0x01		// printf

#define UART_GW_TO_COOR		0x05	//data from gateway to coordinator
#define UART_COOR_TO_GW		0x06	//data from coordinator to gateway

// cmd2
#define GW_UART_PERMITJOIN		0x01	// control permit joining
#define GW_UART_GETDEVICELIST	0x02	// get the associated devices list
#define GW_UART_NODEDESC			0x03	// get node desc
#define GW_UART_EPLIST					0x04	// get endpoint list
#define GW_UART_SIMPLEDESC		0x05	// get simple desc
#define GW_UART_REMOVE_DEV		0x06	// remove sub device


//#define GW_UART_REPORT_NEW_DEV			0x08	// report to coordinator a new device join

#define GW_UART_STANDARD_FRAME			0x10	// data is standard frame


#ifdef __cplusplus
};
#endif

#endif /* GW_UART_CMD_H */