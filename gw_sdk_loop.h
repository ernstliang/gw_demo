#ifndef _GW_SDK_LOOP_
#define _GW_SDK_LOOP_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"
#include "gw_struct_def.h"

	// processing gw_sdk loop
	int do_gw_sdk_loop();

	// osal Init
	void gw_sdk_init(uint8 taskID);
	uint16 gw_event_loop(uint8 task_id, uint16 events);

	void	do_gw_sdk_cmd(gw_sdk_cmd_t * cmd_ptr);
	void	do_gw_sdk_rm_dev(unsigned int device_id);

	int start_osal_thread();
	int join_osal_thread();

#ifdef __cplusplus
}
#endif

#endif /* _GW_SDK_LOOP_ */