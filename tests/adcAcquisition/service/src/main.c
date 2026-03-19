/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-01-16
 * @brief     ADC Acquisition Service Tests
 *
 *            Unit tests for ADC acquisition service functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Mock osMemoryPool type */
typedef void *osMemoryPoolId_t;

/* Prevent CMSIS OS2 header */
#define CMSIS_OS2_H_

/* Prevent utility and filter headers */
#define ADC_ACQUISITION_UTIL_H
#define ADC_ACQUISITION_FILTER_H

/* Prevent ADC driver header */
#define ZEPHYR_INCLUDE_DRIVERS_ADC_H_

/* Prevent serviceCommon header - we define types manually */
#define SERVICE_COMMON_H

/* Provide ServiceCtrlMsg_t definition */
typedef enum
{
  SVC_CTRL_STOP,
  SVC_CTRL_SUSPEND,
} ServiceCtrlMsg_t;

/* Prevent serviceManager header - we define types manually */
#define SERVICE_MANAGER_H

/* Provide ServiceState_t definition */
typedef enum
{
  SVC_STATE_STOPPED,
  SVC_STATE_RUNNING,
  SVC_STATE_SUSPENDED,
} ServiceState_t;

/* Provide ServicePriority_t definition */
typedef enum
{
  SVC_PRIORITY_CRITICAL = 0,
  SVC_PRIORITY_CORE,
  SVC_PRIORITY_APPLICATION,
  SVC_PRIORITY_COUNT,
} ServicePriority_t;

/* Provide ServiceDescriptor_t definition */
typedef struct
{
  k_tid_t threadId;
  ServicePriority_t priority;
  uint32_t heartbeatIntervalMs;
  int64_t lastHeartbeatMs;
  uint8_t missedHeartbeats;
  ServiceState_t state;
  int (*start)(void);
  int (*stop)(void);
  int (*suspend)(void);
  int (*resume)(void);
} ServiceDescriptor_t;

/* Prevent adcAcquisition header - we define types manually */
#define ADC_ACQUISITION

/* Wrap k_sleep to use mock */
#define k_sleep k_sleep_mock

/* Wrap k_thread_create to use mock */
#define k_thread_create k_thread_create_mock

/* Wrap k_thread_name_set to use mock */
#define k_thread_name_set k_thread_name_set_mock

/* Wrap k_thread_start to use mock */
#define k_thread_start k_thread_start_mock

/* Wrap k_thread_suspend to use mock */
#define k_thread_suspend k_thread_suspend_mock

/* Wrap k_thread_resume to use mock */
#define k_thread_resume k_thread_resume_mock

/* Wrap k_current_get to use mock */
#define k_current_get k_current_get_mock

/* Wrap k_msgq_put to use mock */
#define k_msgq_put k_msgq_put_mock

/* Wrap k_msgq_get to use mock */
#define k_msgq_get k_msgq_get_mock

/* Define service name before including source */
#define ADC_AQC_SERVICE_NAME adcAcquisition

/* Define test mode iteration count */
#define ADC_ACQ_RUN_ITERATIONS 2

/* Provide SrvMsgPayload_t definition */
typedef struct
{
  osMemoryPoolId_t poolId;
  size_t dataLen;
  uint8_t data[];
} SrvMsgPayload_t;

/* Provide AdcSubCallback_t type */
typedef int (*AdcSubCallback_t)(SrvMsgPayload_t *data);

/* Provide AdcConfig_t type */
typedef struct
{
  uint32_t samplingRate;
  int32_t filterTau;
} AdcConfig_t;

/* Provide AdcSubConfig_t type */
typedef struct
{
  size_t maxSubCount;
  size_t activeSubCount;
  uint32_t notificationRate;
} AdcSubConfig_t;

/* Mock kernel functions */
FAKE_VOID_FUNC(k_sleep_mock, k_timeout_t);
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *,
                size_t, k_thread_entry_t, void *, void *, void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
FAKE_VOID_FUNC(k_thread_start_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_suspend_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_resume_mock, k_tid_t);
FAKE_VALUE_FUNC(k_tid_t, k_current_get_mock);
FAKE_VALUE_FUNC(int, k_msgq_put_mock, struct k_msgq *, const void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_msgq_get_mock, struct k_msgq *, void *, k_timeout_t);

/* Mock service manager functions */
FAKE_VALUE_FUNC(int, serviceManagerConfirmState, k_tid_t, ServiceState_t);
FAKE_VALUE_FUNC(int, serviceManagerUpdateHeartbeat, k_tid_t);
FAKE_VALUE_FUNC(int, serviceManagerRegisterSrv, const ServiceDescriptor_t *);

/* Mock utility functions */
FAKE_VALUE_FUNC(int, adcAcqUtilInitAdc, AdcConfig_t *);
FAKE_VALUE_FUNC(int, adcAcqUtilInitSubscriptions, AdcSubConfig_t *);
FAKE_VALUE_FUNC(int, adcAcqUtilStartTrigger);
FAKE_VALUE_FUNC(int, adcAcqUtilStopTrigger);
FAKE_VALUE_FUNC(int, adcAcqUtilProcessData);
FAKE_VALUE_FUNC(int, adcAcqUtilNotifySubscribers);
FAKE_VALUE_FUNC(size_t, adcAcqUtilGetChanCount);
FAKE_VALUE_FUNC(int, adcAcqUtilAddSubscription, AdcSubCallback_t);
FAKE_VALUE_FUNC(int, adcAcqUtilRemoveSubscription, AdcSubCallback_t);
FAKE_VALUE_FUNC(int, adcAcqUtilSetSubPauseState, AdcSubCallback_t, bool);

/* Mock filter functions */
FAKE_VALUE_FUNC(int, adcAcqFilterInit, size_t);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(k_sleep_mock) \
  FAKE(k_thread_create_mock) \
  FAKE(k_thread_name_set_mock) \
  FAKE(k_thread_start_mock) \
  FAKE(k_thread_suspend_mock) \
  FAKE(k_thread_resume_mock) \
  FAKE(k_current_get_mock) \
  FAKE(k_msgq_put_mock) \
  FAKE(k_msgq_get_mock) \
  FAKE(serviceManagerConfirmState) \
  FAKE(serviceManagerUpdateHeartbeat) \
  FAKE(serviceManagerRegisterSrv) \
  FAKE(adcAcqUtilInitAdc) \
  FAKE(adcAcqUtilInitSubscriptions) \
  FAKE(adcAcqUtilStartTrigger) \
  FAKE(adcAcqUtilStopTrigger) \
  FAKE(adcAcqUtilProcessData) \
  FAKE(adcAcqUtilNotifySubscribers) \
  FAKE(adcAcqUtilGetChanCount) \
  FAKE(adcAcqUtilAddSubscription) \
  FAKE(adcAcqUtilRemoveSubscription) \
  FAKE(adcAcqUtilSetSubPauseState) \
  FAKE(adcAcqFilterInit)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adcAcquisition, LOG_LEVEL_DBG);

/* Prevent LOG_MODULE_REGISTER in source file */
#undef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)

/* Mock Kconfig options needed by source */
#define CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE          1024
#define CONFIG_ENYA_ADC_ACQUISITION_SAMPLING_RATE_US    500
#define CONFIG_ENYA_ADC_ACQUISITION_FILTER_TAU          31
#define CONFIG_ENYA_ADC_ACQUISITION_MAX_SUB_COUNT       4
#define CONFIG_ENYA_ADC_ACQUISITION_NOTIFICATION_RATE_MS 100
#define CONFIG_ENYA_ADC_ACQUISITION_THREAD_PRIORITY     5
#define CONFIG_ENYA_ADC_ACQUISITION_SERVICE_PRIORITY    1
#define CONFIG_ENYA_ADC_ACQUISITION_HEARTBEAT_INTERVAL_MS 1000

/* Include service implementation */
#include "adcAcquisition.c"

/* Capture helper for k_msgq_put — saves the message value before the caller's stack unwinds */
static ServiceCtrlMsg_t captured_ctrl_msg;

static int k_msgq_put_capture(struct k_msgq *q, const void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(data)
    captured_ctrl_msg = *(const ServiceCtrlMsg_t *)data;
  return k_msgq_put_mock_fake.return_val;
}

/* Descriptor capture helper for serviceManagerRegisterSrv */
static ServiceDescriptor_t captured_descriptor;

static int serviceManagerRegisterSrv_capture(const ServiceDescriptor_t *descriptor)
{
  if(descriptor)
    captured_descriptor = *descriptor;
  return serviceManagerRegisterSrv_fake.return_val;
}

/* Queue test helpers for k_msgq_get */
static ServiceCtrlMsg_t test_ctrl_queue_messages[4];
static size_t test_ctrl_queue_msg_count = 0;
static size_t test_ctrl_queue_msg_read = 0;

static int k_msgq_get_no_message(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(data);
  ARG_UNUSED(timeout);
  return -EAGAIN;
}

static int k_msgq_get_from_ctrl_queue(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(test_ctrl_queue_msg_read < test_ctrl_queue_msg_count)
  {
    memcpy(data, &test_ctrl_queue_messages[test_ctrl_queue_msg_read++], sizeof(ServiceCtrlMsg_t));
    return 0;
  }
  return -EAGAIN;
}

/* Test suite setup and teardown */
static void *service_tests_setup(void)
{
  return NULL;
}

static void service_tests_before(void *f)
{
  ARG_UNUSED(f);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
  test_ctrl_queue_msg_count = 0;
  test_ctrl_queue_msg_read = 0;
  captured_ctrl_msg = (ServiceCtrlMsg_t)0xFF;
  k_msgq_put_mock_fake.custom_fake = k_msgq_put_capture;
  memset(&captured_descriptor, 0, sizeof(captured_descriptor));
  serviceManagerRegisterSrv_fake.custom_fake = serviceManagerRegisterSrv_capture;
}

/**
 * @test The run function must log error but continue when adcAcqUtilStopTrigger fails on STOP.
 */
ZTEST(adc_service_tests, test_run_stop_trigger_failure)
{
  /* Setup: queue delivers STOP, stopTrigger fails */
  test_ctrl_queue_messages[0] = SVC_CTRL_STOP;
  test_ctrl_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;
  adcAcqUtilStopTrigger_fake.return_val = -EIO;

  /* Execute */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify stopTrigger and confirmState were still called */
  zassert_equal(adcAcqUtilStopTrigger_fake.call_count, 1,
                "adcAcqUtilStopTrigger should be called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_STOPPED,
                "serviceManagerConfirmState should be called with SVC_STATE_STOPPED");
  /* Thread returned on STOP: process/notify/heartbeat not called */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 0,
                "adcAcqUtilProcessData should not be called after STOP");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 0,
                "serviceManagerUpdateHeartbeat should not be called after STOP");
}

/**
 * @test The run function must stop ADC trigger and confirm state on STOP message.
 */
ZTEST(adc_service_tests, test_run_stop_success)
{
  /* Setup: first iteration delivers STOP */
  test_ctrl_queue_messages[0] = SVC_CTRL_STOP;
  test_ctrl_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;
  adcAcqUtilStopTrigger_fake.return_val = 0;

  /* Execute */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify STOP flow */
  zassert_equal(adcAcqUtilStopTrigger_fake.call_count, 1,
                "adcAcqUtilStopTrigger should be called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_STOPPED,
                "serviceManagerConfirmState should be called with SVC_STATE_STOPPED");
  /* Thread returned on STOP: no processing after */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 0,
                "adcAcqUtilProcessData should not be called after STOP");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 0,
                "serviceManagerUpdateHeartbeat should not be called after STOP");
}

/**
 * @test The run function must log error but continue when adcAcqUtilStopTrigger fails on SUSPEND.
 */
ZTEST(adc_service_tests, test_run_suspend_trigger_failure)
{
  /* Setup: first iteration delivers SUSPEND, stopTrigger fails */
  test_ctrl_queue_messages[0] = SVC_CTRL_SUSPEND;
  test_ctrl_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;
  adcAcqUtilStopTrigger_fake.return_val = -EIO;

  /* Execute */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify SUSPEND flow */
  zassert_equal(adcAcqUtilStopTrigger_fake.call_count, 1,
                "adcAcqUtilStopTrigger should be called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_SUSPENDED,
                "serviceManagerConfirmState should be called with SVC_STATE_SUSPENDED");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 1,
                "k_thread_suspend should be called once");
}

/**
 * @test The run function must stop ADC trigger, confirm state and suspend thread on SUSPEND message.
 */
ZTEST(adc_service_tests, test_run_suspend_success)
{
  /* Setup: first iteration delivers SUSPEND */
  test_ctrl_queue_messages[0] = SVC_CTRL_SUSPEND;
  test_ctrl_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;
  adcAcqUtilStopTrigger_fake.return_val = 0;

  /* Execute */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify SUSPEND flow */
  zassert_equal(adcAcqUtilStopTrigger_fake.call_count, 1,
                "adcAcqUtilStopTrigger should be called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_SUSPENDED,
                "serviceManagerConfirmState should be called with SVC_STATE_SUSPENDED");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 1,
                "k_thread_suspend should be called once");
}

/**
 * @test The run function must log warning on unknown control message.
 */
ZTEST(adc_service_tests, test_run_unknown_ctrl_msg)
{
  /* Setup: deliver an unknown ctrl message (cast to ServiceCtrlMsg_t) */
  test_ctrl_queue_messages[0] = (ServiceCtrlMsg_t)99;
  test_ctrl_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;

  /* Execute */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify no stop/suspend actions taken */
  zassert_equal(adcAcqUtilStopTrigger_fake.call_count, 0,
                "adcAcqUtilStopTrigger should not be called for unknown message");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 0,
                "serviceManagerConfirmState should not be called for unknown message");
  /* Processing still happens (no early return) */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should still be called for each iteration");
}

/**
 * @test The run function must log error but continue when adcAcqUtilProcessData fails.
 */
ZTEST(adc_service_tests, test_run_process_data_failure)
{
  /* Setup: no ctrl message, adcAcqUtilProcessData returns error */
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_no_message;
  adcAcqUtilProcessData_fake.return_val = -EIO;
  adcAcqUtilNotifySubscribers_fake.return_val = 0;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify queue was polled each iteration */
  zassert_equal(k_msgq_get_mock_fake.call_count, 2,
                "k_msgq_get should be called twice for 2 iterations");

  /* Verify adcAcqUtilProcessData was called twice (2 iterations) */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was still called twice despite process data failure */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should still be called twice");

  /* Verify heartbeat was updated each iteration */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2,
                "serviceManagerUpdateHeartbeat should be called twice");
}

/**
 * @test The run function must log error but continue when adcAcqUtilNotifySubscribers fails.
 */
ZTEST(adc_service_tests, test_run_notify_subscribers_failure)
{
  /* Setup: no ctrl message, adcAcqUtilNotifySubscribers returns error */
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_no_message;
  adcAcqUtilProcessData_fake.return_val = 0;
  adcAcqUtilNotifySubscribers_fake.return_val = -EIO;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify queue was polled each iteration */
  zassert_equal(k_msgq_get_mock_fake.call_count, 2,
                "k_msgq_get should be called twice for 2 iterations");

  /* Verify adcAcqUtilProcessData was called twice */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was called twice despite failure */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should be called twice");

  /* Verify heartbeat was updated each iteration */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2,
                "serviceManagerUpdateHeartbeat should be called twice");
}

/**
 * @test The run function must poll control queue, process data, notify subscribers,
 * and update heartbeat each iteration with correct parameters.
 */
ZTEST(adc_service_tests, test_run_success)
{
  k_timeout_t expected_timeout = K_MSEC(100);

  /* Setup: no ctrl message, all processing succeeds */
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_no_message;
  adcAcqUtilProcessData_fake.return_val = 0;
  adcAcqUtilNotifySubscribers_fake.return_val = 0;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify k_msgq_get was called twice with correct timeout */
  zassert_equal(k_msgq_get_mock_fake.call_count, 2,
                "k_msgq_get should be called twice for 2 iterations");
  zassert_equal(k_msgq_get_mock_fake.arg2_history[0].ticks, expected_timeout.ticks,
                "k_msgq_get should be called with K_MSEC(100) timeout");
  zassert_equal(k_msgq_get_mock_fake.arg2_history[1].ticks, expected_timeout.ticks,
                "k_msgq_get should be called with K_MSEC(100) timeout");

  /* Verify adcAcqUtilProcessData was called twice */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was called twice */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should be called twice");

  /* Verify heartbeat was updated each iteration */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2,
                "serviceManagerUpdateHeartbeat should be called twice");

  /* Verify k_sleep was not called (replaced by k_msgq_get timeout) */
  zassert_equal(k_sleep_mock_fake.call_count, 0,
                "k_sleep should not be called");
}

/**
 * @test The onStart callback must return error when adcAcqUtilStartTrigger fails.
 */
ZTEST(adc_service_tests, test_onStart_trigger_failure)
{
  int result;

  /* Setup: adcAcqUtilStartTrigger fails */
  adcAcqUtilStartTrigger_fake.return_val = -EIO;

  /* Execute */
  result = onStart();

  /* Verify */
  zassert_equal(result, -EIO,
                "onStart should return error from adcAcqUtilStartTrigger");
  zassert_equal(k_thread_start_mock_fake.call_count, 1,
                "k_thread_start should be called once");
  zassert_equal(k_thread_start_mock_fake.arg0_val, &thread,
                "k_thread_start should be called with thread struct");
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/**
 * @test The onStart callback must start the thread and trigger.
 */
ZTEST(adc_service_tests, test_onStart_success)
{
  int result;

  /* Setup: all succeed */
  adcAcqUtilStartTrigger_fake.return_val = 0;

  /* Execute */
  result = onStart();

  /* Verify */
  zassert_equal(result, 0, "onStart should return 0 on success");
  zassert_equal(k_thread_start_mock_fake.call_count, 1,
                "k_thread_start should be called once");
  zassert_equal(k_thread_start_mock_fake.arg0_val, &thread,
                "k_thread_start should be called with thread struct");
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/**
 * @test The onStop callback must return error when k_msgq_put fails.
 */
ZTEST(adc_service_tests, test_onStop_msgq_put_failure)
{
  int result;

  /* Setup: k_msgq_put fails */
  k_msgq_put_mock_fake.return_val = -EAGAIN;

  /* Execute */
  result = onStop();

  /* Verify */
  zassert_equal(result, -EAGAIN,
                "onStop should return error from k_msgq_put");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(captured_ctrl_msg, SVC_CTRL_STOP,
                "k_msgq_put should be called with SVC_CTRL_STOP");
}

/**
 * @test The onStop callback must enqueue a stop message to the control queue.
 */
ZTEST(adc_service_tests, test_onStop_success)
{
  int result;

  /* Setup: k_msgq_put succeeds */
  k_msgq_put_mock_fake.return_val = 0;

  /* Execute */
  result = onStop();

  /* Verify */
  zassert_equal(result, 0, "onStop should return 0 on success");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(captured_ctrl_msg, SVC_CTRL_STOP,
                "k_msgq_put should be called with SVC_CTRL_STOP");
}

/**
 * @test The onSuspend callback must return error when k_msgq_put fails.
 */
ZTEST(adc_service_tests, test_onSuspend_msgq_put_failure)
{
  int result;

  /* Setup: k_msgq_put fails */
  k_msgq_put_mock_fake.return_val = -EAGAIN;

  /* Execute */
  result = onSuspend();

  /* Verify */
  zassert_equal(result, -EAGAIN,
                "onSuspend should return error from k_msgq_put");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(captured_ctrl_msg, SVC_CTRL_SUSPEND,
                "k_msgq_put should be called with SVC_CTRL_SUSPEND");
}

/**
 * @test The onSuspend callback must enqueue a suspend message to the control queue.
 */
ZTEST(adc_service_tests, test_onSuspend_success)
{
  int result;

  /* Setup: k_msgq_put succeeds */
  k_msgq_put_mock_fake.return_val = 0;

  /* Execute */
  result = onSuspend();

  /* Verify */
  zassert_equal(result, 0, "onSuspend should return 0 on success");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(captured_ctrl_msg, SVC_CTRL_SUSPEND,
                "k_msgq_put should be called with SVC_CTRL_SUSPEND");
}

/**
 * @test The onResume callback must return error when adcAcqUtilStartTrigger fails.
 */
ZTEST(adc_service_tests, test_onResume_trigger_failure)
{
  int result;

  /* Setup: adcAcqUtilStartTrigger fails */
  adcAcqUtilStartTrigger_fake.return_val = -EIO;

  /* Execute */
  result = onResume();

  /* Verify */
  zassert_equal(result, -EIO,
                "onResume should return error from adcAcqUtilStartTrigger");
  zassert_equal(k_thread_resume_mock_fake.call_count, 1,
                "k_thread_resume should be called once");
  zassert_equal(k_thread_resume_mock_fake.arg0_val, &thread,
                "k_thread_resume should be called with thread struct");
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/**
 * @test The onResume callback must resume the thread and restart the trigger.
 */
ZTEST(adc_service_tests, test_onResume_success)
{
  int result;

  /* Setup: all succeed */
  adcAcqUtilStartTrigger_fake.return_val = 0;

  /* Execute */
  result = onResume();

  /* Verify */
  zassert_equal(result, 0, "onResume should return 0 on success");
  zassert_equal(k_thread_resume_mock_fake.call_count, 1,
                "k_thread_resume should be called once");
  zassert_equal(k_thread_resume_mock_fake.arg0_val, &thread,
                "k_thread_resume should be called with thread struct");
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/**
 * @test The adcAcqInit function must return error when adcAcqUtilInitAdc fails.
 */
ZTEST(adc_service_tests, test_init_adc_init_failure)
{
  int result;

  /* Setup: adcAcqUtilInitAdc returns error */
  adcAcqUtilInitAdc_fake.return_val = -EIO;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, -EIO,
                "adcAcqInit should return error from adcAcqUtilInitAdc");

  /* Verify adcAcqUtilInitAdc was called once */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");

  /* Verify subsequent functions were not called */
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 0,
                "adcAcqUtilInitSubscriptions should not be called");
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0,
                "serviceManagerRegisterSrv should not be called");
}

/**
 * @test The adcAcqInit function must return error when adcAcqUtilInitSubscriptions fails.
 */
ZTEST(adc_service_tests, test_init_subscriptions_init_failure)
{
  int result;

  /* Setup: adcAcqUtilInitAdc succeeds, adcAcqUtilInitSubscriptions fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from adcAcqUtilInitSubscriptions");

  /* Verify call sequence */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");

  /* Verify subsequent functions were not called */
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0,
                "serviceManagerRegisterSrv should not be called");
}

/**
 * @test The adcAcqInit function must return error when adcAcqFilterInit fails.
 */
ZTEST(adc_service_tests, test_init_filter_init_failure)
{
  int result;

  /* Setup: adcAcqUtilInitAdc and adcAcqUtilInitSubscriptions succeed, adcAcqFilterInit fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from adcAcqFilterInit");

  /* Verify call sequence */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqUtilGetChanCount_fake.call_count, 1,
                "adcAcqUtilGetChanCount should be called once");
  zassert_equal(adcAcqFilterInit_fake.call_count, 1,
                "adcAcqFilterInit should be called once");
  zassert_equal(adcAcqFilterInit_fake.arg0_val, 4,
                "adcAcqFilterInit should be called with channel count 4");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0,
                "serviceManagerRegisterSrv should not be called");
}

/**
 * @test The adcAcqInit function must return error when k_thread_name_set fails.
 */
ZTEST(adc_service_tests, test_init_thread_name_set_failure)
{
  int result;

  /* Setup: all init functions succeed, k_thread_name_set fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = (k_tid_t)&thread;
  k_thread_name_set_mock_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from k_thread_name_set");

  /* Verify k_thread_create and k_thread_name_set were called */
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, (k_tid_t)&thread,
                "k_thread_name_set should be called with the thread ID");

  /* Verify serviceManagerRegisterSrv was NOT called */
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0,
                "serviceManagerRegisterSrv should not be called when name_set fails");
}

/**
 * @test The adcAcqInit function must return error when serviceManagerRegisterSrv fails.
 */
ZTEST(adc_service_tests, test_init_register_fails)
{
  int result;

  /* Setup: all succeed except serviceManagerRegisterSrv */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = (k_tid_t)&thread;
  k_thread_name_set_mock_fake.return_val = 0;
  serviceManagerRegisterSrv_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from serviceManagerRegisterSrv");

  /* Verify serviceManagerRegisterSrv was called */
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 1,
                "serviceManagerRegisterSrv should be called once");
}

/**
 * @test The adcAcqInit function must initialize all components and register with service manager.
 */
ZTEST(adc_service_tests, test_init_success)
{
  int result;

  /* Setup: all functions succeed */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = (k_tid_t)&thread;
  k_thread_name_set_mock_fake.return_val = 0;
  serviceManagerRegisterSrv_fake.return_val = 0;

  /* Execute */
  result = adcAcqInit();

  /* Verify return value */
  zassert_equal(result, 0, "adcAcqInit should return 0 on success");

  /* Verify all init functions were called */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqUtilGetChanCount_fake.call_count, 1,
                "adcAcqUtilGetChanCount should be called once");
  zassert_equal(adcAcqFilterInit_fake.call_count, 1,
                "adcAcqFilterInit should be called once");
  zassert_equal(adcAcqFilterInit_fake.arg0_val, 4,
                "adcAcqFilterInit should be called with channel count 4");

  /* Verify k_thread_create was called with correct parameters */
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_create_mock_fake.arg0_val, &thread,
                "k_thread_create should be called with thread struct");
  zassert_equal(k_thread_create_mock_fake.arg2_val, CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE,
                "k_thread_create should be called with correct stack size");
  zassert_equal(k_thread_create_mock_fake.arg3_val, run,
                "k_thread_create should be called with run function");
  zassert_equal((uintptr_t)k_thread_create_mock_fake.arg4_val,
                CONFIG_ENYA_ADC_ACQUISITION_NOTIFICATION_RATE_MS,
                "k_thread_create should pass notification rate as p1");
  zassert_is_null(k_thread_create_mock_fake.arg5_val,
                  "k_thread_create should be called with NULL as p2");
  zassert_is_null(k_thread_create_mock_fake.arg6_val,
                  "k_thread_create should be called with NULL as p3");

  /* Verify k_thread_name_set was called with correct parameters */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, (k_tid_t)&thread,
                "k_thread_name_set should be called with the thread ID");
  zassert_str_equal(k_thread_name_set_mock_fake.arg1_val, "adcAcquisition",
                    "k_thread_name_set should be called with service name");

  /* Verify serviceManagerRegisterSrv was called with correct descriptor */
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 1,
                "serviceManagerRegisterSrv should be called once");
  zassert_equal(captured_descriptor.threadId, (k_tid_t)&thread,
                "descriptor threadId should be the created thread");
  zassert_equal(captured_descriptor.priority,
                CONFIG_ENYA_ADC_ACQUISITION_SERVICE_PRIORITY,
                "descriptor priority should match Kconfig");
  zassert_equal(captured_descriptor.heartbeatIntervalMs,
                CONFIG_ENYA_ADC_ACQUISITION_HEARTBEAT_INTERVAL_MS,
                "descriptor heartbeatIntervalMs should match Kconfig");
  zassert_equal(captured_descriptor.start, onStart,
                "descriptor start callback should be onStart");
  zassert_equal(captured_descriptor.stop, onStop,
                "descriptor stop callback should be onStop");
  zassert_equal(captured_descriptor.suspend, onSuspend,
                "descriptor suspend callback should be onSuspend");
  zassert_equal(captured_descriptor.resume, onResume,
                "descriptor resume callback should be onResume");
}

/* Dummy callback for subscription tests */
static int dummyCallback(SrvMsgPayload_t *data)
{
  ARG_UNUSED(data);
  return 0;
}

/**
 * @test The adcAcqSubscribe function must return error when adcAcqUtilAddSubscription fails.
 */
ZTEST(adc_service_tests, test_subscribe_failure)
{
  int result;

  /* Setup: adcAcqUtilAddSubscription returns error */
  adcAcqUtilAddSubscription_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqSubscribe(dummyCallback);

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqSubscribe should return error from adcAcqUtilAddSubscription");

  /* Verify adcAcqUtilAddSubscription was called with correct parameter */
  zassert_equal(adcAcqUtilAddSubscription_fake.call_count, 1,
                "adcAcqUtilAddSubscription should be called once");
  zassert_equal(adcAcqUtilAddSubscription_fake.arg0_val, dummyCallback,
                "adcAcqUtilAddSubscription should be called with callback");
}

/**
 * @test The adcAcqSubscribe function must add a subscription callback.
 */
ZTEST(adc_service_tests, test_subscribe_success)
{
  int result;

  /* Setup: adcAcqUtilAddSubscription succeeds */
  adcAcqUtilAddSubscription_fake.return_val = 0;

  /* Execute */
  result = adcAcqSubscribe(dummyCallback);

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAcqSubscribe should return 0 on success");

  /* Verify adcAcqUtilAddSubscription was called with correct parameter */
  zassert_equal(adcAcqUtilAddSubscription_fake.call_count, 1,
                "adcAcqUtilAddSubscription should be called once");
  zassert_equal(adcAcqUtilAddSubscription_fake.arg0_val, dummyCallback,
                "adcAcqUtilAddSubscription should be called with callback");
}

/**
 * @test The adcAcqUnsubscribe function must return error when adcAcqUtilRemoveSubscription fails.
 */
ZTEST(adc_service_tests, test_unsubscribe_failure)
{
  int result;

  /* Setup: adcAcqUtilRemoveSubscription returns error */
  adcAcqUtilRemoveSubscription_fake.return_val = -ENOENT;

  /* Execute */
  result = adcAcqUnsubscribe(dummyCallback);

  /* Verify return value */
  zassert_equal(result, -ENOENT,
                "adcAcqUnsubscribe should return error from adcAcqUtilRemoveSubscription");

  /* Verify adcAcqUtilRemoveSubscription was called with correct parameter */
  zassert_equal(adcAcqUtilRemoveSubscription_fake.call_count, 1,
                "adcAcqUtilRemoveSubscription should be called once");
  zassert_equal(adcAcqUtilRemoveSubscription_fake.arg0_val, dummyCallback,
                "adcAcqUtilRemoveSubscription should be called with callback");
}

/**
 * @test The adcAcqUnsubscribe function must remove a subscription callback.
 */
ZTEST(adc_service_tests, test_unsubscribe_success)
{
  int result;

  /* Setup: adcAcqUtilRemoveSubscription succeeds */
  adcAcqUtilRemoveSubscription_fake.return_val = 0;

  /* Execute */
  result = adcAcqUnsubscribe(dummyCallback);

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAcqUnsubscribe should return 0 on success");

  /* Verify adcAcqUtilRemoveSubscription was called with correct parameter */
  zassert_equal(adcAcqUtilRemoveSubscription_fake.call_count, 1,
                "adcAcqUtilRemoveSubscription should be called once");
  zassert_equal(adcAcqUtilRemoveSubscription_fake.arg0_val, dummyCallback,
                "adcAcqUtilRemoveSubscription should be called with callback");
}

/**
 * @test The adcAcqPauseSubscription function must return error when adcAcqUtilSetSubPauseState fails.
 */
ZTEST(adc_service_tests, test_pause_subscription_failure)
{
  int result;

  /* Setup: adcAcqUtilSetSubPauseState returns error */
  adcAcqUtilSetSubPauseState_fake.return_val = -ENOENT;

  /* Execute */
  result = adcAcqPauseSubscription(dummyCallback);

  /* Verify return value */
  zassert_equal(result, -ENOENT,
                "adcAcqPauseSubscription should return error from adcAcqUtilSetSubPauseState");

  /* Verify adcAcqUtilSetSubPauseState was called with correct parameters */
  zassert_equal(adcAcqUtilSetSubPauseState_fake.call_count, 1,
                "adcAcqUtilSetSubPauseState should be called once");
  zassert_equal(adcAcqUtilSetSubPauseState_fake.arg0_val, dummyCallback,
                "adcAcqUtilSetSubPauseState should be called with callback");
  zassert_true(adcAcqUtilSetSubPauseState_fake.arg1_val,
               "adcAcqUtilSetSubPauseState should be called with true for pause");
}

/**
 * @test The adcAcqPauseSubscription function must pause a subscription.
 */
ZTEST(adc_service_tests, test_pause_subscription_success)
{
  int result;

  /* Setup: adcAcqUtilSetSubPauseState succeeds */
  adcAcqUtilSetSubPauseState_fake.return_val = 0;

  /* Execute */
  result = adcAcqPauseSubscription(dummyCallback);

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAcqPauseSubscription should return 0 on success");

  /* Verify adcAcqUtilSetSubPauseState was called with correct parameters */
  zassert_equal(adcAcqUtilSetSubPauseState_fake.call_count, 1,
                "adcAcqUtilSetSubPauseState should be called once");
  zassert_equal(adcAcqUtilSetSubPauseState_fake.arg0_val, dummyCallback,
                "adcAcqUtilSetSubPauseState should be called with callback");
  zassert_true(adcAcqUtilSetSubPauseState_fake.arg1_val,
               "adcAcqUtilSetSubPauseState should be called with true for pause");
}

/**
 * @test The adcAqcUnpauseSubscription function must return error when adcAcqUtilSetSubPauseState fails.
 */
ZTEST(adc_service_tests, test_unpause_subscription_failure)
{
  int result;

  /* Setup: adcAcqUtilSetSubPauseState returns error */
  adcAcqUtilSetSubPauseState_fake.return_val = -ENOENT;

  /* Execute */
  result = adcAqcUnpauseSubscription(dummyCallback);

  /* Verify return value */
  zassert_equal(result, -ENOENT,
                "adcAqcUnpauseSubscription should return error from adcAcqUtilSetSubPauseState");

  /* Verify adcAcqUtilSetSubPauseState was called with correct parameters */
  zassert_equal(adcAcqUtilSetSubPauseState_fake.call_count, 1,
                "adcAcqUtilSetSubPauseState should be called once");
  zassert_equal(adcAcqUtilSetSubPauseState_fake.arg0_val, dummyCallback,
                "adcAcqUtilSetSubPauseState should be called with callback");
  zassert_false(adcAcqUtilSetSubPauseState_fake.arg1_val,
                "adcAcqUtilSetSubPauseState should be called with false for unpause");
}

/**
 * @test The adcAqcUnpauseSubscription function must unpause a subscription.
 */
ZTEST(adc_service_tests, test_unpause_subscription_success)
{
  int result;

  /* Setup: adcAcqUtilSetSubPauseState succeeds */
  adcAcqUtilSetSubPauseState_fake.return_val = 0;

  /* Execute */
  result = adcAqcUnpauseSubscription(dummyCallback);

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAqcUnpauseSubscription should return 0 on success");

  /* Verify adcAcqUtilSetSubPauseState was called with correct parameters */
  zassert_equal(adcAcqUtilSetSubPauseState_fake.call_count, 1,
                "adcAcqUtilSetSubPauseState should be called once");
  zassert_equal(adcAcqUtilSetSubPauseState_fake.arg0_val, dummyCallback,
                "adcAcqUtilSetSubPauseState should be called with callback");
  zassert_false(adcAcqUtilSetSubPauseState_fake.arg1_val,
                "adcAcqUtilSetSubPauseState should be called with false for unpause");
}

ZTEST_SUITE(adc_service_tests, NULL, service_tests_setup, service_tests_before, NULL, NULL);
