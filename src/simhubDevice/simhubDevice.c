/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevice.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Service
 *
 *            Service thread, CDC UART async RX loop, packet dispatch, and
 *            service manager lifecycle callbacks.
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
#define SIMHUB_DEV_RX_TIMEOUT_US 1000
#define SIMHUB_DEV_CTRL_POLL_MS  100

typedef struct
{
  const struct device *uart;
  struct led_rgb frame[SIMHUB_LED_COUNT];
} SimhubDeviceCtx_t;

static SimhubDeviceCtx_t ctx;

static uint8_t rxBuf0[SIMHUB_DEV_RX_BUF_SIZE];
static uint8_t rxBuf1[SIMHUB_DEV_RX_BUF_SIZE];
static uint8_t *rxNextBuf = rxBuf1;

RING_BUF_DECLARE(rxRingBuf, SIMHUB_DEV_RING_BUF_SIZE);

static K_SEM_DEFINE(rxSem, 0, 1);
static K_SEM_DEFINE(txSem, 1, 1);

K_MSGQ_DEFINE(simhubDevCtrlQueue, sizeof(ServiceCtrlMsg_t), 4, 4);

K_THREAD_STACK_DEFINE(simhubDevStack, CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE);
static struct k_thread thread;

static void uartCallback(const struct device *dev, struct uart_event *evt, void *userData)
{
  switch(evt->type)
  {
    case UART_TX_DONE:
      k_sem_give(&txSem);
      break;

    case UART_RX_RDY:
      ring_buf_put(&rxRingBuf, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len);
      k_sem_give(&rxSem);
      break;

    case UART_RX_BUF_REQUEST:
      uart_rx_buf_rsp(dev, rxNextBuf, SIMHUB_DEV_RX_BUF_SIZE);
      break;

    case UART_RX_BUF_RELEASED:
      rxNextBuf = evt->data.rx_buf.buf;
      break;

    default:
      break;
  }
}

static void dispatchPkt(void)
{
  static uint8_t txBuf[SIMHUB_DEV_TX_BUF_SIZE];
  int len;
  struct led_rgb *frame;

  switch(simhubDevUtilGetPktType())
  {
    case SIMHUB_PKT_PROTO:
      len = simhubDevUtilProcessProto(txBuf, sizeof(txBuf));
      if(len > 0)
      {
        k_sem_take(&txSem, K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
        uart_tx(ctx.uart, txBuf, len, SYS_FOREVER_US);
      }
      break;

    case SIMHUB_PKT_LED_COUNT:
      len = simhubDevUtilProcessLedCount(txBuf, sizeof(txBuf));
      if(len > 0)
      {
        k_sem_take(&txSem, K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
        uart_tx(ctx.uart, txBuf, len, SYS_FOREVER_US);
      }
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

static int rxEnable(void)
{
  rxNextBuf = rxBuf1;
  ring_buf_reset(&rxRingBuf);
  return uart_rx_enable(ctx.uart, rxBuf0, sizeof(rxBuf0), SIMHUB_DEV_RX_TIMEOUT_US);
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

  err = uart_callback_set(ctx.uart, uartCallback, NULL);
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set UART callback", err);
    return;
  }

  uint32_t dtr = 0;
  while(!dtr)
  {
    uart_line_ctrl_get(ctx.uart, UART_LINE_CTRL_DTR, &dtr);
    k_sleep(K_MSEC(SIMHUB_DEV_CTRL_POLL_MS));
  }

  err = rxEnable();
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to enable UART RX", err);
    return;
  }

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
          uart_rx_disable(ctx.uart);
          simhubDevUtilReset();
          serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
          return;

        case SVC_CTRL_SUSPEND:
          uart_rx_disable(ctx.uart);
          simhubDevUtilReset();
          serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
          k_thread_suspend(k_current_get());
          err = rxEnable();
          if(err < 0)
            LOG_ERR("ERROR %d: unable to re-enable UART RX after resume", err);
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
