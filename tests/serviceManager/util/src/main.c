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

ZTEST_SUITE(serviceMngrUtil, NULL, util_tests_setup, util_tests_before, NULL, NULL);
