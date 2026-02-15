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

#include "serviceManager.h"
#include "serviceManagerUtil.h"

LOG_MODULE_REGISTER(serviceManager, CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL);

int serviceManagerInit(void)
{
  int err;

  /* Initialize hardware watchdog */
  err = serviceMngrUtilInitHardWdg();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to initialize hardware watchdog", err);
    return err;
  }

  /* Initialize service registry */
  err = serviceMngrUtilInitSrvRegistry();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to initialize service registry", err);
    return err;
  }

  LOG_INF("service manager initialized");

  return 0;
}

/** @} */
