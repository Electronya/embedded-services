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
#define k_thread_create    k_thread_create_mock
#define k_thread_name_set  k_thread_name_set_mock
#define k_thread_start     k_thread_start_mock
#define k_thread_suspend   k_thread_suspend_mock
#define k_thread_resume    k_thread_resume_mock
#define k_current_get      k_current_get_mock
#define k_msgq_put         k_msgq_put_mock
#define k_msgq_get         k_msgq_get_mock
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

/* Mock kernel functions */
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *,
                size_t, k_thread_entry_t, void *, void *, void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
FAKE_VOID_FUNC(k_thread_start_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_suspend_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_resume_mock, k_tid_t);
FAKE_VALUE_FUNC(k_tid_t, k_current_get_mock);
FAKE_VALUE_FUNC(int, k_msgq_put_mock, struct k_msgq *, const void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_msgq_get_mock, struct k_msgq *, void *, k_timeout_t);
/* Mock serviceManager functions */
FAKE_VALUE_FUNC(int, serviceManagerConfirmState, k_tid_t, ServiceState_t);
FAKE_VALUE_FUNC(int, serviceManagerUpdateHeartbeat, k_tid_t);
FAKE_VALUE_FUNC(int, serviceManagerRegisterSrv, const ServiceDescriptor_t *);

/* Mock ledStripUtil functions */
FAKE_VALUE_FUNC(int, ledStripUtilInitStrip);
FAKE_VALUE_FUNC(int, ledStripUtilInitFramebuffers);
FAKE_VALUE_FUNC(int, ledStripUtilUpdateStrip, uint8_t);

#define FFF_FAKES_LIST(FAKE) \
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
  FAKE(ledStripUtilInitStrip) \
  FAKE(ledStripUtilInitFramebuffers) \
  FAKE(ledStripUtilUpdateStrip)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ledStrip, LOG_LEVEL_DBG);

/* Prevent LOG_MODULE_REGISTER in source file */
#undef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)

/* Include service implementation */
#include "ledStrip.c"

/* Descriptor capture helper for serviceManagerRegisterSrv */
static ServiceDescriptor_t captured_descriptor;

static int serviceManagerRegisterSrv_capture(const ServiceDescriptor_t *descriptor)
{
  if(descriptor)
    captured_descriptor = *descriptor;
  return serviceManagerRegisterSrv_fake.return_val;
}

/* Queue test helpers for k_msgq_get */
// static ServiceCtrlMsg_t test_ctrl_queue_messages[4];
// static size_t test_ctrl_queue_msg_count = 0;
// static size_t test_ctrl_queue_msg_read = 0;

static int k_msgq_get_no_message(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(data);
  ARG_UNUSED(timeout);
  return -EAGAIN;
}

// static int k_msgq_get_from_ctrl_queue(struct k_msgq *q, void *data, k_timeout_t timeout)
// {
//   ARG_UNUSED(q);
//   ARG_UNUSED(timeout);
//   if(test_ctrl_queue_msg_read < test_ctrl_queue_msg_count)
//   {
//     memcpy(data, &test_ctrl_queue_messages[test_ctrl_queue_msg_read++],
//            sizeof(ServiceCtrlMsg_t));
//     return 0;
//   }
//   return -EAGAIN;
// }

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *service_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void service_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  // test_ctrl_queue_msg_count = 0;
  // test_ctrl_queue_msg_read = 0;
  memset(&captured_descriptor, 0, sizeof(captured_descriptor));

  serviceManagerRegisterSrv_fake.custom_fake = serviceManagerRegisterSrv_capture;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_no_message;
}

ZTEST_SUITE(ledStrip, NULL, service_tests_setup, service_tests_before, NULL, NULL);
