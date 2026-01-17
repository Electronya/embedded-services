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

/* Prevent adcAcquisition header - we define types manually */
#define ADC_ACQUISITION

/* Wrap k_sleep to use mock */
#define k_sleep k_sleep_mock

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

/* Mock utility functions */
FAKE_VALUE_FUNC(int, adcAcqUtilInitAdc, AdcConfig_t *);
FAKE_VALUE_FUNC(int, adcAcqUtilInitSubscriptions, AdcSubConfig_t *);
FAKE_VALUE_FUNC(int, adcAcqUtilStartTrigger);
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
  FAKE(adcAcqUtilInitAdc) \
  FAKE(adcAcqUtilInitSubscriptions) \
  FAKE(adcAcqUtilStartTrigger) \
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
#define CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE 1024

/* Include service implementation */
#include "adcAcquisition.c"

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
}

/**
 * Requirement: The run function must call adcAcqUtilProcessData each iteration.
 * When adcAcqUtilProcessData fails, it must log an error but continue execution.
 */
ZTEST(adc_service_tests, test_run_process_data_failure)
{
  k_timeout_t expected_timeout = K_MSEC(100);

  /* Setup: adcAcqUtilProcessData returns error */
  adcAcqUtilProcessData_fake.return_val = -EIO;
  adcAcqUtilNotifySubscribers_fake.return_val = 0;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify k_sleep was called twice with correct timeout */
  zassert_equal(k_sleep_mock_fake.call_count, 2,
                "k_sleep should be called twice for 2 iterations");
  zassert_equal(k_sleep_mock_fake.arg0_history[0].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");
  zassert_equal(k_sleep_mock_fake.arg0_history[1].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");

  /* Verify adcAcqUtilProcessData was called twice (2 iterations) */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was still called twice despite process data failure */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should still be called twice");
}

/**
 * Requirement: The run function must call adcAcqUtilNotifySubscribers each iteration.
 * When adcAcqUtilNotifySubscribers fails, it must log an error but continue execution.
 */
ZTEST(adc_service_tests, test_run_notify_subscribers_failure)
{
  k_timeout_t expected_timeout = K_MSEC(100);

  /* Setup: adcAcqUtilNotifySubscribers returns error */
  adcAcqUtilProcessData_fake.return_val = 0;
  adcAcqUtilNotifySubscribers_fake.return_val = -EIO;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify k_sleep was called twice with correct timeout */
  zassert_equal(k_sleep_mock_fake.call_count, 2,
                "k_sleep should be called twice for 2 iterations");
  zassert_equal(k_sleep_mock_fake.arg0_history[0].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");
  zassert_equal(k_sleep_mock_fake.arg0_history[1].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");

  /* Verify adcAcqUtilProcessData was called twice */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was called twice despite failure */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should be called twice");
}

/**
 * Requirement: The run function must call k_sleep, adcAcqUtilProcessData, and
 * adcAcqUtilNotifySubscribers each iteration with correct parameters.
 */
ZTEST(adc_service_tests, test_run_success)
{
  k_timeout_t expected_timeout = K_MSEC(100);

  /* Setup: all functions return success */
  adcAcqUtilProcessData_fake.return_val = 0;
  adcAcqUtilNotifySubscribers_fake.return_val = 0;

  /* Execute run function with notification rate of 100ms */
  run((void *)(uintptr_t)100, NULL, NULL);

  /* Verify k_sleep was called twice with correct timeout */
  zassert_equal(k_sleep_mock_fake.call_count, 2,
                "k_sleep should be called twice for 2 iterations");
  zassert_equal(k_sleep_mock_fake.arg0_history[0].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");
  zassert_equal(k_sleep_mock_fake.arg0_history[1].ticks, expected_timeout.ticks,
                "k_sleep should be called with K_MSEC(100)");

  /* Verify adcAcqUtilProcessData was called twice */
  zassert_equal(adcAcqUtilProcessData_fake.call_count, 2,
                "adcAcqUtilProcessData should be called twice");

  /* Verify adcAcqUtilNotifySubscribers was called twice */
  zassert_equal(adcAcqUtilNotifySubscribers_fake.call_count, 2,
                "adcAcqUtilNotifySubscribers should be called twice");
}

ZTEST_SUITE(adc_service_tests, NULL, service_tests_setup, service_tests_before, NULL, NULL);
