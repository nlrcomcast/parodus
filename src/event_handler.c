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
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include "connection.h"
#include "close_retry.h"
static pthread_t sysevent_tid;
static void *parodus_sysevent_handler (void *data)
{
    for (;;)
    {
 	FILE *file;
	if((file = fopen("/tmp/parodus_sysevent", "r")) != NULL) {       
				    ParodusInfo("Received firewall_flush_conntrack event,Close the connection and retry again\n");		
				    set_global_reconnect_reason("Firewall_Restart");
				    set_global_reconnect_status(true);
				    set_close_retry();
                    break;
    }
    sleep(2);                        
    }
    while(1)
    {
	    sleep(10);
	}	    
}
void EventHandler()
{
    ParodusInfo("Registered Conn Status Change callback function \n");
    pthread_create(&sysevent_tid, NULL, parodus_sysevent_handler, NULL);    
}

