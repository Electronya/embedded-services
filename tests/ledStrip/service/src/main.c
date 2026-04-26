/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-04-25
 * @brief     LED Strip Service Tests
 *
 *            Unit tests for LED strip service functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Prevent CMSIS OS2 header - we'll define types manually */
#define CMSIS_OS2_H_

/* Prevent LED strip driver header */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_

/* Prevent ledStripUtil header - replaced by mocks */
#define LEDSTRIPUTIL_H

/* Mock CMSIS OS2 types */
typedef void *osMemoryPoolId_t;

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

/* Wrap kernel functions to use mocks */
#define k_thread_create         k_thread_create_mock
#define k_thread_name_set       k_thread_name_set_mock
#define k_thread_start          k_thread_start_mock
#define k_thread_suspend        k_thread_suspend_mock
#define k_thread_resume         k_thread_resume_mock
#define k_thread_abort          k_thread_abort_mock
#define k_current_get           k_current_get_mock
#define k_msgq_put              k_msgq_put_mock
#define k_msgq_get              k_msgq_get_mock
#define k_timer_status_sync     k_timer_status_sync_mock
#define k_timer_stop            k_timer_stop_mock

/* Mock Kconfig options */
#define CONFIG_ENYA_LED_STRIP                        1
#define CONFIG_ENYA_LED_STRIP_LOG_LEVEL              3
#define CONFIG_ENYA_LED_STRIP_STACK_SIZE             1024
#define CONFIG_ENYA_LED_STRIP_THREAD_PRIORITY        5
#define CONFIG_ENYA_LED_STRIP_SERVICE_PRIORITY       2
#define CONFIG_ENYA_LED_STRIP_HEARTBEAT_INTERVAL_MS  1000
#define CONFIG_ENYA_LED_STRIP_NUM_CHANNELS           3
#define CONFIG_ENYA_LED_STRIP_REFRESH_RATE_HZ        60
#define CONFIG_ENYA_LED_STRIP_MSG_COUNT              4

/* Define test mode iteration count */
#define LED_STRIP_RUN_ITERATIONS 2

/* Include ledStrip.h to get LedPixel_t before mock declarations */
#include "ledStrip.h"

/* Mock kernel functions */
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *,
                size_t, k_thread_entry_t, void *, void *, void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
FAKE_VOID_FUNC(k_thread_start_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_suspend_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_resume_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_abort_mock, k_tid_t);
FAKE_VALUE_FUNC(k_tid_t, k_current_get_mock);
FAKE_VALUE_FUNC(int, k_msgq_put_mock, struct k_msgq *, const void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_msgq_get_mock, struct k_msgq *, void *, k_timeout_t);
FAKE_VALUE_FUNC(uint32_t, k_timer_status_sync_mock, struct k_timer *);
FAKE_VOID_FUNC(k_timer_stop_mock, struct k_timer *);

/* Mock serviceManager functions */
FAKE_VALUE_FUNC(int, serviceManagerConfirmState, k_tid_t, ServiceState_t);
FAKE_VALUE_FUNC(int, serviceManagerUpdateHeartbeat, k_tid_t);
FAKE_VALUE_FUNC(int, serviceManagerRegisterSrv, const ServiceDescriptor_t *);

/* Mock ledStripUtil functions */
FAKE_VALUE_FUNC(int, ledStripUtilInitStrip);
FAKE_VALUE_FUNC(int, ledStripUtilInitFramebuffers);
FAKE_VOID_FUNC(ledStripUtilSetBrightness, uint8_t);
FAKE_VOID_FUNC(ledStripUtilActivateFrame, LedPixel_t *);
FAKE_VALUE_FUNC(int, ledStripUtilPushFrame);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(k_thread_create_mock) \
  FAKE(k_thread_name_set_mock) \
  FAKE(k_thread_start_mock) \
  FAKE(k_thread_suspend_mock) \
  FAKE(k_thread_resume_mock) \
  FAKE(k_thread_abort_mock) \
  FAKE(k_current_get_mock) \
  FAKE(k_msgq_put_mock) \
  FAKE(k_msgq_get_mock) \
  FAKE(k_timer_status_sync_mock) \
  FAKE(k_timer_stop_mock) \
  FAKE(serviceManagerConfirmState) \
  FAKE(serviceManagerUpdateHeartbeat) \
  FAKE(serviceManagerRegisterSrv) \
  FAKE(ledStripUtilInitStrip) \
  FAKE(ledStripUtilInitFramebuffers) \
  FAKE(ledStripUtilSetBrightness) \
  FAKE(ledStripUtilActivateFrame) \
  FAKE(ledStripUtilPushFrame)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ledStrip, LOG_LEVEL_DBG);

/* Prevent LOG_MODULE_REGISTER in source file */
#undef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)

/* Include service implementation */
#include "ledStrip.c"

/* Test fixture */
struct ledStrip_fixture
{
  LedStripMessage_t test_queue_messages[4];
  size_t test_queue_msg_count;
  size_t test_queue_msg_read;
  LedStripMessage_t captured_msg;
  ServiceDescriptor_t captured_descriptor;
};

static struct ledStrip_fixture test_fixture;
static struct ledStrip_fixture *current_fixture;

/* Message capture helper for k_msgq_put */
static int k_msgq_put_capture(struct k_msgq *q, const void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(data)
    current_fixture->captured_msg = *(const LedStripMessage_t *)data;
  return k_msgq_put_mock_fake.return_val;
}

/* Descriptor capture helper for serviceManagerRegisterSrv */
static int serviceManagerRegisterSrv_capture(const ServiceDescriptor_t *descriptor)
{
  if(descriptor)
    current_fixture->captured_descriptor = *descriptor;
  return serviceManagerRegisterSrv_fake.return_val;
}

/* Queue test helpers for k_msgq_get */
static int k_msgq_get_no_message(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(data);
  ARG_UNUSED(timeout);
  return -EAGAIN;
}

static int k_msgq_get_from_run_queue(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(current_fixture->test_queue_msg_read < current_fixture->test_queue_msg_count)
  {
    memcpy(data, &current_fixture->test_queue_messages[current_fixture->test_queue_msg_read++],
           sizeof(LedStripMessage_t));
    return 0;
  }
  return -EAGAIN;
}

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *service_tests_setup(void)
{
  return &test_fixture;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void service_tests_before(void *f)
{
  current_fixture = (struct ledStrip_fixture *)f;

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  memset(current_fixture, 0, sizeof(*current_fixture));

  k_msgq_put_mock_fake.custom_fake = k_msgq_put_capture;
  serviceManagerRegisterSrv_fake.custom_fake = serviceManagerRegisterSrv_capture;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_no_message;
}

/**
 * @test The run function must log error but continue when confirming the running state fails.
 */
ZTEST_F(ledStrip, test_run_confirmStateFails)
{
  serviceManagerConfirmState_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once");
  zassert_equal(k_timer_status_sync_mock_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "k_timer_status_sync should be called for each iteration");
  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should still be called despite confirmState failure");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should still be called despite confirmState failure");
}

/**
 * @test The run function must log error but still abort the thread when
 *       confirming the stopped state fails on a STOP message.
 */
ZTEST_F(ledStrip, test_run_stopConfirmStateFails)
{
  fixture->test_queue_messages[0].type = LED_STRIP_STOP_MSG;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;
  serviceManagerConfirmState_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(k_timer_stop_mock_fake.call_count, 1,
                "k_timer_stop should be called on STOP");
  zassert_equal(serviceManagerConfirmState_fake.arg1_history[1], SVC_STATE_STOPPED,
                "confirmState should be called with SVC_STATE_STOPPED");
  zassert_equal(k_thread_abort_mock_fake.call_count, 1,
                "k_thread_abort should be called even when confirming the state fails");
}

/**
 * @test The run function must log error but still suspend the thread when
 *       confirming the suspended state fails on a SUSPEND message.
 */
ZTEST_F(ledStrip, test_run_suspendConfirmStateFails)
{
  fixture->test_queue_messages[0].type = LED_STRIP_SUSPEND_MSG;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;
  serviceManagerConfirmState_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(k_timer_stop_mock_fake.call_count, 1,
                "k_timer_stop should be called on SUSPEND");
  zassert_equal(serviceManagerConfirmState_fake.arg1_history[1], SVC_STATE_SUSPENDED,
                "confirmState should be called with SVC_STATE_SUSPENDED");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 1,
                "k_thread_suspend should be called even when confirming the state fails");
}

/**
 * @test The run function must log error but continue when an unsupported message type is received.
 */
ZTEST_F(ledStrip, test_run_unsupportedMsgType)
{
  fixture->test_queue_messages[0].type = (LedStipMsgType_t)99;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 0,
                "ledStripUtilActivateFrame should not be called for unsupported message type");
  zassert_equal(k_thread_abort_mock_fake.call_count, 0,
                "k_thread_abort should not be called for unsupported message type");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 0,
                "k_thread_suspend should not be called for unsupported message type");
  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should still be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should still be called for each iteration");
}

/**
 * @test The run function must log error but continue when pushing the frame fails.
 */
ZTEST_F(ledStrip, test_run_pushFrameFails)
{
  ledStripUtilPushFrame_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should still be called despite pushing the frame failing");
}

/**
 * @test The run function must log error but continue when updating the heartbeat fails.
 */
ZTEST_F(ledStrip, test_run_updateHeartbeatFails)
{
  serviceManagerUpdateHeartbeat_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should be called for each iteration despite failure");
}

/**
 * @test The run function must confirm the running state, push the frame and update
 *       the heartbeat each iteration when no messages are received.
 */
ZTEST_F(ledStrip, test_run_successNoMessage)
{
  k_tid_t mockTid = (k_tid_t)0x1234;

  k_current_get_mock_fake.return_val = mockTid;

  run(NULL, NULL, NULL);

  zassert_equal(serviceManagerConfirmState_fake.call_count, 1,
                "serviceManagerConfirmState should be called once at startup");
  zassert_equal(serviceManagerConfirmState_fake.arg0_val, mockTid,
                "serviceManagerConfirmState should be called with current thread ID");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_RUNNING,
                "serviceManagerConfirmState should be called with SVC_STATE_RUNNING");
  zassert_equal(k_timer_status_sync_mock_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "k_timer_status_sync should be called for each iteration");
  zassert_equal(k_msgq_get_mock_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "k_msgq_get should be called for each iteration");
  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 0,
                "ledStripUtilActivateFrame should not be called when no messages are received");
  zassert_equal(k_timer_stop_mock_fake.call_count, 0,
                "k_timer_stop should not be called when no messages are received");
  zassert_equal(k_thread_abort_mock_fake.call_count, 0,
                "k_thread_abort should not be called when no messages are received");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 0,
                "k_thread_suspend should not be called when no messages are received");
  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.arg0_val, mockTid,
                "serviceManagerUpdateHeartbeat should be called with current thread ID");
}

/**
 * @test The run function must activate the new frame and push it when a new frame
 *       message is received.
 */
ZTEST_F(ledStrip, test_run_successNewFrame)
{
  LedPixel_t mockFrame;
  k_tid_t mockTid = (k_tid_t)0x1234;

  fixture->test_queue_messages[0].type = LED_STRIP_NEW_FRAME_MSG;
  fixture->test_queue_messages[0].framebuffer = &mockFrame;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;
  k_current_get_mock_fake.return_val = mockTid;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 1,
                "ledStripUtilActivateFrame should be called once for the new frame message");
  zassert_equal(ledStripUtilActivateFrame_fake.arg0_val, &mockFrame,
                "ledStripUtilActivateFrame should be called with the frame from the message");
  zassert_equal(k_thread_abort_mock_fake.call_count, 0,
                "k_thread_abort should not be called on a new frame message");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 0,
                "k_thread_suspend should not be called on a new frame message");
  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should be called for each iteration");
}

/**
 * @test The run function must stop the timer, confirm the stopped state and abort
 *       the thread when a STOP message is received.
 */
ZTEST_F(ledStrip, test_run_successStop)
{
  k_tid_t mockTid = (k_tid_t)0x1234;

  fixture->test_queue_messages[0].type = LED_STRIP_STOP_MSG;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;
  k_current_get_mock_fake.return_val = mockTid;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 0,
                "ledStripUtilActivateFrame should not be called on a STOP message");
  zassert_equal(k_timer_stop_mock_fake.call_count, 1,
                "k_timer_stop should be called once on STOP");
  zassert_equal(serviceManagerConfirmState_fake.arg1_history[1], SVC_STATE_STOPPED,
                "confirmState should be called with SVC_STATE_STOPPED");
  zassert_equal(serviceManagerConfirmState_fake.arg0_history[1], mockTid,
                "confirmState should be called with the current thread ID");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 0,
                "k_thread_suspend should not be called on a STOP message");
  zassert_equal(k_thread_abort_mock_fake.call_count, 1,
                "k_thread_abort should be called once on STOP");
  zassert_equal(k_thread_abort_mock_fake.arg0_val, mockTid,
                "k_thread_abort should be called with the current thread ID");
}

/**
 * @test The run function must stop the timer, confirm the suspended state and suspend
 *       the thread when a SUSPEND message is received.
 */
ZTEST_F(ledStrip, test_run_successSuspend)
{
  k_tid_t mockTid = (k_tid_t)0x1234;

  fixture->test_queue_messages[0].type = LED_STRIP_SUSPEND_MSG;
  fixture->test_queue_msg_count = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_run_queue;
  k_current_get_mock_fake.return_val = mockTid;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 0,
                "ledStripUtilActivateFrame should not be called on a SUSPEND message");
  zassert_equal(k_timer_stop_mock_fake.call_count, 1,
                "k_timer_stop should be called once on SUSPEND");
  zassert_equal(serviceManagerConfirmState_fake.arg1_history[1], SVC_STATE_SUSPENDED,
                "confirmState should be called with SVC_STATE_SUSPENDED");
  zassert_equal(serviceManagerConfirmState_fake.arg0_history[1], mockTid,
                "confirmState should be called with the current thread ID");
  zassert_equal(k_thread_abort_mock_fake.call_count, 0,
                "k_thread_abort should not be called on a SUSPEND message");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 1,
                "k_thread_suspend should be called once on SUSPEND");
  zassert_equal(k_thread_suspend_mock_fake.arg0_val, mockTid,
                "k_thread_suspend should be called with the current thread ID");
}

/**
 * @test The run function must set the brightness when a brightness message is received.
 */
ZTEST_F(ledStrip, test_run_successBrightness)
{
  fixture->test_queue_messages[0].type       = LED_STRIP_BRIGHTNESS_MSG;
  fixture->test_queue_messages[0].brightness = 128;
  fixture->test_queue_msg_count              = 1;
  k_msgq_get_mock_fake.custom_fake           = k_msgq_get_from_run_queue;

  run(NULL, NULL, NULL);

  zassert_equal(ledStripUtilSetBrightness_fake.call_count, 1,
                "ledStripUtilSetBrightness should be called once for the brightness message");
  zassert_equal(ledStripUtilSetBrightness_fake.arg0_val, 128,
                "ledStripUtilSetBrightness should be called with the brightness from the message");
  zassert_equal(ledStripUtilActivateFrame_fake.call_count, 0,
                "ledStripUtilActivateFrame should not be called on a brightness message");
  zassert_equal(k_thread_abort_mock_fake.call_count, 0,
                "k_thread_abort should not be called on a brightness message");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 0,
                "k_thread_suspend should not be called on a brightness message");
  zassert_equal(ledStripUtilPushFrame_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "ledStripUtilPushFrame should be called for each iteration");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, LED_STRIP_RUN_ITERATIONS,
                "serviceManagerUpdateHeartbeat should be called for each iteration");
}

/**
 * @test The onStart callback must start the service thread.
 */
ZTEST_F(ledStrip, test_onStart_success)
{
  int result;

  result = onStart();

  zassert_equal(result, 0, "onStart should return 0");
  zassert_equal(k_thread_start_mock_fake.call_count, 1,
                "k_thread_start should be called once");
  zassert_equal(k_thread_start_mock_fake.arg0_val, &thread,
                "k_thread_start should be called with the service thread");
}

/**
 * @test The onStop callback must return error when queuing the stop message fails.
 */
ZTEST_F(ledStrip, test_onStop_msgqPutFails)
{
  int result;

  k_msgq_put_mock_fake.return_val = -EAGAIN;

  result = onStop();

  zassert_equal(result, -EAGAIN,
                "onStop should return error from k_msgq_put");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The onStop callback must enqueue a stop message to the service queue.
 */
ZTEST_F(ledStrip, test_onStop_success)
{
  int result;

  result = onStop();

  zassert_equal(result, 0, "onStop should return 0");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(fixture->captured_msg.type, LED_STRIP_STOP_MSG,
                "k_msgq_put should be called with a STOP message");
}

/**
 * @test The onSuspend callback must return error when queuing the suspend message fails.
 */
ZTEST_F(ledStrip, test_onSuspend_msgqPutFails)
{
  int result;

  k_msgq_put_mock_fake.return_val = -EAGAIN;

  result = onSuspend();

  zassert_equal(result, -EAGAIN,
                "onSuspend should return error from k_msgq_put");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The onSuspend callback must enqueue a suspend message to the service queue.
 */
ZTEST_F(ledStrip, test_onSuspend_success)
{
  int result;

  result = onSuspend();

  zassert_equal(result, 0, "onSuspend should return 0");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
  zassert_equal(fixture->captured_msg.type, LED_STRIP_SUSPEND_MSG,
                "k_msgq_put should be called with a SUSPEND message");
}

/**
 * @test The onResume callback must restart the frame timer and resume the service thread.
 */
ZTEST_F(ledStrip, test_onResume_success)
{
  int result;

  result = onResume();

  zassert_equal(result, 0, "onResume should return 0");
  zassert_equal(k_thread_resume_mock_fake.call_count, 1,
                "k_thread_resume should be called once");
  zassert_equal(k_thread_resume_mock_fake.arg0_val, &thread,
                "k_thread_resume should be called with the service thread");
}

/**
 * @test The init function must return error when initializing the strip fails.
 */
ZTEST_F(ledStrip, test_ledStripInit_initStripFails)
{
  int result;

  ledStripUtilInitStrip_fake.return_val = -EIO;

  result = ledStripInit();

  zassert_equal(result, -EIO, "ledStripInit should return error from initStrip");
  zassert_equal(ledStripUtilInitStrip_fake.call_count, 1,
                "ledStripUtilInitStrip should be called once");
  zassert_equal(ledStripUtilInitFramebuffers_fake.call_count, 0,
                "ledStripUtilInitFramebuffers should not be called when initStrip fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when initStrip fails");
}

/**
 * @test The init function must return error when initializing the framebuffers fails.
 */
ZTEST_F(ledStrip, test_ledStripInit_initFramebuffersFails)
{
  int result;

  ledStripUtilInitFramebuffers_fake.return_val = -EIO;

  result = ledStripInit();

  zassert_equal(result, -EIO, "ledStripInit should return error from initFramebuffers");
  zassert_equal(ledStripUtilInitFramebuffers_fake.call_count, 1,
                "ledStripUtilInitFramebuffers should be called once");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when initFramebuffers fails");
}

/**
 * @test The init function must return error when setting the thread name fails.
 */
ZTEST_F(ledStrip, test_ledStripInit_threadNameSetFails)
{
  int result;

  k_thread_name_set_mock_fake.return_val = -EIO;

  result = ledStripInit();

  zassert_equal(result, -EIO, "ledStripInit should return error from k_thread_name_set");
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0,
                "serviceManagerRegisterSrv should not be called when setting thread name fails");
}

/**
 * @test The init function must return error when registering the service fails.
 */
ZTEST_F(ledStrip, test_ledStripInit_registerSrvFails)
{
  int result;

  serviceManagerRegisterSrv_fake.return_val = -EIO;

  result = ledStripInit();

  zassert_equal(result, -EIO, "ledStripInit should return error from serviceManagerRegisterSrv");
  zassert_equal(ledStripUtilInitStrip_fake.call_count, 1,
                "ledStripUtilInitStrip should be called once");
  zassert_equal(ledStripUtilInitFramebuffers_fake.call_count, 1,
                "ledStripUtilInitFramebuffers should be called once");
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 1,
                "serviceManagerRegisterSrv should be called once");
}

/**
 * @test The init function must initialize the strip, the framebuffers, create the thread
 *       and register the service with the correct descriptor.
 */
ZTEST_F(ledStrip, test_ledStripInit_success)
{
  int result;
  k_tid_t mockTid = (k_tid_t)0x1234;

  k_thread_create_mock_fake.return_val = mockTid;

  result = ledStripInit();

  zassert_equal(result, 0, "ledStripInit should return 0");
  zassert_equal(ledStripUtilInitStrip_fake.call_count, 1,
                "ledStripUtilInitStrip should be called once");
  zassert_equal(ledStripUtilInitFramebuffers_fake.call_count, 1,
                "ledStripUtilInitFramebuffers should be called once");
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_create_mock_fake.arg0_val, &thread,
                "k_thread_create should be called with the service thread");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, mockTid,
                "k_thread_name_set should be called with the thread ID returned by k_thread_create");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 1,
                "serviceManagerRegisterSrv should be called once");
  zassert_equal(fixture->captured_descriptor.threadId, mockTid,
                "descriptor threadId should match the created thread ID");
  zassert_equal(fixture->captured_descriptor.priority, CONFIG_ENYA_LED_STRIP_SERVICE_PRIORITY,
                "descriptor priority should match Kconfig");
  zassert_equal(fixture->captured_descriptor.heartbeatIntervalMs,
                CONFIG_ENYA_LED_STRIP_HEARTBEAT_INTERVAL_MS,
                "descriptor heartbeatIntervalMs should match Kconfig");
  zassert_equal(fixture->captured_descriptor.start, onStart,
                "descriptor start callback should be onStart");
  zassert_equal(fixture->captured_descriptor.stop, onStop,
                "descriptor stop callback should be onStop");
  zassert_equal(fixture->captured_descriptor.suspend, onSuspend,
                "descriptor suspend callback should be onSuspend");
  zassert_equal(fixture->captured_descriptor.resume, onResume,
                "descriptor resume callback should be onResume");
}

ZTEST_SUITE(ledStrip, NULL, service_tests_setup, service_tests_before, NULL, NULL);
