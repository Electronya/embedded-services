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
#include "adcAcquisitionUtil.h"

/**
 * @brief   Register logger.
 */
LOG_MODULE_REGISTER(ADC_AQC_SERVICE_NAME, CONFIG_ENYA_ADC_ACQUISITION_LOG_LEVEL);

// TODO: Get the ADC and channel config from the device tree when zephyr 4.3 release.

int adcAcqInit(AdcConfig_t *adcConfig, uint32_t priority, k_tid_t *threadId)
{
  int err;

  if(!adcConfig)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid ADC configuration", err);
    return err;
  }

  err = adcAcqUtilInitAdc(adcConfig);
  if(err < 0)
    return err;

  *threadId = adcAcqUtilInitWorkQueue(priority);

  return 0;
}

int adcAcqSubscribe(AdcSubCallback_t callback)
{
  return adcAcqUtilAddSubscription(callback);
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
