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

/* Wrap k_thread_create to use mock */
#define k_thread_create k_thread_create_mock

/* Wrap k_thread_name_set to use mock */
#define k_thread_name_set k_thread_name_set_mock

/* Wrap k_thread_start to use mock */
#define k_thread_start k_thread_start_mock

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
  FAKE(k_thread_create_mock) \
  FAKE(k_thread_name_set_mock) \
  FAKE(k_thread_start_mock) \
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

/**
 * Requirement: The adcAcqInit function must return -EINVAL when adcConfig is NULL.
 */
ZTEST(adc_service_tests, test_init_null_adc_config)
{
  AdcSubConfig_t subConfig = {0};
  k_tid_t threadId;
  int result;

  /* Execute with NULL adcConfig */
  result = adcAcqInit(NULL, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -EINVAL,
                "adcAcqInit should return -EINVAL when adcConfig is NULL");

  /* Verify no utility functions were called */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 0,
                "adcAcqUtilInitAdc should not be called");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 0,
                "adcAcqUtilInitSubscriptions should not be called");
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
}

/**
 * Requirement: The adcAcqInit function must return -EINVAL when adcSubConfig is NULL.
 */
ZTEST(adc_service_tests, test_init_null_sub_config)
{
  AdcConfig_t adcConfig = {0};
  k_tid_t threadId;
  int result;

  /* Execute with NULL adcSubConfig */
  result = adcAcqInit(&adcConfig, NULL, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -EINVAL,
                "adcAcqInit should return -EINVAL when adcSubConfig is NULL");

  /* Verify no utility functions were called */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 0,
                "adcAcqUtilInitAdc should not be called");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 0,
                "adcAcqUtilInitSubscriptions should not be called");
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
}

/**
 * Requirement: The adcAcqInit function must return error when adcAcqUtilInitAdc fails.
 */
ZTEST(adc_service_tests, test_init_adc_init_failure)
{
  AdcConfig_t adcConfig = {.samplingRate = 500, .filterTau = 31};
  AdcSubConfig_t subConfig = {0};
  k_tid_t threadId;
  int result;

  /* Setup: adcAcqUtilInitAdc returns error */
  adcAcqUtilInitAdc_fake.return_val = -EIO;

  /* Execute */
  result = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -EIO,
                "adcAcqInit should return error from adcAcqUtilInitAdc");

  /* Verify adcAcqUtilInitAdc was called with correct parameter */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitAdc_fake.arg0_val, &adcConfig,
                "adcAcqUtilInitAdc should be called with adcConfig");

  /* Verify subsequent functions were not called */
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 0,
                "adcAcqUtilInitSubscriptions should not be called");
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
}

/**
 * Requirement: The adcAcqInit function must return error when adcAcqUtilInitSubscriptions fails.
 */
ZTEST(adc_service_tests, test_init_subscriptions_init_failure)
{
  AdcConfig_t adcConfig = {.samplingRate = 500, .filterTau = 31};
  AdcSubConfig_t subConfig = {.maxSubCount = 4, .activeSubCount = 0, .notificationRate = 100};
  k_tid_t threadId;
  int result;

  /* Setup: adcAcqUtilInitAdc succeeds, adcAcqUtilInitSubscriptions fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from adcAcqUtilInitSubscriptions");

  /* Verify adcAcqUtilInitAdc was called with correct parameter */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitAdc_fake.arg0_val, &adcConfig,
                "adcAcqUtilInitAdc should be called with adcConfig");

  /* Verify adcAcqUtilInitSubscriptions was called with correct parameter */
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.arg0_val, &subConfig,
                "adcAcqUtilInitSubscriptions should be called with subConfig");

  /* Verify subsequent functions were not called */
  zassert_equal(adcAcqFilterInit_fake.call_count, 0,
                "adcAcqFilterInit should not be called");
}

/**
 * Requirement: The adcAcqInit function must return error when adcAcqFilterInit fails.
 */
ZTEST(adc_service_tests, test_init_filter_init_failure)
{
  AdcConfig_t adcConfig = {.samplingRate = 500, .filterTau = 31};
  AdcSubConfig_t subConfig = {.maxSubCount = 4, .activeSubCount = 0, .notificationRate = 100};
  k_tid_t threadId;
  int result;

  /* Setup: adcAcqUtilInitAdc and adcAcqUtilInitSubscriptions succeed, adcAcqFilterInit fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from adcAcqFilterInit");

  /* Verify adcAcqUtilInitAdc was called with correct parameter */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitAdc_fake.arg0_val, &adcConfig,
                "adcAcqUtilInitAdc should be called with adcConfig");

  /* Verify adcAcqUtilInitSubscriptions was called with correct parameter */
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.arg0_val, &subConfig,
                "adcAcqUtilInitSubscriptions should be called with subConfig");

  /* Verify adcAcqFilterInit was called with channel count */
  zassert_equal(adcAcqUtilGetChanCount_fake.call_count, 1,
                "adcAcqUtilGetChanCount should be called once");
  zassert_equal(adcAcqFilterInit_fake.call_count, 1,
                "adcAcqFilterInit should be called once");
  zassert_equal(adcAcqFilterInit_fake.arg0_val, 4,
                "adcAcqFilterInit should be called with channel count 4");
}

/**
 * Requirement: The adcAcqInit function must return error when k_thread_name_set fails.
 */
ZTEST(adc_service_tests, test_init_thread_name_set_failure)
{
  AdcConfig_t adcConfig = {.samplingRate = 500, .filterTau = 31};
  AdcSubConfig_t subConfig = {.maxSubCount = 4, .activeSubCount = 0, .notificationRate = 100};
  k_tid_t threadId;
  int result;

  /* Setup: all init functions succeed, k_thread_name_set fails */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = (k_tid_t)&thread;
  k_thread_name_set_mock_fake.return_val = -ENOMEM;

  /* Execute */
  result = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, -ENOMEM,
                "adcAcqInit should return error from k_thread_name_set");

  /* Verify all init functions were called */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqFilterInit_fake.call_count, 1,
                "adcAcqFilterInit should be called once");

  /* Verify k_thread_create was called */
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");

  /* Verify k_thread_name_set was called */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
}

/**
 * Requirement: The adcAcqInit function must initialize all components and create the thread.
 */
ZTEST(adc_service_tests, test_init_success)
{
  AdcConfig_t adcConfig = {.samplingRate = 500, .filterTau = 31};
  AdcSubConfig_t subConfig = {.maxSubCount = 4, .activeSubCount = 0, .notificationRate = 100};
  k_tid_t threadId;
  int result;

  /* Setup: all functions succeed */
  adcAcqUtilInitAdc_fake.return_val = 0;
  adcAcqUtilInitSubscriptions_fake.return_val = 0;
  adcAcqUtilGetChanCount_fake.return_val = 4;
  adcAcqFilterInit_fake.return_val = 0;
  k_thread_create_mock_fake.return_val = (k_tid_t)&thread;
  k_thread_name_set_mock_fake.return_val = 0;

  /* Execute */
  result = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAcqInit should return 0 on success");

  /* Verify threadId was set */
  zassert_equal(threadId, (k_tid_t)&thread,
                "threadId should be set to the created thread");

  /* Verify adcAcqUtilInitAdc was called with correct parameter */
  zassert_equal(adcAcqUtilInitAdc_fake.call_count, 1,
                "adcAcqUtilInitAdc should be called once");
  zassert_equal(adcAcqUtilInitAdc_fake.arg0_val, &adcConfig,
                "adcAcqUtilInitAdc should be called with adcConfig");

  /* Verify adcAcqUtilInitSubscriptions was called with correct parameter */
  zassert_equal(adcAcqUtilInitSubscriptions_fake.call_count, 1,
                "adcAcqUtilInitSubscriptions should be called once");
  zassert_equal(adcAcqUtilInitSubscriptions_fake.arg0_val, &subConfig,
                "adcAcqUtilInitSubscriptions should be called with subConfig");

  /* Verify adcAcqFilterInit was called with channel count */
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
  zassert_equal((uintptr_t)k_thread_create_mock_fake.arg4_val, subConfig.notificationRate,
                "k_thread_create should be called with notification rate as p1");
  zassert_is_null(k_thread_create_mock_fake.arg5_val,
                  "k_thread_create should be called with NULL as p2");
  zassert_is_null(k_thread_create_mock_fake.arg6_val,
                  "k_thread_create should be called with NULL as p3");

  /* Verify k_thread_name_set was called with correct parameters */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, &thread,
                "k_thread_name_set should be called with thread struct");
  zassert_str_equal(k_thread_name_set_mock_fake.arg1_val, "adcAcquisition",
                    "k_thread_name_set should be called with service name");
}

/**
 * Requirement: The adcAcqStart function must return error when adcAcqUtilStartTrigger fails.
 */
ZTEST(adc_service_tests, test_start_trigger_failure)
{
  int result;

  /* Setup: adcAcqUtilStartTrigger returns error */
  adcAcqUtilStartTrigger_fake.return_val = -EIO;

  /* Execute */
  result = adcAcqStart();

  /* Verify return value */
  zassert_equal(result, -EIO,
                "adcAcqStart should return error from adcAcqUtilStartTrigger");

  /* Verify k_thread_start was called with thread struct */
  zassert_equal(k_thread_start_mock_fake.call_count, 1,
                "k_thread_start should be called once");
  zassert_equal(k_thread_start_mock_fake.arg0_val, &thread,
                "k_thread_start should be called with thread struct");

  /* Verify adcAcqUtilStartTrigger was called */
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/**
 * Requirement: The adcAcqStart function must start the thread and trigger.
 */
ZTEST(adc_service_tests, test_start_success)
{
  int result;

  /* Setup: adcAcqUtilStartTrigger succeeds */
  adcAcqUtilStartTrigger_fake.return_val = 0;

  /* Execute */
  result = adcAcqStart();

  /* Verify return value */
  zassert_equal(result, 0,
                "adcAcqStart should return 0 on success");

  /* Verify k_thread_start was called with thread struct */
  zassert_equal(k_thread_start_mock_fake.call_count, 1,
                "k_thread_start should be called once");
  zassert_equal(k_thread_start_mock_fake.arg0_val, &thread,
                "k_thread_start should be called with thread struct");

  /* Verify adcAcqUtilStartTrigger was called */
  zassert_equal(adcAcqUtilStartTrigger_fake.call_count, 1,
                "adcAcqUtilStartTrigger should be called once");
}

/* Dummy callback for subscription tests */
static int dummyCallback(SrvMsgPayload_t *data)
{
  ARG_UNUSED(data);
  return 0;
}

/**
 * Requirement: The adcAcqSubscribe function must return error when adcAcqUtilAddSubscription fails.
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
 * Requirement: The adcAcqSubscribe function must add a subscription callback.
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
 * Requirement: The adcAcqUnsubscribe function must return error when adcAcqUtilRemoveSubscription fails.
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
 * Requirement: The adcAcqUnsubscribe function must remove a subscription callback.
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
 * Requirement: The adcAcqPauseSubscription function must return error when adcAcqUtilSetSubPauseState fails.
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
 * Requirement: The adcAcqPauseSubscription function must pause a subscription.
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
 * Requirement: The adcAqcUnpauseSubscription function must return error when adcAcqUtilSetSubPauseState fails.
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
 * Requirement: The adcAqcUnpauseSubscription function must unpause a subscription.
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
