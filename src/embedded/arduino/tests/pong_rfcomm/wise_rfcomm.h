/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#pragma once

#include <stdint.h>

#define DATA_PACKAGE_SIZE           18
#define RFCOMM_VERSION              0x1

/* Data type */
#define SENSOR_INFO_DATA_TYPE       0x1
#define SENSOR_CMD_DATA_TYPE        0x2
#define DEVICE_PROT_DATA_TYPE       0x3

/* Device protocol commands */
#define DEVICE_PROT_CONNECT_REQ     0x1
#define DEVICE_PROT_CONNECT_ADDR    0x2
#define DEVICE_PROT_CONNECT_CHK     0x99

/* Sizes */
#define SENSOR_INFO_DATA_SIZE	    0x3
#define DEVICE_PROT_CONN_DATA_SIZE  0x1

/* Sensors type */
#define TEMPERATURE_SENSOR_TYPE     0x1
#define LUMINANCE_SENSOR_TYPE 	    0x2
#define PIR_SENSOR_TYPE             0x3

struct device_version {
	uint8_t major	:4;
	uint8_t minor	:4;
};

struct rfcomm_sensor_info {
    uint8_t    sensor_address;
    uint8_t    sensor_type;
    uint8_t    sebsor_data_len;
};

struct rfcomm_device_prot {
    uint8_t 	device_cmd;
    uint8_t     device_data[8];
};

struct rfcomm_data {
	struct control { 
		uint8_t is_fragmeneted	: 1;
		uint8_t version		: 3;
		uint8_t is_broadcast	: 1;
		uint8_t reserved	: 3;
	};
	
	struct data_info {
		uint8_t data_type	: 3;
		uint8_t data_size	: 5;
	};
	
	union data_package {
		struct _unframeneted {
			uint8_t 	data[DATA_PACKAGE_SIZE];
		} unframeneted;
		
		struct _fragmented {
			uint8_t		frame_number;
			uint8_t		data[DATA_PACKAGE_SIZE - 1];
		} fragmented;
	};
	
	uint16_t	magic_number;			// 2  byte
	control 	control_flags;			// 1  byte
        uint8_t         sender[5];                      // 5  byte
        uint8_t		target[5];			// 5  byte
	data_info	data_information;		// 1  byte
	data_package 	data_frame;			// 18 byte
};

