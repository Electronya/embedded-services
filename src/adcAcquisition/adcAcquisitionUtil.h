/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionUtil.h
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Utilities
 *
 *            ADC acquisition Utilities declaration.
 *
 * @ingroup   adc-acquisition
 *
 * @{
 */

#ifndef ADC_ACQUISITION_UTIL
#define ADC_ACQUISITION_UTIL

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#include "adcAcquisition.h"

/**
 * @brief   The ADC acquisition service name.
 */
#define ADC_AQC_SERVICE_NAME                                    adcAcquisition

/**
 * @brief   Initialize the ADC.
 *
 * @param[in]   dev: The ADC device.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilInitAdc(AdcConfig_t *adcConfig);

/**
 * @brief   Initialize the ADC subscriptions.
 *
 * @param[in]   adcSubConfig: The ADC subscription configuration.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilInitSubscriptions(AdcSubConfig_t *adcSubConfig);

/**
 * @brief   Start the ADC trigger.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilStartTrigger(void);

/**
 * @brief   Process the acquired data.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilProcessData(void);

/**
 * @brief   Notify the active subscribers.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilNotifySubscribers(void);

/**
 * @brief   Add a new subscription.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilAddSubscription(AdcSubCallback_t callback);

/**
 * @brief   Remove a subscription.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, -ESRCH if subscription not found.
 */
int adcAcqUtilRemoveSubscription(AdcSubCallback_t callback);

/**
 * @brief   Set the subscription pause state.
 *
 * @param[in]   callback: The subscription callback.
 * @param[in]   isPaused: The pause state flag.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilSetSubPauseState(AdcSubCallback_t callback, bool isPaused);

/**
 * @brief   Get the ADC channel count.
 *
 * @return  The ADC channel count.
 */
size_t adcAcqUtilGetChanCount(void);

/**
 * @brief   Get the raw value of a channel.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  rawVal: The raw value pointer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilGetRaw(size_t chanId, uint32_t *rawVal);

/**
 * @brief   Get the volt value of a channel.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  voltVal: The volt value pointer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqUtilGetVolt(size_t chanId, float *voltVal);

#endif    /* ADC_ACQUISITION_UTIL */

/** @} */
