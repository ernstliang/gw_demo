#ifndef _OSAL_TASK_H_
#define _OSAL_TASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define TASK_NO_TASK			0xFF

#include "common.h"

	typedef unsigned short (*pTaskEventHandlerFn)(unsigned char task_id, unsigned short event);

	extern const pTaskEventHandlerFn tasksArr[];
	extern const uint8 tasksCnt;
	extern uint16 *tasksEvents;

	extern void osalInitTasks();

#ifdef __cplusplus
}
#endif

#endif /* _OSAL_TASK_H_ */