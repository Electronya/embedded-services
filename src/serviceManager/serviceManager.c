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

typedef enum
{
  SVC_MGR_MSG_START,
  SVC_MGR_MSG_STOP,
  SVC_MGR_MSG_SUSPEND,
  SVC_MGR_MSG_RESUME,
} ServiceMgrMsgType_t;

typedef struct
{
  ServiceMgrMsgType_t type;
  size_t index;
} ServiceMgrMsg_t;

K_THREAD_STACK_DEFINE(serviceManagerStack, CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE);
static struct k_thread serviceManagerThread;
K_MSGQ_DEFINE(serviceManagerQueue, sizeof(ServiceMgrMsg_t), CONFIG_SVC_MGR_MAX_SERVICES, 4);

static void run(void *p1, void *p2, void *p3)
{
  size_t index;
  int err;
  bool canFeedWdg;
  ServiceMgrMsg_t msg;
  ServiceDescriptor_t *descriptor;

  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

#ifdef CONFIG_ZTEST
  for(size_t i = 0; i < SVC_MGR_RUN_ITERATIONS; i++)
#else
  for(;;)
#endif
  {
    if(k_msgq_get(&serviceManagerQueue, &msg, K_MSEC(CONFIG_SVC_MGR_LOOP_PERIOD_MS)) == 0)
    {
      switch(msg.type)
      {
        case SVC_MGR_MSG_START:
          err = serviceMngrUtilStartService(msg.index);
          if(err < 0)
            LOG_ERR("ERROR %d: failed to start service %zu", err, msg.index);
        break;
        case SVC_MGR_MSG_STOP:
          err = serviceMngrUtilStopService(msg.index);
          if(err < 0)
            LOG_ERR("ERROR %d: failed to stop service %zu", err, msg.index);
        break;
        case SVC_MGR_MSG_SUSPEND:
          err = serviceMngrUtilSuspendService(msg.index);
          if(err < 0)
            LOG_ERR("ERROR %d: failed to suspend service %zu", err, msg.index);
        break;
        case SVC_MGR_MSG_RESUME:
          err = serviceMngrUtilResumeService(msg.index);
          if(err < 0)
            LOG_ERR("ERROR %d: failed to resume service %zu", err, msg.index);
        break;
        default:
          LOG_WRN("unknown message type %d", msg.type);
        break;
      }
    }

    canFeedWdg = true;

    for(index = 0; (descriptor = serviceMngrUtilGetRegEntryByIndex(index)) != NULL; index++)
    {
      err = serviceMngrUtilCheckSrvHeartbeat(index);
      if(err < 0)
      {
        canFeedWdg = false;
        LOG_ERR("ERROR %d: service %s heartbeat timeout", err,
                k_thread_name_get(descriptor->threadId));
      }
    }

    if(canFeedWdg)
    {
      err = serviceMngrUtilFeedHardWdg();
      if(err < 0)
        LOG_ERR("ERROR %d: failed to feed hardware watchdog", err);
    }
  }
}

static int enqueueRequest(ServiceMgrMsgType_t type, k_tid_t threadId)
{
  int index;
  ServiceMgrMsg_t msg;
  int err;

  index = serviceMngrUtilGetIndexFromId(threadId);
  if(index < 0)
  {
    LOG_ERR("ERROR %d: failed to find service by thread ID", index);
    return index;
  }

  msg.type = type;
  msg.index = (size_t)index;

  err = k_msgq_put(&serviceManagerQueue, &msg, K_NO_WAIT);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to enqueue state change request", err);
    return err;
  }

  return 0;
}

int serviceManagerInit(uint32_t priority, k_tid_t *threadId)
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

  /* Start service manager thread */
  *threadId = k_thread_create(&serviceManagerThread, serviceManagerStack,
                              CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE,
                              run, NULL, NULL, NULL,
                              K_PRIO_PREEMPT(priority), 0, K_FOREVER);
  k_thread_name_set(*threadId, "serviceManager");

  LOG_INF("service manager initialized");

  return 0;
}

int serviceManagerRegisterSrv(const ServiceDescriptor_t *descriptor)
{
  int err;

  /* Add service to registry */
  err = serviceMngrUtilAddSrvToRegistry(descriptor);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: failed to register service", err);
    return err;
  }

  LOG_DBG("service registered successfully");

  return 0;
}

int serviceManagerStartAll(void)
{
  int err;
  ServiceDescriptor_t *descriptor;

  for(ServicePriority_t priority = SVC_PRIORITY_CRITICAL; priority < SVC_PRIORITY_COUNT; priority++)
  {
    for(size_t index = 0; (descriptor = serviceMngrUtilGetRegEntryByIndex(index)) != NULL; index++)
    {
      if(descriptor->priority != priority)
        continue;

      err = serviceMngrUtilStartService(index);
      if(err < 0)
      {
        LOG_ERR("ERROR %d: failed to start service %s", err,
                k_thread_name_get(descriptor->threadId));
        return err;
      }
    }
  }

  LOG_INF("all services started");

  return 0;
}

int serviceManagerRequestStart(k_tid_t threadId)
{
  return enqueueRequest(SVC_MGR_MSG_START, threadId);
}

int serviceManagerRequestStop(k_tid_t threadId)
{
  return enqueueRequest(SVC_MGR_MSG_STOP, threadId);
}

int serviceManagerRequestSuspend(k_tid_t threadId)
{
  return enqueueRequest(SVC_MGR_MSG_SUSPEND, threadId);
}

int serviceManagerRequestResume(k_tid_t threadId)
{
  return enqueueRequest(SVC_MGR_MSG_RESUME, threadId);
}

int serviceManagerUpdateHeartbeat(k_tid_t threadId)
{
  int index;

  index = serviceMngrUtilGetIndexFromId(threadId);
  if(index < 0)
  {
    LOG_ERR("ERROR %d: failed to find service by thread ID", index);
    return index;
  }

  return serviceMngrUtilUpdateSrvHeartbeat(index);
}

/** @} */
