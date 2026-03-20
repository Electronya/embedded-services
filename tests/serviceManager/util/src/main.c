/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Util Tests
 *
 *            Unit tests for service manager utility functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Prevent watchdog driver header - we'll define types manually */
#define ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_

/* Wrap functions to use mocks */
#define device_is_ready device_is_ready_mock
#define wdt_install_timeout wdt_install_timeout_mock
#define wdt_setup wdt_setup_mock
#define wdt_feed wdt_feed_mock
#define k_uptime_get k_uptime_get_mock
#define k_thread_name_get k_thread_name_get_mock

/* Define watchdog types manually */
struct wdt_timeout_cfg {
  uint32_t flags;
  struct {
    uint32_t min;
    uint32_t max;
  } window;
  void (*callback)(const struct device *, int);
};

/* Define watchdog constants */
#define WDT_FLAG_RESET_SOC (1 << 0)
#define WDT_OPT_PAUSE_HALTED_BY_DBG (1 << 0)

/* Mock Kconfig options */
#define CONFIG_ENYA_SERVICE_MANAGER 1
#define CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL 3
#define CONFIG_SVC_MGR_WDT_TIMEOUT_MS 5000
#define CONFIG_SVC_MGR_MAX_SERVICES 16
#define SVC_MGR_MAX_MISSED_HEARTBEATS 3

/* FFF fakes list */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(device_is_ready_mock) \
  FAKE(wdt_install_timeout_mock) \
  FAKE(wdt_setup_mock) \
  FAKE(wdt_feed_mock) \
  FAKE(k_uptime_get_mock) \
  FAKE(k_thread_name_get_mock) \
  FAKE(mock_start_callback) \
  FAKE(mock_stop_callback) \
  FAKE(mock_suspend_callback) \
  FAKE(mock_resume_callback)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(serviceManager, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Mock device and watchdog functions */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, wdt_install_timeout_mock, const struct device *, const struct wdt_timeout_cfg *);
FAKE_VALUE_FUNC(int, wdt_setup_mock, const struct device *, uint8_t);
FAKE_VALUE_FUNC(int, wdt_feed_mock, const struct device *, int);

/* Mock kernel functions */
FAKE_VALUE_FUNC(int64_t, k_uptime_get_mock);
FAKE_VALUE_FUNC(const char *, k_thread_name_get_mock, k_tid_t);

/* Mock service callback functions */
FAKE_VALUE_FUNC(int, mock_start_callback);
FAKE_VALUE_FUNC(int, mock_stop_callback);
FAKE_VALUE_FUNC(int, mock_suspend_callback);
FAKE_VALUE_FUNC(int, mock_resume_callback);

/* Storage for captured watchdog timeout config */
static struct wdt_timeout_cfg captured_wdt_cfg;

/* Custom fake for wdt_install_timeout to capture config */
static int wdt_install_timeout_custom_fake(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
  if (cfg != NULL) {
    captured_wdt_cfg = *cfg;
  }
  return wdt_install_timeout_mock_fake.return_val;
}

/* Mock device structure */
static const struct device mock_wdg_dev __attribute__((unused)) = {
  .name = "mock_watchdog"
};

/* Override device tree macros AFTER including headers */
#undef DT_ALIAS
#define DT_ALIAS(name) DT_N_NODELABEL_iwdg

#undef DEVICE_DT_GET
#define DEVICE_DT_GET(node_id) (&mock_wdg_dev)

#include "serviceManagerUtil.c"

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *util_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void util_tests_before(void *fixture)
{
  /* Reset all fakes */
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  /* Set default return values */
  k_thread_name_get_mock_fake.return_val = "";

  /* Set custom fake for wdt_install_timeout */
  wdt_install_timeout_mock_fake.custom_fake = wdt_install_timeout_custom_fake;

  /* Clear captured config */
  memset(&captured_wdt_cfg, 0, sizeof(captured_wdt_cfg));

  /* Clear and populate service registry */
  memset(serviceRegistry, 0, sizeof(serviceRegistry));

  /* Add test descriptors at various indices */
  serviceRegistry[0].threadId = (k_tid_t)0x1000;
  serviceRegistry[0].priority = SVC_PRIORITY_CRITICAL;
  serviceRegistry[0].heartbeatIntervalMs = 500;
  serviceRegistry[0].start = mock_start_callback;
  serviceRegistry[0].stop = mock_stop_callback;
  serviceRegistry[0].suspend = mock_suspend_callback;
  serviceRegistry[0].resume = mock_resume_callback;

  serviceRegistry[5].threadId = (k_tid_t)0x5000;
  serviceRegistry[5].priority = SVC_PRIORITY_APPLICATION;
  serviceRegistry[5].heartbeatIntervalMs = 2000;
  serviceRegistry[5].start = mock_start_callback;
  serviceRegistry[5].stop = mock_stop_callback;
  serviceRegistry[5].suspend = mock_suspend_callback;
  serviceRegistry[5].resume = mock_resume_callback;

  serviceRegistry[7].threadId = (k_tid_t)0x7000;
  serviceRegistry[7].priority = SVC_PRIORITY_CORE;
  serviceRegistry[7].heartbeatIntervalMs = 1000;
  serviceRegistry[7].start = mock_start_callback;
  serviceRegistry[7].stop = mock_stop_callback;
  serviceRegistry[7].suspend = mock_suspend_callback;
  serviceRegistry[7].resume = mock_resume_callback;

  registeredServiceCount = 8;
}

/**
 * @test The serviceMngrUtilInitHardWdg function must return error when device is not ready.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_deviceNotReady)
{
  int result;

  /* Setup: device_is_ready returns false */
  device_is_ready_mock_fake.return_val = false;

  /* Execute */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -ENODEV, "Expected -ENODEV when device not ready");
  zassert_equal(device_is_ready_mock_fake.call_count, 1, "device_is_ready should be called once");
}

/**
 * @test The serviceMngrUtilInitHardWdg function must return error when wdt_install_timeout fails.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_installTimeoutFails)
{
  int result;

  /* Setup: device is ready, wdt_install_timeout fails */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = -EINVAL;

  /* Execute */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when wdt_install_timeout fails");
  zassert_equal(wdt_install_timeout_mock_fake.call_count, 1, "wdt_install_timeout should be called once");
}

/**
 * @test The serviceMngrUtilInitHardWdg function must return error when wdt_setup fails.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_setupFails)
{
  int result;

  /* Setup: device ready, install timeout succeeds, wdt_setup fails */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = 0;
  wdt_setup_mock_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when wdt_setup fails");
  zassert_equal(wdt_setup_mock_fake.call_count, 1, "wdt_setup should be called once");
}

/**
 * @test The serviceMngrUtilInitHardWdg function must successfully initialize hardware watchdog.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_success)
{
  int result;

  /* Setup: all operations succeed */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = 0;
  wdt_setup_mock_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilInitHardWdg();

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");

  /* Verify device pointer was passed correctly */
  zassert_equal(device_is_ready_mock_fake.arg0_val, &mock_wdg_dev,
                "device_is_ready should be called with the watchdog device");
  zassert_equal(wdt_install_timeout_mock_fake.arg0_val, &mock_wdg_dev,
                "wdt_install_timeout should be called with the watchdog device");
  zassert_equal(wdt_setup_mock_fake.arg0_val, &mock_wdg_dev,
                "wdt_setup should be called with the watchdog device");

  /* Verify watchdog config was set correctly */
  zassert_equal(captured_wdt_cfg.flags, WDT_FLAG_RESET_SOC,
                "Watchdog flags should be WDT_FLAG_RESET_SOC");
  zassert_equal(captured_wdt_cfg.window.min, 0U,
                "Watchdog window min should be 0");
  zassert_equal(captured_wdt_cfg.window.max, CONFIG_SVC_MGR_WDT_TIMEOUT_MS,
                "Watchdog window max should be CONFIG_SVC_MGR_WDT_TIMEOUT_MS");
  zassert_is_null(captured_wdt_cfg.callback,
                  "Watchdog callback should be NULL");

  /* Verify wdt_setup was called with correct options */
  zassert_equal(wdt_setup_mock_fake.arg1_val, WDT_OPT_PAUSE_HALTED_BY_DBG,
                "wdt_setup should be called with WDT_OPT_PAUSE_HALTED_BY_DBG");
}

/**
 * @test The serviceMngrUtilInitSrvRegistry function must successfully initialize service registry.
 */
ZTEST(serviceMngrUtil, test_initSrvRegistry_success)
{
  int result;

  /* Execute */
  result = serviceMngrUtilInitSrvRegistry();

  /* Verify */
  zassert_equal(result, 0, "Expected success (0)");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must return error when descriptor is NULL.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_nullDescriptor)
{
  int result;

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(NULL);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL descriptor");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must return error when registry is full.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_registryFull)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Fill the registry to capacity */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Clear registry and fill it */
  registeredServiceCount = 0;
  for (size_t i = 0; i < CONFIG_SVC_MGR_MAX_SERVICES; i++) {
    result = serviceMngrUtilAddSrvToRegistry(&descriptor);
    zassert_equal(result, 0, "Should succeed adding service %zu", i);
  }

  /* Try to add one more (should fail) */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -ENOMEM, "Expected -ENOMEM when registry is full");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must return error when thread ID is NULL.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_nullThreadId)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Setup descriptor with NULL thread ID */
  descriptor.threadId = NULL;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL thread ID");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must return error when priority is invalid.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_invalidPriority)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Setup descriptor with invalid priority */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_COUNT;  /* Invalid: equal to count */
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for invalid priority");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must return error when heartbeat interval is zero.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_zeroHeartbeatInterval)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Setup descriptor with zero heartbeat interval */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 0;  /* Invalid: zero */
  descriptor.missedHeartbeats = 0;

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for zero heartbeat interval");
}

/**
 * @test The serviceMngrUtilAddSrvToRegistry function must successfully add service to registry.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_success)
{
  int result;
  ServiceDescriptor_t descriptor = {0};

  /* Clear registry for this test */
  registeredServiceCount = 0;

  /* Setup valid descriptor with all fields */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.lastHeartbeatMs = 999;  /* Should be reset to 0 */
  descriptor.missedHeartbeats = 5;   /* Should be reset to 0 */
  descriptor.state = SVC_STATE_RUNNING;  /* Should be set to STOPPED */
  descriptor.start = NULL;
  descriptor.stop = NULL;
  descriptor.suspend = NULL;
  descriptor.resume = NULL;

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");

  /* Verify descriptor was copied correctly */
  zassert_equal(serviceRegistry[0].threadId, (k_tid_t)0x1000,
                "Thread ID should be copied");
  zassert_equal(serviceRegistry[0].priority, SVC_PRIORITY_CORE,
                "Priority should be copied");
  zassert_equal(serviceRegistry[0].heartbeatIntervalMs, 1000,
                "Heartbeat interval should be copied");
  zassert_is_null(serviceRegistry[0].start,
                "Start callback should be copied");
  zassert_is_null(serviceRegistry[0].stop,
                "Stop callback should be copied");
  zassert_is_null(serviceRegistry[0].suspend,
                "Suspend callback should be copied");
  zassert_is_null(serviceRegistry[0].resume,
                "Resume callback should be copied");

  /* Verify runtime fields were initialized */
  zassert_equal(serviceRegistry[0].lastHeartbeatMs, 0,
                "Last heartbeat should be initialized to 0");
  zassert_equal(serviceRegistry[0].missedHeartbeats, 0,
                "Missed heartbeats should be initialized to 0");
  zassert_equal(serviceRegistry[0].state, SVC_STATE_STOPPED,
                "State should be initialized to STOPPED");
}

/**
 * @test The serviceMngrUtilGetRegEntryByIndex function must return NULL when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_getRegEntryByIndex_indexOutOfBounds)
{
  ServiceDescriptor_t *entry;

  /* Set empty registry */
  registeredServiceCount = 0;

  /* Try to get entry at index 0 from empty registry */
  entry = serviceMngrUtilGetRegEntryByIndex(0);

  /* Verify */
  zassert_is_null(entry, "Expected NULL for index out of bounds");
}

/**
 * @test The serviceMngrUtilGetRegEntryByIndex function must successfully retrieve registry entry by index.
 */
ZTEST(serviceMngrUtil, test_getRegEntryByIndex_success)
{
  ServiceDescriptor_t *entry;

  /* Get entry at index 5 (setup already populated it) */
  entry = serviceMngrUtilGetRegEntryByIndex(5);

  /* Verify */
  zassert_not_null(entry, "Expected valid pointer");
  zassert_equal(entry->threadId, (k_tid_t)0x5000, "Thread ID should match");
  zassert_equal(entry->priority, SVC_PRIORITY_APPLICATION, "Priority should match");
  zassert_equal(entry->heartbeatIntervalMs, 2000, "Heartbeat interval should match");
}

/**
 * @test The serviceMngrUtilGetIndexFromId function must return error when thread ID is NULL.
 */
ZTEST(serviceMngrUtil, test_getIndexFromId_nullThreadId)
{
  int result;

  /* Execute with NULL thread ID */
  result = serviceMngrUtilGetIndexFromId(NULL);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL thread ID");
}

/**
 * @test The serviceMngrUtilGetIndexFromId function must return error when thread ID is not found.
 */
ZTEST(serviceMngrUtil, test_getIndexFromId_threadIdNotFound)
{
  int result;

  /* Search for non-existent thread ID (setup has 0x1000, 0x5000, 0x7000) */
  result = serviceMngrUtilGetIndexFromId((k_tid_t)0x9999);

  /* Verify */
  zassert_equal(result, -ENOENT, "Expected -ENOENT when thread ID not found");
}

/**
 * @test The serviceMngrUtilGetIndexFromId function must successfully return index for matching thread ID.
 */
ZTEST(serviceMngrUtil, test_getIndexFromId_success)
{
  int result;

  /* Search for thread ID at index 7 (setup already populated it) */
  result = serviceMngrUtilGetIndexFromId((k_tid_t)0x7000);

  /* Verify */
  zassert_equal(result, 7, "Expected index 7 for thread ID 0x7000");
}

/**
 * @test The serviceMngrUtilStartService function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_startService_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilStartService(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilStartService function must return error when start callback is NULL.
 */
ZTEST(serviceMngrUtil, test_startService_nullCallback)
{
  int result;

  /* Set callback to NULL at index 5 */
  serviceRegistry[5].start = NULL;

  /* Execute */
  result = serviceMngrUtilStartService(5);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL start callback");
}

/**
 * @test The serviceMngrUtilStartService function must return error when start callback fails.
 */
ZTEST(serviceMngrUtil, test_startService_callbackFails)
{
  int result;

  /* Setup mock to fail */
  mock_start_callback_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilStartService(5);

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when start callback fails");
  zassert_equal(mock_start_callback_fake.call_count, 1, "Start callback should be called once");
}

/**
 * @test The serviceMngrUtilStartService function must successfully start service.
 */
ZTEST(serviceMngrUtil, test_startService_success)
{
  int result;

  /* Setup mock to succeed */
  mock_start_callback_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilStartService(5);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(mock_start_callback_fake.call_count, 1, "Start callback should be called once");

  /* Verify service state was updated */
  zassert_equal(serviceRegistry[5].state, SVC_STATE_RUNNING, "Service state should be RUNNING");
}

/**
 * @test The serviceMngrUtilStopService function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_stopService_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilStopService(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilStopService function must return error when stop callback is NULL.
 */
ZTEST(serviceMngrUtil, test_stopService_nullCallback)
{
  int result;

  /* Set callback to NULL at index 5 */
  serviceRegistry[5].stop = NULL;

  /* Execute */
  result = serviceMngrUtilStopService(5);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL stop callback");
}

/**
 * @test The serviceMngrUtilStopService function must return error when stop callback fails.
 */
ZTEST(serviceMngrUtil, test_stopService_callbackFails)
{
  int result;

  /* Setup mock to fail */
  mock_stop_callback_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilStopService(5);

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when stop callback fails");
  zassert_equal(mock_stop_callback_fake.call_count, 1, "Stop callback should be called once");
}

/**
 * @test The serviceMngrUtilStopService function must successfully stop service.
 */
ZTEST(serviceMngrUtil, test_stopService_success)
{
  int result;

  /* Setup mock to succeed */
  mock_stop_callback_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilStopService(5);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(mock_stop_callback_fake.call_count, 1, "Stop callback should be called once");
  /* Note: state is confirmed by the service itself via serviceManagerConfirmState, not here */
}

/**
 * @test The serviceMngrUtilSuspendService function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_suspendService_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilSuspendService(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilSuspendService function must return error when suspend callback is NULL.
 */
ZTEST(serviceMngrUtil, test_suspendService_nullCallback)
{
  int result;

  /* Set callback to NULL at index 5 */
  serviceRegistry[5].suspend = NULL;

  /* Execute */
  result = serviceMngrUtilSuspendService(5);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL suspend callback");
}

/**
 * @test The serviceMngrUtilSuspendService function must return error when suspend callback fails.
 */
ZTEST(serviceMngrUtil, test_suspendService_callbackFails)
{
  int result;

  /* Setup mock to fail */
  mock_suspend_callback_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilSuspendService(5);

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when suspend callback fails");
  zassert_equal(mock_suspend_callback_fake.call_count, 1, "Suspend callback should be called once");
}

/**
 * @test The serviceMngrUtilSuspendService function must successfully suspend service.
 */
ZTEST(serviceMngrUtil, test_suspendService_success)
{
  int result;

  /* Setup mock to succeed */
  mock_suspend_callback_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilSuspendService(5);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(mock_suspend_callback_fake.call_count, 1, "Suspend callback should be called once");
  /* Note: state is confirmed by the service itself via serviceManagerConfirmState, not here */
}

/**
 * @test The serviceMngrUtilResumeService function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_resumeService_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilResumeService(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilResumeService function must return error when resume callback is NULL.
 */
ZTEST(serviceMngrUtil, test_resumeService_nullCallback)
{
  int result;

  /* Set callback to NULL at index 5 */
  serviceRegistry[5].resume = NULL;

  /* Execute */
  result = serviceMngrUtilResumeService(5);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for NULL resume callback");
}

/**
 * @test The serviceMngrUtilResumeService function must return error when resume callback fails.
 */
ZTEST(serviceMngrUtil, test_resumeService_callbackFails)
{
  int result;

  /* Setup mock to fail */
  mock_resume_callback_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilResumeService(5);

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when resume callback fails");
  zassert_equal(mock_resume_callback_fake.call_count, 1, "Resume callback should be called once");
}

/**
 * @test The serviceMngrUtilResumeService function must successfully resume service.
 */
ZTEST(serviceMngrUtil, test_resumeService_success)
{
  int result;

  /* Setup mock to succeed */
  mock_resume_callback_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilResumeService(5);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(mock_resume_callback_fake.call_count, 1, "Resume callback should be called once");

  /* Verify service state was updated */
  zassert_equal(serviceRegistry[5].state, SVC_STATE_RUNNING, "Service state should be RUNNING");
}

/**
 * @test The serviceMngrUtilSetSrvState function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_setSrvState_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilSetSrvState(8, SVC_STATE_RUNNING);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilSetSrvState function must successfully set service state to running without resetting heartbeat.
 */
ZTEST(serviceMngrUtil, test_setSrvState_running)
{
  int result;

  serviceRegistry[5].missedHeartbeats = 2;
  serviceRegistry[5].lastHeartbeatMs = 1000;

  result = serviceMngrUtilSetSrvState(5, SVC_STATE_RUNNING);

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceRegistry[5].state, SVC_STATE_RUNNING, "Service state should be RUNNING");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 2,
                "Missed heartbeats should not be reset when transitioning to RUNNING");
  zassert_equal(k_uptime_get_mock_fake.call_count, 0,
                "k_uptime_get should not be called when transitioning to RUNNING");
}

/**
 * @test The serviceMngrUtilSetSrvState function must reset heartbeat tracking when setting service state to suspended.
 */
ZTEST(serviceMngrUtil, test_setSrvState_suspended)
{
  int result;

  serviceRegistry[5].missedHeartbeats = 2;
  serviceRegistry[5].lastHeartbeatMs = 0;
  k_uptime_get_mock_fake.return_val = 7000;

  result = serviceMngrUtilSetSrvState(5, SVC_STATE_SUSPENDED);

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceRegistry[5].state, SVC_STATE_SUSPENDED, "Service state should be SUSPENDED");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 0,
                "Missed heartbeats should be reset to 0 on suspend");
  zassert_equal(serviceRegistry[5].lastHeartbeatMs, 7000,
                "lastHeartbeatMs should be reset to current uptime on suspend");
}

/**
 * @test The serviceMngrUtilSetSrvState function must reset heartbeat tracking when setting service state to stopped.
 */
ZTEST(serviceMngrUtil, test_setSrvState_stopped)
{
  int result;

  serviceRegistry[5].missedHeartbeats = 2;
  serviceRegistry[5].lastHeartbeatMs = 0;
  k_uptime_get_mock_fake.return_val = 9000;

  result = serviceMngrUtilSetSrvState(5, SVC_STATE_STOPPED);

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceRegistry[5].state, SVC_STATE_STOPPED, "Service state should be STOPPED");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 0,
                "Missed heartbeats should be reset to 0 on stop");
  zassert_equal(serviceRegistry[5].lastHeartbeatMs, 9000,
                "lastHeartbeatMs should be reset to current uptime on stop");
}

/**
 * @test The serviceMngrUtilUpdateSrvHeartbeat function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_updateSrvHeartbeat_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilUpdateSrvHeartbeat(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilUpdateSrvHeartbeat function must successfully update heartbeat timestamp and reset missed count.
 */
ZTEST(serviceMngrUtil, test_updateSrvHeartbeat_success)
{
  int result;

  /* Set pre-existing missed heartbeats */
  serviceRegistry[5].missedHeartbeats = 3;
  k_uptime_get_mock_fake.return_val = 5000;

  /* Execute */
  result = serviceMngrUtilUpdateSrvHeartbeat(5);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");

  /* Verify heartbeat fields were updated */
  zassert_equal(serviceRegistry[5].lastHeartbeatMs, 5000,
                "Last heartbeat should be updated to current uptime");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 0,
                "Missed heartbeats should be reset to 0");
}

/**
 * @test The serviceMngrUtilFeedHardWdg function must return error when wdt_feed fails.
 */
ZTEST(serviceMngrUtil, test_feedHardWdg_fails)
{
  int result;

  /* Setup: wdt_feed fails */
  wdt_feed_mock_fake.return_val = -EIO;

  /* Execute */
  result = serviceMngrUtilFeedHardWdg();

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when wdt_feed fails");
  zassert_equal(wdt_feed_mock_fake.call_count, 1, "wdt_feed should be called once");
}

/**
 * @test The serviceMngrUtilFeedHardWdg function must successfully feed the hardware watchdog.
 */
ZTEST(serviceMngrUtil, test_feedHardWdg_success)
{
  int result;

  /* Setup: wdt_feed succeeds, channel ID set by init */
  wdt_install_timeout_mock_fake.return_val = 0;
  device_is_ready_mock_fake.return_val = true;
  serviceMngrUtilInitHardWdg();
  wdt_feed_mock_fake.return_val = 0;

  /* Execute */
  result = serviceMngrUtilFeedHardWdg();

  /* Verify */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(wdt_feed_mock_fake.call_count, 1, "wdt_feed should be called once");
  zassert_equal(wdt_feed_mock_fake.arg0_val, &mock_wdg_dev,
                "wdt_feed should be called with the watchdog device");
  zassert_equal(wdt_feed_mock_fake.arg1_val, 0, "wdt_feed should use channel 0");
}

/**
 * @test The serviceMngrUtilCheckSrvHeartbeat function must return error when index is out of bounds.
 */
ZTEST(serviceMngrUtil, test_checkSrvHeartbeat_indexOutOfBounds)
{
  int result;

  /* Execute with index beyond registered count (setup has count = 8) */
  result = serviceMngrUtilCheckSrvHeartbeat(8);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL for index out of bounds");
}

/**
 * @test The serviceMngrUtilCheckSrvHeartbeat function must return 0 without checking heartbeat when service is not running.
 */
ZTEST(serviceMngrUtil, test_checkSrvHeartbeat_serviceNotRunning)
{
  int result;

  /* Setup: service is suspended with stale heartbeat that would otherwise trigger a miss */
  serviceRegistry[5].state = SVC_STATE_SUSPENDED;
  serviceRegistry[5].lastHeartbeatMs = 0;
  serviceRegistry[5].missedHeartbeats = 0;
  k_uptime_get_mock_fake.return_val = 5000;

  /* Execute */
  result = serviceMngrUtilCheckSrvHeartbeat(5);

  /* Verify: returns 0, missed count untouched, k_uptime_get not called */
  zassert_equal(result, 0, "Expected 0 for non-running service");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 0,
                "Missed heartbeat count should not be incremented for suspended service");
  zassert_equal(k_uptime_get_mock_fake.call_count, 0,
                "k_uptime_get should not be called for non-running service");
}

/**
 * @test The serviceMngrUtilCheckSrvHeartbeat function must not increment missed count when heartbeat is on time.
 */
ZTEST(serviceMngrUtil, test_checkSrvHeartbeat_heartbeatOnTime)
{
  int result;

  /* lastHeartbeatMs=4000, interval=2000, now=5000 -> elapsed=1000 < 2000 */
  serviceRegistry[5].state = SVC_STATE_RUNNING;
  serviceRegistry[5].lastHeartbeatMs = 4000;
  k_uptime_get_mock_fake.return_val = 5000;

  /* Execute */
  result = serviceMngrUtilCheckSrvHeartbeat(5);

  /* Verify missed count is 0 and was not incremented */
  zassert_equal(result, 0, "Expected missed count 0 when heartbeat on time");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 0,
                "Missed heartbeat count should not be incremented");
}

/**
 * @test The serviceMngrUtilCheckSrvHeartbeat function must increment missed count when heartbeat interval elapsed.
 */
ZTEST(serviceMngrUtil, test_checkSrvHeartbeat_heartbeatMissed)
{
  int result;

  /* lastHeartbeatMs=2000, interval=2000, now=5000 -> elapsed=3000 >= 2000 */
  serviceRegistry[5].state = SVC_STATE_RUNNING;
  serviceRegistry[5].lastHeartbeatMs = 2000;
  k_uptime_get_mock_fake.return_val = 5000;
  k_thread_name_get_mock_fake.return_val = "test_service";

  /* Execute */
  result = serviceMngrUtilCheckSrvHeartbeat(5);

  /* Verify missed count was incremented to 1 */
  zassert_equal(result, 1, "Expected missed count 1 after first missed heartbeat");
  zassert_equal(serviceRegistry[5].missedHeartbeats, 1,
                "Missed heartbeat count should be incremented to 1");

  /* Verify k_thread_name_get was called with correct thread ID */
  zassert_equal(k_thread_name_get_mock_fake.arg0_val, (k_tid_t)0x5000,
                "k_thread_name_get should be called with the service thread ID");
}

/**
 * @test The serviceMngrUtilCheckSrvHeartbeat function must return error when missed heartbeat count exceeds maximum.
 */
ZTEST(serviceMngrUtil, test_checkSrvHeartbeat_timeout)
{
  int result;

  /* Pre-set missed heartbeats at max (3), one more will trigger timeout */
  serviceRegistry[5].state = SVC_STATE_RUNNING;
  serviceRegistry[5].missedHeartbeats = SVC_MGR_MAX_MISSED_HEARTBEATS;
  serviceRegistry[5].lastHeartbeatMs = 2000;
  k_uptime_get_mock_fake.return_val = 5000;
  k_thread_name_get_mock_fake.return_val = "test_service";

  /* Execute */
  result = serviceMngrUtilCheckSrvHeartbeat(5);

  /* Verify error is returned */
  zassert_equal(result, -ETIMEDOUT, "Expected -ETIMEDOUT when missed heartbeats exceed maximum");
}

ZTEST_SUITE(serviceMngrUtil, NULL, util_tests_setup, util_tests_before, NULL, NULL);
