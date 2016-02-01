#ifndef _UART_CMD_FORMAT_H_
#define _UART_CMD_FORMAT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include "gw_struct_def.h"

	// data in coming from zigbee
	void parse_data_from_zb(uint8 *msg);
	void parse_standard_frame(uint8 *msg);

	void parse_zb_eplist(uint8 *zb_data, uint16 zb_len);
	void parse_zb_simpledesc(uint8 *msg);
	void parse_device_list(uint8 *msg);

	void report_new_dev(device_t *dev_ptr);

	void device_info_step2(uint16 nwkAddr);
	void device_info_step3(uint16 nwkAddr, uint8 * eplist, uint8 epCnt);

	// format serial frame
	void format_uart_get_device_list();
	void format_uart_permit_join(uint8 open, uint8 time);
	void format_uart_get_endpoint_list(uint16 addr);
	void format_uart_get_simple_desc(uint16 addr, uint8 ep);
	void format_uart_remove_dev(uint16 nwkAddr, unsigned long long extAddr);

	// common functions
	uint8 uart_calc_FCS(uint8 *data, int len);
	void uart_data_request(uint8 cmd0, uint8 cmd1, uint8 *data, uint16 len);
	void uart_standard_frame(uint16 addr, uint8 ep, uint16 cluster_id, uint8 * zb_data, uint16 zb_data_len);

	void gw_pass_through_data(unsigned int device_id, unsigned char * data, unsigned short data_len);

	void parse_sub_dev_join(unsigned short nwkAddr, unsigned char ep, unsigned char* data, unsigned short data_len);

	void print_sub_log(uint8 * data, uint16 data_len);
	

#ifdef __cplusplus
}
#endif

#endif /* _UART_CMD_FORMAT_H_ */