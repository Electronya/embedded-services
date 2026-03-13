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
#define SVC_MGR_MAX_MISSED_HEARTBEATS 3

static const struct device *wdgDev = DEVICE_DT_GET(WDG_NODE);
static int wdgChannel = -1;

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

  wdgChannel = wdt_install_timeout(wdgDev, &wdtCfg);
  if(wdgChannel < 0)
  {
    LOG_ERR("ERROR %d: failed to install watchdog timeout", wdgChannel);
    return wdgChannel;
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
  memcpy(&serviceRegistry[registeredServiceCount], descriptor, sizeof(ServiceDescriptor_t));

  /* Initialize runtime fields */
  serviceRegistry[registeredServiceCount].lastHeartbeatMs = 0;
  serviceRegistry[registeredServiceCount].missedHeartbeats = 0;
  serviceRegistry[registeredServiceCount].state = SVC_STATE_STOPPED;

  registeredServiceCount++;

  LOG_INF("service registered (thread: %p, priority: %d, heartbeat: %d ms, total: %d)",
          descriptor->threadId, descriptor->priority, descriptor->heartbeatIntervalMs,
          registeredServiceCount);

  return 0;
}

ServiceDescriptor_t *serviceMngrUtilGetRegEntryByIndex(size_t index)
{
  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return NULL;
  }

  return &serviceRegistry[index];
}

int serviceMngrUtilGetIndexFromId(k_tid_t threadId)
{
  /* Validate thread ID */
  if(threadId == NULL)
  {
    LOG_ERR("ERROR %d: thread ID is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Search for thread ID in registry */
  for(size_t i = 0; i < registeredServiceCount; i++)
  {
    if(serviceRegistry[i].threadId == threadId)
    {
      return (int)i;
    }
  }

  LOG_ERR("ERROR %d: thread ID not found in registry", -ENOENT);
  return -ENOENT;
}

int serviceMngrUtilStartService(size_t index)
{
  int err;

  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Validate start callback */
  if(serviceRegistry[index].start == NULL)
  {
    LOG_ERR("ERROR %d: start callback is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Call start callback */
  err = serviceRegistry[index].start();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: service start callback failed", err);
    return err;
  }

  /* Update service state */
  serviceRegistry[index].state = SVC_STATE_RUNNING;

  LOG_INF("service started (index: %zu)", index);

  return 0;
}

int serviceMngrUtilStopService(size_t index)
{
  int err;

  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Validate stop callback */
  if(serviceRegistry[index].stop == NULL)
  {
    LOG_ERR("ERROR %d: stop callback is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Call stop callback */
  err = serviceRegistry[index].stop();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: service stop callback failed", err);
    return err;
  }

  /* Update service state */
  serviceRegistry[index].state = SVC_STATE_STOPPED;

  LOG_INF("service stopped (index: %zu)", index);

  return 0;
}

int serviceMngrUtilSuspendService(size_t index)
{
  int err;

  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Validate suspend callback */
  if(serviceRegistry[index].suspend == NULL)
  {
    LOG_ERR("ERROR %d: suspend callback is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Call suspend callback */
  err = serviceRegistry[index].suspend();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: service suspend callback failed", err);
    return err;
  }

  /* Update service state */
  serviceRegistry[index].state = SVC_STATE_SUSPENDED;

  LOG_INF("service suspended (index: %zu)", index);

  return 0;
}

int serviceMngrUtilResumeService(size_t index)
{
  int err;

  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Validate resume callback */
  if(serviceRegistry[index].resume == NULL)
  {
    LOG_ERR("ERROR %d: resume callback is NULL", -EINVAL);
    return -EINVAL;
  }

  /* Call resume callback */
  err = serviceRegistry[index].resume();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: service resume callback failed", err);
    return err;
  }

  /* Update service state */
  serviceRegistry[index].state = SVC_STATE_RUNNING;

  LOG_INF("service resumed (index: %zu)", index);

  return 0;
}

int serviceMngrUtilUpdateSrvHeartbeat(size_t index)
{
  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Update heartbeat timestamp and reset missed count */
  serviceRegistry[index].lastHeartbeatMs = k_uptime_get();
  serviceRegistry[index].missedHeartbeats = 0;

  LOG_DBG("heartbeat updated (index: %zu, time: %lld ms)", index,
          serviceRegistry[index].lastHeartbeatMs);

  return 0;
}

int serviceMngrUtilCheckSrvHeartbeat(size_t index)
{
  int64_t elapsed;

  /* Validate index */
  if(index >= registeredServiceCount)
  {
    LOG_ERR("ERROR %d: index out of bounds", -EINVAL);
    return -EINVAL;
  }

  /* Check if heartbeat interval has elapsed */
  elapsed = k_uptime_get() - serviceRegistry[index].lastHeartbeatMs;
  if(elapsed >= serviceRegistry[index].heartbeatIntervalMs)
  {
    serviceRegistry[index].missedHeartbeats++;
    LOG_WRN("missed heartbeat (service: %s, missed: %d)",
            k_thread_name_get(serviceRegistry[index].threadId),
            serviceRegistry[index].missedHeartbeats);
  }

  /* Return error if too many heartbeats missed */
  if(serviceRegistry[index].missedHeartbeats > SVC_MGR_MAX_MISSED_HEARTBEATS)
  {
    LOG_ERR("ERROR %d: service %s heartbeat timeout", -ETIMEDOUT,
            k_thread_name_get(serviceRegistry[index].threadId));
    return -ETIMEDOUT;
  }

  return serviceRegistry[index].missedHeartbeats;
}

int serviceMngrUtilFeedHardWdg(void)
{
  int err;

  err = wdt_feed(wdgDev, wdgChannel);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to feed hardware watchdog", err);
    return err;
  }

  LOG_DBG("hardware watchdog fed");

  return 0;
}

/** @} */
