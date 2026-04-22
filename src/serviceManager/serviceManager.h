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
 * @defgroup serviceManager Service Manager
 *
 * @{
 */

#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <zephyr/kernel.h>

/**
 * @brief Service state values.
 */
typedef enum
{
  SVC_STATE_STOPPED,      /**< Thread created but not started */
  SVC_STATE_RUNNING,      /**< Thread is active */
  SVC_STATE_SUSPENDED     /**< Thread is suspended */
} ServiceState_t;

/**
 * @brief Service priority levels.
 */
typedef enum
{
  SVC_PRIORITY_CRITICAL = 0,  /**< Critical services (must run) */
  SVC_PRIORITY_CORE,          /**< Core services */
  SVC_PRIORITY_APPLICATION,   /**< Application services */
  SVC_PRIORITY_COUNT
} ServicePriority_t;

/**
 * @brief Service descriptor structure.
 */
typedef struct
{
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
 *          Initializes the hardware watchdog, service registry, and starts
 *          the service manager thread. All configuration is driven by
 *          Kconfig symbols.
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

/**
 * @brief   Start all registered services in priority order.
 *
 *          Iterates through priority levels from critical to application,
 *          starting each registered service in order.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerStartAll(void);

/**
 * @brief   Request a service to start asynchronously.
 *
 * @param[in]   threadId: The thread ID of the target service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerRequestStart(k_tid_t threadId);

/**
 * @brief   Request a service to stop asynchronously.
 *
 * @param[in]   threadId: The thread ID of the target service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerRequestStop(k_tid_t threadId);

/**
 * @brief   Request a service to suspend asynchronously.
 *
 * @param[in]   threadId: The thread ID of the target service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerRequestSuspend(k_tid_t threadId);

/**
 * @brief   Request a service to resume asynchronously.
 *
 * @param[in]   threadId: The thread ID of the target service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerRequestResume(k_tid_t threadId);

/**
 * @brief   Confirm a state change for a service.
 *
 *          Called by the service itself after completing a STOP or SUSPEND state
 *          change. Must be called before k_thread_suspend() for SUSPEND, and
 *          before exiting the thread loop for STOP.
 *
 * @param[in]   threadId: The thread ID of the service confirming its state.
 * @param[in]   state: The new confirmed state.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerConfirmState(k_tid_t threadId, ServiceState_t state);

/**
 * @brief   Update the heartbeat timestamp for a service.
 *
 *          Called by a service to signal it is still alive.
 *
 * @param[in]   threadId: The thread ID of the service updating its heartbeat.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceManagerUpdateHeartbeat(k_tid_t threadId);

#endif /* SERVICE_MANAGER_H */

/** @} */
