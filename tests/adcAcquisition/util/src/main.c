/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-01-10
 * @brief     ADC Acquisition Util Tests
 *
 *            Unit tests for ADC acquisition utility functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Forward declare needed types without including driver headers */
struct adc_dt_spec;
struct adc_sequence;
struct counter_top_cfg;

/* Mock osMemoryPool type */
typedef void *osMemoryPoolId_t;

/* Wrap device_is_ready since it's defined by Zephyr */
#define device_is_ready device_is_ready_mock

/* Prevent CMSIS OS2 header */
#define CMSIS_OS2_H_

/* Prevent filter header */
#define ADC_ACQUISITION_FILTER

/* Prevent ADC acquisition main header - we'll define types manually */
#define ADC_ACQUISITION

/* Prevent ADC and counter driver headers - they conflict with mocks */
#define ZEPHYR_INCLUDE_DRIVERS_ADC_H_
#define ZEPHYR_INCLUDE_DRIVERS_COUNTER_H_
#define ZEPHYR_DEVICE_H_

#define FFF_FAKES_LIST(FAKE) \
  FAKE(adc_is_ready_dt) \
  FAKE(adc_channel_setup_dt) \
  FAKE(adc_read_async) \
  FAKE(adc_raw_to_millivolts) \
  FAKE(counter_us_to_ticks) \
  FAKE(counter_set_top_value) \
  FAKE(counter_start) \
  FAKE(counter_stop) \
  FAKE(device_is_ready_mock) \
  FAKE(k_malloc) \
  FAKE(adcAcqFilterPushData) \
  FAKE(adcAcqFilterGetThirdOrderData) \
  FAKE(osMemoryPoolNew) \
  FAKE(osMemoryPoolAlloc) \
  FAKE(osMemoryPoolFree) \
  FAKE(mock_subscription_callback)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adcAcquisition, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Redefine LOG_ERR to avoid dereferencing invalid pointers in error messages */
#undef LOG_ERR
#define LOG_ERR(...) do {} while (0)

/* Mock Kconfig options */
#define CONFIG_ENYA_ADC_VREF_STM32_CCR 1
#define CONFIG_ENYA_ADC_VREF_STABILIZATION_US 15

/* Mock STM32 registers */
typedef struct
{
  uint32_t CCR;
} ADC_Common_TypeDef;

static ADC_Common_TypeDef mock_adc1_common __attribute__((unused)) = {0};
#define ADC1_COMMON (&mock_adc1_common)
#define ADC_CCR_VREFEN (1 << 22)

/* Flag to simulate VREFEN bit not being set (hardware failure) */
static bool mock_vrefen_fails = false;
#define READ_ADC1_COMMON_CCR() (mock_vrefen_fails ? 0 : mock_adc1_common.CCR)

/* Mock VREFINT_CAL_ADDR */
static uint16_t mock_vrefint_cal __attribute__((unused)) = 1500;
#define VREFINT_CAL_ADDR (&mock_vrefint_cal)

/* Mock soc.h */
#define _SOC__H_

/* ADC gain enum - minimal definition matching Zephyr's enum */
enum adc_gain {
  ADC_GAIN_1 = 0,
};

/* ADC channel config - minimal definition */
struct adc_channel_cfg {
  enum adc_gain gain;
};

/* Mock ADC devicetree macro */
#define ADC_DT_SPEC_GET_BY_IDX(node, idx) \
  {.dev = (const struct device *)0x1000, .channel_id = idx, \
   .channel_cfg = {.gain = ADC_GAIN_1}, .resolution = 12, .oversampling = 0}

/* Mock timer device and devicetree macros for adc-trigger alias */
static const struct device mock_timer_device __attribute__((unused)) = {0};
#undef DEVICE_DT_GET
#define DEVICE_DT_GET(node) (&mock_timer_device)
#undef DT_ALIAS
#define DT_ALIAS(alias) alias

/* Define types from adcAcquisition.h before including source */
typedef struct
{
  uint32_t samplingRate;
  int32_t filterTau;
} AdcConfig_t;

typedef struct
{
  size_t activeSubCount;
  size_t maxSubCount;
} AdcSubConfig_t;

typedef struct SrvMsgPayload SrvMsgPayload_t;
typedef int (*AdcSubCallback_t)(SrvMsgPayload_t *data);

struct SrvMsgPayload {
  osMemoryPoolId_t poolId;                          /**< Memory pool to return buffer to. */
  size_t dataLen;                                   /**< Actual data length in bytes. */
  uint8_t data[];                                   /**< Flexible array of data bytes. */
};

/* Provide minimal type definitions that adcAcquisitionUtil.c needs */
enum adc_action {
  ADC_ACTION_CONTINUE = 0,
  ADC_ACTION_FINISH = 1,
};

struct adc_sequence_options {
  uint16_t extra_samplings;
  uint16_t interval_us;
  enum adc_action (*callback)(const struct device *dev, const struct adc_sequence *sequence, uint16_t sampling_index);
  void *user_data;
};

struct adc_sequence {
  struct adc_sequence_options *options;
  uint32_t channels;
  void *buffer;
  size_t buffer_size;
  uint8_t resolution;
  uint8_t oversampling;
  bool calibrate;
};

struct adc_dt_spec {
  const struct device *dev;
  uint8_t channel_id;
  struct adc_channel_cfg channel_cfg;
  uint8_t resolution;
  uint8_t oversampling;
};

struct counter_top_cfg {
  uint32_t flags;
  uint32_t ticks;
  void (*callback)(const struct device *dev, void *user_data);
  void *user_data;
};

/* Mock ADC functions */
FAKE_VALUE_FUNC(bool, adc_is_ready_dt, const struct adc_dt_spec *);
FAKE_VALUE_FUNC(int, adc_channel_setup_dt, const struct adc_dt_spec *);
FAKE_VALUE_FUNC(int, adc_read_async, const struct device *, const struct adc_sequence *, struct k_poll_signal *);
FAKE_VALUE_FUNC(int, adc_raw_to_millivolts, int32_t, enum adc_gain, uint8_t, int32_t *);

/* Mock counter/timer functions */
FAKE_VALUE_FUNC(uint32_t, counter_us_to_ticks, const struct device *, uint64_t);
FAKE_VALUE_FUNC(int, counter_set_top_value, const struct device *, const struct counter_top_cfg *);
FAKE_VALUE_FUNC(int, counter_start, const struct device *);
FAKE_VALUE_FUNC(int, counter_stop, const struct device *);

/* Mock device functions */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);

/* Mock memory functions */
FAKE_VALUE_FUNC(void *, k_malloc, size_t);

/* Mock filter functions */
FAKE_VALUE_FUNC(int, adcAcqFilterPushData, size_t, int32_t, int32_t);
FAKE_VALUE_FUNC(int, adcAcqFilterGetThirdOrderData, size_t, int32_t *);

/* Mock osMemoryPool functions */
FAKE_VALUE_FUNC(osMemoryPoolId_t, osMemoryPoolNew, uint32_t, uint32_t, void *);
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(int, osMemoryPoolFree, osMemoryPoolId_t, void *);

/* Mock subscription callback */
FAKE_VALUE_FUNC(int, mock_subscription_callback, SrvMsgPayload_t *);

/* Override VREF readback for unit testing (mock_vrefen_fails simulation) */
#define STM32_ADC_VREF_REG_READ READ_ADC1_COMMON_CCR()

/* Include utility implementation */
#include "adcAcquisitionUtil.c"

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
  extern size_t chanCount;
  extern size_t voltBufSize;
  extern volatile bool adcBusy;
  extern uint16_t *buffer;
  extern float *voltValues;
  extern AdcConfig_t config;

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  /* Reset mock ADC register */
  mock_adc1_common.CCR = 0;
  mock_vrefen_fails = false;

  /* Reset chanCount and voltBufSize */
  chanCount = 0;
  voltBufSize = 0;

  /* Reset adcBusy flag */
  adcBusy = false;

  /* Reset buffer and voltValues pointers */
  buffer = NULL;
  voltValues = NULL;

  /* Reset config structure */
  memset(&config, 0, sizeof(config));
}

/* ===========================================================================
 * configureChannels
 * =========================================================================*/

/**
 * @test The configureChannels function must return -EBUSY when
 * ADC device is not ready.
 */
ZTEST(adc_util_tests, test_configure_channels_adc_not_ready)
{
  extern int configureChannels(void);
  int result;

  /* Configure mock to return false (ADC not ready) */
  adc_is_ready_dt_fake.return_val = false;

  /* Call configureChannels - should fail */
  result = configureChannels();

  zassert_equal(result, -EBUSY,
                "configureChannels should return -EBUSY when ADC is not ready");
  zassert_true(adc_is_ready_dt_fake.call_count >= 1,
                "adc_is_ready_dt should be called at least once");
}

/**
 * @test The configureChannels function must return an error when
 * adc_channel_setup_dt fails.
 */
ZTEST(adc_util_tests, test_configure_channels_setup_failure)
{
  extern int configureChannels(void);
  int result;

  /* Configure mocks - ADC is ready but setup fails */
  adc_is_ready_dt_fake.return_val = true;
  adc_channel_setup_dt_fake.return_val = -EINVAL;

  /* Call configureChannels - should fail */
  result = configureChannels();

  zassert_equal(result, -EINVAL,
                "configureChannels should return -EINVAL when channel setup fails");
  zassert_true(adc_is_ready_dt_fake.call_count >= 1,
                "adc_is_ready_dt should be called");
  zassert_true(adc_channel_setup_dt_fake.call_count >= 1,
                "adc_channel_setup_dt should be called");
}

/**
 * @test The configureChannels function must successfully configure
 * all ADC channels when all operations succeed.
 */
ZTEST(adc_util_tests, test_configure_channels_success)
{
  extern int configureChannels(void);
  extern struct adc_sequence sequence;
  extern const struct adc_dt_spec adcChannels[];
  int result;

  /* Configure mocks - ADC is ready and setup succeeds */
  adc_is_ready_dt_fake.return_val = true;
  adc_channel_setup_dt_fake.return_val = 0;

  /* Call configureChannels - should succeed */
  result = configureChannels();

  zassert_equal(result, 0,
                "configureChannels should return 0 on success");
  zassert_equal(adc_is_ready_dt_fake.call_count, 2,
                "adc_is_ready_dt should be called exactly twice for 2 channels");
  zassert_equal(adc_is_ready_dt_fake.arg0_history[0], &adcChannels[0],
                "adc_is_ready_dt first call should be with adcChannels[0]");
  zassert_equal(adc_is_ready_dt_fake.arg0_history[1], &adcChannels[1],
                "adc_is_ready_dt second call should be with adcChannels[1]");
  zassert_equal(adc_channel_setup_dt_fake.call_count, 2,
                "adc_channel_setup_dt should be called exactly twice for 2 channels");
  zassert_equal(adc_channel_setup_dt_fake.arg0_history[0], &adcChannels[0],
                "adc_channel_setup_dt first call should be with adcChannels[0]");
  zassert_equal(adc_channel_setup_dt_fake.arg0_history[1], &adcChannels[1],
                "adc_channel_setup_dt second call should be with adcChannels[1]");
  zassert_equal(sequence.channels, (BIT(0) | BIT(1)),
                "sequence.channels should be set to 0x03 (BIT(0) | BIT(1))");
}

/* ===========================================================================
 * triggerConversion
 * =========================================================================*/

/**
 * @test The triggerConversion function must clear adcBusy flag when
 * adc_read_async fails.
 */
ZTEST(adc_util_tests, test_trigger_conversion_adc_read_failure)
{
  extern void triggerConversion(const struct device *dev, void *user_data);
  extern volatile bool adcBusy;

  /* Configure mock to return error from adc_read_async */
  adc_read_async_fake.return_val = -EIO;

  /* Ensure adcBusy starts as false */
  adcBusy = false;

  /* Call triggerConversion - should fail */
  triggerConversion((const struct device *)0x1000, NULL);

  /* Verify adc_read_async was called */
  zassert_equal(adc_read_async_fake.call_count, 1,
                "adc_read_async should be called once");

  /* Verify adcBusy is cleared on error */
  zassert_false(adcBusy,
                "adcBusy should be cleared when adc_read_async fails");
}

/**
 * @test The triggerConversion function must skip conversion when
 * ADC is busy.
 */
ZTEST(adc_util_tests, test_trigger_conversion_adc_busy)
{
  extern void triggerConversion(const struct device *dev, void *user_data);
  extern volatile bool adcBusy;

  /* Set adcBusy to true to simulate ADC already busy */
  adcBusy = true;

  /* Call triggerConversion - should return early without calling adc_read_async */
  triggerConversion((const struct device *)0x1000, NULL);

  /* Verify adc_read_async was not called */
  zassert_equal(adc_read_async_fake.call_count, 0,
                "adc_read_async should not be called when ADC is busy");

  /* Verify adcBusy is still true */
  zassert_true(adcBusy,
               "adcBusy should remain true after skipping conversion");
}

/**
 * @test The triggerConversion function must successfully start ADC
 * conversion when ADC is not busy.
 */
ZTEST(adc_util_tests, test_trigger_conversion_success)
{
  extern void triggerConversion(const struct device *dev, void *user_data);
  extern volatile bool adcBusy;
  extern const struct device *adc;
  extern struct adc_sequence sequence;

  /* Set up adc device pointer (matches ADC_DT_SPEC_GET_BY_IDX mock) */
  adc = (const struct device *)0x1000;

  /* Configure mock to return success from adc_read_async */
  adc_read_async_fake.return_val = 0;

  /* Ensure adcBusy starts as false */
  adcBusy = false;

  /* Call triggerConversion - should succeed */
  triggerConversion((const struct device *)0x1000, NULL);

  /* Verify adc_read_async was called once */
  zassert_equal(adc_read_async_fake.call_count, 1,
                "adc_read_async should be called once");
  zassert_equal(adc_read_async_fake.arg0_val, adc,
                "adc_read_async should be called with adc device");
  zassert_equal(adc_read_async_fake.arg1_val, &sequence,
                "adc_read_async should be called with sequence pointer");
  zassert_is_null(adc_read_async_fake.arg2_val,
                  "adc_read_async should be called with NULL signal");

  /* Verify adcBusy is set to true */
  zassert_true(adcBusy,
               "adcBusy should be true after successful conversion start");
}

/* ===========================================================================
 * configureTimer
 * =========================================================================*/

/**
 * @test The configureTimer function must return -EBUSY when
 * timer device is not ready.
 */
ZTEST(adc_util_tests, test_configure_timer_device_not_ready)
{
  extern int configureTimer(void);
  int result;

  /* Configure mock to return false (device not ready) */
  device_is_ready_mock_fake.return_val = false;

  /* Call configureTimer - should fail */
  result = configureTimer();

  zassert_equal(result, -EBUSY,
                "configureTimer should return -EBUSY when device is not ready");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once");
}

/**
 * @test The configureTimer function must successfully configure
 * the timer when device is ready.
 */
ZTEST(adc_util_tests, test_configure_timer_success)
{
  extern int configureTimer(void);
  extern struct counter_top_cfg triggerConfig;
  extern void triggerConversion(const struct device *dev, void *user_data);
  extern AdcConfig_t config;
  int result;
  const uint32_t expected_ticks = 1000;
  const uint32_t expected_sampling_rate = 500;

  /* Set config.samplingRate for counter_us_to_ticks call */
  config.samplingRate = expected_sampling_rate;

  /* Configure mocks - device is ready and counter_us_to_ticks returns expected value */
  device_is_ready_mock_fake.return_val = true;
  counter_us_to_ticks_fake.return_val = expected_ticks;

  /* Call configureTimer - should succeed */
  result = configureTimer();

  zassert_equal(result, 0,
                "configureTimer should return 0 on success");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once");
  zassert_equal(device_is_ready_mock_fake.arg0_val, &mock_timer_device,
                "device_is_ready should be called with trigger timer device");
  zassert_equal(counter_us_to_ticks_fake.call_count, 1,
                "counter_us_to_ticks should be called once");
  zassert_equal(counter_us_to_ticks_fake.arg0_val, &mock_timer_device,
                "counter_us_to_ticks should be called with trigger timer device");
  zassert_equal(counter_us_to_ticks_fake.arg1_val, expected_sampling_rate,
                "counter_us_to_ticks should be called with config.samplingRate");
  zassert_equal(triggerConfig.flags, 0,
                "triggerConfig.flags should be set to 0");
  zassert_equal(triggerConfig.ticks, expected_ticks,
                "triggerConfig.ticks should be set to value from counter_us_to_ticks");
  zassert_equal(triggerConfig.callback, triggerConversion,
                "triggerConfig.callback should be set to triggerConversion");
  zassert_equal(triggerConfig.user_data, NULL,
                "triggerConfig.user_data should be set to NULL");
}

/* ===========================================================================
 * calculateVdd
 * =========================================================================*/

/**
 * @test The calculateVdd function must correctly calculate VDD
 * when vrefVal equals the calibration value (VDD = 3000 mV).
 */
ZTEST(adc_util_tests, test_calculate_vdd_at_calibration_voltage)
{
  extern int32_t calculateVdd(int32_t vrefVal);
  int32_t vdd;

  /* VREFINT_CAL_VOLTAGE * vrefCal / vrefVal = 3000 * 1500 / 1500 = 3000 mV */
  vdd = calculateVdd(1500);

  zassert_equal(vdd, 3000,
                "VDD should be 3000 mV when vrefVal equals calibration value");
}

/**
 * @test The calculateVdd function must correctly calculate VDD
 * when vrefVal is lower than calibration (indicating higher VDD).
 */
ZTEST(adc_util_tests, test_calculate_vdd_higher_voltage)
{
  extern int32_t calculateVdd(int32_t vrefVal);
  int32_t vdd;

  /* 3000 * 1500 / 1364 = 3299 mV */
  vdd = calculateVdd(1364);

  zassert_equal(vdd, 3299,
                "VDD should be 3299 mV when vrefVal is 1364");
}

/**
 * @test The calculateVdd function must correctly calculate VDD
 * when vrefVal is higher than calibration (indicating lower VDD).
 */
ZTEST(adc_util_tests, test_calculate_vdd_lower_voltage)
{
  extern int32_t calculateVdd(int32_t vrefVal);
  int32_t vdd;

  /* 3000 * 1500 / 1667 = 2699 mV */
  vdd = calculateVdd(1667);

  zassert_equal(vdd, 2699,
                "VDD should be 2699 mV when vrefVal is 1667");
}

/* ===========================================================================
 * adcSeqCallback
 * =========================================================================*/

/**
 * @test The adcSeqCallback function must assert when adc_raw_to_millivolts
 * returns an error (gain not reversible).
 */
ZTEST(adc_util_tests, test_adc_seq_callback_millivolts_conversion_failure)
{
  extern enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex);
  extern size_t chanCount;
  extern uint16_t *buffer;
  uint16_t test_buffer[2];

  chanCount = 2;
  test_buffer[0] = 1234;
  test_buffer[1] = 5678;
  buffer = test_buffer;

  /* Configure adc_raw_to_millivolts to return error (gain not reversible) */
  adc_raw_to_millivolts_fake.return_val = -EINVAL;

  /* Tell ztest to expect the __ASSERT to fire */
  ztest_set_assert_valid(true);
  adcSeqCallback((const struct device *)0x1000, NULL, 0);

  zassert_equal(adc_raw_to_millivolts_fake.call_count, 1,
                "adc_raw_to_millivolts should be called once before assert fires");

  /* Clean up */
  buffer = NULL;
}

/**
 * @test The adcSeqCallback function must clear adcBusy flag and
 * return ADC_ACTION_FINISH even when filter push fails.
 */
ZTEST(adc_util_tests, test_adc_seq_callback_filter_push_failure)
{
  extern enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex);
  extern volatile bool adcBusy;
  extern size_t chanCount;
  extern uint16_t *buffer;
  uint16_t test_buffer[2];
  enum adc_action result;

  /* Set up test state */
  chanCount = 2;
  adcBusy = true;

  /* Initialize buffer with test data (buffer[0] is VREF_CHANNEL_INDEX) */
  test_buffer[0] = 1234;
  test_buffer[1] = 5678;
  buffer = test_buffer;

  /* adc_raw_to_millivolts succeeds, filter push fails */
  adc_raw_to_millivolts_fake.return_val = 0;
  adcAcqFilterPushData_fake.return_val = -EIO;

  /* Call adcSeqCallback */
  result = adcSeqCallback((const struct device *)0x1000, NULL, 0);

  /* Verify filter push was called for each channel plus Vdd */
  zassert_equal(adcAcqFilterPushData_fake.call_count, 3,
                "adcAcqFilterPushData should be called 3 times (chanCount + Vdd)");

  /* Verify adcBusy is cleared even on error */
  zassert_false(adcBusy,
                "adcBusy should be cleared even when filter push fails");

  /* Verify function returns ADC_ACTION_FINISH */
  zassert_equal(result, ADC_ACTION_FINISH,
                "adcSeqCallback should return ADC_ACTION_FINISH");

  /* Clean up */
  buffer = NULL;
}

/**
 * @test The adcSeqCallback function must convert samples to mV, push
 * all channels and Vdd to the filter, and clear adcBusy.
 */
ZTEST(adc_util_tests, test_adc_seq_callback_success)
{
  extern enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex);
  extern volatile bool adcBusy;
  extern size_t chanCount;
  extern uint16_t *buffer;
  extern AdcConfig_t config;
  uint16_t test_buffer[2];
  enum adc_action result;
  /* ref = calculateVdd(buffer[VREF_CHANNEL_INDEX=0]) = 3000 * 1500 / 1234 = 3646 mV */
  const int32_t expected_ref = 3646;

  /* Set up test state */
  chanCount = 2;
  adcBusy = true;
  config.filterTau = 100;

  /* buffer[0] is VREF_CHANNEL_INDEX */
  test_buffer[0] = 1234;
  test_buffer[1] = 5678;
  buffer = test_buffer;

  /* Configure mocks to succeed */
  adc_raw_to_millivolts_fake.return_val = 0;
  adcAcqFilterPushData_fake.return_val = 0;

  /* Call adcSeqCallback */
  result = adcSeqCallback((const struct device *)0x1000, NULL, 0);

  /* Verify adc_raw_to_millivolts was called for each channel */
  zassert_equal(adc_raw_to_millivolts_fake.call_count, 2,
                "adc_raw_to_millivolts should be called twice for 2 channels");
  zassert_equal(adc_raw_to_millivolts_fake.arg0_history[0], expected_ref,
                "adc_raw_to_millivolts first call should use calculated ref");
  zassert_equal(adc_raw_to_millivolts_fake.arg1_history[0], ADC_GAIN_1,
                "adc_raw_to_millivolts should use channel gain");
  zassert_equal(adc_raw_to_millivolts_fake.arg2_history[0], 12,
                "adc_raw_to_millivolts should use channel resolution");
  zassert_equal(adc_raw_to_millivolts_fake.arg0_history[1], expected_ref,
                "adc_raw_to_millivolts second call should use same ref");

  /* Verify filter push was called for each channel plus Vdd */
  zassert_equal(adcAcqFilterPushData_fake.call_count, 3,
                "adcAcqFilterPushData should be called 3 times (chanCount + Vdd)");
  zassert_equal(adcAcqFilterPushData_fake.arg0_history[0], 0,
                "First push should be for channel 0");
  zassert_equal(adcAcqFilterPushData_fake.arg1_history[0], 1234,
                "First push value should be buffer[0] (mock does not convert)");
  zassert_equal(adcAcqFilterPushData_fake.arg2_history[0], 100,
                "First push should use config.filterTau");
  zassert_equal(adcAcqFilterPushData_fake.arg0_history[1], 1,
                "Second push should be for channel 1");
  zassert_equal(adcAcqFilterPushData_fake.arg1_history[1], 5678,
                "Second push value should be buffer[1] (mock does not convert)");
  zassert_equal(adcAcqFilterPushData_fake.arg2_history[1], 100,
                "Second push should use config.filterTau");
  zassert_equal(adcAcqFilterPushData_fake.arg0_history[2], 2,
                "Third push should be for Vdd slot (chanCount)");
  zassert_equal(adcAcqFilterPushData_fake.arg1_history[2], expected_ref,
                "Third push value should be the calculated Vdd");
  zassert_equal(adcAcqFilterPushData_fake.arg2_history[2], 100,
                "Third push should use config.filterTau");

  /* Verify adcBusy is cleared */
  zassert_false(adcBusy,
                "adcBusy should be cleared after successful conversion");

  /* Verify function returns ADC_ACTION_FINISH */
  zassert_equal(result, ADC_ACTION_FINISH,
                "adcSeqCallback should return ADC_ACTION_FINISH");

  /* Clean up */
  buffer = NULL;
}

/* ===========================================================================
 * setupSequence
 * =========================================================================*/

/**
 * @test The setupSequence function must correctly initialize the
 * ADC sequence and sequence options structures.
 */
ZTEST(adc_util_tests, test_setup_sequence)
{
  extern void setupSequence(void);
  extern struct adc_sequence sequence;
  extern struct adc_sequence_options seqOptions;
  extern enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex);
  extern size_t chanCount;
  extern uint16_t *buffer;
  uint16_t test_buffer[2];

  /* Set up test state */
  chanCount = 2;
  test_buffer[0] = 0;
  test_buffer[1] = 0;
  buffer = test_buffer;

  /* Call setupSequence */
  setupSequence();

  /* Verify sequence structure is initialized correctly */
  zassert_equal(sequence.oversampling, 0,
                "sequence.oversampling should be 0 (read from DTS, no oversampling)");
  zassert_equal(sequence.resolution, OVERSAMPLING_RESOLUTION,
                "sequence.resolution should be set to OVERSAMPLING_RESOLUTION");
  zassert_false(sequence.calibrate,
                "sequence.calibrate should be set to false");
  zassert_equal(sequence.options, &seqOptions,
                "sequence.options should point to seqOptions");
  zassert_equal(sequence.buffer, buffer,
                "sequence.buffer should point to buffer");
  zassert_equal(sequence.buffer_size, chanCount * sizeof(uint16_t),
                "sequence.buffer_size should be chanCount * sizeof(uint16_t)");

  /* Verify seqOptions structure is initialized correctly */
  zassert_equal(seqOptions.extra_samplings, EXTRA_SAMPLINGS_SETTING,
                "seqOptions.extra_samplings should be set to EXTRA_SAMPLINGS_SETTING");
  zassert_equal(seqOptions.interval_us, CHANNEL_INTERVAL,
                "seqOptions.interval_us should be set to CHANNEL_INTERVAL");
  zassert_equal(seqOptions.callback, adcSeqCallback,
                "seqOptions.callback should be set to adcSeqCallback");

  /* Clean up */
  buffer = NULL;
}

/* ===========================================================================
 * adcAcqUtilInitAdc
 * =========================================================================*/

/**
 * @test The adcAcqUtilInitAdc function must return -ENOSPC when
 * buffer allocation fails.
 */
ZTEST(adc_util_tests, test_init_adc_buffer_allocation_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call adcAcqUtilInitAdc - should fail due to buffer allocation failure */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, -ENOSPC,
                "adcAcqUtilInitAdc should return -ENOSPC when buffer allocation fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once before failing");
}

/**
 * @test The adcAcqUtilInitAdc function must return -ENOSPC when
 * volt values allocation fails.
 */
ZTEST(adc_util_tests, test_init_adc_volt_values_allocation_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  int result;

  /* Configure k_malloc to succeed for buffer, fail for voltValues */
  void *malloc_returns[] = {fake_buffer, NULL};
  SET_RETURN_SEQ(k_malloc, malloc_returns, 2);

  /* Call adcAcqUtilInitAdc - should fail due to volt values allocation failure */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, -ENOSPC,
                "adcAcqUtilInitAdc should return -ENOSPC when volt values allocation fails");
  zassert_equal(k_malloc_fake.call_count, 2,
                "k_malloc should be called twice before failing");
}

/**
 * @test The adcAcqUtilInitAdc function must return -EBUSY when
 * channel configuration fails due to ADC not ready.
 */
ZTEST(adc_util_tests, test_init_adc_configure_channels_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[3];
  int result;

  /* Configure k_malloc to succeed (return valid pointers) */
  void *malloc_returns[] = {fake_buffer, fake_volt_values};
  SET_RETURN_SEQ(k_malloc, malloc_returns, 2);

  /* Configure adc_is_ready_dt to return false (ADC not ready) */
  adc_is_ready_dt_fake.return_val = false;

  /* Call adcAcqUtilInitAdc - should fail due to channel configuration failure */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, -EBUSY,
                "adcAcqUtilInitAdc should return -EBUSY when ADC is not ready");
  zassert_equal(k_malloc_fake.call_count, 2,
                "k_malloc should be called twice for buffer and voltValues");
  zassert_equal(adc_is_ready_dt_fake.call_count, 1,
                "adc_is_ready_dt should be called exactly once before failing");
}

/**
 * @test The adcAcqUtilInitAdc function must return -EBUSY when
 * timer configuration fails due to timer device not ready.
 */
ZTEST(adc_util_tests, test_init_adc_configure_timer_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[3];
  int result;

  /* Configure k_malloc to succeed (return valid pointers) */
  void *malloc_returns[] = {fake_buffer, fake_volt_values};
  SET_RETURN_SEQ(k_malloc, malloc_returns, 2);

  /* Configure ADC channel setup to succeed */
  adc_is_ready_dt_fake.return_val = true;
  adc_channel_setup_dt_fake.return_val = 0;

  /* Configure timer device_is_ready to return false (timer not ready) */
  device_is_ready_mock_fake.return_val = false;

  /* Call adcAcqUtilInitAdc - should fail due to timer configuration failure */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, -EBUSY,
                "adcAcqUtilInitAdc should return -EBUSY when timer is not ready");
  zassert_equal(k_malloc_fake.call_count, 2,
                "k_malloc should be called twice for buffer and voltValues");
  zassert_equal(adc_is_ready_dt_fake.call_count, 2,
                "adc_is_ready_dt should be called twice for 2 channels");
  zassert_equal(adc_channel_setup_dt_fake.call_count, 2,
                "adc_channel_setup_dt should be called twice for 2 channels");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once for timer");
}

/**
 * @test The adcAcqUtilInitAdc function must return -EIO when
 * enabling VREFINT fails.
 */
ZTEST(adc_util_tests, test_init_adc_enable_vrefint_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[3];
  int result;

  /* Configure k_malloc to succeed (return valid pointers) */
  void *malloc_returns[] = {fake_buffer, fake_volt_values};
  SET_RETURN_SEQ(k_malloc, malloc_returns, 2);

  /* Configure ADC channel setup to succeed */
  adc_is_ready_dt_fake.return_val = true;
  adc_channel_setup_dt_fake.return_val = 0;

  /* Configure timer to be ready */
  device_is_ready_mock_fake.return_val = true;

  /* Simulate VREFEN bit not being set (hardware failure) */
  mock_vrefen_fails = true;

  /* Call adcAcqUtilInitAdc - should fail due to VREFINT enable failure */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, -EIO,
                "adcAcqUtilInitAdc should return -EIO when VREFINT enable fails");
}

/**
 * @test The adcAcqUtilInitAdc function must successfully initialize
 * the ADC when all operations succeed.
 */
ZTEST(adc_util_tests, test_init_adc_success)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[3];
  int result;

  /* Configure k_malloc to succeed (return valid pointers) */
  void *malloc_returns[] = {fake_buffer, fake_volt_values};
  SET_RETURN_SEQ(k_malloc, malloc_returns, 2);

  /* Configure ADC channel setup to succeed */
  adc_is_ready_dt_fake.return_val = true;
  adc_channel_setup_dt_fake.return_val = 0;

  /* Configure timer to succeed */
  device_is_ready_mock_fake.return_val = true;
  counter_us_to_ticks_fake.return_val = 1000;

  /* Call adcAcqUtilInitAdc - should succeed */
  result = adcAcqUtilInitAdc(&adcConfig);

  zassert_equal(result, 0,
                "adcAcqUtilInitAdc should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 2,
                "k_malloc should be called twice for buffer and voltValues");
  zassert_equal(adc_is_ready_dt_fake.call_count, 2,
                "adc_is_ready_dt should be called twice for 2 channels");
  zassert_equal(adc_channel_setup_dt_fake.call_count, 2,
                "adc_channel_setup_dt should be called twice for 2 channels");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once for timer");
  zassert_equal(counter_us_to_ticks_fake.call_count, 1,
                "counter_us_to_ticks should be called once");
}

/* ===========================================================================
 * adcAcqUtilInitSubscriptions
 * =========================================================================*/

/**
 * @test The adcAcqUtilInitSubscriptions function must return -ENOSPC
 * when subscription allocation fails.
 */
ZTEST(adc_util_tests, test_init_subscriptions_allocation_failure)
{
  AdcSubConfig_t subConfig = {
    .maxSubCount = 4,
    .activeSubCount = 0
  };
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call adcAcqUtilInitSubscriptions - should fail due to allocation failure */
  result = adcAcqUtilInitSubscriptions(&subConfig);

  zassert_equal(result, -ENOSPC,
                "adcAcqUtilInitSubscriptions should return -ENOSPC when allocation fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once before failing");
}

/**
 * @test The adcAcqUtilInitSubscriptions function must return -ENOMEM
 * when memory pool creation fails.
 */
ZTEST(adc_util_tests, test_init_subscriptions_pool_creation_failure)
{
  AdcSubConfig_t subConfig = {
    .maxSubCount = 4,
    .activeSubCount = 0
  };
  static uint8_t fake_subscriptions[64];
  int result;

  /* Configure k_malloc to succeed (return valid pointer) */
  k_malloc_fake.return_val = fake_subscriptions;

  /* Configure osMemoryPoolNew to return NULL (pool creation failure) */
  osMemoryPoolNew_fake.return_val = NULL;

  /* Call adcAcqUtilInitSubscriptions - should fail due to pool creation failure */
  result = adcAcqUtilInitSubscriptions(&subConfig);

  zassert_equal(result, -ENOMEM,
                "adcAcqUtilInitSubscriptions should return -ENOMEM when pool creation fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once for subscriptions");
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once before failing");
}

/**
 * @test The adcAcqUtilInitSubscriptions function must successfully
 * initialize subscriptions when all operations succeed.
 */
ZTEST(adc_util_tests, test_init_subscriptions_success)
{
  extern size_t chanCount;
  extern size_t voltBufSize;
  AdcSubConfig_t subConfigInput = {
    .maxSubCount = 4,
    .activeSubCount = 0
  };
  static uint8_t fake_subscriptions[64];
  static uint8_t fake_pool[1];
  int result;
  size_t expectedBlockCount;
  size_t expectedBlockSize;

  /* Set chanCount and voltBufSize for block size calculation */
  chanCount = 2;
  voltBufSize = chanCount + 1;

  /* Calculate expected parameters for osMemoryPoolNew */
  expectedBlockCount = 2 * subConfigInput.maxSubCount;
  expectedBlockSize = sizeof(SrvMsgPayload_t) + (voltBufSize * sizeof(float));

  /* Configure k_malloc to succeed (return valid pointer) */
  k_malloc_fake.return_val = fake_subscriptions;

  /* Configure osMemoryPoolNew to succeed (return non-NULL) */
  osMemoryPoolNew_fake.return_val = fake_pool;

  /* Call adcAcqUtilInitSubscriptions - should succeed */
  result = adcAcqUtilInitSubscriptions(&subConfigInput);

  zassert_equal(result, 0,
                "adcAcqUtilInitSubscriptions should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once for subscriptions");
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
  zassert_equal(osMemoryPoolNew_fake.arg0_val, expectedBlockCount,
                "osMemoryPoolNew should be called with correct block count");
  zassert_equal(osMemoryPoolNew_fake.arg1_val, expectedBlockSize,
                "osMemoryPoolNew should be called with correct block size");
  zassert_is_null(osMemoryPoolNew_fake.arg2_val,
                  "osMemoryPoolNew should be called with NULL attr");
}

/* ===========================================================================
 * adcAcqUtilStartTrigger
 * =========================================================================*/

/**
 * @test The adcAcqUtilStartTrigger function must return an error when
 * counter_set_top_value fails.
 */
ZTEST(adc_util_tests, test_start_trigger_set_top_value_failure)
{
  int result;

  /* Configure counter_set_top_value to return error */
  counter_set_top_value_fake.return_val = -EIO;

  /* Call adcAcqUtilStartTrigger - should fail */
  result = adcAcqUtilStartTrigger();

  zassert_equal(result, -EIO,
                "adcAcqUtilStartTrigger should return -EIO when counter_set_top_value fails");
  zassert_equal(counter_set_top_value_fake.call_count, 1,
                "counter_set_top_value should be called once");
  zassert_equal(counter_start_fake.call_count, 0,
                "counter_start should not be called when set_top_value fails");
}

/**
 * @test The adcAcqUtilStartTrigger function must return an error when
 * counter_start fails.
 */
ZTEST(adc_util_tests, test_start_trigger_counter_start_failure)
{
  int result;

  /* Configure counter_set_top_value to succeed */
  counter_set_top_value_fake.return_val = 0;

  /* Configure counter_start to return error */
  counter_start_fake.return_val = -EIO;

  /* Call adcAcqUtilStartTrigger - should fail */
  result = adcAcqUtilStartTrigger();

  zassert_equal(result, -EIO,
                "adcAcqUtilStartTrigger should return -EIO when counter_start fails");
  zassert_equal(counter_set_top_value_fake.call_count, 1,
                "counter_set_top_value should be called once");
  zassert_equal(counter_start_fake.call_count, 1,
                "counter_start should be called once");
}

/**
 * @test The adcAcqUtilStartTrigger function must successfully start
 * the trigger timer when all operations succeed.
 */
ZTEST(adc_util_tests, test_start_trigger_success)
{
  extern struct counter_top_cfg triggerConfig;
  int result;

  /* Configure counter_set_top_value to succeed */
  counter_set_top_value_fake.return_val = 0;

  /* Configure counter_start to succeed */
  counter_start_fake.return_val = 0;

  /* Call adcAcqUtilStartTrigger - should succeed */
  result = adcAcqUtilStartTrigger();

  zassert_equal(result, 0,
                "adcAcqUtilStartTrigger should return 0 on success");
  zassert_equal(counter_set_top_value_fake.call_count, 1,
                "counter_set_top_value should be called once");
  zassert_equal(counter_set_top_value_fake.arg0_val, &mock_timer_device,
                "counter_set_top_value should be called with trigger timer device");
  zassert_equal(counter_set_top_value_fake.arg1_val, &triggerConfig,
                "counter_set_top_value should be called with triggerConfig pointer");
  zassert_equal(counter_start_fake.call_count, 1,
                "counter_start should be called once");
  zassert_equal(counter_start_fake.arg0_val, &mock_timer_device,
                "counter_start should be called with trigger timer device");
}

/* ===========================================================================
 * adcAcqUtilStopTrigger
 * =========================================================================*/

/**
 * @test The adcAcqUtilStopTrigger function must return an error when
 * counter_stop fails.
 */
ZTEST(adc_util_tests, test_stop_trigger_counter_stop_failure)
{
  int result;

  counter_stop_fake.return_val = -EIO;

  result = adcAcqUtilStopTrigger();

  zassert_equal(result, -EIO,
                "adcAcqUtilStopTrigger should return -EIO when counter_stop fails");
  zassert_equal(counter_stop_fake.call_count, 1,
                "counter_stop should be called once");
}

/**
 * @test The adcAcqUtilStopTrigger function must successfully stop
 * the trigger timer.
 */
ZTEST(adc_util_tests, test_stop_trigger_success)
{
  int result;

  counter_stop_fake.return_val = 0;

  result = adcAcqUtilStopTrigger();

  zassert_equal(result, 0,
                "adcAcqUtilStopTrigger should return 0 on success");
  zassert_equal(counter_stop_fake.call_count, 1,
                "counter_stop should be called once");
  zassert_equal(counter_stop_fake.arg0_val, &mock_timer_device,
                "counter_stop should be called with trigger timer device");
}

/* ===========================================================================
 * adcAcqUtilProcessData
 * =========================================================================*/

/**
 * @test The adcAcqUtilProcessData function must return an error when
 * the first filter read fails.
 */
ZTEST(adc_util_tests, test_process_data_first_read_failure)
{
  extern size_t chanCount;
  extern size_t voltBufSize;
  int result;

  chanCount = 2;
  voltBufSize = 3;

  /* Configure adcAcqFilterGetThirdOrderData to return error on first call */
  adcAcqFilterGetThirdOrderData_fake.return_val = -EIO;

  /* Call adcAcqUtilProcessData - should fail on first read */
  result = adcAcqUtilProcessData();

  zassert_equal(result, -EIO,
                "adcAcqUtilProcessData should return -EIO when first read fails");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.call_count, 1,
                "adcAcqFilterGetThirdOrderData should be called once before failing");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_val, 0,
                "Failed read should be at index 0");
}

/**
 * @test The adcAcqUtilProcessData function must return an error when
 * a mid-loop filter read fails.
 */
ZTEST(adc_util_tests, test_process_data_mid_read_failure)
{
  extern size_t chanCount;
  extern size_t voltBufSize;
  extern float *voltValues;
  float test_volt_values[3];
  int result;
  int return_vals[] = {0, -EIO};

  chanCount = 2;
  voltBufSize = 3;
  voltValues = test_volt_values;

  /* First read succeeds, second fails */
  SET_RETURN_SEQ(adcAcqFilterGetThirdOrderData, return_vals, 2);

  /* Call adcAcqUtilProcessData - should fail on second read */
  result = adcAcqUtilProcessData();

  zassert_equal(result, -EIO,
                "adcAcqUtilProcessData should return -EIO when second read fails");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.call_count, 2,
                "adcAcqFilterGetThirdOrderData should be called twice before failing");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_history[0], 0,
                "First read should be at index 0");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_history[1], 1,
                "Second read should be at index 1");
}

/**
 * Custom fake for adcAcqFilterGetThirdOrderData that provides mV test data.
 * Call 0 (index 0 / VREF_CHANNEL_INDEX): 1210 mV
 * Call 1 (index 1 / channel 1):          1500 mV
 * Call 2 (index 2 / Vdd slot):           3300 mV
 */
static int process_data_test_values[] = {1210, 1500, 3300};
static size_t process_data_call_idx = 0;

static int adcAcqFilterGetThirdOrderData_process_success(size_t chanId, int32_t *data)
{
  *data = process_data_test_values[process_data_call_idx];
  process_data_call_idx++;
  return 0;
}

/**
 * @test The adcAcqUtilProcessData function must successfully read all
 * filter values and populate voltValues in volts.
 */
ZTEST(adc_util_tests, test_process_data_success)
{
  extern float *voltValues;
  extern size_t chanCount;
  extern size_t voltBufSize;
  float test_volt_values[3];
  int result;

  chanCount = 2;
  voltBufSize = 3;
  voltValues = test_volt_values;
  process_data_call_idx = 0;

  adcAcqFilterGetThirdOrderData_fake.custom_fake = adcAcqFilterGetThirdOrderData_process_success;

  /* Call adcAcqUtilProcessData - should succeed */
  result = adcAcqUtilProcessData();

  zassert_equal(result, 0,
                "adcAcqUtilProcessData should return 0 on success");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.call_count, 3,
                "adcAcqFilterGetThirdOrderData should be called voltBufSize (3) times");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_history[0], 0,
                "First read should be at index 0");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_history[1], 1,
                "Second read should be at index 1");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_history[2], 2,
                "Third read should be at index 2 (Vdd slot)");
  zassert_within(voltValues[0], 1.21f, 0.001f,
                 "voltValues[0] should be 1210 mV / 1000 = 1.21 V");
  zassert_within(voltValues[1], 1.5f, 0.001f,
                 "voltValues[1] should be 1500 mV / 1000 = 1.5 V");
  zassert_within(voltValues[2], 3.3f, 0.001f,
                 "voltValues[2] should be 3300 mV / 1000 = 3.3 V");

  /* Clean up */
  voltValues = NULL;
}

/* ===========================================================================
 * adcAcqUtilNotifySubscribers
 * =========================================================================*/

/**
 * @test The adcAcqUtilNotifySubscribers function must skip callback
 * when memory pool allocation fails.
 */
ZTEST(adc_util_tests, test_notify_subscribers_pool_alloc_failure)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  extern osMemoryPoolId_t subDataPool;
  AdcSubEntry_t test_subscriptions[1];
  int result;

  /* Set up one active, non-paused subscription */
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = false;
  subscriptions = test_subscriptions;
  subConfig.activeSubCount = 1;
  subDataPool = (osMemoryPoolId_t)0x1000;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call adcAcqUtilNotifySubscribers */
  result = adcAcqUtilNotifySubscribers();

  zassert_equal(result, 0,
                "adcAcqUtilNotifySubscribers should return 0 even on allocation failure");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, subDataPool,
                "osMemoryPoolAlloc should be called with subDataPool");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
}

/**
 * @test The adcAcqUtilNotifySubscribers function must free the buffer
 * when callback returns an error.
 */
ZTEST(adc_util_tests, test_notify_subscribers_callback_failure)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  extern osMemoryPoolId_t subDataPool;
  extern size_t chanCount;
  extern size_t voltBufSize;
  extern float *voltValues;
  AdcSubEntry_t test_subscriptions[1];
  static uint8_t fake_buffer[64];
  float test_volt_values[3] = {1.21f, 1.5f, 3.3f};
  int result;

  /* Set up channel count and voltValues for memcpy */
  chanCount = 2;
  voltBufSize = 3;
  voltValues = test_volt_values;

  /* Set up one active, non-paused subscription */
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = false;
  subscriptions = test_subscriptions;
  subConfig.activeSubCount = 1;
  subDataPool = (osMemoryPoolId_t)0x1000;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call adcAcqUtilNotifySubscribers */
  result = adcAcqUtilNotifySubscribers();

  zassert_equal(result, 0,
                "adcAcqUtilNotifySubscribers should return 0 even on callback failure");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once when callback fails");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, subDataPool,
                "osMemoryPoolFree should be called with subDataPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, fake_buffer,
                "osMemoryPoolFree should be called with allocated buffer");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  voltValues = NULL;
}

/**
 * @test The adcAcqUtilNotifySubscribers function must successfully
 * notify all active subscribers when all operations succeed.
 */
ZTEST(adc_util_tests, test_notify_subscribers_success)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  extern osMemoryPoolId_t subDataPool;
  extern size_t chanCount;
  extern size_t voltBufSize;
  extern float *voltValues;
  AdcSubEntry_t test_subscriptions[1];
  static uint8_t fake_buffer[64];
  float test_volt_values[3] = {1.21f, 1.5f, 3.3f};
  int result;

  /* Set up channel count and voltValues for memcpy */
  chanCount = 2;
  voltBufSize = 3;
  voltValues = test_volt_values;

  /* Set up one active, non-paused subscription */
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = false;
  subscriptions = test_subscriptions;
  subConfig.activeSubCount = 1;
  subDataPool = (osMemoryPoolId_t)0x1000;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call adcAcqUtilNotifySubscribers */
  result = adcAcqUtilNotifySubscribers();

  zassert_equal(result, 0,
                "adcAcqUtilNotifySubscribers should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, subDataPool,
                "osMemoryPoolAlloc should be called with subDataPool");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, (SrvMsgPayload_t *)fake_buffer,
                "Callback should be called with allocated buffer");
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when callback succeeds");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  voltValues = NULL;
}

/**
 * @test The adcAcqUtilNotifySubscribers function must skip
 * paused subscriptions and not allocate memory or call their callbacks.
 */
ZTEST(adc_util_tests, test_notify_subscribers_skips_paused)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  extern osMemoryPoolId_t subDataPool;
  extern size_t chanCount;
  extern size_t voltBufSize;
  extern float *voltValues;
  AdcSubEntry_t test_subscriptions[2];
  static uint8_t fake_buffer[64];
  float test_volt_values[3] = {1.21f, 1.5f, 3.3f};
  int result;

  /* Set up channel count and voltValues for memcpy */
  chanCount = 2;
  voltBufSize = 3;
  voltValues = test_volt_values;

  /* Set up two subscriptions: one paused, one active */
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = true;  /* This one should be skipped */
  test_subscriptions[1].callback = mock_subscription_callback;
  test_subscriptions[1].isPaused = false; /* This one should be called */
  subscriptions = test_subscriptions;
  subConfig.activeSubCount = 2;
  subDataPool = (osMemoryPoolId_t)0x1000;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call adcAcqUtilNotifySubscribers */
  result = adcAcqUtilNotifySubscribers();

  zassert_equal(result, 0,
                "adcAcqUtilNotifySubscribers should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once (paused subscription skipped)");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once (paused subscription skipped)");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  voltValues = NULL;
}

/* ===========================================================================
 * adcAcqUtilAddSubscription
 * =========================================================================*/

/**
 * @test The adcAcqUtilAddSubscription function must return -ENOSPC
 * when the maximum subscription count is reached.
 */
ZTEST(adc_util_tests, test_add_subscription_max_reached)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  /* Set up subscriptions array */
  subscriptions = test_subscriptions;

  /* Set maxSubCount to 4 and activeSubCount to 3 (one slot left) */
  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 3;

  /* Try to add subscription - should fail because 3 + 1 >= 4 */
  result = adcAcqUtilAddSubscription(mock_subscription_callback);

  zassert_equal(result, -ENOSPC,
                "adcAcqUtilAddSubscription should return -ENOSPC when max reached");
  zassert_equal(subConfig.activeSubCount, 3,
                "activeSubCount should remain unchanged on failure");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/**
 * @test The adcAcqUtilAddSubscription function must successfully add
 * a subscription when there is available space.
 */
ZTEST(adc_util_tests, test_add_subscription_success)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  /* Initialize subscriptions array */
  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  subscriptions = test_subscriptions;

  /* Set maxSubCount to 4 and activeSubCount to 0 (empty) */
  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 0;

  /* Add subscription - should succeed */
  result = adcAcqUtilAddSubscription(mock_subscription_callback);

  zassert_equal(result, 0,
                "adcAcqUtilAddSubscription should return 0 on success");
  zassert_equal(subConfig.activeSubCount, 1,
                "activeSubCount should be incremented to 1");
  zassert_equal(subscriptions[0].callback, mock_subscription_callback,
                "subscription callback should be stored at index 0");
  zassert_false(subscriptions[0].isPaused,
                "subscription isPaused should be set to false");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/* ===========================================================================
 * adcAcqUtilRemoveSubscription
 * =========================================================================*/

/**
 * @test The adcAcqUtilRemoveSubscription function must return -ESRCH
 * when the subscription callback is not found.
 */
ZTEST(adc_util_tests, test_remove_subscription_not_found)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  /* Initialize subscriptions array with a different callback */
  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  test_subscriptions[0].callback = (AdcSubCallback_t)0xDEADBEEF;
  test_subscriptions[0].isPaused = false;
  subscriptions = test_subscriptions;

  /* Set maxSubCount to 4 and activeSubCount to 1 */
  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 1;

  /* Try to remove a subscription that doesn't exist */
  result = adcAcqUtilRemoveSubscription(mock_subscription_callback);

  zassert_equal(result, -ESRCH,
                "adcAcqUtilRemoveSubscription should return -ESRCH when not found");
  zassert_equal(subConfig.activeSubCount, 1,
                "activeSubCount should remain unchanged when subscription not found");
  zassert_equal(test_subscriptions[0].callback, (AdcSubCallback_t)0xDEADBEEF,
                "existing subscription should remain unchanged");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/**
 * @test The adcAcqUtilRemoveSubscription function must successfully
 * remove a subscription and shift remaining subscriptions down.
 */
ZTEST(adc_util_tests, test_remove_subscription_success)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  AdcSubCallback_t other_callback = (AdcSubCallback_t)0xDEADBEEF;
  int result;

  /* Initialize subscriptions array with 2 subscriptions */
  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = false;
  test_subscriptions[1].callback = other_callback;
  test_subscriptions[1].isPaused = true;
  subscriptions = test_subscriptions;

  /* Set maxSubCount to 4 and activeSubCount to 2 */
  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 2;

  /* Remove the first subscription */
  result = adcAcqUtilRemoveSubscription(mock_subscription_callback);

  zassert_equal(result, 0,
                "adcAcqUtilRemoveSubscription should return 0 on success");
  zassert_equal(subConfig.activeSubCount, 1,
                "activeSubCount should be decremented to 1");
  zassert_equal(test_subscriptions[0].callback, other_callback,
                "remaining subscription should be shifted to index 0");
  zassert_true(test_subscriptions[0].isPaused,
               "shifted subscription should preserve isPaused state");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/* ===========================================================================
 * adcAcqUtilSetSubPauseState
 * =========================================================================*/

/**
 * @test The adcAcqUtilSetSubPauseState function must return -ESRCH
 * when the subscription callback is not found.
 */
ZTEST(adc_util_tests, test_set_sub_pause_state_not_found)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  /* Initialize subscriptions array with a different callback */
  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  test_subscriptions[0].callback = (AdcSubCallback_t)0xDEADBEEF;
  test_subscriptions[0].isPaused = false;
  subscriptions = test_subscriptions;

  /* Set maxSubCount to 4 and activeSubCount to 1 */
  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 1;

  /* Try to set pause state for a subscription that doesn't exist */
  result = adcAcqUtilSetSubPauseState(mock_subscription_callback, true);

  zassert_equal(result, -ESRCH,
                "adcAcqUtilSetSubPauseState should return -ESRCH when not found");
  zassert_false(test_subscriptions[0].isPaused,
                "existing subscription isPaused should remain unchanged");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/**
 * @test The adcAcqUtilSetSubPauseState function must successfully
 * pause a subscription when setting isPaused to true.
 */
ZTEST(adc_util_tests, test_set_sub_pause_state_pause_success)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = false;
  test_subscriptions[1].callback = (AdcSubCallback_t)0xDEADBEEF;
  test_subscriptions[1].isPaused = false;
  subscriptions = test_subscriptions;

  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 2;

  /* Pause the subscription */
  result = adcAcqUtilSetSubPauseState(mock_subscription_callback, true);

  zassert_equal(result, 0,
                "adcAcqUtilSetSubPauseState should return 0 on success");
  zassert_true(test_subscriptions[0].isPaused,
               "subscription isPaused should be set to true");
  zassert_false(test_subscriptions[1].isPaused,
                "other subscription isPaused should remain false");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/**
 * @test The adcAcqUtilSetSubPauseState function must successfully
 * unpause a subscription when setting isPaused to false.
 */
ZTEST(adc_util_tests, test_set_sub_pause_state_unpause_success)
{
  extern AdcSubConfig_t subConfig;
  extern AdcSubEntry_t *subscriptions;
  AdcSubEntry_t test_subscriptions[4];
  int result;

  memset(test_subscriptions, 0, sizeof(test_subscriptions));
  test_subscriptions[0].callback = mock_subscription_callback;
  test_subscriptions[0].isPaused = true;
  subscriptions = test_subscriptions;

  subConfig.maxSubCount = 4;
  subConfig.activeSubCount = 1;

  /* Unpause the subscription */
  result = adcAcqUtilSetSubPauseState(mock_subscription_callback, false);

  zassert_equal(result, 0,
                "adcAcqUtilSetSubPauseState should return 0 on success");
  zassert_false(test_subscriptions[0].isPaused,
                "subscription isPaused should be set to false");

  /* Clean up */
  subscriptions = NULL;
  subConfig.activeSubCount = 0;
  subConfig.maxSubCount = 0;
}

/* ===========================================================================
 * adcAcqUtilGetChanCount
 * =========================================================================*/

/**
 * @test The adcAcqUtilGetChanCount function must return the volt buffer
 * size (channel count + 1 for Vdd).
 */
ZTEST(adc_util_tests, test_get_chan_count_returns_channel_count)
{
  size_t count;

  /* Without initialization, voltBufSize should be 0 */
  count = adcAcqUtilGetChanCount();

  zassert_equal(count, 0, "Channel count should be 0 before initialization");
}

/* ===========================================================================
 * adcAcqUtilGetRaw
 * =========================================================================*/

/**
 * @test The adcAcqUtilGetRaw function must return -EINVAL when
 * channel ID is greater than or equal to voltBufSize.
 */
ZTEST(adc_util_tests, test_get_raw_invalid_channel_id)
{
  uint32_t rawVal;
  int result;

  /* voltBufSize is 0 (reset by before), so any chanId is invalid */
  result = adcAcqUtilGetRaw(0, &rawVal);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetRaw should return -EINVAL for invalid channel ID");
}

/**
 * @test The adcAcqUtilGetRaw function must return -EINVAL when
 * rawVal pointer is NULL.
 */
ZTEST(adc_util_tests, test_get_raw_null_pointer)
{
  extern size_t voltBufSize;
  int result;

  /* Set voltBufSize to 1 so chanId 0 passes the bounds check */
  voltBufSize = 1;

  /* Try to get raw value with NULL pointer */
  result = adcAcqUtilGetRaw(0, NULL);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetRaw should return -EINVAL for NULL rawVal pointer");
}

/**
 * @test The adcAcqUtilGetRaw function must return an error when
 * adcAcqFilterGetThirdOrderData fails.
 */
ZTEST(adc_util_tests, test_get_raw_filter_error)
{
  extern size_t voltBufSize;
  uint32_t rawVal;
  int result;

  voltBufSize = 3;

  /* Configure mock to return error from adcAcqFilterGetThirdOrderData */
  adcAcqFilterGetThirdOrderData_fake.return_val = -EIO;

  /* Try to get raw value - should fail due to filter error */
  result = adcAcqUtilGetRaw(0, &rawVal);

  zassert_equal(result, -EIO,
                "adcAcqUtilGetRaw should return -EIO when filter function fails");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.call_count, 1,
                "adcAcqFilterGetThirdOrderData should be called once");
}

/**
 * Custom fake function to simulate successful filter data retrieval.
 */
static int adcAcqFilterGetThirdOrderData_success(size_t chanId, int32_t *data)
{
  *data = 1234;
  return 0;
}

/**
 * @test The adcAcqUtilGetRaw function must successfully retrieve
 * the raw ADC value when all parameters are valid.
 */
ZTEST(adc_util_tests, test_get_raw_success)
{
  extern size_t voltBufSize;
  uint32_t rawVal;
  int result;

  voltBufSize = 3;

  adcAcqFilterGetThirdOrderData_fake.custom_fake = adcAcqFilterGetThirdOrderData_success;

  /* Get raw value - should succeed */
  result = adcAcqUtilGetRaw(0, &rawVal);

  zassert_equal(result, 0,
                "adcAcqUtilGetRaw should return 0 on success");
  zassert_equal(rawVal, 1234,
                "rawVal should be set to the value returned by filter");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.call_count, 1,
                "adcAcqFilterGetThirdOrderData should be called once");
  zassert_equal(adcAcqFilterGetThirdOrderData_fake.arg0_val, 0,
                "Filter should be called with correct channel ID");
}

/* ===========================================================================
 * adcAcqUtilGetVolt
 * =========================================================================*/

/**
 * @test The adcAcqUtilGetVolt function must return -EINVAL when
 * channel ID is greater than or equal to voltBufSize.
 */
ZTEST(adc_util_tests, test_get_volt_invalid_channel_id)
{
  float voltVal;
  int result;

  /* voltBufSize is 0 (reset by before), so any chanId is invalid */
  result = adcAcqUtilGetVolt(0, &voltVal);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetVolt should return -EINVAL for invalid channel ID");
}

/**
 * @test The adcAcqUtilGetVolt function must return -EINVAL when
 * voltVal pointer is NULL.
 */
ZTEST(adc_util_tests, test_get_volt_null_pointer)
{
  extern size_t voltBufSize;
  int result;

  /* Set voltBufSize to 1 so chanId 0 passes the bounds check */
  voltBufSize = 1;

  /* Try to get voltage value with NULL pointer */
  result = adcAcqUtilGetVolt(0, NULL);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetVolt should return -EINVAL for NULL voltVal pointer");
}

/**
 * @test The adcAcqUtilGetVolt function must successfully retrieve
 * the voltage value when all parameters are valid.
 */
ZTEST(adc_util_tests, test_get_volt_success)
{
  extern size_t voltBufSize;
  extern float *voltValues;
  float voltVal;
  float testVoltages[3];
  int result;

  voltBufSize = 3;

  testVoltages[0] = 1.21f;
  testVoltages[1] = 1.5f;
  testVoltages[2] = 3.3f;
  voltValues = testVoltages;

  /* Get voltage value - should succeed */
  result = adcAcqUtilGetVolt(0, &voltVal);

  zassert_equal(result, 0,
                "adcAcqUtilGetVolt should return 0 on success");
  zassert_equal(voltVal, 1.21f,
                "voltVal should be set to the value from voltValues array");

  /* Clean up */
  voltValues = NULL;
}

ZTEST_SUITE(adc_util_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
