/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreTypes.h
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Common Types
 *
 *            Datastore service common type definitions.
 *
 * @ingroup   datastore
 *
 * @{
 */

#ifndef DATASTORE_TYPES_H
#define DATASTORE_TYPES_H

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
 * @brief   Button state enumeration.
 */
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

#endif    /* DATASTORE_TYPES_H */

/** @} */
