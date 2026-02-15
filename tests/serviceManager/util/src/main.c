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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Rename functions before Zephyr defines them */
#define device_is_ready device_is_ready_mock
#define wdt_install_timeout wdt_install_timeout_mock
#define wdt_setup wdt_setup_mock

/* Prevent device and watchdog driver headers - we'll define types manually */
#define ZEPHYR_INCLUDE_DEVICE_H_
#define ZEPHYR_INCLUDE_DRIVERS_WATCHDOG_H_

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

DEFINE_FFF_GLOBALS;

/* Define device structure manually (mimics minimal Zephyr device structure) */
struct device {
  const char *name;
};

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
  FAKE(wdt_setup_mock)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(serviceManager, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Mock device and watchdog functions */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, wdt_install_timeout_mock, const struct device *, const struct wdt_timeout_cfg *);
FAKE_VALUE_FUNC(int, wdt_setup_mock, const struct device *, uint8_t);

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
#define DEVICE_DT_GET(node) (&mock_wdg_dev)

/* Include utility implementation */
#include "serviceManagerUtil.c"

/**
 * Test setup function.
 */
static void *util_tests_setup(void)
{
  return NULL;
}

/**
 * Test before function.
 */
static void util_tests_before(void *fixture)
{
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  /* Clear captured config */
  memset(&captured_wdt_cfg, 0, sizeof(captured_wdt_cfg));
}

/**
 * @brief Test serviceMngrUtilInitHardWdg failure when device is not ready.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_deviceNotReady)
{
  int result;

  /* Configure mock: device is not ready */
  device_is_ready_mock_fake.return_val = false;

  /* Call function under test */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -ENODEV, "Expected -ENODEV when device not ready");
  zassert_equal(device_is_ready_mock_fake.call_count, 1, "device_is_ready should be called once");
  zassert_equal(wdt_install_timeout_mock_fake.call_count, 0, "wdt_install_timeout should not be called");
  zassert_equal(wdt_setup_mock_fake.call_count, 0, "wdt_setup should not be called");
}

/**
 * @brief Test serviceMngrUtilInitHardWdg failure when wdt_install_timeout fails.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_installTimeoutFails)
{
  int result;

  /* Configure mocks: device is ready but install timeout fails */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = -EINVAL;

  /* Call function under test */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when wdt_install_timeout fails");
  zassert_equal(device_is_ready_mock_fake.call_count, 1, "device_is_ready should be called once");
  zassert_equal(wdt_install_timeout_mock_fake.call_count, 1, "wdt_install_timeout should be called once");
  zassert_equal(wdt_setup_mock_fake.call_count, 0, "wdt_setup should not be called when install fails");
}

/**
 * @brief Test serviceMngrUtilInitHardWdg failure when wdt_setup fails.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_setupFails)
{
  int result;

  /* Configure mocks: device is ready, install succeeds, but setup fails */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = 0;
  wdt_setup_mock_fake.return_val = -EIO;

  /* Call function under test */
  result = serviceMngrUtilInitHardWdg();

  /* Verify */
  zassert_equal(result, -EIO, "Expected -EIO when wdt_setup fails");
  zassert_equal(device_is_ready_mock_fake.call_count, 1, "device_is_ready should be called once");
  zassert_equal(wdt_install_timeout_mock_fake.call_count, 1, "wdt_install_timeout should be called once");
  zassert_equal(wdt_setup_mock_fake.call_count, 1, "wdt_setup should be called once");
}

/**
 * @brief Test serviceMngrUtilInitHardWdg success case.
 */
ZTEST(serviceMngrUtil, test_initHardWdg_success)
{
  int result;

  /* Configure mocks: all operations succeed */
  device_is_ready_mock_fake.return_val = true;
  wdt_install_timeout_mock_fake.return_val = 0;
  wdt_install_timeout_mock_fake.custom_fake = wdt_install_timeout_custom_fake;
  wdt_setup_mock_fake.return_val = 0;

  /* Call function under test */
  result = serviceMngrUtilInitHardWdg();

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");

  /* Verify device_is_ready called with correct device */
  zassert_equal(device_is_ready_mock_fake.call_count, 1, "device_is_ready should be called once");
  zassert_equal(device_is_ready_mock_fake.arg0_val, &mock_wdg_dev, "device_is_ready should be called with watchdog device");

  /* Verify wdt_install_timeout called with correct parameters */
  zassert_equal(wdt_install_timeout_mock_fake.call_count, 1, "wdt_install_timeout should be called once");
  zassert_equal(wdt_install_timeout_mock_fake.arg0_val, &mock_wdg_dev, "wdt_install_timeout should be called with watchdog device");

  /* Verify captured config values */
  zassert_equal(captured_wdt_cfg.flags, WDT_FLAG_RESET_SOC, "wdt_install_timeout config flags should be WDT_FLAG_RESET_SOC");
  zassert_equal(captured_wdt_cfg.window.min, 0U, "wdt_install_timeout config window.min should be 0");
  zassert_equal(captured_wdt_cfg.window.max, CONFIG_SVC_MGR_WDT_TIMEOUT_MS, "wdt_install_timeout config window.max should be CONFIG_SVC_MGR_WDT_TIMEOUT_MS");
  zassert_is_null(captured_wdt_cfg.callback, "wdt_install_timeout config callback should be NULL");

  /* Verify wdt_setup called with correct parameters */
  zassert_equal(wdt_setup_mock_fake.call_count, 1, "wdt_setup should be called once");
  zassert_equal(wdt_setup_mock_fake.arg0_val, &mock_wdg_dev, "wdt_setup should be called with watchdog device");
  zassert_equal(wdt_setup_mock_fake.arg1_val, WDT_OPT_PAUSE_HALTED_BY_DBG, "wdt_setup should be called with WDT_OPT_PAUSE_HALTED_BY_DBG");
}

/**
 * @brief Test serviceMngrUtilInitSrvRegistry success case.
 */
ZTEST(serviceMngrUtil, test_initSrvRegistry_success)
{
  int result;

  /* Call function under test */
  result = serviceMngrUtilInitSrvRegistry();

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry failure when descriptor is NULL.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_nullDescriptor)
{
  int result;

  /* Call function under test with NULL descriptor */
  result = serviceMngrUtilAddSrvToRegistry(NULL);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when descriptor is NULL");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry failure when registry is full.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_registryFull)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry first */
  serviceMngrUtilInitSrvRegistry();

  /* Set up valid descriptor */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Fill the registry to capacity */
  for(int i = 0; i < CONFIG_SVC_MGR_MAX_SERVICES; i++)
  {
    result = serviceMngrUtilAddSrvToRegistry(&descriptor);
    zassert_equal(result, 0, "Service registration should succeed");
  }

  /* Try to add one more service - should fail */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -ENOMEM, "Expected -ENOMEM when registry is full");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry failure when threadId is NULL.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_nullThreadId)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry first */
  serviceMngrUtilInitSrvRegistry();

  /* Set up descriptor with NULL threadId */
  descriptor.threadId = NULL;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Call function under test */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when threadId is NULL");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry failure when priority is invalid.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_invalidPriority)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry first */
  serviceMngrUtilInitSrvRegistry();

  /* Set up descriptor with invalid priority */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_COUNT;  /* Invalid: >= SVC_PRIORITY_COUNT */
  descriptor.heartbeatIntervalMs = 1000;
  descriptor.missedHeartbeats = 0;

  /* Call function under test */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when priority is invalid");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry failure when heartbeat interval is zero.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_zeroHeartbeatInterval)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry first */
  serviceMngrUtilInitSrvRegistry();

  /* Set up descriptor with zero heartbeat interval */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 0;  /* Invalid: heartbeat interval cannot be zero */
  descriptor.missedHeartbeats = 0;

  /* Call function under test */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify */
  zassert_equal(result, -EINVAL, "Expected -EINVAL when heartbeat interval is zero");
}

/**
 * @brief Test serviceMngrUtilAddSrvToRegistry success case.
 */
ZTEST(serviceMngrUtil, test_addSrvToRegistry_success)
{
  int result;
  ServiceDescriptor_t descriptor;

  /* Initialize registry first */
  serviceMngrUtilInitSrvRegistry();

  /* Set up valid descriptor for critical priority service */
  descriptor.threadId = (k_tid_t)0x1000;
  descriptor.priority = SVC_PRIORITY_CRITICAL;
  descriptor.heartbeatIntervalMs = 500;
  descriptor.missedHeartbeats = 0;

  /* Call function under test */
  result = serviceMngrUtilAddSrvToRegistry(&descriptor);

  /* Verify result */
  zassert_equal(result, 0, "Expected success (0)");

  /* Add another service with different priority */
  descriptor.threadId = (k_tid_t)0x2000;
  descriptor.priority = SVC_PRIORITY_CORE;
  descriptor.heartbeatIntervalMs = 1000;

  result = serviceMngrUtilAddSrvToRegistry(&descriptor);
  zassert_equal(result, 0, "Expected success (0) for second service");

  /* Add third service with application priority */
  descriptor.threadId = (k_tid_t)0x3000;
  descriptor.priority = SVC_PRIORITY_APPLICATION;
  descriptor.heartbeatIntervalMs = 2000;

  result = serviceMngrUtilAddSrvToRegistry(&descriptor);
  zassert_equal(result, 0, "Expected success (0) for third service");
}

ZTEST_SUITE(serviceMngrUtil, NULL, util_tests_setup, util_tests_before, NULL, NULL);
