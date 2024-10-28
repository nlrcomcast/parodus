/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 * Copyright [2014] [Cisco Systems, Inc.]
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/* This is a stub file that will be overridden in a patch */

#include "event_handler.h"
#include <pthread.h>
#include <unistd.h>
#include "ParodusInternal.h"
static pthread_t sysevent_tid;
static void *parodus_sysevent_handler (void *data)
{
	ParodusInfo("parodus_sysevent_handler thread started\n");
    while (1)
    {
        sleep(1);
        if(access( "/tmp/wan_down" , F_OK ) == 0)
        {
			ParodusInfo("Received wan-stop event, Close the connection\n");
			set_global_reconnect_reason("wan_down");
			set_global_reconnect_status(true);
		  	set_interface_down_event();
			ParodusInfo("Interface_down_event is set\n");				
			pause_heartBeatTimer();            
            sleep(10);
        }
        if(access( "/tmp/wan_up" , F_OK ) == 0)
        {
            ParodusInfo("Received wan-started event, Close the connection and retry again \n");
            reset_interface_down_event();
            ParodusInfo("Interface_down_event is reset\n");
            resume_heartBeatTimer();
            set_close_retry();
            sleep(10);
        }

    }
    
}
void EventHandler()
{


		pthread_create(&sysevent_tid, NULL, parodus_sysevent_handler, NULL);

	return;    
}