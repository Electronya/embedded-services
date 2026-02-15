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
 * @brief Service state values.
 */
typedef enum {
  SVC_STATE_STOPPED,      /**< Thread created but not started */
  SVC_STATE_RUNNING,      /**< Thread is active */
  SVC_STATE_SUSPENDED     /**< Thread is suspended */
} ServiceState_t;

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
  k_tid_t threadId;                   /**< Service thread ID */
  ServicePriority_t priority;         /**< Service priority level */
  uint32_t heartbeatIntervalMs;       /**< Heartbeat interval in milliseconds */
  int64_t lastHeartbeatMs;            /**< Last heartbeat timestamp */
  uint8_t missedHeartbeats;           /**< Missed heartbeat counter */
  ServiceState_t state;               /**< Current service state */
  int (*start)(void);                 /**< Start notification callback */
  int (*stop)(void);                  /**< Stop notification callback */
  int (*suspend)(void);               /**< Suspend notification callback */
  int (*resume)(void);                /**< Resume notification callback */
} ServiceDescriptor_t;

/**
 * @brief   Initialize the service manager.
 *
 *          Initializes the hardware watchdog and service registry.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerInit(void);

/**
 * @brief   Register a service with the service manager.
 *
 *          Adds a service to the registry for monitoring and lifecycle management.
 *
 * @param[in]   descriptor: The service descriptor to register.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerRegisterSrv(const ServiceDescriptor_t *descriptor);

#endif /* SERVICE_MANAGER_H */

/** @} */
