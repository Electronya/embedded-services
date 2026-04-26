/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStrip.c
 * @author    jbacon
 * @date      2026-04-24
 * @brief     LED Strip Updater
 *
 *            LED strip updater service.
 *
 * @ingroup ledStrip LED Strip Updater
 *
 * @{
 */

#include <zephyr/logging/log.h>

#include "ledStrip.h"
#include "ledStripUtil.h"
#include "serviceManager.h"

/* Setting module logging */
LOG_MODULE_REGISTER(LED_STRIP_LOGGER_NAME, CONFIG_ENYA_LED_STRIP_LOG_LEVEL);

/**
 * @brief The thread stack.
 */
K_THREAD_STACK_DEFINE(ledStripStack, CONFIG_ENYA_LED_STRIP_STACK_SIZE);

/**
 * @brief   The service message queue size.
 */
#define LED_STRIP_MSG_QUEUE_SIZE 5

/**
 * @brief   The framerate.
 */
#define LED_STRIP_FRAME_RATE (1000 / CONFIG_ENYA_LED_STRIP_REFRESH_RATE_HZ)
/**
 * @brief   The service message types.
 */
typedef enum
{
  LED_STRIP_STOP_MSG = 0,
  LED_STRIP_SUSPEND_MSG,
  LED_STRIP_NEW_FRAME_MSG,
  LED_STRIP_BRIGHTNESS_MSG,
  LED_STIP_MGS_TYPE_COUNT
} LedStipMsgType_t;

/**
 * @brief   The service message.
 */
typedef struct
{
  LedStipMsgType_t type;
  uint8_t brightness;
  LedPixel_t *framebuffer;
} LedStripMessage_t;

/**
 * @brief   The LED strip thread.
 */
static struct k_thread thread;

/**
 * @brief   The service state.
 */
static ServiceState_t state = SVC_STATE_STOPPED;

/**
 * @brief   The service message queue.
 */
K_MSGQ_DEFINE(ledStipMsgQueue, sizeof(LedStripMessage_t), LED_STRIP_MSG_QUEUE_SIZE, 4);

/**
 * @brief   The frame timer.
 */
K_TIMER_DEFINE(frameTimer, NULL, NULL);

/**
 * @brief   LED strip updater thread.
 *
 * @param[in]   p1: Thread first parameter.
 * @param[in]   p2: Thread second parameter.
 * @param[in]   p3: Thread third parameter.
 */
static void run(void *p1, void *p2, void *p3)
{
  int err;
  LedStripMessage_t msg;

  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  state = SVC_STATE_RUNNING;
  err   = serviceManagerConfirmState(k_current_get(), state);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to confirm service state", err);

  k_timer_start(&frameTimer, K_USEC(LED_STRIP_FRAME_RATE), K_USEC(LED_STRIP_FRAME_RATE));

#ifdef LED_STRIP_RUN_ITERATIONS
  for(size_t i = 0; i < LED_STRIP_RUN_ITERATIONS; ++i)
#else
  for(;;)
#endif
  {
    k_timer_status_sync(&frameTimer);

    while(k_msgq_get(&ledStipMsgQueue, &msg, K_NO_WAIT) == 0)
    {
      switch(msg.type)
      {
        case LED_STRIP_NEW_FRAME_MSG:
          ledStripUtilActivateFrame(msg.framebuffer);
          break;
        case LED_STRIP_STOP_MSG:
          state = SVC_STATE_STOPPED;
          k_timer_stop(&frameTimer);
          err = serviceManagerConfirmState(k_current_get(), state);
          if(err < 0)
            LOG_ERR("ERROR %d: unable to confirme stopped state", err);
          k_thread_abort(k_current_get());
          break;
        case LED_STRIP_SUSPEND_MSG:
          state = SVC_STATE_SUSPENDED;
          k_timer_stop(&frameTimer);
          err = serviceManagerConfirmState(k_current_get(), state);
          if(err < 0)
            LOG_ERR("ERROR %d: unable to confirm the suspended state", err);
          k_thread_suspend(k_current_get());
          break;
        case LED_STRIP_BRIGHTNESS_MSG:
          ledStripUtilSetBrightness(msg.brightness);
          break;
        default:
          err = -ENOTSUP;
          LOG_ERR("ERROR %d: unsupported message type %d", err, msg.type);
      }
    }

    err = ledStripUtilPushFrame();
    if(err < 0)
      LOG_ERR("ERROR %d: failed to push active frame", err);

    err = serviceManagerUpdateHeartbeat(k_current_get());
    if(err < 0)
      LOG_ERR("ERROR %d: unable to update heartbeat", err);
  }
}

/**
 * @brief   Service start callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStart(void)
{
  k_thread_start(&thread);

  return 0;
}

/**
 * @brief   Service stop callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStop(void)
{
  int err;
  LedStripMessage_t msg = {.type = LED_STRIP_STOP_MSG};

  err = k_msgq_put(&ledStipMsgQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to queue the stop message", err);

  return err;
}

/**
 * @brief   Service suspend callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onSuspend(void)
{
  int err;
  LedStripMessage_t msg = {.type = LED_STRIP_SUSPEND_MSG};

  err = k_msgq_put(&ledStipMsgQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to queue the suspend message", err);

  return err;
}

/**
 * @brief   Service resume callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onResume(void)
{
  k_timer_start(&frameTimer, K_USEC(LED_STRIP_FRAME_RATE), K_USEC(LED_STRIP_FRAME_RATE));
  k_thread_resume(&thread);

  return 0;
}

int ledStripInit(void)
{
  int err;
  k_tid_t threadId;
  ServiceDescriptor_t descriptior = {.priority            = CONFIG_ENYA_LED_STRIP_SERVICE_PRIORITY,
                                     .heartbeatIntervalMs = CONFIG_ENYA_LED_STRIP_HEARTBEAT_INTERVAL_MS,
                                     .start               = onStart,
                                     .stop                = onStop,
                                     .suspend             = onSuspend,
                                     .resume              = onResume};

  err = ledStripUtilInitStrip();
  if(err < 0)
    return err;

  err = ledStripUtilInitFramebuffers();
  if(err < 0)
    return err;

  threadId = k_thread_create(&thread, ledStripStack, CONFIG_ENYA_LED_STRIP_STACK_SIZE, run, NULL, NULL, NULL,
                             CONFIG_ENYA_LED_STRIP_THREAD_PRIORITY, 0, K_NO_WAIT);

  err = k_thread_name_set(threadId, STRINGIFY(LED_STRIP_LOGGER_NAME));
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set thread name", err);
    return err;
  }

  descriptior.threadId = threadId;

  err = serviceManagerRegisterSrv(&descriptior);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to register service", err);

  return err;
}

/** @} */
