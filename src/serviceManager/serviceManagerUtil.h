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

#endif /* SERVICE_MANAGER_UTIL_H */

/** @} */
