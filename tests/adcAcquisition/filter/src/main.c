/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    Claude AI Assistant
 * @date      2025-12-29
 * @brief     ADC Acquisition Filter Unit Tests
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Mock k_malloc function */
FAKE_VALUE_FUNC(void *, k_malloc, size_t);

/* List of fakes for easy reset */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(k_malloc)

/* Stub for logging backend (not available in unit test environment) */
void z_log_minimal_printk(const char *fmt, ...)
{
  /* No-op for unit tests */
}

/* Prevent adcAcquisitionUtil.h inclusion and provide needed macros */
#define ADC_ACQUISITION_UTIL
#define ADC_AQC_SERVICE_NAME adcAcquisition

/* Include filter header */
#include "adcAcquisitionFilter.h"

/* Setup logging before including filter implementation */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adcAcquisition, LOG_LEVEL_DBG);

/* Redefine LOG_MODULE_DECLARE to prevent redefinition when filter includes logging.h */
#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

#include "adcAcquisitionFilter.c"

/* Fixture for tests that need an initialized filter */
struct adc_filter_with_init_tests_fixture
{
  int32_t filter_memory[64]; /* Static memory for filter buffer */
};

static struct adc_filter_with_init_tests_fixture testFixture;

static void *filter_with_init_setup(void)
{
  int err;

  /* Setup k_malloc to return our fixture memory */
  k_malloc_fake.return_val = testFixture.filter_memory;

  err = adcAcqFilterInit(4);
  zassume_equal(err, 0, "Filter init should succeed in fixture setup");

  return &testFixture;
}

static void filter_with_init_before(void *f)
{
  ARG_UNUSED(f);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
  /* Zero the filter buffer between tests */
  memset(testFixture.filter_memory, 0, sizeof(testFixture.filter_memory));
}

static void filter_with_init_after(void *f)
{
  ARG_UNUSED(f);
}

static void filter_with_init_teardown(void *f)
{
  ARG_UNUSED(f);
  /* Reset module state */
  filterBuf = NULL;
  filterCount = 0;
}

/* Fixture for init tests (no pre-initialization) */
static void *filter_no_init_setup(void)
{
  return NULL;
}

static void filter_no_init_before(void *f)
{
  ARG_UNUSED(f);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
  /* Reset module state */
  filterBuf = NULL;
  filterCount = 0;
}

static void filter_no_init_after(void *f)
{
  ARG_UNUSED(f);
}

static void filter_no_init_teardown(void *f)
{
  ARG_UNUSED(f);
  /* Reset module state */
  filterBuf = NULL;
  filterCount = 0;
}

/* adcAcqFilterInit tests - error cases first */

/**
 * Requirement: The adcAcqFilterInit function must return -ENOSPC when k_malloc fails.
 */
ZTEST(adc_filter_init_tests, test_filter_init_malloc_fails)
{
  int err;

  k_malloc_fake.return_val = NULL;

  err = adcAcqFilterInit(4);

  zassert_equal(k_malloc_fake.call_count, 1, "k_malloc should be called once");
  zassert_equal(err, -ENOSPC, "Filter init should return -ENOSPC when malloc fails");
}

/**
 * Requirement: The adcAcqFilterInit function must return 0 when initialized with 0 channels.
 */
ZTEST(adc_filter_init_tests, test_filter_init_zero_channels)
{
  static int32_t test_memory[4];
  int err;

  k_malloc_fake.return_val = test_memory;

  err = adcAcqFilterInit(0);

  zassert_equal(k_malloc_fake.call_count, 1, "k_malloc should still be called");
  zassert_equal(err, 0, "Filter init with 0 channels should succeed");
  zassert_equal(filterCount, 0, "Filter count should be 0");
}

/**
 * Requirement: The adcAcqFilterInit function must call k_malloc with the correct size
 * for the filter buffer based on channel count.
 */
ZTEST(adc_filter_init_tests, test_filter_init_correct_malloc_size)
{
  static int32_t test_memory[64];
  int err;
  size_t expected_size;

  k_malloc_fake.return_val = test_memory;

  err = adcAcqFilterInit(4);

  zassert_equal(err, 0, "Filter init should succeed");
  zassert_equal(k_malloc_fake.call_count, 1, "k_malloc should be called once");

  /* Filter buffer size = chanCount * (FILTER_MAX_ORDER + 1) * sizeof(int32_t) */
  /* = 4 * 4 * 4 = 64 bytes */
  expected_size = 4 * 4 * sizeof(int32_t);
  zassert_equal(k_malloc_fake.arg0_val, expected_size,
                "k_malloc should be called with correct size");
}

/**
 * Requirement: The adcAcqFilterInit function must initialize filterBuf pointer
 * to the allocated memory.
 */
ZTEST(adc_filter_init_tests, test_filter_init_sets_filter_buf)
{
  static int32_t test_memory[64];
  int err;

  k_malloc_fake.return_val = test_memory;

  err = adcAcqFilterInit(4);

  zassert_equal(err, 0, "Filter init should succeed");
  zassert_equal_ptr(filterBuf, test_memory, "filterBuf should point to allocated memory");
}

/**
 * Requirement: The adcAcqFilterInit function must set filterCount to the channel count.
 */
ZTEST(adc_filter_init_tests, test_filter_init_sets_filter_count)
{
  static int32_t test_memory[64];
  int err;

  k_malloc_fake.return_val = test_memory;

  err = adcAcqFilterInit(4);

  zassert_equal(err, 0, "Filter init should succeed");
  zassert_equal(filterCount, 4, "filterCount should be set to 4");
}

/**
 * Requirement: The adcAcqFilterInit function must zero-initialize the filter buffer.
 */
ZTEST(adc_filter_init_tests, test_filter_init_zeros_buffer)
{
  static int32_t test_memory[64];
  int err;
  int i;

  /* Pre-fill with non-zero values */
  memset(test_memory, 0xFF, sizeof(test_memory));
  k_malloc_fake.return_val = test_memory;

  err = adcAcqFilterInit(4);

  zassert_equal(err, 0, "Filter init should succeed");

  /* Verify buffer is zeroed */
  for (i = 0; i < 16; i++)
  {
    zassert_equal(test_memory[i], 0, "Filter buffer should be zero-initialized");
  }
}

/* adcAcqFilterPushData tests - error cases first */

/**
 * Requirement: The adcAcqFilterPushData function must return -EINVAL
 * when the channel ID is greater than or equal to the initialized channel count.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_invalid_channel)
{
  int err;

  err = adcAcqFilterPushData(4, 1000, 31);
  zassert_equal(err, -EINVAL, "Push data to invalid channel should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterPushData function must clamp tau to minimum value (1)
 * when tau is less than 1.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_tau_too_small)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 0);
  zassert_equal(err, 0, "Push data with tau=0 should succeed (tau clamped to 1)");
}

/**
 * Requirement: The adcAcqFilterPushData function must clamp tau to maximum value (511)
 * when tau is greater than 511.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_tau_too_large)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 512);
  zassert_equal(err, 0, "Push data with tau=512 should succeed (tau clamped to 511)");
}

/**
 * Requirement: The adcAcqFilterPushData function must return 0 when pushing data
 * to a valid channel with valid tau.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_valid)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data to channel 0 should succeed");
}

/**
 * Requirement: The adcAcqFilterPushData function must accept tau=1 (minimum valid value).
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_tau_minimum)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 1);
  zassert_equal(err, 0, "Push data with tau=1 should succeed");
}

/**
 * Requirement: The adcAcqFilterPushData function must accept tau=511 (maximum valid value).
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_tau_maximum)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 511);
  zassert_equal(err, 0, "Push data with tau=511 should succeed");
}

/**
 * Requirement: The adcAcqFilterPushData function must store the raw data at index 0
 * of the channel's filter buffer.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_stores_raw_value)
{
  struct adc_filter_with_init_tests_fixture *f;
  int err;

  f = (struct adc_filter_with_init_tests_fixture *)fixture;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  /* Channel 0 base index is 0, raw data is at index 0 (prescaled by 9 bits) */
  zassert_equal(f->filter_memory[0], 1000 << 9, "Raw data should be stored prescaled at correct index");
}

/**
 * Requirement: The adcAcqFilterPushData function must independently handle data
 * for each channel.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_push_data_independent_channels)
{
  int err;
  int32_t filtData0;
  int32_t filtData1;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data to channel 0 should succeed");

  err = adcAcqFilterPushData(1, 2000, 31);
  zassert_equal(err, 0, "Push data to channel 1 should succeed");

  err = adcAcqFilterGetRawData(0, &filtData0);
  zassert_equal(err, 0, "Get raw data from channel 0 should succeed");

  err = adcAcqFilterGetRawData(1, &filtData1);
  zassert_equal(err, 0, "Get raw data from channel 1 should succeed");

  zassert_equal(filtData0, 1000, "Channel 0 data should be independent");
  zassert_equal(filtData1, 2000, "Channel 1 data should be independent");
}

/* adcAcqFilterGetRawData tests - error cases first */

/**
 * Requirement: The adcAcqFilterGetRawData function must return -EINVAL
 * when the channel ID is greater than or equal to the initialized channel count.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_raw_data_invalid_channel)
{
  int err;
  int32_t filtData;

  err = adcAcqFilterGetRawData(4, &filtData);
  zassert_equal(err, -EINVAL, "Get raw data from invalid channel should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetRawData function must return -EINVAL
 * when passed a NULL output pointer.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_raw_data_null_pointer)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetRawData(0, NULL);
  zassert_equal(err, -EINVAL, "Get raw data with NULL pointer should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetRawData function must return the most recently
 * pushed raw data value.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_raw_data_returns_pushed_value)
{
  int err;
  int32_t filtData;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetRawData(0, &filtData);
  zassert_equal(err, 0, "Get raw data should succeed");
  zassert_equal(filtData, 1000, "Raw data should match pushed value");
}

/**
 * Requirement: The adcAcqFilterGetRawData function must return the unfiltered input value.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_raw_data_unfiltered)
{
  int err;
  int32_t rawData;
  const int32_t input = 10000;
  int i;

  for (i = 0; i < 5; i++)
  {
    err = adcAcqFilterPushData(0, input, 51);
    zassert_equal(err, 0, "Push data should succeed");
  }

  err = adcAcqFilterGetRawData(0, &rawData);
  zassert_equal(err, 0, "Get raw data should succeed");
  zassert_equal(rawData, input, "Raw data should equal input");
}

/* adcAcqFilterGetFirstOrderData tests - error cases first */

/**
 * Requirement: The adcAcqFilterGetFirstOrderData function must return -EINVAL
 * when the channel ID is invalid.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_first_order_invalid_channel)
{
  int err;
  int32_t filtData;

  err = adcAcqFilterGetFirstOrderData(4, &filtData);
  zassert_equal(err, -EINVAL, "Get first order from invalid channel should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetFirstOrderData function must return -EINVAL
 * when passed a NULL output pointer.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_first_order_null_pointer)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetFirstOrderData(0, NULL);
  zassert_equal(err, -EINVAL, "Get first order with NULL pointer should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetFirstOrderData function must return filtered data
 * that increases from initial state towards the input value.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_first_order_initial_convergence)
{
  int err;
  int32_t filtData;
  const int32_t tau = 51; /* α ≈ 0.1 */
  const int32_t input = 10000;

  err = adcAcqFilterPushData(0, input, tau);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetFirstOrderData(0, &filtData);
  zassert_equal(err, 0, "Get first order data should succeed");

  /* With α ≈ 0.1 and initial state of 0: y[0] = 0 + 0.1 * (10000 - 0) ≈ 1000 */
  zassert_within(filtData, 1000, 100, "Filter output should be approximately 1000");
}

/**
 * Requirement: The adcAcqFilterGetFirstOrderData function must maintain filter state
 * across multiple calls and continue exponential convergence.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_first_order_state_maintained)
{
  int err;
  int32_t filtData;
  const int32_t tau = 51; /* α ≈ 0.1 */
  const int32_t input = 10000;

  /* First sample */
  err = adcAcqFilterPushData(0, input, tau);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetFirstOrderData(0, &filtData);
  zassert_equal(err, 0, "Get first order data should succeed");

  /* Second sample */
  err = adcAcqFilterPushData(0, input, tau);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetFirstOrderData(0, &filtData);
  zassert_equal(err, 0, "Get first order data should succeed");

  /* y[1] = 1000 + 0.1 * (10000 - 1000) ≈ 1900 */
  zassert_within(filtData, 1900, 100, "Filter should continue converging");
}

/* adcAcqFilterGetSecondOrderData tests - error cases first */

/**
 * Requirement: The adcAcqFilterGetSecondOrderData function must return -EINVAL
 * when the channel ID is invalid.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_second_order_invalid_channel)
{
  int err;
  int32_t filtData;

  err = adcAcqFilterGetSecondOrderData(4, &filtData);
  zassert_equal(err, -EINVAL, "Get second order from invalid channel should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetSecondOrderData function must return -EINVAL
 * when passed a NULL output pointer.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_second_order_null_pointer)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetSecondOrderData(0, NULL);
  zassert_equal(err, -EINVAL, "Get second order with NULL pointer should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetSecondOrderData function must provide stronger filtering
 * than first-order, resulting in output values that lag further behind the input.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_second_order_stronger_filtering)
{
  int err;
  int32_t firstOrder;
  int32_t secondOrder;
  const int32_t input = 10000;
  int i;

  for (i = 0; i < 5; i++)
  {
    err = adcAcqFilterPushData(0, input, 51);
    zassert_equal(err, 0, "Push data should succeed");
  }

  err = adcAcqFilterGetFirstOrderData(0, &firstOrder);
  zassert_equal(err, 0, "Get first order data should succeed");

  err = adcAcqFilterGetSecondOrderData(0, &secondOrder);
  zassert_equal(err, 0, "Get second order data should succeed");

  zassert_true(secondOrder < firstOrder, "Second order should be more filtered than first order");
  zassert_true(firstOrder < input, "First order should be less than raw input");
  zassert_true(secondOrder < input, "Second order should be less than raw input");
}

/* adcAcqFilterGetThirdOrderData tests - error cases first */

/**
 * Requirement: The adcAcqFilterGetThirdOrderData function must return -EINVAL
 * when the channel ID is invalid.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_third_order_invalid_channel)
{
  int err;
  int32_t filtData;

  err = adcAcqFilterGetThirdOrderData(4, &filtData);
  zassert_equal(err, -EINVAL, "Get third order from invalid channel should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetThirdOrderData function must return -EINVAL
 * when passed a NULL output pointer.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_get_third_order_null_pointer)
{
  int err;

  err = adcAcqFilterPushData(0, 1000, 31);
  zassert_equal(err, 0, "Push data should succeed");

  err = adcAcqFilterGetThirdOrderData(0, NULL);
  zassert_equal(err, -EINVAL, "Get third order with NULL pointer should return -EINVAL");
}

/**
 * Requirement: The adcAcqFilterGetThirdOrderData function must provide the strongest filtering,
 * with output values lagging further behind than both first-order and second-order filters.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_third_order_strongest_filtering)
{
  int err;
  int32_t firstOrder;
  int32_t secondOrder;
  int32_t thirdOrder;
  const int32_t input = 10000;
  int i;

  for (i = 0; i < 10; i++)
  {
    err = adcAcqFilterPushData(0, input, 51);
    zassert_equal(err, 0, "Push data should succeed");
  }

  err = adcAcqFilterGetFirstOrderData(0, &firstOrder);
  zassert_equal(err, 0, "Get first order data should succeed");

  err = adcAcqFilterGetSecondOrderData(0, &secondOrder);
  zassert_equal(err, 0, "Get second order data should succeed");

  err = adcAcqFilterGetThirdOrderData(0, &thirdOrder);
  zassert_equal(err, 0, "Get third order data should succeed");

  zassert_true(thirdOrder < secondOrder, "Third order should be more filtered than second");
  zassert_true(secondOrder < firstOrder, "Second order should be more filtered than first");
  zassert_true(thirdOrder > 0, "Third order should be positive");
  zassert_true(thirdOrder < input, "Third order should be less than input");
}

/* Integration tests - filter behavior */

/**
 * Requirement: The filter must produce monotonically increasing output for a positive step input,
 * never overshooting the target value.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_monotonic_convergence)
{
  int err;
  int32_t filtData;
  int32_t previousData;
  const int32_t input = 10000;
  int i;

  previousData = 0;

  for (i = 0; i < 50; i++)
  {
    err = adcAcqFilterPushData(0, input, 51);
    zassert_equal(err, 0, "Push data should succeed");

    err = adcAcqFilterGetFirstOrderData(0, &filtData);
    zassert_equal(err, 0, "Get first order data should succeed");

    zassert_true(filtData >= previousData, "Output should increase monotonically");
    zassert_true(filtData <= input, "Output should not exceed input");

    previousData = filtData;
  }
}

/**
 * Requirement: The filter must settle to within a small tolerance of the input value
 * after sufficient settling time.
 */
ZTEST_F(adc_filter_with_init_tests, test_filter_settling_time)
{
  int err;
  int32_t filtData;
  const int32_t input = 10000;
  int i;

  for (i = 0; i < 50; i++)
  {
    err = adcAcqFilterPushData(0, input, 51);
    zassert_equal(err, 0, "Push data should succeed");

    err = adcAcqFilterGetFirstOrderData(0, &filtData);
    zassert_equal(err, 0, "Get first order data should succeed");
  }

  zassert_within(filtData, input, 100, "Filter should settle close to input");
}

/* Test suite definitions */
ZTEST_SUITE(adc_filter_init_tests, NULL, filter_no_init_setup, filter_no_init_before,
             filter_no_init_after, filter_no_init_teardown);

ZTEST_SUITE(adc_filter_with_init_tests, NULL, filter_with_init_setup, filter_with_init_before,
             filter_with_init_after, filter_with_init_teardown);
