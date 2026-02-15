/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      serviceManager.h
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager
 *
 *            Service manager module that acts as a virtual watchdog,
 *            controlling hardware watchdog, starting registered services
 *            in priority order, managing service state, and monitoring
 *            service health via heartbeats.
 *
 * @ingroup   serviceManager
 * @{
 */

#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <zephyr/kernel.h>

/**
 * @brief Service priority levels.
 */
typedef enum {
  SVC_PRIORITY_CRITICAL = 0,  /**< Critical services (must run) */
  SVC_PRIORITY_CORE,          /**< Core services */
  SVC_PRIORITY_APPLICATION,   /**< Application services */
  SVC_PRIORITY_COUNT
} ServicePriority_t;

/**
 * @brief Service descriptor structure.
 */
typedef struct {
  k_tid_t threadId;               /**< Service thread ID */
  ServicePriority_t priority;     /**< Service priority level */
  uint32_t heartbeatIntervalMs;   /**< Heartbeat interval in milliseconds */
  uint8_t missedHeartbeats;       /**< Missed heartbeat counter */
} ServiceDescriptor_t;

#endif /* SERVICE_MANAGER_H */

/** @} */
