/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisition.c
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Service
 *
 *            ADC acquisition service API implementation.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#include <zephyr/logging/log.h>

#include "adcAcquisition.h"
#include "adcAcquisitionFilter.h"
#include "adcAcquisitionUtil.h"
#include "serviceCommon.h"
#include "serviceManager.h"

/**
 * @brief   Register logger.
 */
LOG_MODULE_REGISTER(ADC_AQC_SERVICE_NAME, CONFIG_ENYA_ADC_ACQUISITION_LOG_LEVEL);

/**
 * @brief   Defining the ADC thread stack area.
 */
K_THREAD_STACK_DEFINE(adcStack, CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE);

#ifdef CONFIG_ZTEST
#ifndef ADC_ACQ_RUN_ITERATIONS
#define ADC_ACQ_RUN_ITERATIONS 1
#endif
#endif

K_MSGQ_DEFINE(adcAcqCtrlQueue, sizeof(ServiceCtrlMsg_t), 4, 4);

/**
 * @brief   The ADC thread.
 */
static struct k_thread thread;

/**
 * @brief   The ADC acquisition thread.
 *
 * @param[in]   p1: The first parameter.
 * @param[in]   p2: The second parameter.
 * @param[in]   p3: The third parameter.
 */
void run(void *p1, void *p2, void *p3)
{
  int err;
  uint32_t notificationRate = (uint32_t)(uintptr_t)p1;
  ServiceCtrlMsg_t ctrlMsg;

  LOG_INF("ADC acquisition thread started, notification rate: %d ms", notificationRate);

#ifdef CONFIG_ZTEST
  for(size_t i = 0; i < ADC_ACQ_RUN_ITERATIONS; ++i)
#else
  for(;;)
#endif
  {
    if(k_msgq_get(&adcAcqCtrlQueue, &ctrlMsg, K_MSEC(notificationRate)) == 0)
    {
      switch(ctrlMsg)
      {
        case SVC_CTRL_STOP:
          err = adcAcqUtilStopTrigger();
          if(err < 0)
            LOG_ERR("ERROR %d: unable to stop ADC trigger", err);
          serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
          return;
        case SVC_CTRL_SUSPEND:
          err = adcAcqUtilStopTrigger();
          if(err < 0)
            LOG_ERR("ERROR %d: unable to stop ADC trigger", err);
          serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
          k_thread_suspend(k_current_get());
          break;
        default:
          LOG_WRN("unknown ADC control message %d", ctrlMsg);
          break;
      }
    }

    err = adcAcqUtilProcessData();
    if(err < 0)
      LOG_ERR("ERROR %d: unable to process ADC data", err);

    err = adcAcqUtilNotifySubscribers();
    if(err < 0)
      LOG_ERR("ERROR %d: unable to notify ADC subscribers", err);

    serviceManagerUpdateHeartbeat(k_current_get());
  }
}

/**
 * @brief   Start callback: starts the ADC thread and trigger.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStart(void)
{
  int err;

  k_thread_start(&thread);

  err = adcAcqUtilStartTrigger();
  if(err < 0)
    LOG_ERR("ERROR %d: unable to start ADC trigger", err);

  return err;
}

/**
 * @brief   Stop callback: enqueues a stop message to the control queue.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStop(void)
{
  int err;
  ServiceCtrlMsg_t msg = SVC_CTRL_STOP;

  err = k_msgq_put(&adcAcqCtrlQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue ADC stop message", err);

  return err;
}

/**
 * @brief   Suspend callback: enqueues a suspend message to the control queue.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onSuspend(void)
{
  int err;
  ServiceCtrlMsg_t msg = SVC_CTRL_SUSPEND;

  err = k_msgq_put(&adcAcqCtrlQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue ADC suspend message", err);

  return err;
}

/**
 * @brief   Resume callback: resumes the ADC thread and restarts the trigger.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onResume(void)
{
  int err;

  k_thread_resume(&thread);

  err = adcAcqUtilStartTrigger();
  if(err < 0)
    LOG_ERR("ERROR %d: unable to restart ADC trigger", err);

  return err;
}

int adcAcqInit(void)
{
  int err;
  k_tid_t threadId;
  AdcConfig_t adcConfig = {
    .samplingRate = CONFIG_ENYA_ADC_ACQUISITION_SAMPLING_RATE_US,
    .filterTau    = CONFIG_ENYA_ADC_ACQUISITION_FILTER_TAU,
  };
  AdcSubConfig_t adcSubConfig = {
    .maxSubCount      = CONFIG_ENYA_ADC_ACQUISITION_MAX_SUB_COUNT,
    .activeSubCount   = 0,
    .notificationRate = CONFIG_ENYA_ADC_ACQUISITION_NOTIFICATION_RATE_MS,
  };
  ServiceDescriptor_t descriptor = {
    .priority            = CONFIG_ENYA_ADC_ACQUISITION_SERVICE_PRIORITY,
    .heartbeatIntervalMs = CONFIG_ENYA_ADC_ACQUISITION_HEARTBEAT_INTERVAL_MS,
    .start               = onStart,
    .stop                = onStop,
    .suspend             = onSuspend,
    .resume              = onResume,
  };

  err = adcAcqUtilInitAdc(&adcConfig);
  if(err < 0)
    return err;

  err = adcAcqUtilInitSubscriptions(&adcSubConfig);
  if(err < 0)
    return err;

  err = adcAcqFilterInit(adcAcqUtilGetChanCount());
  if(err < 0)
    return err;

  threadId = k_thread_create(&thread, adcStack, CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE, run,
                             (void *)(uintptr_t)CONFIG_ENYA_ADC_ACQUISITION_NOTIFICATION_RATE_MS,
                             NULL, NULL,
                             K_PRIO_PREEMPT(CONFIG_ENYA_ADC_ACQUISITION_THREAD_PRIORITY),
                             0, K_FOREVER);

  err = k_thread_name_set(threadId, STRINGIFY(ADC_AQC_SERVICE_NAME));
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set ADC acquisition thread name", err);
    return err;
  }

  descriptor.threadId = threadId;

  err = serviceManagerRegisterSrv(&descriptor);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to register ADC acquisition service", err);

  return err;
}

int adcAcqSubscribe(AdcSubCallback_t callback)
{
  return adcAcqUtilAddSubscription(callback);
}

int adcAcqUnsubscribe(AdcSubCallback_t callback)
{
  return adcAcqUtilRemoveSubscription(callback);
}

int adcAcqPauseSubscription(AdcSubCallback_t callback)
{
  return adcAcqUtilSetSubPauseState(callback, true);
}

int adcAqcUnpauseSubscription(AdcSubCallback_t callback)
{
  return adcAcqUtilSetSubPauseState(callback, false);
}

/** @} */
