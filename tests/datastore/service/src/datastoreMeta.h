/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      datastoreMeta.h
 * @author    jbacon
 * @date      2026-02-05
 * @brief     Datastore Meta Data for Tests
 *
 *            Minimal datastore service meta value definition for testing.
 *            Only contains X-macro definitions for datapoints.
 *
 * @ingroup   datastore
 *
 * @{
 */

#ifndef DATASTORE_META
#define DATASTORE_META

/**
 * @brief   Binary datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_BINARY_DATAPOINTS       X(BINARY_FIRST_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, true) \
                                          X(BINARY_SECOND_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, false) \
                                          X(BINARY_THIRD_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, true) \
                                          X(BINARY_FOURTH_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, false)

/**
 * @brief   Button datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_BUTTON_DATAPOINTS       X(BUTTON_FIRST_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 0) \
                                          X(BUTTON_SECOND_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, 0) \
                                          X(BUTTON_THIRD_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 0) \
                                          X(BUTTON_FOURTH_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, 0)

/**
 * @brief   Float datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_FLOAT_DATAPOINTS        X(FLOAT_FIRST_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, 0.0f) \
                                          X(FLOAT_SECOND_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 1.0f) \
                                          X(FLOAT_THIRD_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, 2.0f) \
                                          X(FLOAT_FOURTH_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 3.0f)

/**
 * @brief   signed integer datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_INT_DATAPOINTS          X(INT_FIRST_DATAPOINT,     DATAPOINT_FLAG_NVM_MASK,  0) \
                                          X(INT_SECOND_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK,  5) \
                                          X(INT_THIRD_DATAPOINT,     DATAPOINT_FLAG_NVM_MASK,  1) \
                                          X(INT_FOURTH_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK,  2)

/**
 * @brief   Multi-state datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_MULTI_STATE_DATAPOINTS  X(MULTI_STATE_FIRST_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 0) \
                                          X(MULTI_STATE_SECOND_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, 1) \
                                          X(MULTI_STATE_THIRD_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 2) \
                                          X(MULTI_STATE_FOURTH_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, 3)

/**
 * @brief   Unsigned integer datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_UINT_DATAPOINTS         X(UINT_FIRST_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK, 0) \
                                          X(UINT_SECOND_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, 1) \
                                          X(UINT_THIRD_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK, 2) \
                                          X(UINT_FOURTH_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, 3)

#endif    /* DATASTORE_META */

/** @} */

