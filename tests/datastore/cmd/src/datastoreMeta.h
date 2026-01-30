/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreMeta.h
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Test Metadata
 *
 *            Test datapoint definitions for datastore command tests.
 *
 * @ingroup   datastore
 *
 * @{
 */

#ifndef DATASTORE_META_H
#define DATASTORE_META_H

#include "datastoreTypes.h"

/* Multi-state enums not needed for test */

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
#define DATASTORE_BUTTON_DATAPOINTS       X(BUTTON_FIRST_DATAPOINT,  DATAPOINT_NO_FLAG_MASK, BUTTON_UNPRESSED) \
                                          X(BUTTON_SECOND_DATAPOINT, DATAPOINT_NO_FLAG_MASK, BUTTON_UNPRESSED)

/**
 * @brief   Float datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_FLOAT_DATAPOINTS        X(FLOAT_FIRST_DATAPOINT,   DATAPOINT_NO_FLAG_MASK, 0.0f) \
                                          X(FLOAT_SECOND_DATAPOINT,  DATAPOINT_NO_FLAG_MASK, 0.0f)

/**
 * @brief   Signed integer datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_INT_DATAPOINTS          X(INT_FIRST_DATAPOINT,     DATAPOINT_NO_FLAG_MASK, 0) \
                                          X(INT_SECOND_DATAPOINT,    DATAPOINT_NO_FLAG_MASK, 0)

/**
 * @brief   Multi-state datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_MULTI_STATE_DATAPOINTS  X(MULTI_STATE_FIRST_DATAPOINT,  DATAPOINT_NO_FLAG_MASK, 0) \
                                          X(MULTI_STATE_SECOND_DATAPOINT, DATAPOINT_NO_FLAG_MASK, 0)

/**
 * @brief   Unsigned integer datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_UINT_DATAPOINTS         X(UINT_FIRST_DATAPOINT,    DATAPOINT_NO_FLAG_MASK, 0) \
                                          X(UINT_SECOND_DATAPOINT,   DATAPOINT_NO_FLAG_MASK, 0)

#endif    /* DATASTORE_META_H */

/** @} */
