/*
 * Author: Yevgeniy Kiveisha <yevgeniy.kiveisha@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 */

#include <unistd.h>
#include <iostream>
#include <vector>
#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <string>
#include "json/json.h"
#include <stdlib.h>     /* srand, rand */

/* includes for deamonization */
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* includes for wiseup */
#include "common.h"
#include "nrf24l01.h"
#include "filelog.h"
#include "fferror.hpp"
#include "wise_ipc.h"
#include "wise_rfcomm.hpp"
#include "wise-timer.h"
#include "wise_client_handler.h"
#include "commonMethods.hpp"

#include "wiseDBMng.h"
#include "screen.h"

#include <errno.h>

using namespace std;

void stdout_msg ();

uint8_t local_address[5]     = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t wise_stdout_buffer[128];

int 			    running 	    = 0;
comm::NRF24L01      *sensor 	    = NULL;
comm::WiseRFComm    *net            = NULL;
WiseClientHandler   *clientHandler  = NULL;
WiseCommandHandler  *cmdHandler     = NULL;
Screen*             lcd             = NULL;
screen_context      lcdCtx;

/* Message queue for outgoing packets */
vector<nrf24l01_msg_t> messagePull;

/* Our process ID and Session ID */
pid_t pid, sid;

pthread_t       ipcNRFOutListenerThread;
pthread_t       ipcPHPNRFOutListenerThread;

sync_context_t  msgPullSyncContext;

void
dataHandling (rfcomm_data * packet) {
    /* 1. Chick the registarion ID.*/
    wise_status_t deviceStatus = clientHandler->registrationCheck (packet);
    if (deviceStatus != CONNECTED) {
        if (deviceStatus == DISCOVERY) {
            /* 2.1. Send gateway address back to the device */
            clientHandler->sendRegistration (packet);
        }
	} else {
        /* 2.1. Execute the requested command*/
        cmdHandler->commandHandler (packet);
		WiseDBMng::apiUpdateSensorInfo (packet);
		clientHandler->updateSensorInfo (packet);
		
		printf ("(wise-nrfd) [dataHandling] Updating sensor data \n");
    }

    lcdCtx.rxPacketCount++;
}

/*
 * 3d layer - the nRF24l01 is the 2d layer witch provide the data from
 * the air. This layer get the data and remove the ecryption, check if 
 * the data relevant to this device and more.
 * 
 * This method is a handler to the relevant data to this device.
 */
void
netLayerDataArrivedHandler (void * args) {
    dataHandling ((rfcomm_data *) args);
}

/*
 * 3d layer - the nRF24l01 is the 2d layer witch provide the data from
 * the air. This layer get the data and remove the ecryption, check if 
 * the data relevant to this device and more.
 * 
 * This method handling broadcast data.
 */
void
netLayerBroadcastArrivedHandler (void * args) {
    dataHandling ((rfcomm_data *) args);
}

void *
phpCommandListener (void *) {
    /* UNIX domain socket listener for nrf24l01 outgoing packets */
    WiseIPC         *ipcNrfOut = NULL;
    try {
        ipcNrfOut = new WiseIPC ("/tmp/wiseup/nrf_outgoing_queue_php");
        ipcNrfOut->setServer ();
        
        char message[128] ={0};
        int client = -1;
        while (1) {
            client = ipcNrfOut->listenIPC ();
            ipcNrfOut->setBuffer ((uint8_t *)message);
            ipcNrfOut->readMsg (client, 128);

            try {
                nrf24l01_msg_t msg;
                Json::Value root;
                Json::Reader reader;

                std::string json = string(message); 
                bool parsingSuccessful = reader.parse( json, root );
                if (!parsingSuccessful) {
                    std::cout  << "Failed to parse configuration\n"
                               << reader.getFormattedErrorMessages();
                }

                long long   hubAddress      = root.get("sensor_hub_address", 0 ).asInt64();
                int         sensorAddress   = root.get("sensor_address", 0 ).asInt();
                int         sensorAction    = root.get("action", 0 ).asInt();

                printf ("(wise-nrfd) [phpCommandListener] hub = %lld, addr = %d, action = %d \n", hubAddress, sensorAddress, sensorAction);

                uint8_t destination[5] = {0};
				memcpy (destination, &hubAddress, 5);

                rfcomm_data *wisePacketTX = (rfcomm_data *)net->ptrTX;
                wisePacketTX->data_information.data_type      = SENSOR_CMD_DATA_TYPE;
                wisePacketTX->data_information.data_size      = SENSOR_CMD_DATA_TYPE_SIZE;
                wisePacketTX->control_flags.is_fragmeneted    = 0;
                wisePacketTX->control_flags.version           = 1;
                wisePacketTX->control_flags.is_broadcast      = 0;
                wisePacketTX->control_flags.is_ack            = 0;
                wisePacketTX->magic_number[0]                 = 0xAA;
                wisePacketTX->magic_number[1]                 = 0xBB;
                memcpy (wisePacketTX->sender, local_address, 5);
                memcpy (wisePacketTX->target, destination, 5);
                rfcomm_sensor_command* sensorCmd = (rfcomm_sensor_command *)wisePacketTX->data_frame.unframeneted.data;
                sensorCmd->sensor_address        = sensorAddress;
                sensorCmd->command_type          = SENSOR_CMD_RELAY;
                sensorCmd->command_data[0]       = sensorAction;

                memcpy (msg.packet, net->ptrTX, 32);

                pthread_mutex_lock   (&msgPullSyncContext.mutex);
                messagePull.push_back(msg);
                pthread_cond_signal  (&msgPullSyncContext.cond);
                pthread_mutex_unlock (&msgPullSyncContext.mutex);
				
				long long sensorAddr = (hubAddress << 8) | sensorAddress;
				clientHandler->updateSensorUIValue (sensorAddr, sensorAction);
            } catch (...) {
                printf ("(wise-nrfd) ERROR \n");
            }
            
            /* sprintf ((char *) wise_stdout_buffer, "(wise-nrfd) [OUTGOING-THREAD] Got outgoing message.");
            stdout_msg (); */

            close (client);
        }
    } catch (FFError e) {
        std::cout << e.Label.c_str() << std::endl;
    }

    delete ipcNrfOut;
    return NULL;
}

/*
 * Producer */
void *
outgoingMessageListener (void *) {
    /* UNIX domain socket listener for nrf24l01 outgoing packets */
    WiseIPC         *ipcNrfOut = NULL;
    try {
        ipcNrfOut = new WiseIPC ("/tmp/wiseup/nrf_outgoing_queue");
        ipcNrfOut->setServer ();

        int client = -1;
        while (1) {
            client = ipcNrfOut->listenIPC ();

            nrf24l01_msg_t msg;
            try {
                ipcNrfOut->setBuffer (msg.packet);
                ipcNrfOut->readMsg (client, 32);
                pthread_mutex_lock   (&msgPullSyncContext.mutex);
                messagePull.push_back (msg);
                pthread_mutex_unlock (&msgPullSyncContext.mutex);
            }
            catch (...) {	}
            
            printf ("(wise-nrfd) [outgoingMessageListener] Queueing the REQUEST\n");
            close (client);
        }
    } catch (FFError e) {
        std::cout << e.Label.c_str() << std::endl;
    }

    delete ipcNrfOut;
    return NULL;
}

/* 
 * Consumer */
void
outgoingNrf24l01 () {
    if (messagePull.size() != 0) {
        pthread_mutex_lock (&msgPullSyncContext.mutex);
        nrf24l01_msg_t msg = messagePull.back();
        messagePull.pop_back();
        pthread_mutex_unlock (&msgPullSyncContext.mutex);

        rfcomm_data *wisePacket = (rfcomm_data *)msg.packet;
        msg.timestamp = CommonMethods::getTimestampMillis();

        lcd->powerDown ();

        /* initialize random seed: */
        srand (time(NULL));
        wisePacket->packet_id = rand() % 65500 + 1;;

        memcpy (net->ptrTX, msg.packet, MAX_BUFFER);
        net->sendPacket (wisePacket->target);

        if (wisePacket->control_flags.is_ack) {
        }

        lcd->powerUp ();

        lcdCtx.txPacketCount++;

		printf ("(wise-nrfd) [outgoingNrf24l01] Sending to %d %d %d %d %d\n", wisePacket->target[0], wisePacket->target[1], wisePacket->target[2], 
												wisePacket->target[3], wisePacket->target[4]);
		printf ("(wise-nrfd) [outgoingNrf24l01] From to %d %d %d %d %d\n", wisePacket->sender[0], wisePacket->sender[1], wisePacket->sender[2], 
												wisePacket->sender[3], wisePacket->sender[4]);
    }
}

void
deamonize () {
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        /* Print process id to the screen */
        printf("Process_id of child process %d \n", pid);
        exit (EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
            
    /* Open any logs here */        
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        exit(EXIT_FAILURE);
    }

    /* Creatng the current working directory */
    struct stat st = {0};
    if (stat("/tmp/wiseup", &st) == -1) {
        mkdir("/tmp/wiseup", 0700);
        printf("\nCreatng the current working directory...\n");
    }

    /* Change the current working directory */
    if ((chdir("/tmp/wiseup")) < 0) {
        /* Log the failure */
        printf("Change the current working directory... [FAILED]\n");
        exit(EXIT_FAILURE);
    }
    printf("\nChange the current working directory... [SUCCESS]\n");
}

int GetCPULoad() {
    int FileHandler;
    char FileBuffer[1024];
    float load;

    FileHandler = open("/proc/loadavg", O_RDONLY);
    if(FileHandler < 0) {
        return -1; }
    read(FileHandler, FileBuffer, sizeof(FileBuffer) - 1);
    sscanf(FileBuffer, "%f", &load);
    close(FileHandler);
    return (int)(load * 100);
}

int GetCPUTemp() {
    int FileHandler;
    char FileBuffer[1024];
    int load;

    FileHandler = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
    if(FileHandler < 0) {
        return -1; }
    read(FileHandler, FileBuffer, sizeof(FileBuffer) - 1);
    sscanf(FileBuffer, "%d", &load);
    close(FileHandler);
    return (load / 1000);
}

int
main (int argc, char **argv)
{
    // deamonize ();

    umask(0);
    struct stat st = {0};
    if (stat("/tmp/wiseup", &st) == -1) {
        mkdir("/tmp/wiseup", 0777);
        printf("\nCreatng the current working directory...\n");
    }

    int error = pthread_create(&ipcNRFOutListenerThread, NULL, outgoingMessageListener, NULL);
    if (error) {
         printf("ERROR: return code from pthread_create() is %d\n", error);
         exit(-1);
    }

    error = pthread_create(&ipcPHPNRFOutListenerThread, NULL, phpCommandListener, NULL);
    if (error) {
         printf("ERROR: return code from pthread_create() is %d\n", error);
         exit(-1);
    }

	printf("Initiating nrf24l01...\n");
    /* Init nRF24l01 communication */
    sensor = new comm::NRF24L01 (17, 4);
    net = new comm::WiseRFComm (sensor, netLayerDataArrivedHandler, netLayerBroadcastArrivedHandler);
    net->setSender (local_address);

    printf("Starting the listener... [SUCCESS]\n");
	// printf("Close out the standard file descriptors...\n");
    /* Close out the standard file descriptors */
    // close(STDIN_FILENO);
    // close(STDOUT_FILENO);
    // close(STDERR_FILENO);
    
    pthread_cond_init  (&msgPullSyncContext.cond, NULL);
    pthread_mutex_init (&msgPullSyncContext.mutex, NULL);

    WiseTimer* timerNTM = new WiseTimer();
    WiseTimer* timerRUD = new WiseTimer();
    WiseTimer* timerLCD = new WiseTimer();
    clientHandler   = new WiseClientHandler  ();
    cmdHandler      = new WiseCommandHandler ();
	
	WiseDBDAL* 			wiseDAL 		= new WiseDBDAL ();
	WiseDBMng* 			wiseDB  		= new WiseDBMng (wiseDAL);
	
    lcd = new Screen (sensor->getSPIHandler(), 25, 23, 24);
	
	timerNTM->setTimer (1); //set up a delay timer
    timerRUD->setTimer (60);
    timerLCD->setTimer (10);
	
	if (!wiseDB->start()) { exit (-1); }
	
	usleep (200000); // Remove race condition, wiseDB need to start its engine.
	WiseDBMng::apiSetAllSensorNotConnected ();
	
	usleep (200000);
	clientHandler->clentDataBaseInit();
	// clientHandler->printClentInfo();

	lcd->clearscr ();
	
	char Str[16];
    /* The Big Loop */
    while (!running) {
        net->listenForIncoming ();

        if (timerLCD->checkTimer (10) == 1) {
            lcd->setTextSize (1);
            lcd->setTextColor(BLACK, WHITE);

            snprintf(Str, sizeof(Str), "TX: %d", lcdCtx.txPacketCount);
            lcd->setCursor(1, 1);
            lcd->print(Str);

            snprintf(Str, sizeof(Str), "RX: %d", lcdCtx.rxPacketCount);
            lcd->setCursor(1, 10);
            lcd->print(Str);

            lcdCtx.cpuTemperature = GetCPUTemp ();
            snprintf(Str, sizeof(Str), "CPU (C): %dc", lcdCtx.cpuTemperature);
            lcd->setCursor(1, 20);
            lcd->print(Str);

            lcdCtx.cpuUsage = GetCPULoad ();
            snprintf(Str, sizeof(Str), "CPU (%): %d%", lcdCtx.cpuUsage);
            lcd->setCursor(1, 30);
            lcd->print(Str);

            lcd->refresh ();
        }

        if (timerNTM->checkTimer (1) == 1) {
            outgoingNrf24l01 ();
        }
        
        if (timerRUD->checkTimer (60) == 1) {
            clientHandler->removeUnusedDeveices ();
        }
    }
	
	wiseDB->stop();
	delete wiseDB;

	delete sensor;
    delete timerNTM;
    delete timerRUD;
    
    pthread_mutex_destroy (&msgPullSyncContext.mutex);
    pthread_cond_destroy  (&msgPullSyncContext.cond);
    
    pthread_exit (NULL);
    
    exit(EXIT_SUCCESS);
}

/*
WiseIPC *ipcNrfOut = new WiseIPC ("/opt/wiseup/nrf_outgoing_queue");
if (ipcNrfOut->setClient () == SUCCESS) {
    printBuffer (" --> TX ", m_net->ptrTX, 32);
        
    ipcNrfOut->setBuffer((unsigned char *)buffer);
    if (ipcNrfOut->sendMsg(32) == false) { }
    } else {
        sprintf ((char *) wise_stdout_buffer, "(wise-nrfd) [ERROR] - No available outgoing_pipe");
        stdout_msg ();
    }
delete ipcNrfOut;
*/
