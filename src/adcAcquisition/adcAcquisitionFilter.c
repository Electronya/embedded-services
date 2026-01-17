/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionFilter.c
 * @author    jbacon
 * @date      2025-10-11
 * @brief     ADC Acquisition Filter Stage
 *
 *            ADC acquisition filter stage implementation.
 *            This is a simple RC filter. The implementation is base from the
 *            information provided on https://dsplog.com/2007/12/02/digital-implementation-of-rc-low-pass-filter/.
 *            Special thanks to Louis Geoffrion for the first introduction to this filter.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#include <zephyr/logging/log.h>

#include "adcAcquisitionFilter.h"
#include "adcAcquisitionUtil.h"

/* Setting module logging */
LOG_MODULE_DECLARE(ADC_AQC_SERVICE_NAME);

/**
 * @brief   The filter first order index.
 */
#define FILTER_FIRST_ORDER_IDX                                        (1)

/**
 * @brief   The filter second order index.
 */
#define FILTER_SECOND_ORDER_IDX                                       (2)

/**
 * @brief   The filter third order index.
 */
#define FILTER_THIRD_ORDER_IDX                                        (3)

/**
 * @brief   The filter max order.
 */
#define FILTER_MAX_ORDER                                              (3)

/**
 * @brief   The filter prescale value.
 */
#define FILTER_PRESCALE                                               (9)

/**
 * @brief   The minimum tau value.
 */
#define FILTER_MIN_TAU                                                (1)

/**
 * @brief   The maximum tau value.
 */
#define FILTER_MAX_TAU                                                (511)

/**
 * @brief   The filter buffer
 */
int32_t *filterBuf = NULL;

/**
 * @brief   The channel count.
 */
size_t filterCount = 0;

int adcAcqFilterInit(size_t chanCount)
{
  int err;

  filterCount = chanCount;

  filterBuf = k_malloc(filterCount * (FILTER_MAX_ORDER + 1) * sizeof(int32_t));
  if(!filterBuf)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate the filter buffer", err);
    return err;
  }

  memset(filterBuf, 0, filterCount * (FILTER_MAX_ORDER + 1) * sizeof(int32_t));

  return 0;
}

int adcAcqFilterPushData(size_t chanId, int32_t rawData, int32_t tau)
{
  int err;
  size_t chanBaseIdx = chanId * (FILTER_MAX_ORDER + 1);

  if(chanId >= filterCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(tau < FILTER_MIN_TAU)
    tau = FILTER_MIN_TAU;
  else if(tau > FILTER_MAX_TAU)
    tau = FILTER_MAX_TAU;

  rawData <<= FILTER_PRESCALE;
  filterBuf[chanBaseIdx] = rawData;
  filterBuf[chanBaseIdx + 1] += (((rawData - filterBuf[chanBaseIdx + 1]) * tau) >> FILTER_PRESCALE);
  filterBuf[chanBaseIdx + 2] += (((filterBuf[chanBaseIdx + 1] - filterBuf[chanBaseIdx + 2]) * tau) >> FILTER_PRESCALE);
  filterBuf[chanBaseIdx + 3] += (((filterBuf[chanBaseIdx + 2] - filterBuf[chanBaseIdx + 3]) * tau) >> FILTER_PRESCALE);

  return 0;
}

int adcAcqFilterGetRawData(size_t chanId, int32_t *rawData)
{
  int err;
  size_t dataIdx = chanId * (FILTER_MAX_ORDER + 1);

  if(chanId >= filterCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!rawData)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: NULL pointer for raw data", err);
    return err;
  }

  *rawData = filterBuf[dataIdx] >> FILTER_PRESCALE;

  return 0;
}

int adcAcqFilterGetFirstOrderData(size_t chanId, int32_t *filtData)
{
  int err;
  size_t dataIdx = chanId * (FILTER_MAX_ORDER + 1) + FILTER_FIRST_ORDER_IDX;

  if(chanId >= filterCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!filtData)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: NULL pointer for filtered data", err);
    return err;
  }

  *filtData = filterBuf[dataIdx] >> FILTER_PRESCALE;

  return 0;
}

int adcAcqFilterGetSecondOrderData(size_t chanId, int32_t *filtData)
{
  int err;
  size_t dataIdx = chanId * (FILTER_MAX_ORDER + 1) + FILTER_SECOND_ORDER_IDX;

  if(chanId >= filterCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!filtData)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: NULL pointer for filtered data", err);
    return err;
  }

  *filtData = filterBuf[dataIdx] >> FILTER_PRESCALE;

  return 0;
}

int adcAcqFilterGetThirdOrderData(size_t chanId, int32_t *filtData)
{
  int err;
  size_t dataIdx = chanId * (FILTER_MAX_ORDER + 1) + 3;

  if(chanId >= filterCount)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid channel ID %d", err, chanId);
    return err;
  }

  if(!filtData)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: NULL pointer for filtered data", err);
    return err;
  }

  *filtData = filterBuf[dataIdx] >> FILTER_PRESCALE;

  return 0;
}

/** @} */
