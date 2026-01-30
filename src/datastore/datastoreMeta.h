/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreMeta.h
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Meta Data
 *
 *            Datastore service meta value definition.
 *
 * @ingroup   datastore
 *
 * @{
 */

#ifndef DATASTORE_META
#define DATASTORE_META

#include <zephyr/kernel.h>

#include "serviceCommon.h"

#define DATASTORE_LOGGER_NAME datastore

/**
 * @brief   The message count in the datastore queue.
 */
#define DATASTORE_MSG_COUNT                                       (10)

/**
 * @brief   Datapoint no option flags.
 */
#define DATAPOINT_NO_FLAG_MASK                                    (0 << 0)

/**
 * @brief   Datapoint in NVM flag mask.
 */
#define DATAPOINT_FLAG_NVM_MASK                                   (1 << 0)

/**
 * @brief   First multi-state states.
 */
typedef enum
{
  MULTI_STATE_FIRST_STATE_1 = 0,
  MULTI_STATE_FIRST_STATE_2,
  MULTI_STATE_FIRST_STATE_3,
  MULTI_STATE_FIRST_STATE_4,
  MULTI_STATE_FIRST_STATE_COUNT
} MultiStateFirstStates_t;

/**
 * @brief   Second multi-state states.
 */
typedef enum
{
  MULTI_STATE_SECOND_STATE_1 = 0,
  MULTI_STATE_SECOND_STATE_2,
  MULTI_STATE_SECOND_STATE_3,
  MULTI_STATE_SECOND_STATE_4,
  MULTI_STATE_SECOND_STATE_COUNT
} MultiStateSecondStates_t;

/**
 * @brief   Third multi-state states.
 */
typedef enum
{
  MULTI_STATE_THIRD_STATE_1 = 0,
  MULTI_STATE_THIRD_STATE_2,
  MULTI_STATE_THIRD_STATE_3,
  MULTI_STATE_THIRD_STATE_4,
  MULTI_STATE_THIRD_STATE_COUNT
} MultiStateThirdStates_t;

/**
 * @brief   Fourth multi-state states.
 */
typedef enum
{
  MULTI_STATE_FOURTH_STATE_1 = 0,
  MULTI_STATE_FOURTH_STATE_2,
  MULTI_STATE_FOURTH_STATE_3,
  MULTI_STATE_FOURTH_STATE_4,
  MULTI_STATE_FOURTH_STATE_COUNT
} MultiStateFourthStates_t;

typedef enum __attribute__((mode(SI)))
{
  BUTTON_UNPRESSED = 0,
  BUTTON_SHORT_PRESSED,
  BUTTON_LONG_PRESSED,
  BUTTON_STATE_COUNT
} ButtonState_t;

/**
 * @brief   Datapoint type.
 */
typedef enum
{
  DATAPOINT_BINARY = 0,
  DATAPOINT_BUTTON,
  DATAPOINT_FLOAT,
  DATAPOINT_INT,
  DATAPOINT_MULTI_STATE,
  DATAPOINT_UINT,
  DATAPOINT_TYPE_COUNT,
} DatapointType_t;

/**
 * @brief   Datastore datapoint.
 */
typedef struct
{
  Data_t value;                   /**< The value. */
  uint32_t flags;                 /**< The datapoint flags. */
} Datapoint_t;

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
#define DATASTORE_FLOAT_DATAPOINTS        X(FLOAT_FIRST_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, (float)CONFIG_FIRST_FLOAT_DEFAULT_VAL/10.0f) \
                                          X(FLOAT_SECOND_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 1.0f) \
                                          X(FLOAT_THIRD_DATAPOINT,   DATAPOINT_FLAG_NVM_MASK, 2.0f) \
                                          X(FLOAT_FOURTH_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, 3.0f)

/**
 * @brief   signed integer datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_INT_DATAPOINTS          X(INT_FIRST_DATAPOINT,     DATAPOINT_FLAG_NVM_MASK,  0) \
                                          X(INT_SECOND_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK,  CONFIG_SECOND_INT_DEFAULT_VAL) \
                                          X(INT_THIRD_DATAPOINT,     DATAPOINT_FLAG_NVM_MASK,  1) \
                                          X(INT_FOURTH_DATAPOINT,    DATAPOINT_FLAG_NVM_MASK,  2)

/**
 * @brief   Multi-state datapoint information X-macro.
 * @note    X(datapoint ID, option flag, default value)
 */
#define DATASTORE_MULTI_STATE_DATAPOINTS  X(MULTI_STATE_FIRST_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, MULTI_STATE_FIRST_STATE_2) \
                                          X(MULTI_STATE_SECOND_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, MULTI_STATE_SECOND_STATE_4) \
                                          X(MULTI_STATE_THIRD_DATAPOINT,  DATAPOINT_FLAG_NVM_MASK, MULTI_STATE_THIRD_STATE_1) \
                                          X(MULTI_STATE_FOURTH_DATAPOINT, DATAPOINT_FLAG_NVM_MASK, MULTI_STATE_FOURTH_STATE_3)

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
