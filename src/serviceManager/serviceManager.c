/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      serviceManager.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Implementation
 *
 *            Service manager module implementation. Manages service
 *            lifecycle, monitors heartbeats, and controls hardware watchdog.
 *
 * @ingroup   serviceManager
 * @{
 */

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(serviceManager, CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL);

/** @} */
