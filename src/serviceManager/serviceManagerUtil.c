/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      serviceManagerUtil.c
 * @author    jbacon
 * @date      2025-02-15
 * @brief     Service Manager Utilities Implementation
 *
 *            Utility functions implementation for the service manager module.
 *
 * @ingroup   serviceManager
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "serviceManager.h"
#include "serviceManagerUtil.h"

LOG_MODULE_DECLARE(serviceManager, CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL);

#define WDG_NODE DT_ALIAS(watchdog0)

static const struct device *wdgDev = DEVICE_DT_GET(WDG_NODE);

/* Service registry data structures */
static ServiceDescriptor_t serviceRegistry[CONFIG_SVC_MGR_MAX_SERVICES];
static size_t registeredServiceCount = 0;

int serviceMngrUtilInitHardWdg(void)
{
  int err;
  struct wdt_timeout_cfg wdtCfg;

  if(!device_is_ready(wdgDev))
  {
    LOG_ERR("ERROR %d: watchdog device not ready", -ENODEV);
    return -ENODEV;
  }

  wdtCfg.flags = WDT_FLAG_RESET_SOC;
  wdtCfg.window.min = 0U;
  wdtCfg.window.max = CONFIG_SVC_MGR_WDT_TIMEOUT_MS;
  wdtCfg.callback = NULL;

  err = wdt_install_timeout(wdgDev, &wdtCfg);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to install watchdog timeout", err);
    return err;
  }

  err = wdt_setup(wdgDev, WDT_OPT_PAUSE_HALTED_BY_DBG);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to setup watchdog", err);
    return err;
  }

  LOG_INF("hardware watchdog initialized (timeout: %d ms)", CONFIG_SVC_MGR_WDT_TIMEOUT_MS);

  return 0;
}

int serviceMngrUtilInitSrvRegistry(void)
{
  /* Clear the service registry */
  memset(serviceRegistry, 0, sizeof(serviceRegistry));
  registeredServiceCount = 0;

  LOG_INF("service registry initialized (capacity: %d services)", CONFIG_SVC_MGR_MAX_SERVICES);

  return 0;
}

int serviceMngrUtilAddSrvToRegistry(const ServiceDescriptor_t *descriptor)
{
  /* Validate descriptor pointer */
  if(descriptor == NULL)
  {
    LOG_ERR("ERROR %d: descriptor is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Check if registry is full */
  if(registeredServiceCount >= CONFIG_SVC_MGR_MAX_SERVICES)
  {
    LOG_ERR("ERROR %d: service registry is full", -ENOMEM);
    return -ENOMEM;
  }

  /* Validate descriptor fields */
  if(descriptor->threadId == NULL)
  {
    LOG_ERR("ERROR %d: invalid thread ID", -EINVAL);
    return -EINVAL;
  }

  if(descriptor->priority >= SVC_PRIORITY_COUNT)
  {
    LOG_ERR("ERROR %d: invalid priority level", -EINVAL);
    return -EINVAL;
  }

  if(descriptor->heartbeatIntervalMs == 0)
  {
    LOG_ERR("ERROR %d: invalid heartbeat interval", -EINVAL);
    return -EINVAL;
  }

  /* Add service to registry */
  serviceRegistry[registeredServiceCount] = *descriptor;
  serviceRegistry[registeredServiceCount].missedHeartbeats = 0;

  registeredServiceCount++;

  LOG_INF("service registered (thread: %p, priority: %d, heartbeat: %d ms, total: %d)",
          descriptor->threadId, descriptor->priority, descriptor->heartbeatIntervalMs,
          registeredServiceCount);

  return 0;
}

/** @} */
