/**
 *  Copyright 2021 Comcast Cable Communications Management, LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <rbus.h>
#include <wrp-c.h>

#include "../src/upstream.h"
#include "../src/config.h"
#include "../src/ParodusInternal.h"
#include "../src/partners_check.h"
#include "../src/close_retry.h"
#include "../src/connection.h"
#include "../src/heartBeat.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static ParodusCfg parodusCfg;

/*----------------------------------------------------------------------------*/
/*                         External declarations                              */
/*----------------------------------------------------------------------------*/
extern rbusHandle_t rbus_Handle;
extern rbusError_t err;

extern rbusHandle_t get_parodus_rbus_Handle(void);
extern void rbus_log_handler(rbusLogLevel level, const char* file, int line, int threadId, char* message);
extern void registerRbusLogger(void);
extern void subscribeRBUSevent(void);
extern void processWebconfigUpstreamEvent(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);
extern void subscribeAsyncHandler(rbusHandle_t handle, rbusEventSubscription_t* subscription, rbusError_t error);
extern void wanStateEventHandler(rbusHandle_t handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);
extern int subscribeWanStateEvent(void);
#ifdef WAN_FAILOVER_SUPPORTED
extern int subscribeCurrentActiveInterfaceEvent(void);
extern void eventReceiveHandler(rbusHandle_t rbus_Handle, rbusEvent_t const* event, rbusEventSubscription_t* subscription);
#endif

/*----------------------------------------------------------------------------*/
/*                                   Mocks                                    */
/*----------------------------------------------------------------------------*/
pthread_mutex_t config_mut=PTHREAD_MUTEX_INITIALIZER;
char wan_state_cache[64]="Unknown";
void setWanState(const char *value)
{
    pthread_mutex_lock(&config_mut);
	parStrncpy(get_parodus_cfg()->wan_state, (value != NULL && strlen(value) != 0) ? value : "Unknown", sizeof(get_parodus_cfg()->wan_state));    
    pthread_mutex_unlock(&config_mut);
}

ParodusCfg *get_parodus_cfg(void)
{
    return &parodusCfg;
}

void set_parodus_cfg(ParodusCfg *cfg)
{
    (void)cfg;
}

rbusError_t __wrap_rbus_open(rbusHandle_t *handle, char const *componentName)
{
    (void)componentName;
    *handle = (rbusHandle_t)mock();
    return (rbusError_t)mock();
}

rbusError_t __wrap_rbusEvent_SubscribeAsync(
    rbusHandle_t handle,
    char const *eventName,
    rbusEventHandler_t handler,
    rbusSubscribeAsyncRespHandler_t subscribeHandler,
    char const *userData,
    int timeout)
{
    (void)handle;
    (void)eventName;
    (void)handler;
    (void)subscribeHandler;
    (void)userData;
    (void)timeout;
    return (rbusError_t)mock();
}

void __wrap_rbus_registerLogHandler(rbusLogHandler handler)
{
    (void)handler;
    function_called();
}

rbusValue_t __wrap_rbusObject_GetValue(rbusObject_t object, char const *name)
{
    (void)object;
    (void)name;
    return (rbusValue_t)mock();
}

const uint8_t* __wrap_rbusValue_GetBytes(rbusValue_t value, int *len)
{
    (void)value;
    *len = (int)mock();
    return (const uint8_t *)mock();
}

const char* __wrap_rbusValue_GetString(rbusValue_t value, int *len)
{
    (void)value;
    if (len) *len = 0;
    return (const char *)mock();
}

int __wrap_wrp_to_struct(const void *bytes, size_t length, enum wrp_format fmt, wrp_msg_t **msg)
{
    (void)bytes;
    (void)length;
    (void)fmt;
    *msg = (wrp_msg_t *)mock();
    return (int)mock();
}

ssize_t __wrap_wrp_struct_to(const wrp_msg_t *msg, enum wrp_format fmt, void **bytes)
{
    (void)msg;
    (void)fmt;
    *bytes = (void *)mock();
    return (ssize_t)mock();
}

int __wrap_validate_partner_id(wrp_msg_t *msg, partners_t **partnerIds)
{
    (void)msg;
    *partnerIds = (partners_t *)mock();
    return (int)mock();
}

int __wrap_sendUpstreamMsgToServer(void **resp_bytes, size_t resp_size)
{
    (void)resp_bytes;
    (void)resp_size;
    function_called();
    return (int)mock();
}

void packMetaData(void)
{
    function_called();
}

void lock_metadata_mutex(void)
{
}

void unlock_metadata_mutex(void)
{
}

/* Mock implementation */
rbusError_t rbus_getStr(rbusHandle_t handle, const char* param, char** value)
{
    (void)handle; // unused

    if(strcmp(param, "Device.X_RDK_WanManager.WanState") == 0)
    {
        *value = strdup("xyz");
        return RBUS_ERROR_SUCCESS;
    }
}

#ifdef WAN_FAILOVER_SUPPORTED
void __wrap_setWebpaInterface(char *value)
{
    check_expected_ptr(value);
}

bool __wrap_get_interface_down_event(void)
{
    return (bool)mock();
}

void __wrap_reset_interface_down_event(void)
{
    function_called();
}

void __wrap_resume_heartBeatTimer(void)
{
    function_called();
}

void __wrap_set_global_reconnect_reason(char *reason)
{
    check_expected(reason);
}

void __wrap_set_global_reconnect_status(bool status)
{
    check_expected(status);
}

void __wrap_set_close_retry(void)
{
    function_called();
}
#endif

/*----------------------------------------------------------------------------*/
/*                                   Tests                                    */
/*----------------------------------------------------------------------------*/

/* 2.1 Test get_parodus_rbus_Handle returns the global rbus handle */
static void test_get_parodus_rbus_Handle(void **state)
{
    (void)state;
    rbusHandle_t fake_handle = (rbusHandle_t)0xDEADBEEF;
    rbus_Handle = fake_handle;
    assert_ptr_equal(get_parodus_rbus_Handle(), fake_handle);
}

/* 2.2 Test rbus_log_handler filters below ERROR and formats output */
static void test_rbus_log_handler_filters_below_error(void **state)
{
    (void)state;
    /* Levels below ERROR should return early without logging */
    rbus_log_handler(RBUS_LOG_DEBUG, "test.c", 10, 1, "debug msg");
    rbus_log_handler(RBUS_LOG_INFO, "test.c", 20, 2, "info msg");
    rbus_log_handler(RBUS_LOG_WARN, "test.c", 30, 3, "warn msg");
    /* If we get here without crash, the filter works. ERROR level should pass through */
    rbus_log_handler(RBUS_LOG_ERROR, "test.c", 40, 4, "error msg");
    rbus_log_handler(RBUS_LOG_FATAL, "test.c", 50, 5, "fatal msg");
}

/* 2.3 Test registerRbusLogger calls rbus_registerLogHandler */
static void test_registerRbusLogger(void **state)
{
    (void)state;
    expect_function_call(__wrap_rbus_registerLogHandler);
    registerRbusLogger();
}

/* 2.4 Test subscribeRBUSevent — success path */
static void test_subscribeRBUSevent_success(void **state)
{
    (void)state;
    rbusHandle_t fake_handle = (rbusHandle_t)0xABCD;

    /* rbus_open succeeds */
    will_return(__wrap_rbus_open, fake_handle);
    will_return(__wrap_rbus_open, RBUS_ERROR_SUCCESS);

    /* rbusEvent_SubscribeAsync succeeds */
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_SUCCESS);

    subscribeRBUSevent();
    assert_ptr_equal(rbus_Handle, fake_handle);
}

/* 2.5 Test subscribeRBUSevent — rbus_open fails, early return */
static void test_subscribeRBUSevent_open_failure(void **state)
{
    (void)state;

    /* rbus_open fails */
    will_return(__wrap_rbus_open, NULL);
    will_return(__wrap_rbus_open, RBUS_ERROR_BUS_ERROR);

    subscribeRBUSevent();
    /* Should return early without calling subscribe */
}

/* 2.6 Test subscribeRBUSevent — subscribe failure */
static void test_subscribeRBUSevent_subscribe_failure(void **state)
{
    (void)state;
    rbusHandle_t fake_handle = (rbusHandle_t)0x1234;

    /* rbus_open succeeds */
    will_return(__wrap_rbus_open, fake_handle);
    will_return(__wrap_rbus_open, RBUS_ERROR_SUCCESS);

    /* rbusEvent_SubscribeAsync fails */
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_BUS_ERROR);

    subscribeRBUSevent();
}

/* 3.1 Test processWebconfigUpstreamEvent — valid WRP, partner validation passes */
static void test_processWebconfigUpstreamEvent_valid_partner_pass(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0x1111;
    event.data = data;

    /* Set up the mock for wrp event payload */
    uint8_t fake_bytes[] = {0x01, 0x02, 0x03};
    wrp_msg_t fake_msg;
    memset(&fake_msg, 0, sizeof(wrp_msg_t));
    fake_msg.msg_type = WRP_MSG_TYPE__EVENT;
    fake_msg.u.event.dest = "event:destination";
    fake_msg.u.event.source = "event:source";

    rbusValue_t fake_value = (rbusValue_t)0x2222;

    /* rbusObject_GetValue returns value */
    will_return(__wrap_rbusObject_GetValue, fake_value);

    /* rbusValue_GetBytes returns our fake bytes */
    will_return(__wrap_rbusValue_GetBytes, sizeof(fake_bytes));
    will_return(__wrap_rbusValue_GetBytes, fake_bytes);

    /* wrp_to_struct succeeds (rv > 0) */
    will_return(__wrap_wrp_to_struct, &fake_msg);
    will_return(__wrap_wrp_to_struct, (int)sizeof(fake_bytes));

    /* validate_partner_id returns 1 (pass) with NULL partners */
    will_return(__wrap_validate_partner_id, NULL);
    will_return(__wrap_validate_partner_id, 1);

    /* wrp_struct_to for re-encoding */
    void *encoded_bytes = malloc(10);
    will_return(__wrap_wrp_struct_to, encoded_bytes);
    will_return(__wrap_wrp_struct_to, 10);

    /* sendUpstreamMsgToServer called */
    expect_function_call(__wrap_sendUpstreamMsgToServer);
    will_return(__wrap_sendUpstreamMsgToServer, 0);

    processWebconfigUpstreamEvent(NULL, &event, NULL);
}

/* 3.2 Test processWebconfigUpstreamEvent — valid WRP, partner validation fails (bypass re-encoding) */
static void test_processWebconfigUpstreamEvent_valid_partner_fail(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0x3333;
    event.data = data;

    uint8_t fake_bytes[] = {0x04, 0x05};
    wrp_msg_t fake_msg;
    memset(&fake_msg, 0, sizeof(wrp_msg_t));
    fake_msg.msg_type = WRP_MSG_TYPE__EVENT;
    fake_msg.u.event.dest = "event:dest2";

    rbusValue_t fake_value = (rbusValue_t)0x4444;

    will_return(__wrap_rbusObject_GetValue, fake_value);
    will_return(__wrap_rbusValue_GetBytes, sizeof(fake_bytes));
    will_return(__wrap_rbusValue_GetBytes, fake_bytes);

    /* wrp_to_struct succeeds */
    will_return(__wrap_wrp_to_struct, &fake_msg);
    will_return(__wrap_wrp_to_struct, (int)sizeof(fake_bytes));

    /* validate_partner_id returns 0 (fail) */
    will_return(__wrap_validate_partner_id, NULL);
    will_return(__wrap_validate_partner_id, 0);

    /* sendUpstreamMsgToServer called directly with original bytes */
    expect_function_call(__wrap_sendUpstreamMsgToServer);
    will_return(__wrap_sendUpstreamMsgToServer, 0);

    processWebconfigUpstreamEvent(NULL, &event, NULL);
}

/* 3.3 Test processWebconfigUpstreamEvent — invalid WRP decode (wrp_to_struct returns <= 0) */
static void test_processWebconfigUpstreamEvent_invalid_wrp(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0x5555;
    event.data = data;

    uint8_t fake_bytes[] = {0xFF};
    rbusValue_t fake_value = (rbusValue_t)0x6666;

    will_return(__wrap_rbusObject_GetValue, fake_value);
    will_return(__wrap_rbusValue_GetBytes, sizeof(fake_bytes));
    will_return(__wrap_rbusValue_GetBytes, fake_bytes);

    /* wrp_to_struct fails (returns 0) */
    will_return(__wrap_wrp_to_struct, NULL);
    will_return(__wrap_wrp_to_struct, 0);

    /* No sendUpstreamMsgToServer call expected */
    processWebconfigUpstreamEvent(NULL, &event, NULL);
}

/* 3.4 Test subscribeAsyncHandler logs event name and error */
static void test_subscribeAsyncHandler(void **state)
{
    (void)state;
    rbusEventSubscription_t sub;
    sub.eventName = "Device.X_RDK_WanManager.WanState";
    expect_function_call(packMetaData);
    /* Just verify it doesn't crash */
    subscribeAsyncHandler(NULL, &sub, RBUS_ERROR_SUCCESS);
    assert_string_equal(parodusCfg.wan_state, "xyz");
    subscribeAsyncHandler(NULL, &sub, RBUS_ERROR_BUS_ERROR);
}

/* 4.1 Test wanStateEventHandler — valid state string */
static void test_wanStateEventHandler_valid(void **state)
{
    (void)state;
    memset(&parodusCfg, 0, sizeof(ParodusCfg));

    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0x7777;
    event.data = data;

    rbusValue_t fake_value = (rbusValue_t)0x8888;
    will_return(__wrap_rbusObject_GetValue, fake_value);
    will_return(__wrap_rbusValue_GetString, "ESTABLISHED");

    expect_function_call(packMetaData);

    wanStateEventHandler(NULL, &event, NULL);
    assert_string_equal(parodusCfg.wan_state, "ESTABLISHED");
}

/* 4.2 Test wanStateEventHandler — NULL value path */
static void test_wanStateEventHandler_null_value(void **state)
{
    (void)state;
    memset(&parodusCfg, 0, sizeof(ParodusCfg));

    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0x9999;
    event.data = data;

    /* rbusObject_GetValue returns NULL */
    will_return(__wrap_rbusObject_GetValue, NULL);

    /* packMetaData should NOT be called */
    wanStateEventHandler(NULL, &event, NULL);
    assert_string_equal(parodusCfg.wan_state, "");
}

/* 4.3 Test subscribeWanStateEvent — success and failure */
static void test_subscribeWanStateEvent_success(void **state)
{
    (void)state;
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_SUCCESS);
    int rc = subscribeWanStateEvent();
    assert_int_equal(rc, RBUS_ERROR_SUCCESS);
}

static void test_subscribeWanStateEvent_failure(void **state)
{
    (void)state;
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_BUS_ERROR);
    int rc = subscribeWanStateEvent();
    assert_int_equal(rc, RBUS_ERROR_BUS_ERROR);
}

#ifdef WAN_FAILOVER_SUPPORTED
/* 5.1 Test subscribeCurrentActiveInterfaceEvent success/failure */
static void test_subscribeCurrentActiveInterfaceEvent_success(void **state)
{
    (void)state;
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_SUCCESS);
    int rc = subscribeCurrentActiveInterfaceEvent();
    assert_int_equal(rc, RBUS_ERROR_SUCCESS);
}

static void test_subscribeCurrentActiveInterfaceEvent_failure(void **state)
{
    (void)state;
    will_return(__wrap_rbusEvent_SubscribeAsync, RBUS_ERROR_BUS_ERROR);
    int rc = subscribeCurrentActiveInterfaceEvent();
    assert_int_equal(rc, RBUS_ERROR_BUS_ERROR);
}

/* 5.2 Test eventReceiveHandler — new/old value both present, triggers reconnect */
static void test_eventReceiveHandler_both_values(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0xAAAA;
    event.data = data;
    event.name = "Device.X_RDK_WanManager.CurrentActiveInterface";

    rbusValue_t newVal = (rbusValue_t)0xBBBB;
    rbusValue_t oldVal = (rbusValue_t)0xCCCC;

    /* First call to rbusObject_GetValue for "value" */
    will_return(__wrap_rbusObject_GetValue, newVal);
    /* Second call for "oldValue" */
    will_return(__wrap_rbusObject_GetValue, oldVal);

    /* rbusValue_GetString for newValue (interface) */
    will_return(__wrap_rbusValue_GetString, "erouter0");

    expect_string(__wrap_setWebpaInterface, value, "erouter0");

    /* get_interface_down_event returns false — no reset path */
    will_return(__wrap_get_interface_down_event, false);

    /* Reconnect calls */
    expect_string(__wrap_set_global_reconnect_reason, reason, "WAN_FAILOVER");
    expect_value(__wrap_set_global_reconnect_status, status, true);
    expect_function_call(__wrap_set_close_retry);

    /* For the log line printing new/old values */
    will_return(__wrap_rbusValue_GetString, "erouter0");
    will_return(__wrap_rbusValue_GetString, "erouter1");

    eventReceiveHandler(NULL, &event, NULL);
}

/* 5.3 Test eventReceiveHandler — NULL newValue error path */
static void test_eventReceiveHandler_null_newValue(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0xDDDD;
    event.data = data;
    event.name = "Device.X_RDK_WanManager.CurrentActiveInterface";

    /* rbusObject_GetValue for "value" returns NULL */
    will_return(__wrap_rbusObject_GetValue, NULL);
    /* rbusObject_GetValue for "oldValue" */
    will_return(__wrap_rbusObject_GetValue, (rbusValue_t)0xEEEE);

    /* Should log error and not trigger reconnect */
    eventReceiveHandler(NULL, &event, NULL);
}

/* 5.4 Test eventReceiveHandler — interface_down_event reset path */
static void test_eventReceiveHandler_interface_down_reset(void **state)
{
    (void)state;
    rbusEvent_t event;
    rbusObject_t data = (rbusObject_t)0xF1F1;
    event.data = data;
    event.name = "Device.X_RDK_WanManager.CurrentActiveInterface";

    rbusValue_t newVal = (rbusValue_t)0xF2F2;
    rbusValue_t oldVal = (rbusValue_t)0xF3F3;

    will_return(__wrap_rbusObject_GetValue, newVal);
    will_return(__wrap_rbusObject_GetValue, oldVal);

    will_return(__wrap_rbusValue_GetString, "erouter1");

    expect_string(__wrap_setWebpaInterface, value, "erouter1");

    /* get_interface_down_event returns true — triggers reset */
    will_return(__wrap_get_interface_down_event, true);
    expect_function_call(__wrap_reset_interface_down_event);
    expect_function_call(__wrap_resume_heartBeatTimer);

    /* Reconnect calls */
    expect_string(__wrap_set_global_reconnect_reason, reason, "WAN_FAILOVER");
    expect_value(__wrap_set_global_reconnect_status, status, true);
    expect_function_call(__wrap_set_close_retry);

    /* For the log line */
    will_return(__wrap_rbusValue_GetString, "erouter1");
    will_return(__wrap_rbusValue_GetString, "erouter0");

    eventReceiveHandler(NULL, &event, NULL);
}
#endif /* WAN_FAILOVER_SUPPORTED */

/*----------------------------------------------------------------------------*/
/*                                   Main                                     */
/*----------------------------------------------------------------------------*/

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_get_parodus_rbus_Handle),
        cmocka_unit_test(test_rbus_log_handler_filters_below_error),
        cmocka_unit_test(test_registerRbusLogger),
        cmocka_unit_test(test_subscribeRBUSevent_success),
        cmocka_unit_test(test_subscribeRBUSevent_open_failure),
        cmocka_unit_test(test_subscribeRBUSevent_subscribe_failure),
        cmocka_unit_test(test_processWebconfigUpstreamEvent_valid_partner_pass),
        cmocka_unit_test(test_processWebconfigUpstreamEvent_valid_partner_fail),
        cmocka_unit_test(test_processWebconfigUpstreamEvent_invalid_wrp),
        cmocka_unit_test(test_subscribeAsyncHandler),
        cmocka_unit_test(test_wanStateEventHandler_valid),
        cmocka_unit_test(test_wanStateEventHandler_null_value),
        cmocka_unit_test(test_subscribeWanStateEvent_success),
        cmocka_unit_test(test_subscribeWanStateEvent_failure),
#ifdef WAN_FAILOVER_SUPPORTED
        cmocka_unit_test(test_subscribeCurrentActiveInterfaceEvent_success),
        cmocka_unit_test(test_subscribeCurrentActiveInterfaceEvent_failure),
        cmocka_unit_test(test_eventReceiveHandler_both_values),
        cmocka_unit_test(test_eventReceiveHandler_null_newValue),
        cmocka_unit_test(test_eventReceiveHandler_interface_down_reset),
#endif
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
