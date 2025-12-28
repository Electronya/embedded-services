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

/**
 * @brief   The ADc subscription callback type.
 */
typedef int (*AdcSubCallback_t)(float values[], size_t valCount);

/**
 * @brief   The ADC configuration structure.
 *
 *          The ADC device and channels are obtained from devicetree io-channels property.
 *          The trigger timer is obtained from the adc-trigger devicetree alias.
 *          The sampling rate is in microseconds and will be used to set the timer period.
 *          The timer will trigger the ADC conversion.
 *          The filter is a simple RC in integer mathematics to make sure the calculations are fast.
 *          The Tau value is used in the filter stage to set the cut-off frequency of the filter.
 *          TODO: Add filter calculations description.
 */
typedef struct
{
  uint32_t samplingRate;                            /**< The ADC sampling rate [usec]. */
  int32_t filterTau;                                /**< The ADC filter tau value. */
} AdcConfig_t;

/**
 * @brief   The ADC subscriptions configuration structure.
 */
typedef struct
{
  size_t maxSubCount;                               /**< The maximum subscription count. */
  size_t activeSubCount;                            /**< The active subscription count. */
  uint32_t notificationRate;                        /**< The subscription notification rate [msec]. */
} AdcSubConfig_t;

/**
 * @brief   Initialize the ADC acquisition.
 *
 * @param[in]   adcConfig: The ADC conversion configuration.
 * @param[in]   adcSubConfig: The ADC subscription configuration.
 * @param[in]   priority: The service priority.
 * @param[out]  threadId: The thread ID.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqInit(AdcConfig_t *adcConfig, AdcSubConfig_t *adcSubConfig, uint32_t priority, k_tid_t *threadId);

/**
 * @brief   Start the ADC acquisition service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqStart(void);

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
