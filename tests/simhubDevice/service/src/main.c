/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Service Tests
 *
 *            Unit tests for the SimHub device service functions.
 */

#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

/* Prevent led_strip driver header — define only the type we need. */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
struct led_rgb
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

/* Stub DEVICE_DT_GET to return a fixed test device pointer. */
static const struct device testUartDevice;
#undef DEVICE_DT_GET
#define DEVICE_DT_GET(...) (&testUartDevice)

/* Prevent serviceCommon header — define types manually. */
#define SERVICE_COMMON_H
typedef enum
{
  SVC_CTRL_STOP,
  SVC_CTRL_SUSPEND,
} ServiceCtrlMsg_t;

/* Prevent serviceManager header — define types manually. */
#define SERVICE_MANAGER_H
typedef enum
{
  SVC_STATE_STOPPED,
  SVC_STATE_RUNNING,
  SVC_STATE_SUSPENDED,
} ServiceState_t;
typedef enum
{
  SVC_PRIORITY_CRITICAL = 0,
  SVC_PRIORITY_CORE,
  SVC_PRIORITY_APPLICATION,
  SVC_PRIORITY_COUNT,
} ServicePriority_t;
typedef struct
{
  k_tid_t threadId;
  ServicePriority_t priority;
  uint32_t heartbeatIntervalMs;
  int64_t lastHeartbeatMs;
  uint8_t missedHeartbeats;
  ServiceState_t state;
  int (*start)(void);
  int (*stop)(void);
  int (*suspend)(void);
  int (*resume)(void);
} ServiceDescriptor_t;

/* Prevent simhubDevUtil header — define types manually. */
#define SIMHUB_DEV_UTIL_H
#define SIMHUB_DEV_PROTO_H
#define SIMHUB_PROTO_RES_MAX_SIZE 12
#define SIMHUB_LED_COUNT          3
typedef enum
{
  SIMHUB_PKT_PROTO = 0,
  SIMHUB_PKT_LED_COUNT,
  SIMHUB_PKT_LED_DATA,
  SIMHUB_PKT_UNLOCK,
  SIMHUB_PKT_TYPE_COUNT,
} SimhubProtoPktType_t;

/* Prevent ledStrip header — define API manually. */
#define LEDSTRIP_H

/* Wrap kernel functions. */
#define k_thread_create   k_thread_create_mock
#define k_thread_name_set k_thread_name_set_mock
#define k_thread_start    k_thread_start_mock
#define k_thread_resume   k_thread_resume_mock
#define k_thread_suspend  k_thread_suspend_mock
#define k_current_get     k_current_get_mock
#define k_msgq_put        k_msgq_put_mock
#define k_msgq_get        k_msgq_get_mock
#define k_sem_take        k_sem_take_mock
#define k_sem_give        k_sem_give_mock
#define k_sleep           k_sleep_mock
#define device_is_ready   device_is_ready_mock

/* Wrap UART IRQ functions. */
#define uart_irq_callback_user_data_set uart_irq_callback_user_data_set_mock
#define uart_irq_rx_enable              uart_irq_rx_enable_mock
#define uart_irq_rx_disable             uart_irq_rx_disable_mock
#define uart_irq_tx_enable              uart_irq_tx_enable_mock
#define uart_irq_tx_disable             uart_irq_tx_disable_mock
#define uart_irq_update                 uart_irq_update_mock
#define uart_irq_rx_ready               uart_irq_rx_ready_mock
#define uart_irq_tx_ready               uart_irq_tx_ready_mock
#define uart_fifo_read                  uart_fifo_read_mock
#define uart_fifo_fill                  uart_fifo_fill_mock
#define uart_line_ctrl_get              uart_line_ctrl_get_mock

/* Wrap ring buffer functions. */
#define ring_buf_put   ring_buf_put_mock
#define ring_buf_get   ring_buf_get_mock
#define ring_buf_reset ring_buf_reset_mock

/* Set test iteration count. */
#define SIMHUB_DEV_RUN_ITERATIONS 1

/* Setup logging before including the module under test. */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);
#undef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)

/* Define Kconfig symbols needed by the module. */
#define CONFIG_ENYA_SIMHUB_DEVICE_LOG_LEVEL             3
#define CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE            1024
#define CONFIG_ENYA_SIMHUB_DEVICE_THREAD_PRIORITY       6
#define CONFIG_ENYA_SIMHUB_DEVICE_SERVICE_PRIORITY      2
#define CONFIG_ENYA_SIMHUB_DEVICE_HEARTBEAT_INTERVAL_MS 1000

/* Kernel mock fakes. */
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *, size_t, k_thread_entry_t, void *, void *,
                void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
FAKE_VOID_FUNC(k_thread_start_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_resume_mock, k_tid_t);
FAKE_VOID_FUNC(k_thread_suspend_mock, k_tid_t);
FAKE_VALUE_FUNC(k_tid_t, k_current_get_mock);
FAKE_VALUE_FUNC(int, k_msgq_put_mock, struct k_msgq *, const void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_msgq_get_mock, struct k_msgq *, void *, k_timeout_t);
FAKE_VALUE_FUNC(int, k_sem_take_mock, struct k_sem *, k_timeout_t);
FAKE_VOID_FUNC(k_sem_give_mock, struct k_sem *);
FAKE_VOID_FUNC(k_sleep_mock, k_timeout_t);
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);

/* UART IRQ mock fakes. */
FAKE_VOID_FUNC(uart_irq_callback_user_data_set_mock, const struct device *, uart_irq_callback_user_data_t, void *);
FAKE_VOID_FUNC(uart_irq_rx_enable_mock, const struct device *);
FAKE_VOID_FUNC(uart_irq_rx_disable_mock, const struct device *);
FAKE_VOID_FUNC(uart_irq_tx_enable_mock, const struct device *);
FAKE_VOID_FUNC(uart_irq_tx_disable_mock, const struct device *);
FAKE_VALUE_FUNC(int, uart_irq_update_mock, const struct device *);
FAKE_VALUE_FUNC(int, uart_irq_rx_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, uart_irq_tx_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, uart_fifo_read_mock, const struct device *, uint8_t *, int);
FAKE_VALUE_FUNC(int, uart_fifo_fill_mock, const struct device *, const uint8_t *, int);
FAKE_VALUE_FUNC(int, uart_line_ctrl_get_mock, const struct device *, enum uart_line_ctrl, uint32_t *);

/* Ring buffer mock fakes. */
FAKE_VALUE_FUNC(uint32_t, ring_buf_put_mock, struct ring_buf *, const uint8_t *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, ring_buf_get_mock, struct ring_buf *, uint8_t *, uint32_t);
FAKE_VOID_FUNC(ring_buf_reset_mock, struct ring_buf *);

/* Service manager mock fakes. */
FAKE_VALUE_FUNC(int, serviceManagerRegisterSrv, const ServiceDescriptor_t *);
FAKE_VALUE_FUNC(int, serviceManagerConfirmState, k_tid_t, ServiceState_t);
FAKE_VALUE_FUNC(int, serviceManagerUpdateHeartbeat, k_tid_t);

/* simhubDevUtil mock fakes. */
FAKE_VALUE_FUNC(int, simhubDevUtilInit);
FAKE_VOID_FUNC(simhubDevUtilReset);
FAKE_VALUE_FUNC(bool, simhubDevUtilReceivedPkt, uint8_t);
FAKE_VALUE_FUNC(SimhubProtoPktType_t, simhubDevUtilGetPktType);
FAKE_VALUE_FUNC(int, simhubDevUtilProcessProto, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, simhubDevUtilProcessLedCount, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, simhubDevUtilProcessUnlock);
FAKE_VALUE_FUNC(int, simhubDevUtilProcessLedData, struct led_rgb *);

/* ledStrip mock fakes. */
FAKE_VALUE_FUNC(struct led_rgb *, ledStripGetNextFramebuffer);
FAKE_VALUE_FUNC(int, ledStripUpdateFrame, struct led_rgb *);

#define FFF_FAKES_LIST(FAKE)                                                                                                       \
  FAKE(k_thread_create_mock)                                                                                                       \
  FAKE(k_thread_name_set_mock)                                                                                                     \
  FAKE(k_thread_start_mock)                                                                                                        \
  FAKE(k_thread_resume_mock)                                                                                                       \
  FAKE(k_thread_suspend_mock)                                                                                                      \
  FAKE(k_current_get_mock)                                                                                                         \
  FAKE(k_msgq_put_mock)                                                                                                            \
  FAKE(k_msgq_get_mock)                                                                                                            \
  FAKE(k_sem_take_mock)                                                                                                            \
  FAKE(k_sem_give_mock)                                                                                                            \
  FAKE(k_sleep_mock)                                                                                                               \
  FAKE(device_is_ready_mock)                                                                                                       \
  FAKE(uart_irq_callback_user_data_set_mock)                                                                                       \
  FAKE(uart_irq_rx_enable_mock)                                                                                                    \
  FAKE(uart_irq_rx_disable_mock)                                                                                                   \
  FAKE(uart_irq_tx_enable_mock)                                                                                                    \
  FAKE(uart_irq_tx_disable_mock)                                                                                                   \
  FAKE(uart_irq_update_mock)                                                                                                       \
  FAKE(uart_irq_rx_ready_mock)                                                                                                     \
  FAKE(uart_irq_tx_ready_mock)                                                                                                     \
  FAKE(uart_fifo_read_mock)                                                                                                        \
  FAKE(uart_fifo_fill_mock)                                                                                                        \
  FAKE(uart_line_ctrl_get_mock)                                                                                                    \
  FAKE(ring_buf_put_mock)                                                                                                          \
  FAKE(ring_buf_get_mock)                                                                                                          \
  FAKE(ring_buf_reset_mock)                                                                                                        \
  FAKE(serviceManagerRegisterSrv)                                                                                                  \
  FAKE(serviceManagerConfirmState)                                                                                                 \
  FAKE(serviceManagerUpdateHeartbeat)                                                                                              \
  FAKE(simhubDevUtilInit)                                                                                                          \
  FAKE(simhubDevUtilReset)                                                                                                         \
  FAKE(simhubDevUtilReceivedPkt)                                                                                                   \
  FAKE(simhubDevUtilGetPktType)                                                                                                    \
  FAKE(simhubDevUtilProcessProto)                                                                                                  \
  FAKE(simhubDevUtilProcessLedCount)                                                                                               \
  FAKE(simhubDevUtilProcessUnlock)                                                                                                 \
  FAKE(simhubDevUtilProcessLedData)                                                                                                \
  FAKE(ledStripGetNextFramebuffer)                                                                                                 \
  FAKE(ledStripUpdateFrame)

#include "simhubDevice.c"

/* Capture helper for k_msgq_put — saves the message value before the caller's stack unwinds. */
static ServiceCtrlMsg_t capturedCtrlMsg;

static int k_msgq_put_capture(struct k_msgq *q, const void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(data)
    capturedCtrlMsg = *(const ServiceCtrlMsg_t *)data;
  return k_msgq_put_mock_fake.return_val;
}

/* Capture helper for serviceManagerRegisterSrv. */
static ServiceDescriptor_t capturedDescriptor;

static int serviceManagerRegisterSrv_capture(const ServiceDescriptor_t *descriptor)
{
  if(descriptor)
    capturedDescriptor = *descriptor;
  return serviceManagerRegisterSrv_fake.return_val;
}

/* Control queue helpers for k_msgq_get. */
static ServiceCtrlMsg_t testCtrlQueueMessages[4];
static size_t testCtrlQueueMsgCount = 0;
static size_t testCtrlQueueMsgRead  = 0;
static size_t testCtrlQueueSkipCount = 0;

static int k_msgq_get_no_message(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(data);
  ARG_UNUSED(timeout);
  return -EAGAIN;
}

static int k_msgq_get_from_ctrl_queue(struct k_msgq *q, void *data, k_timeout_t timeout)
{
  ARG_UNUSED(q);
  ARG_UNUSED(timeout);
  if(testCtrlQueueSkipCount > 0)
  {
    testCtrlQueueSkipCount--;
    return -EAGAIN;
  }
  if(testCtrlQueueMsgRead < testCtrlQueueMsgCount)
  {
    memcpy(data, &testCtrlQueueMessages[testCtrlQueueMsgRead++], sizeof(ServiceCtrlMsg_t));
    return 0;
  }
  return -EAGAIN;
}

/* DTR fake — returns DTR asserted immediately by default. */
static int uart_line_ctrl_get_dtr_ready(const struct device *dev, enum uart_line_ctrl ctrl, uint32_t *val)
{
  ARG_UNUSED(dev);
  ARG_UNUSED(ctrl);
  *val = 1;
  return 0;
}

/* DTR fake — returns DTR not asserted on first call, then asserted. */
static size_t dtrCallCount = 0;

static int uart_line_ctrl_get_dtr_after_delay(const struct device *dev, enum uart_line_ctrl ctrl, uint32_t *val)
{
  ARG_UNUSED(dev);
  ARG_UNUSED(ctrl);
  *val = (dtrCallCount++ > 0) ? 1 : 0;
  return 0;
}

/* Ring buffer byte feeder for drain loop tests. */
static uint8_t testRingBufBytes[8];
static size_t testRingBufByteCount = 0;
static size_t testRingBufByteRead  = 0;

static uint32_t ring_buf_get_from_test_buf(struct ring_buf *buf, uint8_t *data, uint32_t size)
{
  ARG_UNUSED(buf);
  ARG_UNUSED(size);
  if(testRingBufByteRead < testRingBufByteCount)
  {
    *data = testRingBufBytes[testRingBufByteRead++];
    return 1;
  }
  return 0;
}

static void service_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
  testCtrlQueueMsgCount  = 0;
  testCtrlQueueMsgRead   = 0;
  testCtrlQueueSkipCount = 0;
  capturedCtrlMsg        = (ServiceCtrlMsg_t)0xFF;
  k_msgq_put_mock_fake.custom_fake               = k_msgq_put_capture;
  k_msgq_get_mock_fake.custom_fake               = k_msgq_get_no_message;
  uart_line_ctrl_get_mock_fake.custom_fake       = uart_line_ctrl_get_dtr_ready;
  serviceManagerRegisterSrv_fake.custom_fake     = serviceManagerRegisterSrv_capture;
  dtrCallCount           = 0;
  txRemaining            = 0;
  txPtr                  = NULL;
  memset(&capturedDescriptor, 0, sizeof(capturedDescriptor));
  ctx.uart               = &testUartDevice;
  testRingBufByteCount   = 0;
  testRingBufByteRead    = 0;
}

/**
 * @test uartCallback must always call uart_irq_update
 */
ZTEST(simhubDevice_tests, test_uart_callback_calls_irq_update)
{
  uartCallback(&testUartDevice, NULL);

  zassert_equal(uart_irq_update_mock_fake.call_count, 1, "expected uart_irq_update called once");
  zassert_equal_ptr(uart_irq_update_mock_fake.arg0_val, &testUartDevice, "expected uart_irq_update called with dev");
}

/**
 * @test uartCallback must do nothing beyond uart_irq_update when neither RX
 *       nor TX IRQ is ready
 */
ZTEST(simhubDevice_tests, test_uart_callback_does_nothing_when_rx_not_ready_and_tx_not_ready)
{
  uartCallback(&testUartDevice, NULL);

  zassert_equal(ring_buf_put_mock_fake.call_count, 0, "expected ring_buf_put not called");
  zassert_equal(k_sem_give_mock_fake.call_count, 0, "expected k_sem_give not called");
  zassert_equal(uart_irq_tx_disable_mock_fake.call_count, 0, "expected uart_irq_tx_disable not called");
}

/**
 * @test uartCallback must put data into the ring buffer and give the RX
 *       semaphore when RX is ready and the FIFO contains bytes
 */
ZTEST(simhubDevice_tests, test_uart_callback_puts_ring_buf_and_gives_rx_sem_when_rx_ready_with_data)
{
  uart_irq_rx_ready_mock_fake.return_val = 1;
  uart_fifo_read_mock_fake.return_val    = 6;

  uartCallback(&testUartDevice, NULL);

  zassert_equal(uart_fifo_read_mock_fake.call_count, 1, "expected uart_fifo_read called once");
  zassert_equal_ptr(uart_fifo_read_mock_fake.arg0_val, &testUartDevice, "expected uart_fifo_read called with dev");
  zassert_equal(ring_buf_put_mock_fake.call_count, 1, "expected ring_buf_put called once");
  zassert_equal(ring_buf_put_mock_fake.arg2_val, 6u, "expected ring_buf_put called with fifo_read return value");
  zassert_equal(k_sem_give_mock_fake.call_count, 1, "expected k_sem_give called once");
  zassert_equal_ptr(k_sem_give_mock_fake.arg0_val, &rxSem, "expected k_sem_give called with &rxSem");
}

/**
 * @test uartCallback must not put data into the ring buffer when RX is ready
 *       but the FIFO is empty
 */
ZTEST(simhubDevice_tests, test_uart_callback_does_not_put_ring_buf_when_rx_ready_with_no_data)
{
  uart_irq_rx_ready_mock_fake.return_val = 1;

  uartCallback(&testUartDevice, NULL);

  zassert_equal(ring_buf_put_mock_fake.call_count, 0, "expected ring_buf_put not called");
  zassert_equal(k_sem_give_mock_fake.call_count, 0, "expected k_sem_give not called");
}

/**
 * @test uartCallback must fill the FIFO and advance txPtr when TX is ready
 *       and bytes remain after a partial fill
 */
ZTEST(simhubDevice_tests, test_uart_callback_fills_fifo_and_advances_tx_ptr_on_partial_fill)
{
  static uint8_t testTxData[10];
  uart_irq_tx_ready_mock_fake.return_val = 1;
  uart_fifo_fill_mock_fake.return_val    = 6;
  txPtr       = testTxData;
  txRemaining = 10;

  uartCallback(&testUartDevice, NULL);

  zassert_equal(uart_fifo_fill_mock_fake.call_count, 1, "expected uart_fifo_fill called once");
  zassert_equal_ptr(uart_fifo_fill_mock_fake.arg0_val, &testUartDevice, "expected uart_fifo_fill called with dev");
  zassert_equal(txRemaining, 4u, "expected txRemaining decremented by fill count");
  zassert_equal_ptr(txPtr, testTxData + 6, "expected txPtr advanced by fill count");
  zassert_equal(uart_irq_tx_disable_mock_fake.call_count, 0, "expected uart_irq_tx_disable not called");
  zassert_equal(k_sem_give_mock_fake.call_count, 0, "expected k_sem_give not called");
}

/**
 * @test uartCallback must disable TX IRQ and give the TX semaphore when the
 *       fill completes the remaining bytes
 */
ZTEST(simhubDevice_tests, test_uart_callback_disables_tx_and_gives_tx_sem_when_fill_completes)
{
  static uint8_t testTxData[10];
  uart_irq_tx_ready_mock_fake.return_val = 1;
  uart_fifo_fill_mock_fake.return_val    = 10;
  txPtr       = testTxData;
  txRemaining = 10;

  uartCallback(&testUartDevice, NULL);

  zassert_equal(txRemaining, 0u, "expected txRemaining zero after full fill");
  zassert_equal(uart_irq_tx_disable_mock_fake.call_count, 1, "expected uart_irq_tx_disable called once");
  zassert_equal_ptr(uart_irq_tx_disable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_tx_disable called with dev");
  zassert_equal(k_sem_give_mock_fake.call_count, 1, "expected k_sem_give called once");
  zassert_equal_ptr(k_sem_give_mock_fake.arg0_val, &txSem, "expected k_sem_give called with &txSem");
}

/**
 * @test uartCallback must disable TX IRQ and give the TX semaphore when TX is
 *       ready but txRemaining is already zero
 */
ZTEST(simhubDevice_tests, test_uart_callback_disables_tx_and_gives_tx_sem_when_tx_already_done)
{
  uart_irq_tx_ready_mock_fake.return_val = 1;

  uartCallback(&testUartDevice, NULL);

  zassert_equal(uart_fifo_fill_mock_fake.call_count, 0, "expected uart_fifo_fill not called");
  zassert_equal(uart_irq_tx_disable_mock_fake.call_count, 1, "expected uart_irq_tx_disable called once");
  zassert_equal(k_sem_give_mock_fake.call_count, 1, "expected k_sem_give called once");
  zassert_equal_ptr(k_sem_give_mock_fake.arg0_val, &txSem, "expected k_sem_give called with &txSem");
}

/**
 * @test dispatchPkt must acquire the TX semaphore, call process, then release
 *       the semaphore when proto processing returns no data
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_proto_takes_tx_sem_and_gives_it_back_when_no_data)
{
  simhubDevUtilGetPktType_fake.return_val    = SIMHUB_PKT_PROTO;
  simhubDevUtilProcessProto_fake.return_val  = 0;

  dispatchPkt();

  zassert_equal(k_sem_take_mock_fake.call_count, 1, "expected k_sem_take called once");
  zassert_equal_ptr(k_sem_take_mock_fake.arg0_val, &txSem, "expected k_sem_take called with &txSem");
  zassert_equal(simhubDevUtilProcessProto_fake.call_count, 1, "expected simhubDevUtilProcessProto called once");
  zassert_equal(k_sem_give_mock_fake.call_count, 1, "expected k_sem_give called once");
  zassert_equal_ptr(k_sem_give_mock_fake.arg0_val, &txSem, "expected k_sem_give called with &txSem");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 0, "expected uart_irq_tx_enable not called");
}

/**
 * @test dispatchPkt must acquire the TX semaphore, set txPtr and txRemaining,
 *       and enable TX IRQ when proto processing returns data
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_proto_takes_tx_sem_sets_tx_state_and_enables_tx)
{
  simhubDevUtilGetPktType_fake.return_val    = SIMHUB_PKT_PROTO;
  simhubDevUtilProcessProto_fake.return_val  = 5;

  dispatchPkt();

  zassert_equal(k_sem_take_mock_fake.call_count, 1, "expected k_sem_take called once");
  zassert_equal_ptr(k_sem_take_mock_fake.arg0_val, &txSem, "expected k_sem_take called with &txSem");
  zassert_equal(txRemaining, 5u, "expected txRemaining set to process return value");
  zassert_equal_ptr(txPtr, txBuf, "expected txPtr set to txBuf");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 1, "expected uart_irq_tx_enable called once");
  zassert_equal_ptr(uart_irq_tx_enable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_tx_enable called with ctx.uart");
  zassert_equal(k_sem_give_mock_fake.call_count, 0, "expected k_sem_give not called");
}

/**
 * @test dispatchPkt must acquire the TX semaphore, call process, then release
 *       the semaphore when LED count processing returns no data
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_led_count_takes_tx_sem_and_gives_it_back_when_no_data)
{
  simhubDevUtilGetPktType_fake.return_val       = SIMHUB_PKT_LED_COUNT;
  simhubDevUtilProcessLedCount_fake.return_val  = 0;

  dispatchPkt();

  zassert_equal(k_sem_take_mock_fake.call_count, 1, "expected k_sem_take called once");
  zassert_equal_ptr(k_sem_take_mock_fake.arg0_val, &txSem, "expected k_sem_take called with &txSem");
  zassert_equal(simhubDevUtilProcessLedCount_fake.call_count, 1, "expected simhubDevUtilProcessLedCount called once");
  zassert_equal(k_sem_give_mock_fake.call_count, 1, "expected k_sem_give called once");
  zassert_equal_ptr(k_sem_give_mock_fake.arg0_val, &txSem, "expected k_sem_give called with &txSem");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 0, "expected uart_irq_tx_enable not called");
}

/**
 * @test dispatchPkt must acquire the TX semaphore, set txPtr and txRemaining,
 *       and enable TX IRQ when LED count processing returns data
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_led_count_takes_tx_sem_sets_tx_state_and_enables_tx)
{
  simhubDevUtilGetPktType_fake.return_val       = SIMHUB_PKT_LED_COUNT;
  simhubDevUtilProcessLedCount_fake.return_val  = 3;

  dispatchPkt();

  zassert_equal(k_sem_take_mock_fake.call_count, 1, "expected k_sem_take called once");
  zassert_equal_ptr(k_sem_take_mock_fake.arg0_val, &txSem, "expected k_sem_take called with &txSem");
  zassert_equal(txRemaining, 3u, "expected txRemaining set to process return value");
  zassert_equal_ptr(txPtr, txBuf, "expected txPtr set to txBuf");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 1, "expected uart_irq_tx_enable called once");
  zassert_equal_ptr(uart_irq_tx_enable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_tx_enable called with ctx.uart");
  zassert_equal(k_sem_give_mock_fake.call_count, 0, "expected k_sem_give not called");
}

/**
 * @test dispatchPkt must call processUnlock when the packet type is UNLOCK
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_calls_process_unlock_for_unlock_pkt)
{
  simhubDevUtilGetPktType_fake.return_val = SIMHUB_PKT_UNLOCK;

  dispatchPkt();

  zassert_equal(simhubDevUtilProcessUnlock_fake.call_count, 1, "expected simhubDevUtilProcessUnlock called once");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 0, "expected uart_irq_tx_enable not called");
}

/**
 * @test dispatchPkt must reset the parser and not update the frame when no
 *       framebuffer is available for LED data
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_resets_parser_when_no_framebuffer_for_led_data)
{
  simhubDevUtilGetPktType_fake.return_val    = SIMHUB_PKT_LED_DATA;
  ledStripGetNextFramebuffer_fake.return_val = NULL;

  dispatchPkt();

  zassert_equal(simhubDevUtilReset_fake.call_count, 1, "expected simhubDevUtilReset called once");
  zassert_equal(simhubDevUtilProcessLedData_fake.call_count, 0, "expected simhubDevUtilProcessLedData not called");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 0, "expected ledStripUpdateFrame not called");
}

/**
 * @test dispatchPkt must process LED data and update the frame when a
 *       framebuffer is available
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_processes_led_data_and_updates_frame)
{
  static struct led_rgb testFrame[3];

  simhubDevUtilGetPktType_fake.return_val    = SIMHUB_PKT_LED_DATA;
  ledStripGetNextFramebuffer_fake.return_val = testFrame;

  dispatchPkt();

  zassert_equal(simhubDevUtilProcessLedData_fake.call_count, 1, "expected simhubDevUtilProcessLedData called once");
  zassert_equal_ptr(simhubDevUtilProcessLedData_fake.arg0_val, testFrame,
                    "expected simhubDevUtilProcessLedData called with the framebuffer");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 1, "expected ledStripUpdateFrame called once");
  zassert_equal_ptr(ledStripUpdateFrame_fake.arg0_val, testFrame, "expected ledStripUpdateFrame called with the framebuffer");
  zassert_equal(simhubDevUtilReset_fake.call_count, 0, "expected simhubDevUtilReset not called");
}

/**
 * @test dispatchPkt must reset the parser for an unknown packet type
 */
ZTEST(simhubDevice_tests, test_dispatch_pkt_resets_parser_for_unknown_pkt_type)
{
  simhubDevUtilGetPktType_fake.return_val = (SimhubProtoPktType_t)0xFF;

  dispatchPkt();

  zassert_equal(simhubDevUtilReset_fake.call_count, 1, "expected simhubDevUtilReset called once");
  zassert_equal(uart_irq_tx_enable_mock_fake.call_count, 0, "expected uart_irq_tx_enable not called");
}

/**
 * @test rxEnable must reset the ring buffer and enable UART RX IRQ
 */
ZTEST(simhubDevice_tests, test_rx_enable_resets_ring_buf_and_enables_uart_irq_rx)
{
  rxEnable();

  zassert_equal(ring_buf_reset_mock_fake.call_count, 1, "expected ring_buf_reset called once");
  zassert_equal_ptr(ring_buf_reset_mock_fake.arg0_val, &rxRingBuf, "expected ring_buf_reset called with &rxRingBuf");
  zassert_equal(uart_irq_rx_enable_mock_fake.call_count, 1, "expected uart_irq_rx_enable called once");
  zassert_equal_ptr(uart_irq_rx_enable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_rx_enable called with ctx.uart");
}

/**
 * @test rxDisable must disable UART RX IRQ
 */
ZTEST(simhubDevice_tests, test_rx_disable_disables_uart_irq_rx)
{
  rxDisable();

  zassert_equal(uart_irq_rx_disable_mock_fake.call_count, 1, "expected uart_irq_rx_disable called once");
  zassert_equal_ptr(uart_irq_rx_disable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_rx_disable called with ctx.uart");
}

/**
 * @test run must return early when simhubDevUtilInit fails
 */
ZTEST(simhubDevice_tests, test_run_returns_early_when_simhub_util_init_fails)
{
  simhubDevUtilInit_fake.return_val = -ENOMEM;

  run(NULL, NULL, NULL);

  zassert_equal(uart_irq_callback_user_data_set_mock_fake.call_count, 0,
                "expected uart_irq_callback_user_data_set not called");
  zassert_equal(k_msgq_get_mock_fake.call_count, 0, "expected loop not entered");
}

/**
 * @test run must poll uart_line_ctrl_get until DTR is asserted before enabling
 *       RX
 */
ZTEST(simhubDevice_tests, test_run_waits_for_dtr_before_enabling_rx)
{
  uart_line_ctrl_get_mock_fake.custom_fake = uart_line_ctrl_get_dtr_after_delay;

  run(NULL, NULL, NULL);

  zassert_true(uart_line_ctrl_get_mock_fake.call_count >= 2, "expected uart_line_ctrl_get polled at least twice");
  zassert_equal(uart_line_ctrl_get_mock_fake.arg1_history[0], UART_LINE_CTRL_DTR,
                "expected uart_line_ctrl_get called with UART_LINE_CTRL_DTR");
  zassert_true(k_sleep_mock_fake.call_count >= 1, "expected k_sleep called while polling DTR");
  zassert_equal(uart_irq_rx_enable_mock_fake.call_count, 1, "expected uart_irq_rx_enable called after DTR asserted");
}

/**
 * @test run must confirm STOPPED state and return without enabling RX when
 *       SVC_CTRL_STOP is received during the DTR wait
 */
ZTEST(simhubDevice_tests, test_run_stop_during_dtr_wait_confirms_stopped_and_returns)
{
  testCtrlQueueMessages[0] = SVC_CTRL_STOP;
  testCtrlQueueMsgCount    = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;

  run(NULL, NULL, NULL);

  zassert_equal(serviceManagerConfirmState_fake.call_count, 1, "expected serviceManagerConfirmState called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_STOPPED,
                "expected serviceManagerConfirmState called with SVC_STATE_STOPPED");
  zassert_equal(uart_irq_rx_enable_mock_fake.call_count, 0, "expected uart_irq_rx_enable not called");
  zassert_equal(uart_irq_rx_disable_mock_fake.call_count, 0, "expected uart_irq_rx_disable not called");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 0, "expected heartbeat not updated");
}

/**
 * @test run must set the IRQ callback with the correct device and function
 */
ZTEST(simhubDevice_tests, test_run_sets_uart_irq_callback_with_correct_args)
{
  run(NULL, NULL, NULL);

  zassert_equal(uart_irq_callback_user_data_set_mock_fake.call_count, 1,
                "expected uart_irq_callback_user_data_set called once");
  zassert_equal_ptr(uart_irq_callback_user_data_set_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_callback_user_data_set called with ctx.uart");
  zassert_not_null(uart_irq_callback_user_data_set_mock_fake.arg1_val,
                   "expected uart_irq_callback_user_data_set called with a non-NULL callback");
  zassert_is_null(uart_irq_callback_user_data_set_mock_fake.arg2_val,
                  "expected uart_irq_callback_user_data_set called with NULL user_data");
}

/**
 * @test run must disable RX, reset the parser, confirm STOPPED state and
 *       return without updating the heartbeat a second time on SVC_CTRL_STOP
 */
ZTEST(simhubDevice_tests, test_run_stops_service_on_stop_ctrl_message)
{
  /* Skip the DTR-wait k_msgq_get call so STOP is consumed in the main loop. */
  testCtrlQueueSkipCount   = 1;
  testCtrlQueueMessages[0] = SVC_CTRL_STOP;
  testCtrlQueueMsgCount    = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;

  run(NULL, NULL, NULL);

  zassert_equal(uart_irq_rx_disable_mock_fake.call_count, 1, "expected uart_irq_rx_disable called once");
  zassert_equal_ptr(uart_irq_rx_disable_mock_fake.arg0_val, &testUartDevice,
                    "expected uart_irq_rx_disable called with ctx.uart");
  zassert_equal(simhubDevUtilReset_fake.call_count, 1, "expected simhubDevUtilReset called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1, "expected serviceManagerConfirmState called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_STOPPED,
                "expected serviceManagerConfirmState called with SVC_STATE_STOPPED");
  /* One heartbeat from DTR wait, none from main loop (returns on STOP). */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 1, "expected heartbeat updated once from DTR wait only");
}

/**
 * @test run must disable RX, reset the parser, confirm SUSPENDED state,
 *       suspend the thread and re-enable RX on SVC_CTRL_SUSPEND
 */
ZTEST(simhubDevice_tests, test_run_suspends_service_on_suspend_ctrl_message)
{
  /* Skip the DTR-wait k_msgq_get call so SUSPEND is consumed in the main loop. */
  testCtrlQueueSkipCount   = 1;
  testCtrlQueueMessages[0] = SVC_CTRL_SUSPEND;
  testCtrlQueueMsgCount    = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;

  run(NULL, NULL, NULL);

  zassert_equal(uart_irq_rx_disable_mock_fake.call_count, 1, "expected uart_irq_rx_disable called once");
  zassert_equal(simhubDevUtilReset_fake.call_count, 1, "expected simhubDevUtilReset called once");
  zassert_equal(serviceManagerConfirmState_fake.call_count, 1, "expected serviceManagerConfirmState called once");
  zassert_equal(serviceManagerConfirmState_fake.arg1_val, SVC_STATE_SUSPENDED,
                "expected serviceManagerConfirmState called with SVC_STATE_SUSPENDED");
  zassert_equal(k_thread_suspend_mock_fake.call_count, 1, "expected k_thread_suspend called once");
  /* uart_irq_rx_enable called twice: initial rxEnable and post-resume rxEnable. */
  zassert_equal(uart_irq_rx_enable_mock_fake.call_count, 2,
                "expected uart_irq_rx_enable called twice (init + after resume)");
  /* One heartbeat from DTR wait, one from the main loop body after handling SUSPEND. */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2, "expected heartbeat updated twice");
}

/**
 * @test run must log a warning and continue when an unknown control message is
 *       received
 */
ZTEST(simhubDevice_tests, test_run_continues_on_unknown_ctrl_message)
{
  testCtrlQueueSkipCount   = 1;
  testCtrlQueueMessages[0] = (ServiceCtrlMsg_t)0xFF;
  testCtrlQueueMsgCount    = 1;
  k_msgq_get_mock_fake.custom_fake = k_msgq_get_from_ctrl_queue;

  run(NULL, NULL, NULL);

  zassert_equal(uart_irq_rx_disable_mock_fake.call_count, 0, "expected uart_irq_rx_disable not called");
  /* One heartbeat from DTR wait, one from the main loop body. */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2, "expected heartbeat updated twice");
}

/**
 * @test run must take the RX semaphore and update the heartbeat each iteration
 *       when no bytes are available in the ring buffer
 */
ZTEST(simhubDevice_tests, test_run_polls_rx_sem_and_updates_heartbeat_when_no_data)
{
  run(NULL, NULL, NULL);

  zassert_equal(k_sem_take_mock_fake.call_count, 1, "expected k_sem_take called once");
  zassert_equal_ptr(k_sem_take_mock_fake.arg0_val, &rxSem, "expected k_sem_take called with &rxSem");
  zassert_equal(simhubDevUtilReceivedPkt_fake.call_count, 0, "expected simhubDevUtilReceivedPkt not called");
  /* One heartbeat from DTR wait, one from the main loop body. */
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2, "expected heartbeat updated twice");
}

/**
 * @test run must not dispatch a packet when a byte is read from the ring
 *       buffer but the parser reports the packet is not yet complete
 */
ZTEST(simhubDevice_tests, test_run_does_not_dispatch_pkt_when_received_pkt_incomplete)
{
  testRingBufBytes[0]  = 0xAA;
  testRingBufByteCount = 1;
  ring_buf_get_mock_fake.custom_fake        = ring_buf_get_from_test_buf;
  simhubDevUtilReceivedPkt_fake.return_val  = false;

  run(NULL, NULL, NULL);

  zassert_equal(simhubDevUtilReceivedPkt_fake.call_count, 1, "expected simhubDevUtilReceivedPkt called once");
  zassert_equal(simhubDevUtilReceivedPkt_fake.arg0_val, 0xAAu, "expected simhubDevUtilReceivedPkt called with the byte");
  zassert_equal(simhubDevUtilGetPktType_fake.call_count, 0, "expected simhubDevUtilGetPktType not called");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2, "expected heartbeat updated twice");
}

/**
 * @test run must dispatch a packet when the parser reports a complete packet
 *       after reading a byte from the ring buffer
 */
ZTEST(simhubDevice_tests, test_run_dispatches_pkt_when_received_pkt_completes)
{
  testRingBufBytes[0]  = 0x55;
  testRingBufByteCount = 1;
  ring_buf_get_mock_fake.custom_fake        = ring_buf_get_from_test_buf;
  simhubDevUtilReceivedPkt_fake.return_val  = true;
  simhubDevUtilGetPktType_fake.return_val   = SIMHUB_PKT_UNLOCK;

  run(NULL, NULL, NULL);

  zassert_equal(simhubDevUtilReceivedPkt_fake.call_count, 1, "expected simhubDevUtilReceivedPkt called once");
  zassert_equal(simhubDevUtilGetPktType_fake.call_count, 1, "expected simhubDevUtilGetPktType called once");
  zassert_equal(simhubDevUtilProcessUnlock_fake.call_count, 1, "expected simhubDevUtilProcessUnlock called once");
  zassert_equal(serviceManagerUpdateHeartbeat_fake.call_count, 2, "expected heartbeat updated twice");
}

/**
 * @test onStart must start the service thread and return success
 */
ZTEST(simhubDevice_tests, test_on_start_starts_thread_and_returns_success)
{
  int ret = onStart();

  zassert_equal(ret, 0, "expected onStart to return 0");
  zassert_equal(k_thread_start_mock_fake.call_count, 1, "expected k_thread_start called once");
  zassert_equal_ptr(k_thread_start_mock_fake.arg0_val, &thread, "expected k_thread_start called with &thread");
}

/**
 * @test onStop must propagate the error when enqueueing the stop message fails
 */
ZTEST(simhubDevice_tests, test_on_stop_returns_error_when_msgq_put_fails)
{
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  int ret = onStop();

  zassert_equal(ret, -ENOMEM, "expected onStop to return -ENOMEM");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1, "expected k_msgq_put called once");
  zassert_equal(capturedCtrlMsg, SVC_CTRL_STOP, "expected SVC_CTRL_STOP enqueued");
}

/**
 * @test onStop must enqueue a SVC_CTRL_STOP message and return success
 */
ZTEST(simhubDevice_tests, test_on_stop_enqueues_stop_message_and_returns_success)
{
  int ret = onStop();

  zassert_equal(ret, 0, "expected onStop to return 0");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1, "expected k_msgq_put called once");
  zassert_equal(capturedCtrlMsg, SVC_CTRL_STOP, "expected SVC_CTRL_STOP enqueued");
}

/**
 * @test onSuspend must propagate the error when enqueueing the suspend message
 *       fails
 */
ZTEST(simhubDevice_tests, test_on_suspend_returns_error_when_msgq_put_fails)
{
  k_msgq_put_mock_fake.return_val = -ENOMEM;

  int ret = onSuspend();

  zassert_equal(ret, -ENOMEM, "expected onSuspend to return -ENOMEM");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1, "expected k_msgq_put called once");
  zassert_equal(capturedCtrlMsg, SVC_CTRL_SUSPEND, "expected SVC_CTRL_SUSPEND enqueued");
}

/**
 * @test onSuspend must enqueue a SVC_CTRL_SUSPEND message and return success
 */
ZTEST(simhubDevice_tests, test_on_suspend_enqueues_suspend_message_and_returns_success)
{
  int ret = onSuspend();

  zassert_equal(ret, 0, "expected onSuspend to return 0");
  zassert_equal(k_msgq_put_mock_fake.call_count, 1, "expected k_msgq_put called once");
  zassert_equal(capturedCtrlMsg, SVC_CTRL_SUSPEND, "expected SVC_CTRL_SUSPEND enqueued");
}

/**
 * @test onResume must resume the service thread and return success
 */
ZTEST(simhubDevice_tests, test_on_resume_resumes_thread_and_returns_success)
{
  int ret = onResume();

  zassert_equal(ret, 0, "expected onResume to return 0");
  zassert_equal(k_thread_resume_mock_fake.call_count, 1, "expected k_thread_resume called once");
  zassert_equal_ptr(k_thread_resume_mock_fake.arg0_val, &thread, "expected k_thread_resume called with &thread");
}

/**
 * @test simhubDeviceInit must return -ENODEV and not create the thread when
 *       the UART device is not ready
 */
ZTEST(simhubDevice_tests, test_simhub_device_init_returns_enodev_when_uart_not_ready)
{
  device_is_ready_mock_fake.return_val = false;

  int ret = simhubDeviceInit();

  zassert_equal(ret, -ENODEV, "expected simhubDeviceInit to return -ENODEV");
  zassert_equal(k_thread_create_mock_fake.call_count, 0, "expected k_thread_create not called");
}

/**
 * @test simhubDeviceInit must return the error and not register the service
 *       when setting the thread name fails
 */
ZTEST(simhubDevice_tests, test_simhub_device_init_returns_error_when_thread_name_set_fails)
{
  device_is_ready_mock_fake.return_val    = true;
  k_thread_name_set_mock_fake.return_val = -EINVAL;

  int ret = simhubDeviceInit();

  zassert_equal(ret, -EINVAL, "expected simhubDeviceInit to return -EINVAL");
  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 0, "expected serviceManagerRegisterSrv not called");
}

/**
 * @test simhubDeviceInit must propagate the error when service registration
 *       fails
 */
ZTEST(simhubDevice_tests, test_simhub_device_init_returns_error_when_register_srv_fails)
{
  device_is_ready_mock_fake.return_val        = true;
  k_thread_name_set_mock_fake.return_val     = 0;
  serviceManagerRegisterSrv_fake.return_val  = -ENOMEM;

  int ret = simhubDeviceInit();

  zassert_equal(ret, -ENOMEM, "expected simhubDeviceInit to return -ENOMEM");
}

/**
 * @test simhubDeviceInit must create the thread, register the service with the
 *       correct descriptor fields and return success
 */
ZTEST(simhubDevice_tests, test_simhub_device_init_registers_service_and_returns_success)
{
  static struct k_thread fakeThread;
  k_tid_t fakeThreadId = (k_tid_t)&fakeThread;

  device_is_ready_mock_fake.return_val        = true;
  k_thread_create_mock_fake.return_val        = fakeThreadId;
  k_thread_name_set_mock_fake.return_val     = 0;
  serviceManagerRegisterSrv_fake.return_val  = 0;

  int ret = simhubDeviceInit();

  zassert_equal(ret, 0, "expected simhubDeviceInit to return 0");

  zassert_equal(k_thread_create_mock_fake.call_count, 1, "expected k_thread_create called once");
  zassert_equal_ptr(k_thread_create_mock_fake.arg0_val, &thread, "expected k_thread_create called with &thread");

  zassert_equal(k_thread_name_set_mock_fake.call_count, 1, "expected k_thread_name_set called once");
  zassert_equal_ptr(k_thread_name_set_mock_fake.arg0_val, fakeThreadId,
                    "expected k_thread_name_set called with the returned thread id");

  zassert_equal(serviceManagerRegisterSrv_fake.call_count, 1, "expected serviceManagerRegisterSrv called once");
  zassert_equal(capturedDescriptor.threadId, fakeThreadId, "expected descriptor.threadId set to the created thread id");
  zassert_equal(capturedDescriptor.priority, CONFIG_ENYA_SIMHUB_DEVICE_SERVICE_PRIORITY,
                "expected descriptor.priority set correctly");
  zassert_equal(capturedDescriptor.heartbeatIntervalMs, CONFIG_ENYA_SIMHUB_DEVICE_HEARTBEAT_INTERVAL_MS,
                "expected descriptor.heartbeatIntervalMs set correctly");
  zassert_equal_ptr(capturedDescriptor.start, onStart, "expected descriptor.start set to onStart");
  zassert_equal_ptr(capturedDescriptor.stop, onStop, "expected descriptor.stop set to onStop");
  zassert_equal_ptr(capturedDescriptor.suspend, onSuspend, "expected descriptor.suspend set to onSuspend");
  zassert_equal_ptr(capturedDescriptor.resume, onResume, "expected descriptor.resume set to onResume");
}

ZTEST_SUITE(simhubDevice_tests, NULL, NULL, service_tests_before, NULL, NULL);
