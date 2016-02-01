#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uart_cmd_format.h"
#include "gw_uart_cmd.h"
#include "uart_process.h"
#include "gw_device_mgr.h"
#include "TXGWSDK.h"
#include "zb_cluster_id.h"

#define NEW_PROTOCOL

typedef struct tag_pkg_header
{
	uint8 magic_byte;
	uint8 version;
	uint16 body_len;
	uint8 type;
} pkg_header_t;

extern uint8 gw_task_id;

// 解析zb上报的数据，用于数据收集或上报
void parse_standard_frame( uint8 *msg )
{
	if (!msg) return;

	uint16 nwkAddr = BUILD_UINT16(msg[3], msg[4]);
	uint8 endpoint = msg[5];
	uint16 clustid = BUILD_UINT16(msg[6], msg[7]);
	uint8 zb_len = msg[8];
	printf("recv standard frame, FO nwkAddr: %u, ep: %u, cluster_id: %04X, zb_len: %u\n", nwkAddr, endpoint, clustid, zb_len);

	// CLUSTER_ID_RPT_DEVJOIN在设备加入之前
	if (clustid == CLUSTER_ID_RPT_DEVJOIN)
	{
		parse_sub_dev_join(nwkAddr, endpoint, &msg[9], zb_len);
		return;
	}
	else if (clustid == CLUSTER_ID_SUB_LOG)
	{
		print_sub_log(&msg[9], zb_len);
		return;
	}

	// 
	if (clustid != CLUSTER_ID_QQ_IOT)
	{
		printf("unsupported cluster id [%04X]!\n", clustid);
		return;
	}

	printf("sub_iot: ");
	int j = 0;
	for (; j < zb_len; ++j)
	{
		printf("%02X ", msg[9+j]);
	}
	printf("\n");
	
	device_t dev;
	memset(&dev, 0, sizeof(device_t));
	if (zb_get_device(nwkAddr, &dev))
	{
		tx_gw_report_device_data(dev.device_id, &msg[9], zb_len);
	}
}

void	report_new_dev(device_t *dev_ptr)
{
	if (dev_ptr == NULL)
		return;

	int ret = tx_gw_add_device(dev_ptr->device_id, dev_ptr->net_type);
	if (ret == 0)
	{
		printf("tx_gw_device_join, ret = %d\n", ret);
	}
	else if (ret == gw_err_device_id_already_existed)
	{
		printf("tx_gw_report_device_status online!\n");
		tx_gw_report_device_status(dev_ptr->device_id, GW_DEVICE_STATUS_ONLINE);
	}
}

void parse_zb_eplist(uint8 *zb_data, uint16 zb_len)
{
	uint16 nwkAddr = BUILD_UINT16(zb_data[1], zb_data[2]);

	// 设备信息未完整才继续
	if (zb_is_device_info_ok(nwkAddr) == 1)
		return;

	uint8 cnt = zb_data[3];
	if (cnt > 0)
	{
		device_info_step3(nwkAddr, &zb_data[4], cnt);
	}
}

void parse_zb_simpledesc(uint8 *msg)
{
	uint8 len = msg[0];
	uint8 status = msg[3];
	uint16 nwkAddr = BUILD_UINT16(msg[4], msg[5]);
	// 设备信息未完整才继续
	if (zb_is_device_info_ok(nwkAddr) == 1)
		return;

	if (len > 4)
	{
		uint8 ep = msg[7];
		uint16 AppProfId = BUILD_UINT16(msg[8], msg[9]);
		uint16 AppDeviceId = BUILD_UINT16(msg[10], msg[11]);
		uint8 AppDevVer = msg[12] & 0xF0;
		printf("status= %u, Addr= %u, ep= %u, profid= %u, deviceid= %u, devver= %u\n", status, nwkAddr, ep, AppProfId, AppDeviceId, AppDevVer);

		uint8 is_iot_ep_in = 0;
		uint8 is_iot_ep_out = 0;

		uint8 AppNumInClusters = msg[13];
		printf("cluster_in_cnt: %u\n", AppNumInClusters);
		printf("eps: ");
		int index = 13;
		if (AppNumInClusters > 0)
		{
			int i;
			for (i = 0; i < AppNumInClusters; i++)
			{
				uint16 id = BUILD_UINT16(msg[index + 1], msg[index + 2]);
				printf("%04X ", id);
				if (id == CLUSTER_ID_QQ_IOT)
					is_iot_ep_in = 1;
				index += 2;
			}
			printf("\n");
		}
		uint8 AppNumOutClusters = msg[index + 1];
		printf("cluster_out_cnt: %u\n", AppNumOutClusters);
		printf("eps: ");
		index += 1;
		if (AppNumOutClusters > 0)
		{
			int i;
			for (i = 0; i < AppNumOutClusters; i++)
			{
				uint16 id = BUILD_UINT16(msg[index + 1], msg[index + 2]);
				printf("%04X ", id);
				if (id == CLUSTER_ID_QQ_IOT)
					is_iot_ep_out = 1;
				index += 2;
			}
			printf("\n");
		}

		if (is_iot_ep_in == 1 && is_iot_ep_out == 1)
		{
			// set endpoint to device info
			zb_set_endpoint(nwkAddr, ep);

			if (zb_is_device_info_ok(nwkAddr) != 0)
			{
				device_t device = {0};
				if (zb_get_device(nwkAddr, &device))
				{
					report_new_dev(&device);
				}
			}
		}
	}
}

void parse_device_list(uint8 *msg)
{
	printf("parse_device_list\n");
	if (!msg) return;

	uint16 cnt = BUILD_UINT16(msg[3], msg[4]);
	if (cnt <= 0)
	{
		printf("NO Devices!\n");
		return;
	}

	zb_mac_to_addr_t * m2addr_list = (zb_mac_to_addr_t *)malloc(cnt * sizeof(zb_mac_to_addr_t));
	if (!m2addr_list)
	{
		printf("malloc error!\n");
		return;
	}

	memset(m2addr_list, 0, cnt * sizeof(zb_mac_to_addr_t));
	
	// 2015-12-17 获取的设备列表信息需要去重
	printf("\tIndex\taddr\tstatus\textAddr\n");
	uint8_t bHasDevice = 0;
	uint16 i;
	uint8 n = 4;
	for (i = 0; i < cnt; i++)
	{
		uint16 nwkAddr = BUILD_UINT16(msg[n+1], msg[n+2]);
		uint8 devStatus = msg[n+3];
		unsigned long long extAddr = 0;
		memcpy(&extAddr, &msg[n+4], 8);

		printf("extAddr: ");
		int s;
		for (s = 0; s < 8; ++s)
		{
			printf("%02X ", msg[n+4+s]);
		}
		printf("\n");

		int k;
		uint8_t bexist = 0;
		for (k = 0; k < cnt; ++k)
		{
			if (m2addr_list[k].extAddr == extAddr)
			{
				bexist = 1;
				break;
			}
		}
		if (!bexist)
		{
			bHasDevice = 1;
			m2addr_list[i].extAddr = extAddr;
			m2addr_list[i].nwkAddr = nwkAddr;
		}
		printf("\t%d\t%u\t%u\t%llX\n", i, nwkAddr, devStatus, extAddr);
		n += 3+8;
	}
	if (bHasDevice == 1)
	{
		int n = 0;
		for (; n < cnt; ++n)
		{
			if (zb_add_device(m2addr_list[n].extAddr, m2addr_list[n].nwkAddr) != 0)
				device_info_step2(m2addr_list[n].nwkAddr);
			else
			{
				device_t dev = {0};
				if (zb_get_device(m2addr_list[n].nwkAddr, &dev))
				{
					report_new_dev(&dev);
				}
			}
		}
	}

	free(m2addr_list);
}

void parse_data_from_zb(uint8 *msg)
{
	//printf("parse_data_from_zb get data\n");
	if (msg == NULL)
		return;

	uint8 len = msg[0];
	uint8 cmd0 = msg[1];
	uint8 cmd1 = msg[2];

	if (cmd0 == UART_COOR_TO_GW)
	{
		if (cmd1 == GW_UART_STANDARD_FRAME)
		{
			parse_standard_frame(msg);
		}
		else if (cmd1 == GW_UART_GETDEVICELIST)
		{
			parse_device_list(msg);
		}
		else if (cmd1 == GW_UART_EPLIST)
		{
			parse_zb_eplist(&msg[3], len);
		}
		else if (cmd1 == GW_UART_SIMPLEDESC)
		{
			parse_zb_simpledesc(msg);
		}
		else if (cmd1 == GW_UART_NODEDESC)
		{
			//int len = 1 + 2 + 13;  // Status + nwkAddr + Node descriptor
			//uint8 status = msg[3];
			//uint16 shortAddr = BUILD_UINT16(msg[4], msg[5]);
			//uint8 LogicalType				= msg[6] & 0x07;
			//uint8 ComplexDescAvail	= msg[6] & (1 << 3);
			//uint8 UserDescAvail			= msg[6] & (1 << 4);
			//uint8 APSFlags					= msg[7] & 0x07;
			//uint8 FrequencyBand		= msg[7] & 0xF8;
			//uint8 CapabilityFlags			= msg[8];

			//uint8 ManufacturerCode[2];
			//ManufacturerCode[0]		= msg[9];
			//ManufacturerCode[1]		= msg[10];

			//uint8 MaxBufferSize			= msg[11];

			//uint8 MaxInTransferSize[2];
			//MaxInTransferSize[0]			= msg[12];
			//MaxInTransferSize[1]			= msg[13];

			//uint16 ServerMask = BUILD_UINT16(msg[14], msg[15]);

			//uint8 MaxOutTransferSize[2];
			//MaxOutTransferSize[0]		= msg[16];
			//MaxOutTransferSize[1]		= msg[17];

			//uint8 DescriptorCapability	= msg[18];

			//printf("shortAddr= %u, status= %u, logictype= %u, manufacturer= %u %u\n", shortAddr, status, LogicalType, ManufacturerCode[0], ManufacturerCode[1]);
		}
	}
	else if (cmd0 == GW_UART_LOG)
	{
		uint8 * log = (uint8*)malloc((len + 1)*sizeof(uint8));
		memset(log, 0, (len+1)*sizeof(uint8));
		memcpy(log, &msg[3], len);

		if (cmd1 == GW_UART_HEX)
		{
			printf("LOG: ");
			int i = 0;
			for (; i < len; ++i)
			{
				printf("%02X ", log[i]);
			}
			printf("\n");
		}
		else
		{
			printf("LOG: %s\n", log);
		}
		free(log);
	}
}


void device_info_step2(uint16 nwkAddr)
{
	format_uart_get_endpoint_list(nwkAddr);
}

void device_info_step3(uint16 nwkAddr, uint8 * eplist, uint8 epCnt)
{
	int i;
	for (i = 0; i < epCnt; i++)
	{
		uint8 ep = eplist[i];
		format_uart_get_simple_desc(nwkAddr, ep);
	}
}

/************************************************************************/
/* format serial frame                                                                     */
/************************************************************************/

void format_uart_get_device_list()
{
	printf("format_uart_get_device_list\n");
	uart_data_request(UART_GW_TO_COOR, GW_UART_GETDEVICELIST, NULL, 0);
}

void format_uart_permit_join(uint8 open, uint8 time)
{
	uint8 data[2];
	data[0] = open;
	data[1] = time;
	uart_data_request(UART_GW_TO_COOR, GW_UART_PERMITJOIN, data, sizeof(data));
}

void format_uart_get_endpoint_list(uint16 addr)
{
	uint8 data[2];
	data[0] = LO_UINT16(addr);
	data[1] = HI_UINT16(addr);
	uart_data_request(UART_GW_TO_COOR, GW_UART_EPLIST, data, sizeof(data));
}

void format_uart_get_simple_desc(uint16 addr, uint8 ep)
{
	uint8 data[3];
	data[0] = LO_UINT16(addr);
	data[1] = HI_UINT16(addr);
	data[2] = ep;
	uart_data_request(UART_GW_TO_COOR, GW_UART_SIMPLEDESC, data, sizeof(data));
}

void format_uart_remove_dev(uint16 nwkAddr, unsigned long long extAddr)
{
	uint8 data[10];	// nwkAddr 2Byte, extAddr 8Byte
	data[0] = LO_UINT16(nwkAddr);
	data[1] = HI_UINT16(nwkAddr);
	memcpy(&data[2], &extAddr, 8);
	uart_data_request(UART_GW_TO_COOR, GW_UART_REMOVE_DEV, data, sizeof(data));
}

uint8 uart_calc_FCS(uint8 *data, int len)
{
	uint8 x;
	uint8 xorResult;

	xorResult = 0;

	for(x = 0; x < len; x++, data++)
		xorResult = xorResult ^ *data;

	return xorResult;
}

void uart_data_request(uint8 cmd0, uint8 cmd1, uint8 *data, uint16 len)
{
	//SOP(1), FCS(1), LEN(1), CMD(2), DATA(len)
	uint8 tx_len = 2 + RPC_FRAME_HDR_SZ + len;	
	uint8 *tx_data = (uint8*)malloc(tx_len * sizeof(uint8));
	if (!tx_data)
	{
		printf("no more memory\n");
		return;
	}

	tx_data[0] = 0xFE;	//SOP
	tx_data[1] = len;	//LEN
	tx_data[2] = cmd0;	//CMD0
	tx_data[3] = cmd1;	//CMD1
	if (data)
		memcpy(&tx_data[4], data, len);//DATA
	tx_data[tx_len - 1] = uart_calc_FCS(&tx_data[1], tx_len - 2);//FCS

	// copy data to tx buffer
	uart_data_to_send(tx_data, tx_len);

	free(tx_data);
}

void uart_standard_frame(uint16 addr, uint8 ep, uint16 cluster_id, uint8 * zb_data, uint16 zb_len)
{
	printf("send standard frame, TO nwkAddr: %u, ep: %u, cluster_id: %04X, zb_len: %u\n", addr, ep, cluster_id, zb_len);
	// sizeof(uint16) + sizeof(uint8) + sizeof(uint16) + sizeof(uint8)
	// |    2    |    1   |    2    |    1   |   zb_len   |
	// |  addr |   ep  |cluster|zblen|   zb_data |
	uint16 len = RPC_STANDARD_FRAME_HDR_SZ + zb_len;
	uint8 * data = (uint8 *)malloc(len * sizeof(uint8));
	if (!data)
	{
		printf("no more memory\n");
		return;
	}

	data[0] = LO_UINT16(addr);
	data[1] = HI_UINT16(addr);
	data[2] = ep;
	data[3] = LO_UINT16(cluster_id);
	data[4] = HI_UINT16(cluster_id);
	data[5] = zb_len;
	if (zb_data)
		memcpy(&data[6], zb_data, zb_len);

	uart_data_request(UART_GW_TO_COOR, GW_UART_STANDARD_FRAME, data, len);

	free(data);
}

void gw_pass_through_data( unsigned int device_id, unsigned char * data, unsigned short data_len )
{
	printf("gw_pass_through_data, device_id: %u\n", device_id);

	printf("data: ");
	int i = 0;
	for (; i < data_len; ++i)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");

	// 取device_id对应的nwkAddr和endpoint
	uint16 nwkAddr = 0;
	uint8 ep = 0;
	unsigned long long extAddr = 0;
	if (zb_get_device_info(device_id, &nwkAddr, &ep, &extAddr))
	{
		printf("get_zb_info, device_id: %u cluster_id: %04X nwkAddr: %u ep: %u extAddr: %llX\n", device_id, CLUSTER_ID_QQ_IOT, nwkAddr, ep, extAddr);
		uart_standard_frame(nwkAddr, ep, CLUSTER_ID_QQ_IOT, data, data_len);
	}
	else
	{
		printf("get_zb_info, error\n");
	}
}

void parse_sub_dev_join( unsigned short nwkAddr, unsigned char ep, unsigned char* data, unsigned short data_len )
{
	if (data_len != 9)
	{
		printf("sub_dev_join, data error!\n");
		return;
	}

	printf("dev_ret: %02X\n", data[0]);
	printf("dev_mac: ");
	int i;
	for (i = 0; i < 8; ++i)
	{
		printf("%02X ", data[1+i]);
	}
	printf("\n");

	unsigned long long extAddr = 0;
	memcpy(&extAddr, &data[1], 8);
	unsigned int device_id = zb_add_device(extAddr, nwkAddr);
	if (device_id != 0)
	{
		zb_set_endpoint(nwkAddr, ep);
	}

	// 上报设备
	device_t device = {0};
	if (zb_get_device(nwkAddr, &device))
	{
		report_new_dev(&device);
	}
}

void print_sub_log( uint8 * data, uint16 data_len )
{
	printf("sub_log: ");
	int i;
	for (i = 0; i < data_len; ++i)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}