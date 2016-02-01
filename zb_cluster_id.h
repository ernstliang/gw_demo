#ifndef _ZB_CLUSTER_ID_H_
#define _ZB_CLUSTER_ID_H_

#define CLUSTER_ID_NONE		0		//

#define ZCL_CLUSTER_ID_GEN_ON_OFF                            0x0006

// Pass through cluster id
#define CLUSTER_ID_RPT_DEVJOIN	0xE230
#define CLUSTER_ID_QQ_IOT		0xE231
#define CLUSTER_ID_SUB_LOG		0xE232

// User define led control cluster id
#define CLUSTER_ID_P2P_LED_CONTROL	0x2007		// Set led on/off (coordinator -> end device)
#define CLUSTER_ID_P2P_LED_STATUS		0x2008		// Get led on/off status (coordinator -> end device)
#define CLUSTER_ID_RPT_LED_STATUS		0x2009		// Report led status (end device -> coordinator)

// User define get device product id
#define CLUSTER_ID_GET_PRODUCTID	0x2010	// Get device's product id (coordinator -> end device)
#define CLUSTER_ID_RPT_PRODUCTID	0x2011	// Report device's product id (end device -> coordinator)

// Door lock
#define CLUSTER_ID_SET_DOORLOCK	0x2019
#define CLUSTER_ID_GET_DOORLOCK	0x2020
#define CLUSTER_ID_RPT_DOORLOCK	0x2021
#define CLUSTER_ID_OUT_DOORLOCK	0x2022

// Light Switch
#define CLUSTER_ID_GET_SWITCH		0x2023
#define CLUSTER_ID_RPT_SWITCH		0x2024
#define CLUSTER_ID_OUT_SWITCH		0x2025

#endif