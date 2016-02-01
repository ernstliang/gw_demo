#include "gw_sdk_process.h"
#include "TXGWSDK.h"
#include "osal.h"
#include "utils.h"
#include "gw_struct_def.h"
#include <pthread.h>

extern uint8 gw_task_id;

void on_receive_data(uint32_t device_id,const uint8_t* data, uint16_t data_len)
{
	gw_sdk_cmd_t * msg = (gw_sdk_cmd_t *)osal_msg_allocate(sizeof(gw_sdk_cmd_t));
	if (msg)
	{
		msg->hdr.event = GW_SDK_CMD;
		unsigned char * dup_data = data_dup_len(data, data_len);
		msg->data = dup_data;
		msg->data_len = data_len;
		msg->device_id = device_id;

		if (osal_msg_send(gw_task_id, (uint8 *)msg) != 0)
		{
			data_free(dup_data);
		}
	}
}

void on_device_removed(uint32_t device_id)
{
	printf("on_device_removed: device_id=%u",device_id);
	gw_sdk_rm_dev_t *msg = (gw_sdk_rm_dev_t *)osal_msg_allocate(sizeof(gw_sdk_rm_dev_t));
	if (msg)
	{
		msg->hdr.event = GW_SDK_RM_DEV;
		msg->device_id = device_id;

		osal_msg_send(gw_task_id, (uint8 *)msg);
	}
}

int on_check_discovery_status()
{
	return 1;
}

void* thread_func_initdevice(void * arg)
{
	char license[256] = {0};
	int nLicenseSize = 0;
	if (0 != gw_readfile("./licence.sign.file.txt", license, sizeof(license), &nLicenseSize))
	{
		printf("[error]get license from file failed...\n");
		return NULL;
	}

	char guid[32] = {0};
	int nGUIDSize = 0;
	if(0 != gw_readfile("./GUID_file.txt", guid, sizeof(guid), &nGUIDSize))
	{
		printf("[error]get guid from file failed...\n");
		return NULL;
	}

	char svrPubkey[256] = {0};
	int nPubkeySize = 0;
	if (0 != gw_readfile("./1700002049.pem", svrPubkey, sizeof(svrPubkey), &nPubkeySize))
	{
		printf("[error]get svrPubkey from file failed...\n");
		return NULL;
	}


	//设备信息
	tx_gw_device_info info = {0};
	info.os_platform            = "Linux";
	info.device_name            = "SmartGateway";
	info.device_serial_number   = guid;
	info.device_license         = license;
	info.product_version        = 1;
	info.product_id             = 1700002049;
	info.server_pub_key         = svrPubkey;
	info.test_mode              = 0;
	info.network_type           = gw_network_type_wifi;

	// SDK初始化目录，写入配置、Log输出等信息
	//   为了了解设备的运行状况，存在上传异常错误日志 到 服务器的必要
	//   system_path：SDK会在该目录下写入保证正常运行必需的配置信息
	//   system_path_capicity：是允许SDK在该目录下最多写入多少字节的数据（最小大小：10K，建议大小：100K）
	//   app_path：用于保存运行中产生的log或者crash堆栈
	//   app_path_capicity：同上，（最小大小：300K，建议大小：1M）
	//   temp_path：可能会在该目录下写入临时文件
	//   temp_path_capicity：这个参数实际没有用的，可以忽略
	tx_gw_init_path init_path = {0};
	init_path.system_path = "./";
	init_path.system_path_capicity  = 10240;
	init_path.app_path = "./";
	init_path.app_path_capicity  = 1024000;
	init_path.temp_path = "./";
	init_path.temp_path_capicity  = 102400;

	// 设置log输出函数，如果不想打印log，则无需设置。
	// 建议开发在开发调试阶段开启log，在产品发布的时候禁用log。
	//tx_set_log_func(log_func); //在函数中使用宏控制了，所以一定设置logfunc

	tx_gw_notify gw_notify ={0};
	gw_notify.on_receive_data   = on_receive_data;
	gw_notify.on_device_removed = on_device_removed;
	gw_notify.on_check_discovery_status = on_check_discovery_status;
	int ret =tx_gw_init(&info,&init_path,&gw_notify);
	if (gw_err_null == ret) {
		printf("tx_gw_init success\n");
	}else {
		printf("[error] tx_gw_init failed! err_code=%d\n",ret);
		return NULL;
	}

	return NULL;
}

int init_gw_sdk()
{
	pthread_t ntid = 0;
	int err;
	err = pthread_create(&ntid, NULL, thread_func_initdevice, NULL);
	if(err == 0 && ntid != 0)
	{
		pthread_join(ntid,NULL);
		ntid = 0;
	}

	return 1;
}

int uninit_gw_sdk()
{
	tx_gw_uninit();
	return 1;
}