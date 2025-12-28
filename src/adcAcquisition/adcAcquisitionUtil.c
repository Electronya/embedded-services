/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionUtil.c
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Utilities
 *
 *            ADC acquisition utilities implementation.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <string.h>
#include <soc.h>

#include "adcAcquisitionUtil.h"
#include "adcAcquisitionFilter.h"

/* Setting module logging */
LOG_MODULE_DECLARE(ADC_AQC_SERVICE_NAME);

/**
 * @brief   The oversampling setting value.
 */
#define OVERSAMPLING_SETTING                                            (4)

/**
 * @brief   The oversampling effective resolution.
 */
#define OVERSAMPLING_RESOLUTION                                         (12)

/**
 * @brief   The extra sampling setting.
 */
#define EXTRA_SAMPLINGS_SETTING                                         (0)

/**
 * @brief   The in between channel interval.
 */
#define CHANNEL_INTERVAL                                                (0)

/**
 * @brief   Calibration done at 3.0V
 */
#define VREFINT_CAL_VOLTAGE                                             (3.0f)

/**
 * @brief   The ADC full range value.
 */
#define ADC_FULL_RANGE_VALUE                                            (16383.0f)

/**
 * @brief   The user node from devicetree.
 */
#define USER_NODE                                                       DT_PATH(zephyr_user)

/**
 * @brief   The Vref channel index from devicetree.
 */
#define VREF_CHANNEL_INDEX                                              DT_PROP(USER_NODE, vref_channel_index)

/**
 * @brief  The ADC trigger configuration.
 */
static struct counter_top_cfg triggerConfig;

/**
 * @brief   The ADC device.
 */
static const struct device *adc;

/**
 * @brief   The ADC channel count.
 */
static size_t chanCount;

/**
 * @brief   The ADC acquisition configuration.
 */
static AdcConfig_t config;

/**
 * @brief   The ADC subscription configuration.
 */
static AdcSubConfig_t subConfig;

/**
 * @brief   The ADC buffer.
 */
static uint16_t *buffer = NULL;

/**
 * @brief   The average of the ADC value in volts.
 */
static float *voltValues = NULL;

/**
 * @brief   The ADC conversion sequence.
 */
static struct adc_sequence sequence;

/**
 * @brief   The ADC sequence options.
 */
static struct adc_sequence_options seqOptions;

/**
 * @brief   ADC conversion in progress flag.
 */
static volatile bool adcBusy = false;

/**
 * @brief   The subscription entry.
 */
typedef struct
{
  AdcSubCallback_t callback;
  bool isPaused;
}
AdcSubEntry_t;

/**
 * @brief   The subscriptions.
 */
static AdcSubEntry_t *subscriptions = NULL;

/**
 * @brief   Helper macro for DT_FOREACH_PROP_ELEM to get ADC specs with commas.
 */
#define ADC_DT_SPEC_AND_COMMA(node_id, prop, idx) \
  ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/**
 * @brief   The ADC channels from devicetree io-channels property.
 */
static const struct adc_dt_spec adcChannels[] = {
  DT_FOREACH_PROP_ELEM(USER_NODE, io_channels, ADC_DT_SPEC_AND_COMMA)
};

/**
 * @brief   Allocate the ADC buffers.
 *
 * @param[in]   chanCount: The size of the buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int allocateBuffers(size_t chanCount)
{
  int err = 0;

  buffer = k_malloc(chanCount * sizeof(uint16_t));
  if(!buffer)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the ADC buffer", err);
    return err;
  }

  voltValues = k_malloc(chanCount * sizeof(float));
  if(!chanCount)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the volt average array", err);
  }

  return err;
}

/**
 * @brief   Allocate the subscriptions.
 *
 * @param[in]   maxCount: The maximum subscription count.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int allocateSubscription(size_t maxCount)
{
  int err = 0;

  subscriptions = k_malloc(maxCount * sizeof(AdcSubEntry_t));
  if(!subscriptions)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the subscriptions", err);
  }

  subConfig.activeSubCount = 0;

  return err;
}

/**
 * @brief   Configure the ADC channels and setup the sequence.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int configureChannels(void)
{
  int err;

  sequence.channels = 0;

  for(size_t i = 0; i < ARRAY_SIZE(adcChannels); ++i)
  {
    if(!adc_is_ready_dt(&adcChannels[i]))
    {
      err = -EBUSY;
      LOG_ERR("ERROR %d: ADC controller device %s not ready", err, adcChannels[i].dev->name);
      return err;
    }

    err = adc_channel_setup_dt(&adcChannels[i]);
    if(err < 0)
    {
      LOG_ERR("ERROR %d: unable to setup channel %d", err, adcChannels[i].channel_id);
      return err;
    }

    sequence.channels |= BIT(adcChannels[i].channel_id);
  }

  return 0;
}

/**
 * @brief   The timer interrupt function.
 *
 * @param[in]   dev: The timer device.
 * @param[in]   user_data: The user data.
 */
static void triggerConversion(const struct device *dev, void *user_data)
{
  int err;

  /* Skip if previous conversion still in progress */
  if(adcBusy)
  {
    LOG_WRN("ADC conversion still in progress, skipping trigger");
    return;
  }

  adcBusy = true;
  err = adc_read_async(adc, &sequence, NULL);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to start the ADC conversion", err);
    adcBusy = false;  /* Clear flag on error */
  }
}

/**
 * @brief  Configure the trigger timer.
 *
 * @return int
 */
int configureTimer(void)
{
  int err;

  if(!device_is_ready(config.timer))
  {
    err = -EBUSY;
    LOG_ERR("ERROR %d: timer device busy", err);
    return err;
  }

  triggerConfig.flags = 0;
  triggerConfig.ticks = counter_us_to_ticks(config.timer, config.samplingRate);
  triggerConfig.callback = triggerConversion;
  triggerConfig.user_data = NULL;

  return 0;
}

/**
 * @brief   The sequence callback.
 *
 * @param[in]   dev: The ADC device.
 * @param[in]   sequence: The ADC conversion sequence.
 * @param[in]   samplingIndex: The sample index.
 *
 * @return  ADC_ACTION_FINISH to stop the conversion cycle.
 */
static enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex)
{
  int err;

  for(size_t i = 0; i < chanCount; ++i)
  {
    err = adcAcqFilterPushData(i, (int32_t)buffer[i], config.filterTau);
    if(err < 0)
      LOG_ERR("ERROR %d: unable to push data to the filter", err);
  }

  /* Clear busy flag - conversion complete */
  adcBusy = false;

  return ADC_ACTION_FINISH;
}

/**
 * @brief   Setup the ADC sequence.
 */
static inline void setupSequence(void)
{
  sequence.oversampling = OVERSAMPLING_SETTING;
  sequence.resolution = OVERSAMPLING_RESOLUTION;
  sequence.calibrate = false;
  sequence.options = &seqOptions;
  sequence.buffer = buffer;
  sequence.buffer_size = chanCount * sizeof(uint16_t);

  seqOptions.extra_samplings = EXTRA_SAMPLINGS_SETTING;
  seqOptions.interval_us = CHANNEL_INTERVAL;
  seqOptions.callback = adcSeqCallback;
}

/**
 * @brief   Calculate the real VDD.
 *
 * @param[in]   vrefVal: The acquired internal Vref value.
 *
 * @return  The calculated real VDD.
 */
static inline float calculateVdd(int32_t vrefVal)
{
  uint16_t vrefCal = *VREFINT_CAL_ADDR;

  return VREFINT_CAL_VOLTAGE * (float)vrefCal / (float)vrefVal;
}

int adcAcqUtilInitAdc(AdcConfig_t *adcConfig)
{
  int err;

  adc = adcChannels[0].dev;
  chanCount = ARRAY_SIZE(adcChannels);
  config.timer = adcConfig->timer;
  config.samplingRate = adcConfig->samplingRate;
  config.filterTau = adcConfig->filterTau;

  err = allocateBuffers(chanCount);
  if(err < 0)
    return err;

  err = configureChannels();
  if(err < 0)
    return err;

  setupSequence();

  /* Enable VREFINT in ADC Common Control Register for channel 13 */
  ADC1_COMMON->CCR |= ADC_CCR_VREFEN;

  /* Wait for VREFINT to stabilize (min 12μs per datasheet) */
  k_busy_wait(15);

  err = configureTimer();
  if(err < 0)
    LOG_ERR("ERROR %d: unable to configure the trigger timer", err);

  return err;
}

int adcAcqUtilInitSubscriptions(AdcSubConfig_t *adcSubConfig)
{
  int err;

  memcpy(&subConfig, adcSubConfig, sizeof(AdcSubConfig_t));

  err = allocateSubscription(subConfig.maxSubCount);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to allocate the subscriptions", err);
    return err;
  }

  subConfig.activeSubCount = 0;

  return err;
}

int adcAcqUtilStartTrigger(void)
{
  int err;

  err = counter_set_top_value(config.timer, &triggerConfig);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set the timer trigger", err);
    return err;
  }

  err = counter_start(config.timer);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to start the trigger timer", err);

  return err;
}

int adcAcqUtilProcessData(void)
{
  int err;
  int32_t rawData;
  int32_t rawVref;
  float vdd;

  /* Read Vref from the configured vref channel index */
  err = adcAcqFilterGetThirdOrderData(VREF_CHANNEL_INDEX, &rawVref);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to get vref data from ADC", err);
    return err;
  }

  /* Calculate real VDD from internal Vref reading */
  vdd = calculateVdd(rawVref);

  /* Convert all channels except vref to voltage */
  for(size_t i = 0; i < ARRAY_SIZE(adcChannels); ++i)
  {
    if(i == VREF_CHANNEL_INDEX)
      continue;

    err = adcAcqFilterGetThirdOrderData(i, &rawData);
    if(err < 0)
      return err;

    voltValues[i] = ((float)rawData * vdd) / ADC_FULL_RANGE_VALUE;
  }

  return 0;
}

int adcAcqUtilNotifySubscribers(void)
{
  int err;

  for(size_t i = 0; i < subConfig.activeSubCount; ++i)
  {
    if(!subscriptions[i].isPaused)
    {
      err = subscriptions[i].callback(voltValues, chanCount);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify subscription %d", err, i);
    }
  }

  return 0;
}

int adcAcqUtilAddSubscription(AdcSubCallback_t callback)
{
  int err;

  if(subConfig.activeSubCount + 1 >= subConfig.maxSubCount)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to add the new subscription", err);
    return err;
  }

  subscriptions[subConfig.activeSubCount].callback = callback;
  subscriptions[subConfig.activeSubCount].isPaused = false;

  ++subConfig.activeSubCount;

  return 0;
}

int adcAcqUtilSetSubPauseState(AdcSubCallback_t callback, bool isPaused)
{
  int err = -ESRCH;

  for(size_t i = 0; i < subConfig.activeSubCount && err < 0; ++i)
  {
    if(subscriptions[i].callback == callback)
    {
      subscriptions[i].isPaused = isPaused;
      err = 0;

      if(isPaused)
        LOG_INF("pausing subscription %d", i);
      else
        LOG_INF("unpausing subscription %d", i);
    }
  }

  return err;
}

size_t adcAcqUtilGetChanCount(void)
{
  return chanCount;
}

int adcAcqUtilGetRaw(size_t chanId, uint32_t *rawVal)
{
  int err = 0;

  if(chanId >= chanCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!rawVal)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid raw value pointer", err);
    return err;
  }

  err = adcAcqFilterGetThirdOrderData(chanId, (int32_t *)rawVal);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to get the raw value of channel %d", err, chanId);

  return err;
}

int adcAcqUtilGetVolt(size_t chanId, float *voltVal)
{
  int err = 0;

  if(chanId >= chanCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!voltVal)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid volt value pointer", err);
    return err;
  }

  *voltVal = voltValues[chanId];

  return err;
}

/** @} */
