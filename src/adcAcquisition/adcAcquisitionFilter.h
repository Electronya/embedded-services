/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionRcFilter.h
 * @author    jbacon
 * @date      2025-10-11
 * @brief     ADC Acquisition Service Filter Stage.
 *
 *            ADC acquisition filter stage API.
 *            This is a simple RC filter. The implementation is base from the
 *            information provided on https://dsplog.com/2007/12/02/digital-implementation-of-rc-low-pass-filter/.
 *            Special thanks to Louis Geoffrion for the first introduction to this filter.
 *
 *            Filter Equation:
 *              y[n] = y[n-1] + α × (x[n] - y[n-1])
 *
 *              Where:
 *                - y[n] = current filtered output
 *                - y[n-1] = previous filtered output
 *                - x[n] = current input sample
 *                - α (alpha) = filter coefficient
 *
 *            Filter Parameters
 *              - FILTER_PRESCALE = 9
 *              - FILTER_MIN_TAU = 1
 *              - FILTER_MAX_TAU = 511
 *              - tau = user-configurable parameter (1 to 511)
 *
 *            Alpha calculation:
 *              α = tau / 2^FILTER_PRESCALE = tau / 512
 *
 *            Cutoff Frequency Formulas
 *              fc = (fs / 2π) × (-ln(1 - α))
 *              fc = (fs / 2π) × (-ln(1 - tau/512))
 *
 *            Inverse Calculation: tau from desired fc
 *              α = 1 - exp(-2π × fc / fs)
 *              tau = α × 512
 *
 *            Cascaded Filter Orders
 *              - 1st order: Single RC filter
 *              - 2nd order: Two cascaded RC filters (fc₂ = fc₁ × 0.6436)
 *              - 3rd order: Three cascaded RC filters (fc₃ = fc₁ × 0.5098)
 *
 *              For cascaded identical filters, the effective cutoff frequency decreases:
 *                fc_nth_order = fc_1st_order × √(2^(1/n) - 1)
 *
 * @ingroup   adc-acquisition
 *
 * @{
 */

#ifndef ADC_ACQ_FILTER
#define ADC_ACQ_FILTER

#include <zephyr/kernel.h>

/**
 * @brief   Initialize the filter stage.
 *
 * @param[in]   chanCount: The channel count.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterInit(size_t chanCount);

/**
 * @brief   Push a new raw data to the filter stage.
 *
 * @param[in]   chanId: The channel ID.
 * @param[in]   rawData: The raw data to push.
 * @param[in]   tau: The filter tau value.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterPushData(size_t chanId, int32_t rawData, int32_t tau);

/**
 * @brief   Get the unfiltered data.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  filtData: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterGetRawData(size_t chanId, int32_t *rawData);

/**
 * @brief   Get the first order filtered data.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  filtData: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterGetFirstOrderData(size_t chanId, int32_t *filtData);

/**
 * @brief   Get the second order filtered data.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  filtData: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterGetSecondOrderData(size_t chanId, int32_t *filtData);

/**
 * @brief   Get the third order filtered data.
 *
 * @param[in]   chanId: The channel ID.
 * @param[out]  filtData: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int adcAcqFilterGetThirdOrderData(size_t chanId, int32_t *filtData);

#endif    /* ADC_ACQ_FILTER */

/** @} */
