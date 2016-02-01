//#include "osal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osal_task.h"
#include "gw_sdk_loop.h"

const pTaskEventHandlerFn tasksArr[] = 
{
	gw_event_loop
};

const uint8 tasksCnt = sizeof(tasksArr) / sizeof(tasksArr[0]);
uint16 *tasksEvents;

void osalInitTasks()
{
	uint8 taskID = 0;
	tasksEvents = (uint16 *)malloc(sizeof(uint16) * tasksCnt);
	memset(tasksEvents, 0, (sizeof(uint16) * tasksCnt));

	gw_sdk_init(taskID);
}