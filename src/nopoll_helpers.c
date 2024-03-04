/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file nopoll_helpers.c
 *
 * @description This file is used to manage incomming and outgoing messages.
 *
 */

#include "ParodusInternal.h"
#include "connection.h"
#include "nopoll_helpers.h"
#include "nopoll_handlers.h"
#include "time.h"
#include "config.h"

/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/

#define MAX_SEND_SIZE (60 * 1024)
#define FLUSH_WAIT_TIME (2000000LL)

struct timespec connStuck_start,connStuck_end;
struct timespec *connStuck_startPtr = &connStuck_start;
struct timespec *connStuck_endPtr = &connStuck_end;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

void setMessageHandlers()
{
    nopoll_conn_set_on_msg(get_global_conn(), (noPollOnMessageHandler) listenerOnMessage_queue, NULL);
    nopoll_conn_set_on_ping_msg(get_global_conn(), (noPollOnMessageHandler)listenerOnPingMessage, NULL);
    nopoll_conn_set_on_close(get_global_conn(), (noPollOnCloseHandler)listenerOnCloseMessage, NULL);
}

int cloud_status_is_online (void)
{
	const char *status = get_cloud_status();
	if (NULL == status)
	  return false;
	return (strcmp (status, CLOUD_STATUS_ONLINE) == 0);
}
bool file_exists1(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp); // close the file
    }
    return is_exist;
}
/** To send upstream msgs to server ***/

int sendMessage(noPollConn *conn, void *msg, size_t len)
{
    int bytesWritten = 0;

    if (!cloud_status_is_online ()) {
        ParodusError("Failed to send msg upstream as connection is not OK\n");
        OnboardLog("Failed to send msg upstream as connection is not OK\n");
		return 1;
	}

    ParodusInfo("sendMessage length %zu\n", len);
        if (file_exists1("/tmp/test"))
        {
ParodusInfo("id:%d, session=%d, host_name=%s, peer_close_status=%d, peer_close_reason=%s \n",conn->id,conn->session,conn->host_name,conn->peer_close_status,conn->peer_close_reason);
                nopoll_conn_shutdown(conn);
ParodusInfo("id:%d, session=%d, host_name=%s, peer_close_status=%d, peer_close_reason=%s \n",conn->id,conn->session,conn->host_name,conn->peer_close_status,conn->peer_close_reason);
        }
    bytesWritten = sendResponse(conn, msg, len);
    ParodusPrint("Number of bytes written: %d\n", bytesWritten);
    if (bytesWritten != (int) len) 
    {
        ParodusError("Failed to send bytes %zu, bytes written were=%d (errno=%d, %s)..\n", len, bytesWritten, errno, strerror(errno));
	return 1;
    }
	return 0;
}

int sendResponse(noPollConn * conn, void * buffer, size_t length)
{
    char *cp = buffer;
    int final_len_sent = 0;
    noPollOpCode frame_type = NOPOLL_BINARY_FRAME;

    while (length > 0) 
    {
        int bytes_sent, len_to_send;

        len_to_send = length > MAX_SEND_SIZE ? MAX_SEND_SIZE : length;
        length -= len_to_send;
        bytes_sent = __nopoll_conn_send_common(conn, cp, len_to_send, length > 0 ? nopoll_false : nopoll_true, 0, frame_type);

        if (bytes_sent != len_to_send) 
        {
            if (-1 == bytes_sent || (bytes_sent = nopoll_conn_flush_writes(conn, FLUSH_WAIT_TIME, bytes_sent)) != len_to_send)
            {
                ParodusError("sendResponse() Failed to send all the data\n");
                cp = NULL;
                break;
            }
        }
        cp += len_to_send;
        final_len_sent += len_to_send;
        frame_type = NOPOLL_CONTINUATION_FRAME;
    }
    return final_len_sent;
}

/**
 * @brief __report_log Nopoll log handler 
 * Nopoll log handler for integrating nopoll logs 
 */
void __report_log (noPollCtx * ctx, noPollDebugLevel level, const char * log_msg, noPollPtr user_data)
{
    UNUSED(ctx);
    UNUSED(user_data);
    
    if (level == NOPOLL_LEVEL_DEBUG) 
    {
        //ParodusPrint("%s\n", log_msg);
    }
    if (level == NOPOLL_LEVEL_INFO) 
    {
        ParodusInfo ("%s\n", log_msg);
    }
    if (level == NOPOLL_LEVEL_WARNING) 
    {
        ParodusPrint("%s\n", log_msg);
    }
    if (level == NOPOLL_LEVEL_CRITICAL) 
    {
        ParodusError("%s\n", log_msg );
	OnboardLog("%s\n", log_msg );
    }
    return;
}

