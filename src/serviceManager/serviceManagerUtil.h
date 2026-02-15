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
 * @brief Initialize the hardware watchdog.
 *
 * @return 0 on success, negative errno code on failure.
 */
int serviceMngrUtilInitHardWdg(void);

#endif /* SERVICE_MANAGER_UTIL_H */

/** @} */
