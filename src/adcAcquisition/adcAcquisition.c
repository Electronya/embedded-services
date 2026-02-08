/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisition.c
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Service
 *
 *            ADC acquisition service API implementation.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#include <zephyr/logging/log.h>

#include "adcAcquisition.h"
#include "adcAcquisitionFilter.h"
#include "adcAcquisitionUtil.h"

/**
 * @brief   Register logger.
 */
LOG_MODULE_REGISTER(ADC_AQC_SERVICE_NAME, CONFIG_ENYA_ADC_ACQUISITION_LOG_LEVEL);

/**
 * @brief   Defining the ADC thread stack area.
 */
K_THREAD_STACK_DEFINE(adcStack, CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE);

#ifdef CONFIG_ZTEST
#ifndef ADC_ACQ_RUN_ITERATIONS
#define ADC_ACQ_RUN_ITERATIONS 1
#endif
#endif

/**
 * @brief   The ADC thread.
 */
static struct k_thread thread;

/**
 * @brief   The ADC acquisition thread.
 *
 * @param[in]   p1: The first parameter.
 * @param[in]   p2: The second parameter.
 * @param[in]   p3: The third parameter.
 */
void run(void *p1, void *p2, void *p3)
{
  int err;
  uint32_t notificationRate = (uint32_t)(uintptr_t)p1;

  LOG_INF("ADC acquisition thread started, notification rate: %d ms", notificationRate);

#ifdef CONFIG_ZTEST
  for(size_t i = 0; i < ADC_ACQ_RUN_ITERATIONS; ++i)
#else
  for(;;)
#endif
  {
    k_sleep(K_MSEC(notificationRate));

    err = adcAcqUtilProcessData();
    if(err < 0)
      LOG_ERR("ERROR %d: unable to process ADC data", err);

    err = adcAcqUtilNotifySubscribers();
    if(err < 0)
      LOG_ERR("ERROR %d: unable to notify ADC subscribers", err);
  }
}

int adcAcqInit(AdcConfig_t *adcConfig, AdcSubConfig_t *adcSubConfig, uint32_t priority, k_tid_t *threadId)
{
  int err;

  if(!adcConfig)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid ADC configuration", err);
    return err;
  }

  if(!adcSubConfig)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid ADC subscription configuration", err);
    return err;
  }

  err = adcAcqUtilInitAdc(adcConfig);
  if(err < 0)
    return err;

  err = adcAcqUtilInitSubscriptions(adcSubConfig);
  if(err < 0)
    return err;

  err = adcAcqFilterInit(adcAcqUtilGetChanCount());
  if(err < 0)
    return err;

  *threadId = k_thread_create(&thread, adcStack, CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE, run,
                              (void *)(uintptr_t)adcSubConfig->notificationRate, NULL, NULL,
                              K_PRIO_PREEMPT(priority), 0, K_FOREVER);

  err = k_thread_name_set(&thread, STRINGIFY(ADC_AQC_SERVICE_NAME));
  if(err < 0)
    LOG_ERR("ERROR %d: unable to set ADC acquisition thread name", err);

  return err;
}

int adcAcqStart(void)
{
  int err;

  k_thread_start(&thread);

  err = adcAcqUtilStartTrigger();
  if(err < 0)
    LOG_ERR("ERROR %d: unable to start ADC trigger", err);

  return err;
}

int adcAcqSubscribe(AdcSubCallback_t callback)
{
  return adcAcqUtilAddSubscription(callback);
}

int adcAcqUnsubscribe(AdcSubCallback_t callback)
{
  return adcAcqUtilRemoveSubscription(callback);
}

int adcAcqPauseSubscription(AdcSubCallback_t callback)
{
  return adcAcqUtilSetSubPauseState(callback, true);
}

int adcAqcUnpauseSubscription(AdcSubCallback_t callback)
{
  return adcAcqUtilSetSubPauseState(callback, false);
}

/** @} */
