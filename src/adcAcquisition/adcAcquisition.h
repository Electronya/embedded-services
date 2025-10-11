/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisition.h
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Service
 *
 *            ADC acquisition service API definition.
 *
 * @defgroup  adc-acquisition adc-acquisition
 *
 * @{
 */

#ifndef ADC_ACQUISITION
#define ADC_ACQUISITION

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

typedef int (*AdcSubCallback_t)(float values[], size_t valCount);

/**
 * @brief   ADC conversion configuration.
 */
typedef struct
{
  const struct device *adc;                         /**< The ADC device. */
  const struct adc_channel_cfg *channels;           /**< The ADC channel configurations. */
  size_t chanCount;                                 /**< The ADC channel count. */
  size_t conversionCount;                           /**< The conversion count for averaging. */
  uint32_t samplingRate;                            /**< The ADC sampling rate. */
  size_t maxSubCount;                               /**< The maximum subscription count. */
  size_t activeSubCount;                            /**< The active subscription count. */
}
AdcConfig_t;

/**
 * @brief   Initialize the ADC acquisition.
 *
 * @param[in]   adcConfig: The ADC conversion configuration.
 * @param[in]   priority: The service priority.
 * @param[out]  threadId: The thread ID.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqInit(AdcConfig_t *adcConfig, uint32_t priority, k_tid_t *threadId);

/**
 * @brief   Start the ADC acquisition service.
 */
void adcAcqStart(void);

/**
 * @brief   Subscribe to the ADC service.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqSubscribe(AdcSubCallback_t callback);

/**
 * @brief   Pause a subscription.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqPauseSubscription(AdcSubCallback_t callback);

/**
 * @brief   Unpause a subscription.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAqcUnpauseSubscription(AdcSubCallback_t callback);

#endif    /* ADC_ACQUISITION */

/** @} */
