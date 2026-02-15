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
#define k_thread_resume k_thread_resume_mock
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

/* FFF fakes list */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(device_is_ready_mock) \
  FAKE(wdt_install_timeout_mock) \
  FAKE(wdt_setup_mock) \
  FAKE(k_thread_resume_mock) \
  FAKE(k_thread_name_get_mock)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(serviceManager, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Mock device and watchdog functions */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, wdt_install_timeout_mock, const struct device *, const struct wdt_timeout_cfg *);
FAKE_VALUE_FUNC(int, wdt_setup_mock, const struct device *, uint8_t);

/* Mock thread functions */
FAKE_VOID_FUNC(k_thread_resume_mock, k_tid_t);
FAKE_VALUE_FUNC(const char *, k_thread_name_get_mock, k_tid_t);

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

  /* Set custom fake for wdt_install_timeout */
  wdt_install_timeout_mock_fake.custom_fake = wdt_install_timeout_custom_fake;

  /* Clear captured config */
  memset(&captured_wdt_cfg, 0, sizeof(captured_wdt_cfg));
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

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

  /* Fill the registry to capacity */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

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

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

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

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

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

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

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
  ServiceDescriptor_t descriptor;

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

  /* Setup valid descriptor */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 5;  /* Should be reset to 0 */

  /* Execute */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, 0, "Expected success (0)");

  /* Add another service to verify count increments */
  descriptor.threadId = (k_tid_t)0x2000;
  descriptor.priority = SVC_PRIORITY_APPLICATION;
  descriptor.heartbeatIntervalMs = 2000;

  result = serviceMngrUtilAddSrvToRegistry(&descriptor);
  zassert_equal(result, 0, "Expected success (0) for second service");
}

/**
 * @test The serviceMngrUtilStartServices function must succeed when registry is empty.
 */
ZTEST(serviceMngrUtil, test_startServices_emptyRegistry)
{
  int result;

  /* Initialize empty registry */
  serviceMngrUtilInitSrvRegistry();

  /* Mock thread name get to return valid names */
  k_thread_name_get_mock_fake.return_val = "test_service";

  /* Call function under test */
  result = serviceMngrUtilStartServices();

  /* Verify */
  zassert_equal(result, 0, "Expected success (0) with empty registry");
  zassert_equal(k_thread_resume_mock_fake.call_count, 0, "k_thread_resume should not be called with empty registry");
}

/**
 * @test The serviceMngrUtilStartServices function must start services in priority order.
 */
ZTEST(serviceMngrUtil, test_startServices_priorityOrder)
{
  int result;
  ServiceDescriptor_t descriptor;
  k_tid_t criticalThread = (k_tid_t)0x1000;
  k_tid_t coreThread = (k_tid_t)0x2000;
  k_tid_t appThread = (k_tid_t)0x3000;

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

  /* Add services in non-priority order to verify sorting */
  /* Add CORE service first */
  descriptor.threadId = coreThread;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;
  serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Add APPLICATION service */
  descriptor.threadId = appThread;
  descriptor.priority = SVC_PRIORITY_APPLICATION;
  descriptor.heartbeatIntervalMs = 2000;
  serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Add CRITICAL service last */
  descriptor.threadId = criticalThread;
  descriptor.priority = SVC_PRIORITY_CRITICAL;
  descriptor.heartbeatIntervalMs = 500;
  serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Mock thread name get to return valid names */
  k_thread_name_get_mock_fake.return_val = "test_service";

  /* Call function under test */
  result = serviceMngrUtilStartServices();

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(k_thread_resume_mock_fake.call_count, 3, "k_thread_resume should be called 3 times");

  /* Verify services were started in priority order: CRITICAL, CORE, APPLICATION */
  zassert_equal(k_thread_resume_mock_fake.arg0_history[0], criticalThread,
                "First service started should be CRITICAL");
  zassert_equal(k_thread_resume_mock_fake.arg0_history[1], coreThread,
                "Second service started should be CORE");
  zassert_equal(k_thread_resume_mock_fake.arg0_history[2], appThread,
                "Third service started should be APPLICATION");

  /* Verify k_thread_name_get was called for each service */
  zassert_equal(k_thread_name_get_mock_fake.call_count, 3, "k_thread_name_get should be called 3 times");
}

/**
 * @test The serviceMngrUtilStartServices function must handle NULL thread names gracefully.
 */
ZTEST(serviceMngrUtil, test_startServices_nullThreadName)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry */
  serviceMngrUtilInitSrvRegistry();

  /* Add one service */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;
  serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Mock thread name get to return NULL */
  k_thread_name_get_mock_fake.return_val = NULL;

  /* Call function under test - should handle NULL gracefully */
  result = serviceMngrUtilStartServices();

  /* Verify */
  zassert_equal(result, 0, "Expected success (0) even with NULL thread name");
  zassert_equal(k_thread_resume_mock_fake.call_count, 1, "k_thread_resume should be called once");
  zassert_equal(k_thread_name_get_mock_fake.call_count, 1, "k_thread_name_get should be called once");
}

ZTEST_SUITE(serviceMngrUtil, NULL, util_tests_setup, util_tests_before, NULL, NULL);
