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
#include <zephyr/portability/cmsis_os2.h>

/**
 * @brief   The ADC subscription data structure.
 *
 *          This structure is allocated from the service's memory pool and passed
 *          to subscribers. Subscribers must free the memory back to the pool
 *          after processing using the embedded pool ID.
 */
typedef struct
{
  osMemoryPoolId_t pool;                            /**< Memory pool to return buffer to. */
  size_t valCount;                                  /**< Number of ADC values. */
  float values[];                                   /**< Flexible array of ADC values. */
} AdcSubData_t;

/**
 * @brief   The ADC subscription callback type.
 *
 * @param[in]   data: The ADC data buffer (must be freed by subscriber).
 *
 * @return  0 if successful, the error code otherwise.
 */
typedef int (*AdcSubCallback_t)(AdcSubData_t *data);

/**
 * @brief   The ADC configuration structure.
 *
 *          The ADC device and channels are obtained from devicetree io-channels property.
 *          The trigger timer is obtained from the adc-trigger devicetree alias.
 *          The sampling rate is in microseconds and will be used to set the timer period.
 *          The timer will trigger the ADC conversion.
 *
 *          Filter Description:
 *          The filter is a 3rd-order cascaded RC low-pass filter implemented in integer mathematics.
 *          It uses the digital RC filter equation: y[n] = y[n-1] + α × (x[n] - y[n-1])
 *          Where α = tau / 512 (FILTER_PRESCALE = 9)
 *
 *          Filter Tau Calculation:
 *          To calculate the tau value for a desired 3rd-order cutoff frequency (fc_3rd):
 *            1. Calculate the required 1st-order cutoff: fc_1st = fc_3rd / 0.5098
 *            2. Calculate alpha: α = 1 - exp(-2π × fc_1st / fs)
 *               where fs is the sampling frequency (1/samplingRate)
 *            3. Calculate tau: tau = α × 512
 *            4. Round to nearest integer (valid range: 1 to 511)
 *
 *          Example: For fs = 2000 Hz (samplingRate = 500 μs) and desired fc_3rd = 10 Hz:
 *            fc_1st = 10 / 0.5098 ≈ 19.6 Hz
 *            α = 1 - exp(-2π × 19.6 / 2000) ≈ 0.0614
 *            tau = 0.0614 × 512 ≈ 31
 *
 *          Note: Each RC stage has cutoff fc_1st, but cascading three stages results in:
 *            fc_3rd = fc_1st × 0.5098
 */
typedef struct
{
  uint32_t samplingRate;                            /**< The ADC sampling rate [usec]. */
  int32_t filterTau;                                /**< The ADC filter tau value (1-511). */
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
 * @brief   Unsubscribe from the ADC service.
 *
 * @param[in]   callback: The subscription callback.
 *
 * @return  0 if successful, -ESRCH if subscription not found.
 */
int adcAcqUnsubscribe(AdcSubCallback_t callback);

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
