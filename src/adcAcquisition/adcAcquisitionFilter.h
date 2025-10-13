/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionRcFilter.h
 * @author    jbacon
 * @date      2025-10-11
 * @brief     ADC Acquisition Service Filter Stage.
 *
 *            ADC acquisition filter stage API.
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
