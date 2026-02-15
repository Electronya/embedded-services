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

/* Wrap utility functions to use mocks */
#define serviceMngrUtilInitHardWdg serviceMngrUtilInitHardWdg_mock
#define serviceMngrUtilInitSrvRegistry serviceMngrUtilInitSrvRegistry_mock

/* Mock Kconfig options */
#define CONFIG_ENYA_SERVICE_MANAGER 1
#define CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL 3

/* FFF fakes list */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(serviceMngrUtilInitHardWdg_mock) \
  FAKE(serviceMngrUtilInitSrvRegistry_mock)

/* Mock utility functions */
FAKE_VALUE_FUNC(int, serviceMngrUtilInitHardWdg_mock);
FAKE_VALUE_FUNC(int, serviceMngrUtilInitSrvRegistry_mock);

#include "serviceManager.c"

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
}

/**
 * @test The serviceManagerInit function must return error when hardware watchdog init fails.
 */
ZTEST(serviceManager, test_init_hardWdgFails)
{
  int result;

  /* Setup: hardware watchdog init fails */
  serviceMngrUtilInitHardWdg_mock_fake.return_val = -ENODEV;

  /* Execute */
  result = serviceManagerInit();

  /* Verify */
  zassert_equal(result, -ENODEV, "Expected -ENODEV when hardware watchdog init fails");
  zassert_equal(serviceMngrUtilInitHardWdg_mock_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_mock_fake.call_count, 0,
                "serviceMngrUtilInitSrvRegistry should not be called when watchdog init fails");
}

/**
 * @test The serviceManagerInit function must return error when service registry init fails.
 */
ZTEST(serviceManager, test_init_registryFails)
{
  int result;

  /* Setup: hardware watchdog succeeds, registry init fails */
  serviceMngrUtilInitHardWdg_mock_fake.return_val = 0;
  serviceMngrUtilInitSrvRegistry_mock_fake.return_val = -ENOMEM;

  /* Execute */
  result = serviceManagerInit();

  /* Verify */
  zassert_equal(result, -ENOMEM, "Expected -ENOMEM when registry init fails");
  zassert_equal(serviceMngrUtilInitHardWdg_mock_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_mock_fake.call_count, 1,
                "serviceMngrUtilInitSrvRegistry should be called once");
}

/**
 * @test The serviceManagerInit function must successfully initialize when all components succeed.
 */
ZTEST(serviceManager, test_init_success)
{
  int result;

  /* Setup: all operations succeed */
  serviceMngrUtilInitHardWdg_mock_fake.return_val = 0;
  serviceMngrUtilInitSrvRegistry_mock_fake.return_val = 0;

  /* Execute */
  result = serviceManagerInit();

  /* Verify */
  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(serviceMngrUtilInitHardWdg_mock_fake.call_count, 1,
                "serviceMngrUtilInitHardWdg should be called once");
  zassert_equal(serviceMngrUtilInitSrvRegistry_mock_fake.call_count, 1,
                "serviceMngrUtilInitSrvRegistry should be called once");
}

ZTEST_SUITE(serviceManager, NULL, service_tests_setup, service_tests_before, NULL, NULL);
