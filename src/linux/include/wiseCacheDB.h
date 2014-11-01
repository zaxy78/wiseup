/*
 * Author: Yevgeniy Kiveisha <lodmilak@gmail.com>
 * Copyright (c) 2014 OpenWise.
 */

#pragma once

#include <iostream>
#include <pthread.h>
#include <vector>
#include <stdlib.h>

/* includes for wiseup */
#include "wise_mysql.h"
#include "wise_rfcomm.hpp"
#include "wise_ipc.h"
#include "commonMethods.hpp"

using namespace std;

#define CACHE_DB_UPDATE_SENSORS_VALUE_FROM_RFCOMM_DATA		1
#define CACHE_DB_UPDATE_SENSOR_VALUE						2
#define CACHE_DB_UPDATE_SENSOR_AVAILABLE					3
#define CACHE_DB_ADD_EVENT									4
#define CACHE_DB_REMOVE_EVENT								5

#define PHP_REQUESTOR		1
#define HW_REQUESTOR		2

typedef struct {
	uint8_t		available;
} cache_db_available_arg_t;

typedef struct {
	uint32_t	eventId;
	uint8_t		eventType;
	uint8_t		eventAction[16];
} sensor_event_t;

typedef struct {
	struct control { 
		uint8_t timerType	: 2;
		uint8_t reserved	: 6;
	};
	
	uint32_t	timerId;
	uint64_t 	time;
	control		flags;
	uint8_t		timerAction[16];
} sensor_timer_t;

typedef struct { 
	uint8_t isAvalibale	: 1;
	uint8_t isEvent		: 1;
	uint8_t isValueCng	: 1;
	uint8_t reserved	: 5;
} sensor_control_t;

typedef struct { 
	uint16_t			sensorHWValue;
	uint16_t			sensorUIValue;
} sensor_value_t;

typedef struct { 
	long long 			sensorAddress;
	long long			hubAddress;
	uint8_t				sensorPort;
	uint8_t				sensorType;
	sensor_value_t		value;
	sensor_value_t		backup;
	uint64_t			lastUpdate;
	sensor_control_t	flags;
} sensor_info_t;

typedef struct {
	uint8_t			requestId;
	uint8_t			requestorType;
    sensor_info_t 	sensorInfo;
	rfcomm_data		packet;
	uint8_t			args[16];
} cache_db_msg_t;

class SensorInfo {
public:
	SensorInfo ();
	~SensorInfo ();
	
	void 				addEvent (sensor_event_t &event);
	bool 				removeEvent (uint32_t id);
	bool			 	findEvent (uint32_t id, sensor_event_t& event);
	int 				getEventSize ();
	
	sensor_info_t			sensorInfo;
	vector<sensor_event_t> 	sensorEvents;
};

class CacheDB {
public:
	CacheDB ();
	~CacheDB ();
	
	static void apiUpdateSensorsValueFromRfcommData (rfcomm_data* data);
	static void apiUpdateSensorValue (sensor_info_t& data);
	
	void add (SensorInfo &info);
	bool remove (long long id);
	bool find (long long id, SensorInfo& data);
	void update (long long id, SensorInfo& src);
	int getSize ();
	
	bool start ();
	void stop ();
	
	void updateSensorsValueFromRfcommDataHandler (cache_db_msg_t& data);
	void updateSensorValue (cache_db_msg_t& data);
	
	bool					m_isWorking;
	sync_context_t			m_lock;
	vector<SensorInfo> 		m_db;

private:
	void initDB ();
	void printSensorsInfo ();
	
	MySQL *				m_dbconn;
	pthread_t       	m_worker;
};