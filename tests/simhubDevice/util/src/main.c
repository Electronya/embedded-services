/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Util Tests
 *
 *            Unit tests for the SimHub device protocol dispatch and packet
 *            builder functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Prevent led_strip driver header — define only the type we need. */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
struct led_rgb { uint8_t r; uint8_t g; uint8_t b; };

/* Override DT macros so SIMHUB_LED_COUNT resolves to 3 without real DTS. */
#undef DT_ALIAS
#define DT_ALIAS(name) DT_N_NODELABEL_test_led_strip

#define DT_N_NODELABEL_test_led_strip_P_chain_length 3

/* Setup logging before including the module under test. */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

#include "simhubDevProto.h"

#define FFF_FAKES_LIST(FAKE)                  \
  FAKE(simhubDevProtoReset)                   \
  FAKE(simhubDevProtoGetState)                \
  FAKE(simhubDevProtoGetPktType)              \
  FAKE(simhubDevProtoGetPixelBuf)             \
  FAKE(simhubDevProtoProcessEmptyBuff)        \
  FAKE(simhubDevProtoProcessPreamble)         \
  FAKE(simhubDevProtoProcessCmd)              \
  FAKE(simhubDevProtoProcessLedData)          \
  FAKE(simhubDevProtoProcessLedFooter)

FAKE_VOID_FUNC(simhubDevProtoReset);
FAKE_VALUE_FUNC(SimhubProtoState_t, simhubDevProtoGetState);
FAKE_VALUE_FUNC(SimhubProtoPktType_t, simhubDevProtoGetPktType);
FAKE_VALUE_FUNC(const uint8_t *, simhubDevProtoGetPixelBuf);
FAKE_VALUE_FUNC(bool, simhubDevProtoProcessEmptyBuff, uint8_t);
FAKE_VALUE_FUNC(bool, simhubDevProtoProcessPreamble, uint8_t);
FAKE_VALUE_FUNC(bool, simhubDevProtoProcessCmd, uint8_t);
FAKE_VALUE_FUNC(bool, simhubDevProtoProcessLedData, uint8_t);
FAKE_VALUE_FUNC(bool, simhubDevProtoProcessLedFooter, uint8_t);

#include "simhubDevUtil.c"

static void *util_tests_setup(void)
{
  return NULL;
}

static void util_tests_before(void *fixture)
{
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * @test simhubDevUtilInit must call simhubDevProtoReset to initialize the
 * parser to the initial state and return 0 on success.
 */
ZTEST(simhubDevUtil_tests, test_init_resets_proto_and_returns_success)
{
  int result = simhubDevUtilInit();

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "Init must call simhubDevProtoReset once");
  zassert_equal(result, 0,
                "Init must return 0 on success");
}

/**
 * @test simhubDevUtilReset must call simhubDevProtoReset to reset the parser
 * to the initial state.
 */
ZTEST(simhubDevUtil_tests, test_reset_resets_proto)
{
  simhubDevUtilReset();

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "Reset must call simhubDevProtoReset once");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoReset and return false
 * when the parser is in the SIMHUB_PROTO_RECEIVED_PKT state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_resets_and_returns_false_when_pkt_pending)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_RECEIVED_PKT;

  bool result = simhubDevUtilReceivedPkt(0x00);

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoReset once when packet pending");
  zassert_false(result,
                "ReceivedPkt must return false when packet pending");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoReset and return false
 * when the parser is in an unknown state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_resets_and_returns_false_when_unknown_state)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_STATE_COUNT;

  bool result = simhubDevUtilReceivedPkt(0x00);

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoReset once when state is unknown");
  zassert_false(result,
                "ReceivedPkt must return false when state is unknown");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoProcessEmptyBuff with
 * the received byte and return its result when the parser is in the
 * SIMHUB_PROTO_EMPTY_BUFF state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_dispatches_to_process_empty_buff)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_EMPTY_BUFF;
  simhubDevProtoProcessEmptyBuff_fake.return_val = true;

  bool result = simhubDevUtilReceivedPkt(0xFF);

  zassert_equal(simhubDevProtoProcessEmptyBuff_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoProcessEmptyBuff once");
  zassert_equal(simhubDevProtoProcessEmptyBuff_fake.arg0_val, 0xFF,
                "ReceivedPkt must pass the received byte to simhubDevProtoProcessEmptyBuff");
  zassert_true(result,
               "ReceivedPkt must return the result of simhubDevProtoProcessEmptyBuff");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoProcessPreamble with
 * the received byte and return its result when the parser is in the
 * SIMHUB_PROTO_RECEIVING_PREAMBLE state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_dispatches_to_process_preamble)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_RECEIVING_PREAMBLE;
  simhubDevProtoProcessPreamble_fake.return_val = true;

  bool result = simhubDevUtilReceivedPkt(0xFF);

  zassert_equal(simhubDevProtoProcessPreamble_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoProcessPreamble once");
  zassert_equal(simhubDevProtoProcessPreamble_fake.arg0_val, 0xFF,
                "ReceivedPkt must pass the received byte to simhubDevProtoProcessPreamble");
  zassert_true(result,
               "ReceivedPkt must return the result of simhubDevProtoProcessPreamble");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoProcessCmd with the
 * received byte and return its result when the parser is in the
 * SIMHUB_PROTO_RECEIVING_CMD state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_dispatches_to_process_cmd)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_RECEIVING_CMD;
  simhubDevProtoProcessCmd_fake.return_val = true;

  bool result = simhubDevUtilReceivedPkt(0x70);

  zassert_equal(simhubDevProtoProcessCmd_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoProcessCmd once");
  zassert_equal(simhubDevProtoProcessCmd_fake.arg0_val, 0x70,
                "ReceivedPkt must pass the received byte to simhubDevProtoProcessCmd");
  zassert_true(result,
               "ReceivedPkt must return the result of simhubDevProtoProcessCmd");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoProcessLedData with
 * the received byte and return its result when the parser is in the
 * SIMHUB_PROTO_RECEIVING_LED_DATA state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_dispatches_to_process_led_data)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_RECEIVING_LED_DATA;
  simhubDevProtoProcessLedData_fake.return_val = true;

  bool result = simhubDevUtilReceivedPkt(0xAB);

  zassert_equal(simhubDevProtoProcessLedData_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoProcessLedData once");
  zassert_equal(simhubDevProtoProcessLedData_fake.arg0_val, 0xAB,
                "ReceivedPkt must pass the received byte to simhubDevProtoProcessLedData");
  zassert_true(result,
               "ReceivedPkt must return the result of simhubDevProtoProcessLedData");
}

/**
 * @test simhubDevUtilReceivedPkt must call simhubDevProtoProcessLedFooter with
 * the received byte and return its result when the parser is in the
 * SIMHUB_PROTO_RECEIVING_LED_FOOTER state.
 */
ZTEST(simhubDevUtil_tests, test_received_pkt_dispatches_to_process_led_footer)
{
  simhubDevProtoGetState_fake.return_val = SIMHUB_PROTO_RECEIVING_LED_FOOTER;
  simhubDevProtoProcessLedFooter_fake.return_val = true;

  bool result = simhubDevUtilReceivedPkt(0xFE);

  zassert_equal(simhubDevProtoProcessLedFooter_fake.call_count, 1,
                "ReceivedPkt must call simhubDevProtoProcessLedFooter once");
  zassert_equal(simhubDevProtoProcessLedFooter_fake.arg0_val, 0xFE,
                "ReceivedPkt must pass the received byte to simhubDevProtoProcessLedFooter");
  zassert_true(result,
               "ReceivedPkt must return the result of simhubDevProtoProcessLedFooter");
}

/**
 * @test simhubDevUtilGetPktType must call simhubDevProtoGetPktType and return
 * its result.
 */
ZTEST(simhubDevUtil_tests, test_get_pkt_type_returns_proto_pkt_type)
{
  simhubDevProtoGetPktType_fake.return_val = SIMHUB_PKT_LED_DATA;

  SimhubProtoPktType_t result = simhubDevUtilGetPktType();

  zassert_equal(simhubDevProtoGetPktType_fake.call_count, 1,
                "GetPktType must call simhubDevProtoGetPktType once");
  zassert_equal(result, SIMHUB_PKT_LED_DATA,
                "GetPktType must return the result of simhubDevProtoGetPktType");
}

/**
 * @test simhubDevUtilProcessProto must call simhubDevProtoReset and return
 * -ENOMEM when the TX buffer is too small to hold the proto response.
 */
ZTEST(simhubDevUtil_tests, test_process_proto_resets_and_returns_enomem_when_buf_too_small)
{
  uint8_t txBuf[SIMHUB_PROTO_RES_MAX_SIZE - 1];

  int result = simhubDevUtilProcessProto(txBuf, sizeof(txBuf));

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessProto must call simhubDevProtoReset once when buffer too small");
  zassert_equal(result, -ENOMEM,
                "ProcessProto must return -ENOMEM when buffer too small");
}

/**
 * @test simhubDevUtilProcessProto must copy the proto response into the TX
 * buffer, call simhubDevProtoReset, and return the response length on success.
 */
ZTEST(simhubDevUtil_tests, test_process_proto_writes_response_resets_and_returns_length)
{
  uint8_t txBuf[SIMHUB_PROTO_RES_MAX_SIZE] = {0};

  int result = simhubDevUtilProcessProto(txBuf, sizeof(txBuf));

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessProto must call simhubDevProtoReset once on success");
  zassert_equal(result, SIMHUB_PROTO_RES_MAX_SIZE,
                "ProcessProto must return the response length on success");
  zassert_mem_equal(txBuf, "SIMHUB_1.0\r\n", SIMHUB_PROTO_RES_MAX_SIZE,
                    "ProcessProto must copy the proto response into the TX buffer");
}

/**
 * @test simhubDevUtilProcessLedCount must call simhubDevProtoReset and return
 * -ENOMEM when the TX buffer is too small to hold the LED count response.
 */
ZTEST(simhubDevUtil_tests, test_process_led_count_resets_and_returns_enomem_when_buf_too_small)
{
  uint8_t txBuf[3];

  int result = simhubDevUtilProcessLedCount(txBuf, sizeof(txBuf));

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessLedCount must call simhubDevProtoReset once when buffer too small");
  zassert_equal(result, -ENOMEM,
                "ProcessLedCount must return -ENOMEM when buffer too small");
}

/**
 * @test simhubDevUtilProcessLedCount must write the LED count into the TX
 * buffer, call simhubDevProtoReset, and return the response length on success.
 */
ZTEST(simhubDevUtil_tests, test_process_led_count_writes_response_resets_and_returns_length)
{
  uint8_t txBuf[8] = {0};

  int result = simhubDevUtilProcessLedCount(txBuf, sizeof(txBuf));

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessLedCount must call simhubDevProtoReset once on success");
  zassert_equal(result, 3,
                "ProcessLedCount must return the response length on success");
  zassert_mem_equal(txBuf, "3\r\n", 3,
                    "ProcessLedCount must write the LED count response into the TX buffer");
}

/**
 * @test simhubDevUtilProcessUnlock must call simhubDevProtoReset to discard the
 * unlock command and return 0 on success.
 */
ZTEST(simhubDevUtil_tests, test_process_unlock_resets_proto_and_returns_success)
{
  int result = simhubDevUtilProcessUnlock();

  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessUnlock must call simhubDevProtoReset once");
  zassert_equal(result, 0,
                "ProcessUnlock must return 0 on success");
}

/**
 * @test simhubDevUtilProcessLedData must call simhubDevProtoGetPixelBuf to
 * retrieve the pixel data, unpack the RGB values into the frame, call
 * simhubDevProtoReset, and return 0 on success.
 */
ZTEST(simhubDevUtil_tests, test_process_led_data_unpacks_pixels_resets_and_returns_success)
{
  static const uint8_t pixelBuf[SIMHUB_LED_COUNT * 3] = {
    0x11, 0x22, 0x33,
    0x44, 0x55, 0x66,
    0x77, 0x88, 0x99,
  };
  struct led_rgb frame[SIMHUB_LED_COUNT] = {0};
  simhubDevProtoGetPixelBuf_fake.return_val = pixelBuf;

  int result = simhubDevUtilProcessLedData(frame);

  zassert_equal(simhubDevProtoGetPixelBuf_fake.call_count, 1,
                "ProcessLedData must call simhubDevProtoGetPixelBuf once");
  zassert_equal(simhubDevProtoReset_fake.call_count, 1,
                "ProcessLedData must call simhubDevProtoReset once");
  zassert_equal(result, 0,
                "ProcessLedData must return 0 on success");
  for(int i = 0; i < SIMHUB_LED_COUNT; i++)
  {
    zassert_equal(frame[i].r, pixelBuf[i * 3],
                  "ProcessLedData must unpack red channel for pixel %d", i);
    zassert_equal(frame[i].g, pixelBuf[i * 3 + 1],
                  "ProcessLedData must unpack green channel for pixel %d", i);
    zassert_equal(frame[i].b, pixelBuf[i * 3 + 2],
                  "ProcessLedData must unpack blue channel for pixel %d", i);
  }
}

ZTEST_SUITE(simhubDevUtil_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
