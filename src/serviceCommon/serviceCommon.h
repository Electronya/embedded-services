/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      serviceCommon.h
 * @author    jbacon
 * @date      2026-01-08
 * @brief     Service Common Definitions
 *
 *            Common data structures and definitions used across all services
 *            for inter-service communication.
 *
 * @defgroup  service-common service-common
 *
 * @{
 */

#ifndef SERVICE_COMMON_H
#define SERVICE_COMMON_H

#include <zephyr/portability/cmsis_os2.h>
#include <stddef.h>
#include <stdint.h>

typedef union
{
  float floatVal;                 /**< Float value. */
  uint32_t uintVal;               /**< unsigned integer/multi-state/button value. */
  int32_t intVal;                 /**< signed integer value. */
} Data_t;

/**
 * @brief   The service message payload data structure.
 *
 *          This structure is used for inter-service communication via message queues.
 *          It is allocated from the producer service's memory pool and passed to
 *          consumers. Consumers must free the memory back to the pool after processing
 *          using the embedded pool ID.
 */
typedef struct
{
  osMemoryPoolId_t poolId;                          /**< Memory pool to return buffer to. */
  size_t dataLen;                                   /**< Actual data length in bytes. */
  Data_t data[];                                    /**< Flexible array of data bytes. */
} SrvMsgPayload_t;

#endif    /* SERVICE_COMMON_H */

/** @} */
