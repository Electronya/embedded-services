/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Tests
 *
 *            Unit tests for service manager module functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Prevent serviceManagerUtil header - we'll mock the functions */
#define SERVICE_MANAGER_UTIL_H

/* Mock Kconfig options */
#define CONFIG_ENYA_SERVICE_MANAGER 1
#define CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL 3
#define CONFIG_SVC_MGR_LOOP_PERIOD_MS 100
#define CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE 2048
#define CONFIG_SVC_MGR_MAX_SERVICES 16

/* Include serviceManager.h for type definitions */
#include "serviceManager.h"

/* Mock utility functions */
FAKE_VALUE_FUNC(int, serviceMngrUtilInitHardWdg);
FAKE_VALUE_FUNC(int, serviceMngrUtilInitSrvRegistry);
FAKE_VALUE_FUNC(int, serviceMngrUtilAddSrvToRegistry, const ServiceDescriptor_t *);
FAKE_VALUE_FUNC(int, serviceMngrUtilGetIndexFromId, k_tid_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilStartService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilUpdateSrvHeartbeat, size_t);
FAKE_VALUE_FUNC(ServiceDescriptor_t *, serviceMngrUtilGetRegEntryByIndex, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilCheckSrvHeartbeat, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilFeedHardWdg);
FAKE_VALUE_FUNC(int, serviceMngrUtilStopService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilSuspendService, size_t);
FAKE_VALUE_FUNC(int, serviceMngrUtilResumeService, size_t);

/* Mock kernel functions */
#define k_thread_create k_thread_create_mock
#define k_thread_name_set k_thread_name_set_mock
#define k_thread_name_get k_thread_name_get_mock
#define k_msgq_put k_msgq_put_mock
#define k_msgq_get k_msgq_get_mock
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *,
                size_t, k_thread_entry_t, void *, void *, void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
FAKE_VALUE_FUNC(const char *, k_thread_name_get_mock, k_tid_t);
FAKE_VALUE_FUNC(int, k_msgq_put_mock, struct k_msgq *, const void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_msgq_get_mock, struct k_msgq *, void *, k_timeout_t);

/* FFF fakes list */
#define SVC_MGR_RUN_ITERATIONS 1

#define FFF_FAKES_LIST(FAKE) \
  FAKE(serviceMngrUtilInitHardWdg) \
  FAKE(serviceMngrUtilInitSrvRegistry) \
  FAKE(serviceMngrUtilAddSrvToRegistry) \
  FAKE(serviceMngrUtilGetIndexFromId) \
  FAKE(serviceMngrUtilStartService) \
  FAKE(serviceMngrUtilUpdateSrvHeartbeat) \
  FAKE(serviceMngrUtilGetRegEntryByIndex) \
  FAKE(serviceMngrUtilCheckSrvHeartbeat) \
  FAKE(serviceMngrUtilFeedHardWdg) \
  FAKE(serviceMngrUtilStopService) \
  FAKE(serviceMngrUtilSuspendService) \
  FAKE(serviceMngrUtilResumeService) \
  FAKE(k_thread_create_mock) \
  FAKE(k_thread_name_set_mock) \
  FAKE(k_thread_name_get_mock) \
  FAKE(k_msgq_put_mock) \
  FAKE(k_msgq_get_mock)

#include "serviceManager.c"

/* Queue message test helpers */
static ServiceMgrMsg_t test_queue_messages[4];
static size_t test_queue_msg_count = 0;
static size_t test_queue_msg_read = 0;

static int k_msgq_get_from_test_queue(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);

  if(test_queue_msg_read < test_queue_msg_count)
  {
    memcpy(data, &test_queue_messages[test_queue_msg_read++], sizeof(ServiceMgrMsg_t));
    return 0;
  }
  return -EAGAIN;
}

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
  /* Reset all fakes */
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  /* Reset queue test helpers and install default fake (empty queue) */
  memset(test_queue_messages, 0, sizeof(test_queue_messages));
  test_queue_msg_count = 0;
  test_queue_msg_read = 0;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_test_queue;
}

/* Test descriptors for run tests */
static ServiceDescriptor_t run_test_descriptors[2];

/* Custom fake: returns a descriptor for indices 0-1, NULL for index 2+ */
static ServiceDescriptor_t *getRegEntry_withTwoServices(size_t index)
{
  if(index < 2)
    return &run_test_descriptors[index];
  return NULL;
}

/**
 * @test The serviceManagerRun function must continue when feeding the watchdog fails.
 */
ZTEST(serviceManager, test_run_feedWdgFails)
{
  /* No services registered, canFeedWdg stays true */
  serviceMngrUtilFeedHardWdg_fake.return_val = -EIO;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "serviceMngrUtilFeedHardWdg should be called once");
}

/**
 * @test The serviceManagerRun function must feed the watchdog when no services are registered.
 */
ZTEST(serviceManager, test_run_noServices)
{
  /* serviceMngrUtilGetRegEntryByIndex returns NULL by default */

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilGetRegEntryByIndex_fake.call_count, 1,
                "serviceMngrUtilGetRegEntryByIndex should be called once (index 0 returns NULL)");
  zassert_equal(serviceMngrUtilCheckSrvHeartbeat_fake.call_count, 0,
                "serviceMngrUtilCheckSrvHeartbeat should not be called with no services");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "serviceMngrUtilFeedHardWdg should be called once");
}

/**
 * @test The serviceManagerRun function must not feed the watchdog when a service heartbeat times out.
 */
ZTEST(serviceManager, test_run_heartbeatTimeout)
{
  run_test_descriptors[0].threadId = (k_tid_t)0x1000;
  run_test_descriptors[1].threadId = (k_tid_t)0x2000;

  serviceMngrUtilGetRegEntryByIndex_fake.custom_fake = getRegEntry_withTwoServices;
  serviceMngrUtilCheckSrvHeartbeat_fake.return_val = -ETIMEDOUT;
  k_thread_name_get_mock_fake.return_val = "test_service";

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilCheckSrvHeartbeat_fake.call_count, 2,
                "serviceMngrUtilCheckSrvHeartbeat should be called for each service");
  zassert_equal(serviceMngrUtilCheckSrvHeartbeat_fake.arg0_val, 1,
                "serviceMngrUtilCheckSrvHeartbeat last call should use index 1");
  zassert_equal(k_thread_name_get_mock_fake.call_count, 2,
                "k_thread_name_get should be called for each timed-out service");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 0,
                "serviceMngrUtilFeedHardWdg should not be called on heartbeat timeout");
}

/**
 * @test The serviceManagerRun function must feed the watchdog when all services are healthy.
 */
ZTEST(serviceManager, test_run_allHealthy)
{
  run_test_descriptors[0].threadId = (k_tid_t)0x1000;
  run_test_descriptors[1].threadId = (k_tid_t)0x2000;

  serviceMngrUtilGetRegEntryByIndex_fake.custom_fake = getRegEntry_withTwoServices;
  serviceMngrUtilCheckSrvHeartbeat_fake.return_val = 0;
  k_thread_name_get_mock_fake.return_val = "test_service";

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilGetRegEntryByIndex_fake.call_count, 3,
                "serviceMngrUtilGetRegEntryByIndex should be called 3 times");
  zassert_equal(serviceMngrUtilCheckSrvHeartbeat_fake.call_count, 2,
                "serviceMngrUtilCheckSrvHeartbeat should be called for each service");
  zassert_equal(serviceMngrUtilCheckSrvHeartbeat_fake.arg0_val, 1,
                "serviceMngrUtilCheckSrvHeartbeat last call should use index 1");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "serviceMngrUtilFeedHardWdg should be called once");
}

/**
 * @test The run function must continue when processing a start message fails.
 */
ZTEST(serviceManager, test_run_processStartFails)
{
  test_queue_messages[0].type = SVC_MGR_MSG_START;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;
  serviceMngrUtilStartService_fake.return_val = -EINVAL;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilStartService_fake.call_count, 1,
                "serviceMngrUtilStartService should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_val, 2,
                "serviceMngrUtilStartService should be called with index 2");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "run should continue and feed watchdog after start failure");
}

/**
 * @test The run function must continue when processing a stop message fails.
 */
ZTEST(serviceManager, test_run_processStopFails)
{
  test_queue_messages[0].type = SVC_MGR_MSG_STOP;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;
  serviceMngrUtilStopService_fake.return_val = -EINVAL;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilStopService_fake.call_count, 1,
                "serviceMngrUtilStopService should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.arg0_val, 2,
                "serviceMngrUtilStopService should be called with index 2");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "run should continue and feed watchdog after stop failure");
}

/**
 * @test The run function must continue when processing a suspend message fails.
 */
ZTEST(serviceManager, test_run_processSuspendFails)
{
  test_queue_messages[0].type = SVC_MGR_MSG_SUSPEND;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;
  serviceMngrUtilSuspendService_fake.return_val = -EINVAL;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 1,
                "serviceMngrUtilSuspendService should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.arg0_val, 2,
                "serviceMngrUtilSuspendService should be called with index 2");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "run should continue and feed watchdog after suspend failure");
}

/**
 * @test The run function must continue when processing a resume message fails.
 */
ZTEST(serviceManager, test_run_processResumeFails)
{
  test_queue_messages[0].type = SVC_MGR_MSG_RESUME;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;
  serviceMngrUtilResumeService_fake.return_val = -EINVAL;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 1,
                "serviceMngrUtilResumeService should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.arg0_val, 2,
                "serviceMngrUtilResumeService should be called with index 2");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "run should continue and feed watchdog after resume failure");
}

/**
 * @test The run function must continue when it receives an unknown message type.
 */
ZTEST(serviceManager, test_run_processUnknownMsg)
{
  test_queue_messages[0].type = (ServiceMgrMsgType_t)99;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilStartService_fake.call_count, 0,
                "serviceMngrUtilStartService should not be called");
  zassert_equal(serviceMngrUtilStopService_fake.call_count, 0,
                "serviceMngrUtilStopService should not be called");
  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 0,
                "serviceMngrUtilSuspendService should not be called");
  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 0,
                "serviceMngrUtilResumeService should not be called");
  zassert_equal(serviceMngrUtilFeedHardWdg_fake.call_count, 1,
                "run should continue and feed watchdog on unknown message");
}

/**
 * @test The run function must process a start message from the queue.
 */
ZTEST(serviceManager, test_run_processStart)
{
  test_queue_messages[0].type = SVC_MGR_MSG_START;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilStartService_fake.call_count, 1,
                "serviceMngrUtilStartService should be called once");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_val, 2,
                "serviceMngrUtilStartService should be called with index 2");
}

/**
 * @test The run function must process a stop message from the queue.
 */
ZTEST(serviceManager, test_run_processStop)
{
  test_queue_messages[0].type = SVC_MGR_MSG_STOP;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilStopService_fake.call_count, 1,
                "serviceMngrUtilStopService should be called once");
  zassert_equal(serviceMngrUtilStopService_fake.arg0_val, 2,
                "serviceMngrUtilStopService should be called with index 2");
}

/**
 * @test The run function must process a suspend message from the queue.
 */
ZTEST(serviceManager, test_run_processSuspend)
{
  test_queue_messages[0].type = SVC_MGR_MSG_SUSPEND;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilSuspendService_fake.call_count, 1,
                "serviceMngrUtilSuspendService should be called once");
  zassert_equal(serviceMngrUtilSuspendService_fake.arg0_val, 2,
                "serviceMngrUtilSuspendService should be called with index 2");
}

/**
 * @test The run function must process a resume message from the queue.
 */
ZTEST(serviceManager, test_run_processResume)
{
  test_queue_messages[0].type = SVC_MGR_MSG_RESUME;
  test_queue_messages[0].index = 2;
  test_queue_msg_count = 1;

  run(NULL, NULL, NULL);

  zassert_equal(serviceMngrUtilResumeService_fake.call_count, 1,
                "serviceMngrUtilResumeService should be called once");
  zassert_equal(serviceMngrUtilResumeService_fake.arg0_val, 2,
                "serviceMngrUtilResumeService should be called with index 2");
}

/**
 * @test The serviceManagerInit function must return error when hardware watchdog init fails.
 */
ZTEST(serviceManager, test_init_hardWdgFails)
{
  int result;
  k_tid_t threadId;

  serviceMngrUtilInitHardWdg_fake.return_val = -ENODEV;

  result = serviceManagerInit(5, &threadId);

  zassert_equal(result, -ENODEV, "Expected -ENODEV when hardware watchdog init fails");
  zassert_equal(serviceMngrUtilInitHardWdg_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_fake.call_count, 0,
                "serviceMngrUtilInitSrvRegistry should not be called when watchdog init fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when watchdog init fails");
}

/**
 * @test The serviceManagerInit function must return error when service registry init fails.
 */
ZTEST(serviceManager, test_init_registryFails)
{
  int result;
  k_tid_t threadId;

  serviceMngrUtilInitHardWdg_fake.return_val = 0;
  serviceMngrUtilInitSrvRegistry_fake.return_val = -ENOMEM;

  result = serviceManagerInit(5, &threadId);

  zassert_equal(result, -ENOMEM, "Expected -ENOMEM when registry init fails");
  zassert_equal(serviceMngrUtilInitHardWdg_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_fake.call_count, 1,
                "serviceMngrUtilInitSrvRegistry should be called once");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when registry init fails");
}

/**
 * @test The serviceManagerInit function must successfully initialize when all components succeed.
 */
ZTEST(serviceManager, test_init_success)
{
  int result;
  k_tid_t threadId;
  k_tid_t fakeThreadId = (k_tid_t)0xABCD;

  serviceMngrUtilInitHardWdg_fake.return_val = 0;
  serviceMngrUtilInitSrvRegistry_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = fakeThreadId;

  result = serviceManagerInit(5, &threadId);

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceMngrUtilInitHardWdg_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_fake.call_count, 1,
                "serviceMngrUtilInitSrvRegistry should be called once");
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_create_mock_fake.arg2_val, CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE,
                "k_thread_create should be called with the correct stack size");
  zassert_equal(k_thread_create_mock_fake.arg3_val, run,
                "k_thread_create should be called with run as entry");
  zassert_equal(k_thread_create_mock_fake.arg7_val, K_PRIO_PREEMPT(5),
                "k_thread_create should be called with the correct priority");
  zassert_equal(threadId, fakeThreadId,
                "threadId output should be set to the created thread ID");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, fakeThreadId,
                "k_thread_name_set should be called with the thread ID");
  zassert_equal(strcmp(k_thread_name_set_mock_fake.arg1_val, "serviceManager"), 0,
                "k_thread_name_set should be called with 'serviceManager'");
}

/**
 * @test The serviceManagerRegisterSrv function must return error when registration fails.
 */
ZTEST(serviceManager, test_registerSrv_fails)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Setup: registration fails */
  serviceMngrUtilAddSrvToRegistry_fake.return_val = -EINVAL;

  /* Setup descriptor */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Execute */
  result = serviceManagerRegisterSrv(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when registration fails");
  zassert_equal(serviceMngrUtilAddSrvToRegistry_fake.call_count, 1,
                "serviceMngrUtilAddSrvToRegistry should be called once");
  zassert_equal(serviceMngrUtilAddSrvToRegistry_fake.arg0_val, &descriptor,
                "serviceMngrUtilAddSrvToRegistry should be called with descriptor");
}

/**
 * @test The serviceManagerRegisterSrv function must successfully register service.
 */
ZTEST(serviceManager, test_registerSrv_success)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Setup: registration succeeds */
  serviceMngrUtilAddSrvToRegistry_fake.return_val = 0;

  /* Setup descriptor */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Execute */
  result = serviceManagerRegisterSrv(&descriptor);

  /* Verify */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceMngrUtilAddSrvToRegistry_fake.call_count, 1,
                "serviceMngrUtilAddSrvToRegistry should be called once");
  zassert_equal(serviceMngrUtilAddSrvToRegistry_fake.arg0_val, &descriptor,
                "serviceMngrUtilAddSrvToRegistry should be called with descriptor");
}

/* Test descriptors for startAll tests */
static ServiceDescriptor_t startAll_test_descriptors[3];

/* Custom fake: returns mixed-priority descriptors for startAll tests */
static ServiceDescriptor_t *getRegEntry_withMixedPriorities(size_t index)
{
  if(index < 3)
    return &startAll_test_descriptors[index];
  return NULL;
}

/**
 * @test The serviceManagerStartAll function must return error when a service fails to start.
 */
ZTEST(serviceManager, test_startAll_startFails)
{
  int result;

  startAll_test_descriptors[0].threadId = (k_tid_t)0x1000;
  startAll_test_descriptors[0].priority = SVC_PRIORITY_CRITICAL;

  serviceMngrUtilGetRegEntryByIndex_fake.custom_fake = getRegEntry_withMixedPriorities;
  serviceMngrUtilStartService_fake.return_val = -EINVAL;
  k_thread_name_get_mock_fake.return_val = "test_service";

  result = serviceManagerStartAll();

  zassert_equal(result, -EINVAL,
                "serviceManagerStartAll should return error when a service fails to start");
  zassert_equal(serviceMngrUtilStartService_fake.call_count, 1,
                "serviceMngrUtilStartService should be called once before failing");
  zassert_equal(k_thread_name_get_mock_fake.call_count, 1,
                "k_thread_name_get should be called once for the failed service");
}

/**
 * @test The serviceManagerStartAll function must start all services in priority order.
 */
ZTEST(serviceManager, test_startAll_success)
{
  int result;

  /* Setup descriptors with mixed priorities: APPLICATION, CRITICAL, CORE */
  startAll_test_descriptors[0].threadId = (k_tid_t)0x1000;
  startAll_test_descriptors[0].priority = SVC_PRIORITY_APPLICATION;
  startAll_test_descriptors[1].threadId = (k_tid_t)0x2000;
  startAll_test_descriptors[1].priority = SVC_PRIORITY_CRITICAL;
  startAll_test_descriptors[2].threadId = (k_tid_t)0x3000;
  startAll_test_descriptors[2].priority = SVC_PRIORITY_CORE;

  serviceMngrUtilGetRegEntryByIndex_fake.custom_fake = getRegEntry_withMixedPriorities;
  serviceMngrUtilStartService_fake.return_val = 0;
  k_thread_name_get_mock_fake.return_val = "test_service";

  result = serviceManagerStartAll();

  zassert_equal(result, 0,
                "serviceManagerStartAll should return 0 on success");
  zassert_equal(serviceMngrUtilStartService_fake.call_count, 3,
                "serviceMngrUtilStartService should be called once per service");
  /* Verify priority order: CRITICAL (index 1), CORE (index 2), APPLICATION (index 0) */
  zassert_equal(serviceMngrUtilStartService_fake.arg0_history[0], 1,
                "first service started should be the CRITICAL one (index 1)");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_history[1], 2,
                "second service started should be the CORE one (index 2)");
  zassert_equal(serviceMngrUtilStartService_fake.arg0_history[2], 0,
                "third service started should be the APPLICATION one (index 0)");
}

/**
 * @test The serviceManagerRequestStart function must return error when the thread ID is not found.
 */
ZTEST(serviceManager, test_requestStart_threadNotFound)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = -EINVAL;

  zassert_equal(serviceManagerRequestStart((k_tid_t)0x1000), -EINVAL,
                "Expected -EINVAL when thread ID is not found");
  zassert_equal(k_msgq_put_mock_fake.call_count, 0,
                "k_msgq_put should not be called");
}

/**
 * @test The serviceManagerRequestStart function must return error when the queue is full.
 */
ZTEST(serviceManager, test_requestStart_queueFull)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  zassert_equal(serviceManagerRequestStart((k_tid_t)0x1000), -ENOMEM,
                "Expected -ENOMEM when queue is full");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestStart function must enqueue a start message.
 */
ZTEST(serviceManager, test_requestStart_success)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  serviceMngrUtilGetIndexFromId_fake.arg0_val = (k_tid_t)0x1000;

  zassert_equal(serviceManagerRequestStart((k_tid_t)0x1000), 0,
                "Expected success");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.call_count, 1,
                "serviceMngrUtilGetIndexFromId should be called once");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestStop function must return error when the thread ID is not found.
 */
ZTEST(serviceManager, test_requestStop_threadNotFound)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = -EINVAL;

  zassert_equal(serviceManagerRequestStop((k_tid_t)0x1000), -EINVAL,
                "Expected -EINVAL when thread ID is not found");
  zassert_equal(k_msgq_put_mock_fake.call_count, 0,
                "k_msgq_put should not be called");
}

/**
 * @test The serviceManagerRequestStop function must return error when the queue is full.
 */
ZTEST(serviceManager, test_requestStop_queueFull)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  zassert_equal(serviceManagerRequestStop((k_tid_t)0x1000), -ENOMEM,
                "Expected -ENOMEM when queue is full");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestStop function must enqueue a stop message.
 */
ZTEST(serviceManager, test_requestStop_success)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;

  zassert_equal(serviceManagerRequestStop((k_tid_t)0x1000), 0,
                "Expected success");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestSuspend function must return error when the thread ID is not found.
 */
ZTEST(serviceManager, test_requestSuspend_threadNotFound)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = -EINVAL;

  zassert_equal(serviceManagerRequestSuspend((k_tid_t)0x1000), -EINVAL,
                "Expected -EINVAL when thread ID is not found");
  zassert_equal(k_msgq_put_mock_fake.call_count, 0,
                "k_msgq_put should not be called");
}

/**
 * @test The serviceManagerRequestSuspend function must return error when the queue is full.
 */
ZTEST(serviceManager, test_requestSuspend_queueFull)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  zassert_equal(serviceManagerRequestSuspend((k_tid_t)0x1000), -ENOMEM,
                "Expected -ENOMEM when queue is full");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestSuspend function must enqueue a suspend message.
 */
ZTEST(serviceManager, test_requestSuspend_success)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;

  zassert_equal(serviceManagerRequestSuspend((k_tid_t)0x1000), 0,
                "Expected success");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestResume function must return error when the thread ID is not found.
 */
ZTEST(serviceManager, test_requestResume_threadNotFound)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = -EINVAL;

  zassert_equal(serviceManagerRequestResume((k_tid_t)0x1000), -EINVAL,
                "Expected -EINVAL when thread ID is not found");
  zassert_equal(k_msgq_put_mock_fake.call_count, 0,
                "k_msgq_put should not be called");
}

/**
 * @test The serviceManagerRequestResume function must return error when the queue is full.
 */
ZTEST(serviceManager, test_requestResume_queueFull)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  zassert_equal(serviceManagerRequestResume((k_tid_t)0x1000), -ENOMEM,
                "Expected -ENOMEM when queue is full");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerRequestResume function must enqueue a resume message.
 */
ZTEST(serviceManager, test_requestResume_success)
{
  serviceMngrUtilGetIndexFromId_fake.return_val = 2;

  zassert_equal(serviceManagerRequestResume((k_tid_t)0x1000), 0,
                "Expected success");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1,
                "k_msgq_put should be called once");
}

/**
 * @test The serviceManagerUpdateHeartbeat function must return error when the thread ID is not found.
 */
ZTEST(serviceManager, test_updateHeartbeat_threadNotFound)
{
  int result;

  serviceMngrUtilGetIndexFromId_fake.return_val = -EINVAL;

  result = serviceManagerUpdateHeartbeat((k_tid_t)0x1000);

  zassert_equal(result, -EINVAL,
                "Expected -EINVAL when thread ID is not found");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.call_count, 1,
                "serviceMngrUtilGetIndexFromId should be called once");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(serviceMngrUtilUpdateSrvHeartbeat_fake.call_count, 0,
                "serviceMngrUtilUpdateSrvHeartbeat should not be called");
}

/**
 * @test The serviceManagerUpdateHeartbeat function must return error when the heartbeat update fails.
 */
ZTEST(serviceManager, test_updateHeartbeat_updateFails)
{
  int result;

  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  serviceMngrUtilUpdateSrvHeartbeat_fake.return_val = -EINVAL;

  result = serviceManagerUpdateHeartbeat((k_tid_t)0x1000);

  zassert_equal(result, -EINVAL,
                "Expected -EINVAL when heartbeat update fails");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.call_count, 1,
                "serviceMngrUtilGetIndexFromId should be called once");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(serviceMngrUtilUpdateSrvHeartbeat_fake.call_count, 1,
                "serviceMngrUtilUpdateSrvHeartbeat should be called once");
  zassert_equal(serviceMngrUtilUpdateSrvHeartbeat_fake.arg0_val, 2,
                "serviceMngrUtilUpdateSrvHeartbeat should be called with index 2");
}

/**
 * @test The serviceManagerUpdateHeartbeat function must successfully update the heartbeat.
 */
ZTEST(serviceManager, test_updateHeartbeat_success)
{
  int result;

  serviceMngrUtilGetIndexFromId_fake.return_val = 2;
  serviceMngrUtilUpdateSrvHeartbeat_fake.return_val = 0;

  result = serviceManagerUpdateHeartbeat((k_tid_t)0x1000);

  zassert_equal(result, 0,
                "Expected success (0)");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.call_count, 1,
                "serviceMngrUtilGetIndexFromId should be called once");
  zassert_equal(serviceMngrUtilGetIndexFromId_fake.arg0_val, (k_tid_t)0x1000,
                "serviceMngrUtilGetIndexFromId should be called with the thread ID");
  zassert_equal(serviceMngrUtilUpdateSrvHeartbeat_fake.call_count, 1,
                "serviceMngrUtilUpdateSrvHeartbeat should be called once");
  zassert_equal(serviceMngrUtilUpdateSrvHeartbeat_fake.arg0_val, 2,
                "serviceMngrUtilUpdateSrvHeartbeat should be called with index 2");
}

ZTEST_SUITE(serviceManager, NULL, service_tests_setup, service_tests_before, NULL, NULL);
