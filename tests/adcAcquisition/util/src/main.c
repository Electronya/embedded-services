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
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Forward declare needed types without including driver headers */
struct adc_dt_spec;
struct adc_sequence;
struct counter_top_cfg;

/* Mock ADC functions */
FAKE_VALUE_FUNC(bool, adc_is_ready_dt, const struct adc_dt_spec *);
FAKE_VALUE_FUNC(int, adc_channel_setup_dt, const struct adc_dt_spec *);
FAKE_VALUE_FUNC(int, adc_read_async, const struct device *, const struct adc_sequence *, struct k_poll_signal *);

/* Mock counter/timer functions */
FAKE_VALUE_FUNC(uint32_t, counter_us_to_ticks, const struct device *, uint64_t);
FAKE_VALUE_FUNC(int, counter_set_top_value, const struct device *, const struct counter_top_cfg *);
FAKE_VALUE_FUNC(int, counter_start, const struct device *);

/* Mock device functions - use a wrapper since device_is_ready is defined by Zephyr */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);
#define device_is_ready device_is_ready_mock

/* Mock memory functions */
FAKE_VALUE_FUNC(void *, k_malloc, size_t);

/* Mock filter functions */
FAKE_VALUE_FUNC(int, adcAcqFilterPushData, size_t, int32_t, int32_t);
FAKE_VALUE_FUNC(int, adcAcqFilterGetThirdOrderData, size_t, int32_t *);

/* Mock osMemoryPool types and functions */
typedef void *osMemoryPoolId_t;
FAKE_VALUE_FUNC(osMemoryPoolId_t, osMemoryPoolNew, uint32_t, uint32_t, void *);
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(int, osMemoryPoolFree, osMemoryPoolId_t, void *);

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
  FAKE(counter_us_to_ticks) \
  FAKE(counter_set_top_value) \
  FAKE(counter_start) \
  FAKE(device_is_ready_mock) \
  FAKE(k_malloc) \
  FAKE(adcAcqFilterPushData) \
  FAKE(adcAcqFilterGetThirdOrderData) \
  FAKE(osMemoryPoolNew) \
  FAKE(osMemoryPoolAlloc) \
  FAKE(osMemoryPoolFree)

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

/* Mock VREFINT_CAL_ADDR */
static uint16_t mock_vrefint_cal __attribute__((unused)) = 1500;
#define VREFINT_CAL_ADDR (&mock_vrefint_cal)

/* Mock soc.h */
#define _SOC__H_

/* Mock ADC devicetree macro */
#define ADC_DT_SPEC_GET_BY_IDX(node, idx) {.dev = (const struct device *)0x1000, .channel_id = idx, .resolution = 12}

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
  osMemoryPoolId_t poolId;  /* Placeholder for compilation */
  size_t chanCount;  /* Placeholder for compilation */
  size_t dataLen;  /* Placeholder for compilation */
  void *data;  /* Placeholder for compilation */
  float voltages[];  /* Placeholder for compilation */
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
  uint8_t resolution;
};

struct counter_top_cfg {
  uint32_t flags;
  uint32_t ticks;
  void (*callback)(const struct device *dev, void *user_data);
  void *user_data;
};

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
  extern volatile bool adcBusy;
  extern uint16_t *buffer;
  extern AdcConfig_t config;

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  /* Reset mock ADC register */
  mock_adc1_common.CCR = 0;

  /* Reset chanCount - it's a static variable in adcAcquisitionUtil.c */
  chanCount = 0;

  /* Reset adcBusy flag */
  adcBusy = false;

  /* Reset buffer pointer */
  buffer = NULL;

  /* Reset config structure */
  memset(&config, 0, sizeof(config));
}

/**
 * Requirement: The configureChannels function must return -EBUSY when
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
 * Requirement: The configureChannels function must return an error when
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
 * Requirement: The configureChannels function must successfully configure
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

/**
 * Requirement: The triggerConversion function must clear adcBusy flag when
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
 * Requirement: The triggerConversion function must skip conversion when
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
 * Requirement: The triggerConversion function must successfully start ADC
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

/**
 * Requirement: The configureTimer function must return -EBUSY when
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
 * Requirement: The configureTimer function must successfully configure
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

/**
 * Requirement: The adcSeqCallback function must clear adcBusy flag and
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

  /* Initialize buffer with test data */
  test_buffer[0] = 1234;
  test_buffer[1] = 5678;
  buffer = test_buffer;

  /* Configure mock to return error from adcAcqFilterPushData */
  adcAcqFilterPushData_fake.return_val = -EIO;

  /* Call adcSeqCallback */
  result = adcSeqCallback((const struct device *)0x1000, NULL, 0);

  /* Verify filter push was called for each channel */
  zassert_equal(adcAcqFilterPushData_fake.call_count, 2,
                "adcAcqFilterPushData should be called twice for 2 channels");

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
 * Requirement: The adcSeqCallback function must successfully push data to
 * filters and clear adcBusy flag.
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

  /* Set up test state */
  chanCount = 2;
  adcBusy = true;
  config.filterTau = 100;

  /* Initialize buffer with test data */
  test_buffer[0] = 1234;
  test_buffer[1] = 5678;
  buffer = test_buffer;

  /* Configure mock to return success from adcAcqFilterPushData */
  adcAcqFilterPushData_fake.return_val = 0;

  /* Call adcSeqCallback */
  result = adcSeqCallback((const struct device *)0x1000, NULL, 0);

  /* Verify filter push was called for each channel with correct parameters */
  zassert_equal(adcAcqFilterPushData_fake.call_count, 2,
                "adcAcqFilterPushData should be called twice for 2 channels");
  zassert_equal(adcAcqFilterPushData_fake.arg0_history[0], 0,
                "First call should be for channel 0");
  zassert_equal(adcAcqFilterPushData_fake.arg1_history[0], 1234,
                "First call should use buffer[0] value");
  zassert_equal(adcAcqFilterPushData_fake.arg2_history[0], 100,
                "First call should use config.filterTau");
  zassert_equal(adcAcqFilterPushData_fake.arg0_history[1], 1,
                "Second call should be for channel 1");
  zassert_equal(adcAcqFilterPushData_fake.arg1_history[1], 5678,
                "Second call should use buffer[1] value");
  zassert_equal(adcAcqFilterPushData_fake.arg2_history[1], 100,
                "Second call should use config.filterTau");

  /* Verify adcBusy is cleared */
  zassert_false(adcBusy,
                "adcBusy should be cleared after successful conversion");

  /* Verify function returns ADC_ACTION_FINISH */
  zassert_equal(result, ADC_ACTION_FINISH,
                "adcSeqCallback should return ADC_ACTION_FINISH");

  /* Clean up */
  buffer = NULL;
}

/**
 * Requirement: The setupSequence function must correctly initialize the
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
  zassert_equal(sequence.oversampling, OVERSAMPLING_SETTING,
                "sequence.oversampling should be set to OVERSAMPLING_SETTING");
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

/**
 * Requirement: The calculateVdd function must correctly calculate VDD
 * when vrefVal equals the calibration value (VDD = 3.0V).
 */
ZTEST(adc_util_tests, test_calculate_vdd_at_calibration_voltage)
{
  extern float calculateVdd(int32_t vrefVal);
  float vdd;

  /* When vrefVal equals the calibration value (1500), VDD should be 3.0V
   * Formula: VDD = VREFINT_CAL_VOLTAGE * vrefCal / vrefVal
   *        = 3.0 * 1500 / 1500 = 3.0V
   */
  vdd = calculateVdd(1500);

  zassert_within(vdd, 3.0f, 0.001f,
                 "VDD should be 3.0V when vrefVal equals calibration value");
}

/**
 * Requirement: The calculateVdd function must correctly calculate VDD
 * when vrefVal is lower than calibration (indicating higher VDD).
 */
ZTEST(adc_util_tests, test_calculate_vdd_higher_voltage)
{
  extern float calculateVdd(int32_t vrefVal);
  float vdd;

  /* When vrefVal is lower than calibration, VDD is higher
   * Formula: VDD = VREFINT_CAL_VOLTAGE * vrefCal / vrefVal
   *        = 3.0 * 1500 / 1364 = 3.3V (approximately)
   */
  vdd = calculateVdd(1364);

  zassert_within(vdd, 3.3f, 0.01f,
                 "VDD should be approximately 3.3V when vrefVal is 1364");
}

/**
 * Requirement: The calculateVdd function must correctly calculate VDD
 * when vrefVal is higher than calibration (indicating lower VDD).
 */
ZTEST(adc_util_tests, test_calculate_vdd_lower_voltage)
{
  extern float calculateVdd(int32_t vrefVal);
  float vdd;

  /* When vrefVal is higher than calibration, VDD is lower
   * Formula: VDD = VREFINT_CAL_VOLTAGE * vrefCal / vrefVal
   *        = 3.0 * 1500 / 1667 = 2.7V (approximately)
   */
  vdd = calculateVdd(1667);

  zassert_within(vdd, 2.7f, 0.01f,
                 "VDD should be approximately 2.7V when vrefVal is 1667");
}

/**
 * Requirement: The adcAcqUtilInitAdc function must return -ENOSPC when
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
 * Requirement: The adcAcqUtilInitAdc function must return -EBUSY when
 * channel configuration fails due to ADC not ready.
 */
ZTEST(adc_util_tests, test_init_adc_configure_channels_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[2];
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
 * Requirement: The adcAcqUtilInitAdc function must return -EBUSY when
 * timer configuration fails due to timer device not ready.
 */
ZTEST(adc_util_tests, test_init_adc_configure_timer_failure)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[2];
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
 * Requirement: The adcAcqUtilInitAdc function must successfully initialize
 * the ADC when all operations succeed.
 */
ZTEST(adc_util_tests, test_init_adc_success)
{
  AdcConfig_t adcConfig = {
    .samplingRate = 500,
    .filterTau = 100
  };
  static uint16_t fake_buffer[2];
  static float fake_volt_values[2];
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

/**
 * Requirement: The adcAcqUtilInitSubscriptions function must return -ENOSPC
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
 * Requirement: The adcAcqUtilInitSubscriptions function must return -ENOMEM
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
 * Requirement: The adcAcqUtilInitSubscriptions function must successfully
 * initialize subscriptions when all operations succeed.
 */
ZTEST(adc_util_tests, test_init_subscriptions_success)
{
  extern size_t chanCount;
  AdcSubConfig_t subConfigInput = {
    .maxSubCount = 4,
    .activeSubCount = 0
  };
  static uint8_t fake_subscriptions[64];
  static uint8_t fake_pool[1];
  int result;
  size_t expectedBlockCount;
  size_t expectedBlockSize;

  /* Set chanCount for block size calculation */
  chanCount = 2;

  /* Calculate expected parameters for osMemoryPoolNew */
  expectedBlockCount = 2 * subConfigInput.maxSubCount;  /* 2 * 4 = 8 */
  expectedBlockSize = sizeof(SrvMsgPayload_t) + (chanCount * sizeof(float));

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

/**
 * Requirement: The adcAcqUtilStartTrigger function must return an error when
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
 * Requirement: The adcAcqUtilStartTrigger function must return an error when
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
 * Requirement: The adcAcqUtilGetChanCount function must return the number
 * of configured ADC channels.
 */
ZTEST(adc_util_tests, test_get_chan_count_returns_channel_count)
{
  size_t count;

  /* Without initialization, chanCount should be 0 */
  count = adcAcqUtilGetChanCount();

  zassert_equal(count, 0, "Channel count should be 0 before initialization");
}

/**
 * Requirement: The adcAcqUtilGetRaw function must return -EINVAL when
 * channel ID is greater than or equal to the channel count.
 */
ZTEST(adc_util_tests, test_get_raw_invalid_channel_id)
{
  uint32_t rawVal;
  int result;

  /* Try to get raw value from channel 0 (when chanCount is 0) */
  result = adcAcqUtilGetRaw(0, &rawVal);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetRaw should return -EINVAL for invalid channel ID");
}

/**
 * Requirement: The adcAcqUtilGetRaw function must return -EINVAL when
 * rawVal pointer is NULL.
 */
ZTEST(adc_util_tests, test_get_raw_null_pointer)
{
  extern size_t chanCount;
  int result;

  /* Set chanCount to 1 so we can reach the NULL pointer check */
  chanCount = 1;

  /* Try to get raw value with NULL pointer */
  result = adcAcqUtilGetRaw(0, NULL);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetRaw should return -EINVAL for NULL rawVal pointer");
}

/**
 * Requirement: The adcAcqUtilGetRaw function must return an error when
 * adcAcqFilterGetThirdOrderData fails.
 */
ZTEST(adc_util_tests, test_get_raw_filter_error)
{
  extern size_t chanCount;
  uint32_t rawVal;
  int result;

  /* Set chanCount to 2 (simulating 2 configured channels) */
  chanCount = 2;

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
  /* Set the output value to a known test value */
  *data = 1234;
  return 0;
}

/**
 * Requirement: The adcAcqUtilGetRaw function must successfully retrieve
 * the raw ADC value when all parameters are valid.
 */
ZTEST(adc_util_tests, test_get_raw_success)
{
  extern size_t chanCount;
  uint32_t rawVal;
  int result;

  /* Set chanCount to 2 (simulating 2 configured channels) */
  chanCount = 2;

  /* Configure mock to succeed and set output value */
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

/**
 * Requirement: The adcAcqUtilGetVolt function must return -EINVAL when
 * channel ID is greater than or equal to the channel count.
 */
ZTEST(adc_util_tests, test_get_volt_invalid_channel_id)
{
  extern size_t chanCount;
  float voltVal;
  int result;

  /* Set chanCount to 0 (no channels configured) */
  chanCount = 0;

  /* Try to get voltage value from channel 0 (out of bounds) */
  result = adcAcqUtilGetVolt(0, &voltVal);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetVolt should return -EINVAL for invalid channel ID");
}

/**
 * Requirement: The adcAcqUtilGetVolt function must return -EINVAL when
 * voltVal pointer is NULL.
 */
ZTEST(adc_util_tests, test_get_volt_null_pointer)
{
  extern size_t chanCount;
  int result;

  /* Set chanCount to 1 so we can reach the NULL pointer check */
  chanCount = 1;

  /* Try to get voltage value with NULL pointer */
  result = adcAcqUtilGetVolt(0, NULL);

  zassert_equal(result, -EINVAL,
                "adcAcqUtilGetVolt should return -EINVAL for NULL voltVal pointer");
}

/**
 * Requirement: The adcAcqUtilGetVolt function must successfully retrieve
 * the voltage value when all parameters are valid.
 */
ZTEST(adc_util_tests, test_get_volt_success)
{
  extern size_t chanCount;
  extern float *voltValues;
  float voltVal;
  float testVoltages[2];
  int result;

  /* Set chanCount to 2 (simulating 2 configured channels) */
  chanCount = 2;

  /* Allocate and set voltValues array */
  testVoltages[0] = 3.3f;
  testVoltages[1] = 1.8f;
  voltValues = testVoltages;

  /* Get voltage value - should succeed */
  result = adcAcqUtilGetVolt(0, &voltVal);

  zassert_equal(result, 0,
                "adcAcqUtilGetVolt should return 0 on success");
  zassert_equal(voltVal, 3.3f,
                "voltVal should be set to the value from voltValues array");

  /* Clean up */
  voltValues = NULL;
}

ZTEST_SUITE(adc_util_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
