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

#include <zephyr/logging/log.h>
#include <string.h>

#include "adcAcquisitionUtil.h"

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
 * @brief
 */
#define ADC_WORK_QUEUE_STACK_SIZE                                       (1024)
/**
 * @brief   Defining The work queue stack area.
 */
K_THREAD_STACK_DEFINE(adcWorkQueueStack, ADC_WORK_QUEUE_STACK_SIZE);

/**
 * @brief   The ADC work queue.
 */
static struct k_work_q adcWorkQueue;

/**
 * @brief   The ADC work queue configuration.
 */
static struct k_work_queue_config adcWorkQueueConfig;

/**
 * @brief   The ADC processing work.
 */
static struct k_work adcProcessWork;

/**
 * @brief   The Start conversion work.
 */
static struct k_work_delayable adcStartConvWork;

/**
 * @brief   The ADC acquisition configuration.
 */
static AdcConfig_t config;

/**
 * @brief   The ADC buffer.
 */
static uint16_t *buffer = NULL;

/**
 * @brief   The average of the raw ADC values.
 */
static uint32_t *rawAverages = NULL;

/**
 * @brief   The average of the ADC value in volts.
 */
static float *voltAverages = NULL;

/**
 * @brief   The ADC conversion sequence.
 */
static struct adc_sequence sequence;

/**
 * @brief   The ADC sequence options.
 */
static struct adc_sequence_options seqOptions;

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
AdcSubEntry_t *subscriptions = NULL;

/**
 * @brief   Allocate the ADC buffers.
 *
 * @param[in]   chanCount: The size of the buffer.
 * @param[in]   conversionCount: The conversion count.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int allocateBuffers(size_t chanCount, size_t conversionCount)
{
  int err = 0;

  buffer = k_malloc(chanCount * conversionCount * sizeof(uint16_t));
  if(!buffer)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the ADC buffer", err);
    return err;
  }

  rawAverages = k_malloc(chanCount * sizeof(uint32_t));
  if(!rawAverages)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the raw average array", err);
    return err;
  }

  voltAverages = k_malloc(chanCount * sizeof(float));
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

  config.activeSubCount = 0;

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

  for(size_t i = 0; i < config.chanCount; ++i)
  {
    err = adc_channel_setup(config.adc, config.channels + i);
    if(err < 0)
    {
      LOG_ERR("ERROR %d: unable to setup channel %d", err, config.channels[i].channel_id);
      return err;
    }

    sequence.channels |= BIT(config.channels[i].channel_id);
  }

  return 0;
}

/**
 * @brief   The sequence callback.
 *
 * @param[in]   dev: The ADC device.
 * @param[in]   sequence: The ADC conversion sequence.
 * @param[in]   samplingIndex: The sample index.
 *
 * @return  ADC_ACTION_CONTINUE until all conversion are done, ADC_ACTION_FINISH otherwise.
 */
static enum adc_action adcSeqCallback(const struct device *dev, const struct adc_sequence *sequence, uint16_t samplingIndex)
{
  if(samplingIndex >= sequence->options->extra_samplings)
  {
    k_work_submit_to_queue(&adcWorkQueue, &adcProcessWork);

    return ADC_ACTION_FINISH;
  }

  return ADC_ACTION_CONTINUE;
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
  sequence.buffer_size = config.chanCount * config.conversionCount * sizeof(uint16_t);

  seqOptions.extra_samplings = config.conversionCount - 1; /* the initial conversion is already taken into account */
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
static inline float calculateVdd(uint16_t vrefVal)
{
  uint16_t vrefCal = *VREFINT_CAL_ADDR;

  return VREFINT_CAL_VOLTAGE * (float)vrefCal / (float)vrefVal;
}

/**
 * @brief   Process the ADC data.
 *
 * @param[in]   work: The work structure.
 */
static void processAdcData(struct k_work *work)
{
  uint32_t sum;

  for(size_t chanIdx = 0; chanIdx < config.chanCount; ++chanIdx)
  {
    sum = 0;

    for(size_t i = 0; i < config.conversionCount; ++i)
      sum += buffer[i * config.chanCount + chanIdx];

    rawAverages[chanIdx] = sum / config.conversionCount;
    voltAverages[chanIdx] = calculateVdd(rawAverages[config.chanCount - 1]) * (float)rawAverages[chanIdx] / ADC_FULL_RANGE_VALUE;
  }

  for(size_t i = 0; i < config.activeSubCount; ++i)
    if(!subscriptions[i].isPaused)
      subscriptions[i].callback(voltAverages, config.chanCount);
}

/**
 * @brief   Start a conversion cycle.
 *
 * @param[in]   work: The work structure.
 */
static void startConversion(struct k_work *work)
{
  int err;

  err = adc_read_async(config.adc, &sequence, NULL);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to start the reading the ADC", err);
    return;
  }

  k_work_schedule_for_queue(&adcWorkQueue, &adcStartConvWork, K_MSEC(config.samplingRate));
}

int adcAcqUtilInitAdc(AdcConfig_t *adcConfig)
{
  int err;

  memcpy(&config, adcConfig, sizeof(AdcConfig_t));

  err = allocateBuffers(config.chanCount, config.conversionCount);
  if(err < 0)
    return err;

  err = allocateSubscription(config.maxSubCount);
  if(err < 0)
    return err;

  if(!device_is_ready(config.adc))
  {
    err = -EBUSY;
    LOG_ERR("ERROR %d: ADC device busy", err);
  }

  err = configureChannels();
  if(err < 0)
    return err;

  setupSequence();

  return err;
}

k_tid_t adcAcqUtilInitWorkQueue(uint32_t priority)
{
  LOG_INF("NVIC ISER0: 0x%08x", NVIC->ISER[0]);
  adcWorkQueueConfig.name = STRINGIFY(ADC_AQC_SERVICE_NAME);

  k_work_queue_init(&adcWorkQueue);

  k_work_queue_start(&adcWorkQueue, adcWorkQueueStack, ADC_WORK_QUEUE_STACK_SIZE, K_PRIO_PREEMPT(priority), &adcWorkQueueConfig);

  k_work_init(&adcProcessWork, processAdcData);
  k_work_init_delayable(&adcStartConvWork, startConversion);

  return adcWorkQueue.thread_id;
}

void adcAcqUtilStartWorkQueue(void)
{
  k_work_schedule_for_queue(&adcWorkQueue, &adcStartConvWork, K_MSEC(config.samplingRate));
}

int adcAcqUtilAddSubscription(AdcSubCallback_t callback)
{
  int err;

  if(config.activeSubCount + 1 >= config.maxSubCount)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to add the new subscription", err);
    return err;
  }

  subscriptions[config.activeSubCount].callback = callback;
  subscriptions[config.activeSubCount].isPaused = false;

  ++config.activeSubCount;

  return 0;
}

int adcAcqUtilSetSubPauseState(AdcSubCallback_t callback, bool isPaused)
{
  int err = -ESRCH;

  for(size_t i = 0; i < config.activeSubCount && err < 0; ++i)
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
  return config.chanCount;
}

int adcAcqUtilGetRaw(size_t chanId, uint32_t *rawVal)
{
  int err = 0;

  if(chanId >= config.chanCount)
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

  *rawVal = rawAverages[chanId];

  return err;
}

int adcAcqUtilGetVolt(size_t chanId, float *voltVal)
{
  int err = 0;

  if(chanId >= config.chanCount)
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

  *voltVal = voltAverages[chanId];

  return err;
}

/** @} */
