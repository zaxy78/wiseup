/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#pragma once

#include <cstdlib>
#include <cstring>
#include <stdlib.h>

#include <iostream>
#include <algorithm>
#include <vector>
#include <iterator>
#include <stdio.h>

#include "hiredis.h"
#include "wiseDBMng.h"

using namespace std;

typedef enum {
    DISCOVERY       =   0,
    CONNECTED       =   1,
    UNKNOWN         =   99
} wise_status_t;

class WiseClient {
    public:
		WiseClient (uint8_t * addr);
		~WiseClient ();
		
        friend bool operator== ( const WiseClient &wc1, const WiseClient &wc2 ) {
            for (int i = 0; i < 5; i++) {
                if (wc1.address[i] != wc2.address[i])
                    return false;
            }
            return true;
        }
        
        void printAddress () {
            printf ("[ ");
            for (int i = 0; i < 5; i++) {
                printf ("%x ", address[i]);
            } printf ("]\n");
        }
        
        uint8_t         	address[5];
        uint64_t        	timestamp;
        wise_status_t   	status;
};

class WiseClientHandler {
    public:
        WiseClientHandler (redisContext * redisCtx);
        ~WiseClientHandler ();
		
		void 			setCurrentDataPacket (rfcomm_data* wisePacket);
		long long		getSensorHubAddress ();
		long long		getSensorAddress (rfcomm_sensor_info* sensorInfo);

        wise_status_t   registrationCheck (rfcomm_data* wisePacket);
		void            sendRegistration (rfcomm_data* wisePacket);
		void			updateSensorInfo (rfcomm_data* wisePacket);
		
		void			addNewClient (uint8_t* address);
        WiseClient*     findClient (uint8_t * address);
		void			printClentInfo ();
		
		sync_context_t	lock;

    private:
        /* List of WiseUp clients */
        vector<WiseClient>  m_clients;
		redisContext*		m_redisCtx;
		rfcomm_data* 		m_currentWisePacket;
};

class WiseCommandHandler {
    public:
        WiseCommandHandler ();
        ~WiseCommandHandler ();

        void    commandHandler (rfcomm_data* wisePacket);
};
