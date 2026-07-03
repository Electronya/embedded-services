/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevice.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Service
 *
 *            Service thread, CDC UART IRQ-driven RX/TX loop, packet dispatch,
 *            and service manager lifecycle callbacks.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#include "ledStrip.h"
#include "serviceCommon.h"
#include "serviceManager.h"
#include "simhubDevUtil.h"

LOG_MODULE_REGISTER(simhubDevice, CONFIG_ENYA_SIMHUB_DEVICE_LOG_LEVEL);

#define SIMHUB_DEV_RX_BUF_SIZE   64
#define SIMHUB_DEV_RING_BUF_SIZE 256
#define SIMHUB_DEV_TX_BUF_SIZE   (SIMHUB_PROTO_RES_MAX_SIZE + 4)
#define SIMHUB_DEV_CTRL_POLL_MS  100

typedef struct
{
  const struct device *uart;
  struct led_rgb frame[SIMHUB_LED_COUNT];
} SimhubDeviceCtx_t;

static SimhubDeviceCtx_t ctx;

RING_BUF_DECLARE(rxRingBuf, SIMHUB_DEV_RING_BUF_SIZE);

static K_SEM_DEFINE(rxSem, 0, 1);
static K_SEM_DEFINE(txSem, 1, 1);

static uint8_t txBuf[SIMHUB_DEV_TX_BUF_SIZE];
static const uint8_t *txPtr = NULL;
static size_t txRemaining = 0;

K_MSGQ_DEFINE(simhubDevCtrlQueue, sizeof(ServiceCtrlMsg_t), 4, 4);

K_THREAD_STACK_DEFINE(simhubDevStack, CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE);
static struct k_thread thread;

static void uartCallback(const struct device *dev, void *userData)
{
  ARG_UNUSED(userData);
  uint8_t buf[SIMHUB_DEV_RX_BUF_SIZE];
  int n;

  uart_irq_update(dev);

  if(uart_irq_rx_ready(dev))
  {
    n = uart_fifo_read(dev, buf, sizeof(buf));
    if(n > 0)
    {
      ring_buf_put(&rxRingBuf, buf, (uint32_t)n);
      k_sem_give(&rxSem);
    }
  }

  if(uart_irq_tx_ready(dev))
  {
    if(txRemaining > 0)
    {
      n = uart_fifo_fill(dev, txPtr, txRemaining);
      txPtr += n;
      txRemaining -= (size_t)n;
    }
    if(txRemaining == 0)
    {
      uart_irq_tx_disable(dev);
      k_sem_give(&txSem);
    }
  }
}

static void dispatchPkt(void)
{
  int len;
  struct led_rgb *frame;

  switch(simhubDevUtilGetPktType())
  {
    case SIMHUB_PKT_PROTO:
      k_sem_take(&txSem, K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
      len = simhubDevUtilProcessProto(txBuf, sizeof(txBuf));
      if(len > 0)
      {
        txPtr = txBuf;
        txRemaining = (size_t)len;
        uart_irq_tx_enable(ctx.uart);
      } else
        k_sem_give(&txSem);
      break;

    case SIMHUB_PKT_LED_COUNT:
      k_sem_take(&txSem, K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
      len = simhubDevUtilProcessLedCount(txBuf, sizeof(txBuf));
      if(len > 0)
      {
        txPtr = txBuf;
        txRemaining = (size_t)len;
        uart_irq_tx_enable(ctx.uart);
      } else
        k_sem_give(&txSem);
      break;

    case SIMHUB_PKT_UNLOCK:
      simhubDevUtilProcessUnlock();
      break;

    case SIMHUB_PKT_LED_DATA:
      frame = ledStripGetNextFramebuffer();
      if(frame != NULL)
      {
        simhubDevUtilProcessLedData(frame);
        ledStripUpdateFrame(frame);
      } else
      {
        LOG_WRN("no framebuffer available, discarding LED data");
        simhubDevUtilReset();
      }
      break;

    default:
      LOG_WRN("unknown packet type, resetting parser");
      simhubDevUtilReset();
      break;
  }
}

static void rxEnable(void)
{
  ring_buf_reset(&rxRingBuf);
  uart_irq_rx_enable(ctx.uart);
}

static void rxDisable(void)
{
  uart_irq_rx_disable(ctx.uart);
}

#ifdef CONFIG_ZTEST
#ifndef SIMHUB_DEV_RUN_ITERATIONS
#define SIMHUB_DEV_RUN_ITERATIONS 1
#endif
#endif

static void run(void *p1, void *p2, void *p3)
{
  int err;
  ServiceCtrlMsg_t ctrlMsg;
  uint8_t byte;

  err = simhubDevUtilInit();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to initialize protocol parser", err);
    return;
  }

  uart_irq_callback_user_data_set(ctx.uart, uartCallback, NULL);

  uint32_t dtr = 0;
  while(!dtr)
  {
    if(k_msgq_get(&simhubDevCtrlQueue, &ctrlMsg, K_NO_WAIT) == 0 && ctrlMsg == SVC_CTRL_STOP)
    {
      serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
      return;
    }
    err = uart_line_ctrl_get(ctx.uart, UART_LINE_CTRL_DTR, &dtr);
    if(err < 0)
      LOG_WRN("DTR not asserted");
    k_sleep(K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
    serviceManagerUpdateHeartbeat(k_current_get());
  }

  rxEnable();

  LOG_INF("SimHub device thread started");

#ifdef CONFIG_ZTEST
  for(size_t i = 0; i < SIMHUB_DEV_RUN_ITERATIONS; ++i)
#else
  for(;;)
#endif
  {
    if(k_msgq_get(&simhubDevCtrlQueue, &ctrlMsg, K_NO_WAIT) == 0)
    {
      switch(ctrlMsg)
      {
        case SVC_CTRL_STOP:
          rxDisable();
          simhubDevUtilReset();
          serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
          return;

        case SVC_CTRL_SUSPEND:
          rxDisable();
          simhubDevUtilReset();
          serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
          k_thread_suspend(k_current_get());
          rxEnable();
          break;

        default:
          LOG_WRN("unknown control message %d", ctrlMsg);
          break;
      }
    }

    k_sem_take(&rxSem, K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));

    while(ring_buf_get(&rxRingBuf, &byte, 1) == 1)
    {
      if(simhubDevUtilReceivedPkt(byte))
        dispatchPkt();
    }

    serviceManagerUpdateHeartbeat(k_current_get());
  }
}

static int onStart(void)
{
  k_thread_start(&thread);
  return 0;
}

static int onStop(void)
{
  int err;
  ServiceCtrlMsg_t msg = SVC_CTRL_STOP;

  err = k_msgq_put(&simhubDevCtrlQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue SimHub stop message", err);

  return err;
}

static int onSuspend(void)
{
  int err;
  ServiceCtrlMsg_t msg = SVC_CTRL_SUSPEND;

  err = k_msgq_put(&simhubDevCtrlQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue SimHub suspend message", err);

  return err;
}

static int onResume(void)
{
  k_thread_resume(&thread);
  return 0;
}

int simhubDeviceInit(void)
{
  int err;
  k_tid_t threadId;
  ServiceDescriptor_t descriptor = {
    .priority = CONFIG_ENYA_SIMHUB_DEVICE_SERVICE_PRIORITY,
    .heartbeatIntervalMs = CONFIG_ENYA_SIMHUB_DEVICE_HEARTBEAT_INTERVAL_MS,
    .start = onStart,
    .stop = onStop,
    .suspend = onSuspend,
    .resume = onResume,
  };

  ctx.uart = DEVICE_DT_GET(DT_ALIAS(simhub_uart));
  if(!device_is_ready(ctx.uart))
  {
    LOG_ERR("ERROR: SimHub UART device not ready");
    return -ENODEV;
  }

  threadId = k_thread_create(&thread, simhubDevStack, CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE, run, NULL, NULL, NULL,
                             K_PRIO_PREEMPT(CONFIG_ENYA_SIMHUB_DEVICE_THREAD_PRIORITY), 0, K_FOREVER);

  err = k_thread_name_set(threadId, "simhubDevice");
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set SimHub device thread name", err);
    return err;
  }

  descriptor.threadId = threadId;

  err = serviceManagerRegisterSrv(&descriptor);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to register SimHub device service", err);

  return err;
}

/** @} */
