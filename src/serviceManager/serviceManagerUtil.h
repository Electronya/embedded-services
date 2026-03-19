/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      serviceManagerUtil.h
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Utilities
 *
 *            Utility functions for the service manager module.
 *
 * @ingroup   serviceManager
 * @{
 */

#ifndef SERVICE_MANAGER_UTIL_H
#define SERVICE_MANAGER_UTIL_H

/**
 * @brief   Initialize the hardware watchdog.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilInitHardWdg(void);

/**
 * @brief   Initialize the service registry.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilInitSrvRegistry(void);

/**
 * @brief   Add a service to the registry.
 *
 * @param[in]   descriptor: The service descriptor to add.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilAddSrvToRegistry(const ServiceDescriptor_t *descriptor);

/**
 * @brief   Get a registry entry by index.
 *
 * @param[in]   index: The index of the registry entry to retrieve.
 *
 * @return  Pointer to the service descriptor if successful, NULL otherwise.
 */
ServiceDescriptor_t *serviceMngrUtilGetRegEntryByIndex(size_t index);

/**
 * @brief   Get the registry index from a thread ID.
 *
 * @param[in]   threadId: The thread ID to search for.
 *
 * @return  The index if found, error code otherwise.
 */
int serviceMngrUtilGetIndexFromId(k_tid_t threadId);

/**
 * @brief   Start a service by registry index.
 *
 * @param[in]   index: The registry index of the service to start.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilStartService(size_t index);

/**
 * @brief   Stop a service by registry index.
 *
 * @param[in]   index: The registry index of the service to stop.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilStopService(size_t index);

/**
 * @brief   Suspend a service by registry index.
 *
 * @param[in]   index: The registry index of the service to suspend.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilSuspendService(size_t index);

/**
 * @brief   Resume a service by registry index.
 *
 * @param[in]   index: The registry index of the service to resume.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilResumeService(size_t index);

/**
 * @brief   Set the state of a service in the registry.
 *
 * @param[in]   index: The registry index of the service.
 * @param[in]   state: The new state to set.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilSetSrvState(size_t index, ServiceState_t state);

/**
 * @brief   Update the heartbeat timestamp for a service.
 *
 * @param[in]   index: The registry index of the service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilUpdateSrvHeartbeat(size_t index);

/**
 * @brief   Check if a service has missed its heartbeat interval.
 *          Increments the missed heartbeat count if the interval has elapsed.
 *
 * @param[in]   index: The registry index of the service.
 *
 * @return  The missed heartbeat count if successful, the error code otherwise.
 */
int serviceMngrUtilCheckSrvHeartbeat(size_t index);

/**
 * @brief   Feed the hardware watchdog.
 *
 * @return  0 if successful, the error code otherwise.
 */
int serviceMngrUtilFeedHardWdg(void);

#endif /* SERVICE_MANAGER_UTIL_H */

/** @} */
