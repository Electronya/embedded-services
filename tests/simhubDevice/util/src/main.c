/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub Device Util Tests
 *
 *            Unit tests for the SimHub ARQ session state machine.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <limits.h>

DEFINE_FFF_GLOBALS;

/* Prevent led_strip driver header — define only the type we need. */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
struct led_rgb { uint8_t r; uint8_t g; uint8_t b; };

/* Override DT macros so SIMHUB_LED_COUNT resolves to 3 without real DTS. */
#undef DT_ALIAS
#define DT_ALIAS(name)                                DT_N_NODELABEL_test_led_strip
#define DT_N_NODELABEL_test_led_strip_P_chain_length  3

/* Kconfig values consumed by the module under test. */
#define CONFIG_ENYA_SIMHUB_DEVICE_NAME "TestDevice"
#define CONFIG_ENYA_SIMHUB_DEVICE_UID  "TEST001"

/* Setup logging before including the module under test. */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

#include "simhubArqProto.h"

#define FFF_FAKES_LIST(FAKE)      \
  FAKE(simhubArqParseByte)        \
  FAKE(simhubArqFrameReset)       \
  FAKE(simhubArqBuildAck)         \
  FAKE(simhubArqBuildByte)        \
  FAKE(simhubArqBuildStr)         \
  FAKE(simhubArqBuildStrTerm)     \
  FAKE(mock_tx)

FAKE_VALUE_FUNC(bool, simhubArqParseByte, SimhubArqFrame_t *, uint8_t);
FAKE_VOID_FUNC(simhubArqFrameReset, SimhubArqFrame_t *);
FAKE_VALUE_FUNC(int, simhubArqBuildAck, uint8_t, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, simhubArqBuildByte, uint8_t, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, simhubArqBuildStr, const char *, uint8_t, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, simhubArqBuildStrTerm, uint8_t *, size_t);
FAKE_VALUE_FUNC(int, mock_tx, const uint8_t *, size_t);

#include "simhubDevUtil.c"

/* ---- TX capture ---- */

static uint8_t txCapBuf[64];
static size_t  txCapLen;

static int mock_tx_capture(const uint8_t *buf, size_t len)
{
  txCapLen = len < sizeof(txCapBuf) ? len : sizeof(txCapBuf);
  memcpy(txCapBuf, buf, txCapLen);
  return 0;
}

/* ---- Real build-function implementations used as custom_fakes ---- */

static int buildAck_real(uint8_t id, uint8_t *buf, size_t size)
{
  if(size < 2)
    return -ENOMEM;
  buf[0] = SIMHUB_ARQ_ACK;
  buf[1] = id;
  return 2;
}

static int buildByte_real(uint8_t val, uint8_t *buf, size_t size)
{
  if(size < 2)
    return -ENOMEM;
  buf[0] = SIMHUB_ARQ_BYTE;
  buf[1] = val;
  return 2;
}

static int buildStr_real(const char *str, uint8_t len, uint8_t *buf, size_t size)
{
  if(size < (size_t)(3 + len))
    return -ENOMEM;
  buf[0] = SIMHUB_ARQ_STR;
  buf[1] = len;
  memcpy(&buf[2], str, len);
  buf[2 + len] = 0x20;
  return 3 + len;
}

static int buildStrTerm_real(uint8_t *buf, size_t size)
{
  if(size < 4)
    return -ENOMEM;
  buf[0] = SIMHUB_ARQ_STR;
  buf[1] = 0x01;
  buf[2] = 0x0A;
  buf[3] = 0x20;
  return 4;
}

static int buildAck_fail(uint8_t id, uint8_t *buf, size_t size)
{
  ARG_UNUSED(id);
  ARG_UNUSED(buf);
  ARG_UNUSED(size);
  return -ENOMEM;
}

static int buildByte_fail(uint8_t val, uint8_t *buf, size_t size)
{
  ARG_UNUSED(val);
  ARG_UNUSED(buf);
  ARG_UNUSED(size);
  return -ENOMEM;
}

static int buildStrTerm_fail(uint8_t *buf, size_t size)
{
  ARG_UNUSED(buf);
  ARG_UNUSED(size);
  return -ENOMEM;
}

static int buildStr_succeed_n;
static int buildStr_call_idx;

static int buildStr_fail_after_n(const char *str, uint8_t len, uint8_t *buf, size_t size)
{
  if(buildStr_call_idx++ >= buildStr_succeed_n)
    return -ENOMEM;
  return buildStr_real(str, len, buf, size);
}

static int buildByte_succeed_n;
static int buildByte_call_idx;

static int buildByte_fail_after_n(uint8_t val, uint8_t *buf, size_t size)
{
  if(buildByte_call_idx++ >= buildByte_succeed_n)
    return -ENOMEM;
  return buildByte_real(val, buf, size);
}

/* ---- parseByte custom_fakes — populate rxFrame then return true ---- */

#define TEST_BYTE 0xAB

static bool parseByte_helloFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = SIMHUB_ARQ_BCAST;
  f->len     = 3;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '1';
  f->data[2] = 0x10;
  return true;
}

static bool parseByte_featuresFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x00;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '0';
  return true;
}

static bool parseByte_ledCountFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x01;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '4';
  return true;
}

static bool parseByte_tm1638CountFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x02;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '2';
  return true;
}

static bool parseByte_modulesCountFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x03;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'B';
  return true;
}

static bool parseByte_nameFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x05;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'N';
  return true;
}

static bool parseByte_uidFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x06;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'I';
  return true;
}

static bool parseByte_buttonCountFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x07;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'J';
  return true;
}

static bool parseByte_xListFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x04;
  f->len     = 7;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'X';
  f->data[2] = 'l';
  f->data[3] = 'i';
  f->data[4] = 's';
  f->data[5] = 't';
  f->data[6] = '\n';
  return true;
}

static bool parseByte_xMcutypeFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x08;
  f->len     = 10;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'X';
  f->data[2] = 'm';
  f->data[3] = 'c';
  f->data[4] = 'u';
  f->data[5] = 't';
  f->data[6] = 'y';
  f->data[7] = 'p';
  f->data[8] = 'e';
  f->data[9] = '\n';
  return true;
}

static bool parseByte_xShortSubFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x0A;
  f->len     = 4;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'X';
  f->data[2] = 'z';
  f->data[3] = 'z';
  return true;
}

static bool parseByte_xUnknownSubFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x05;
  f->len     = 6;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 'X';
  f->data[2] = 'z';
  f->data[3] = 'a';
  f->data[4] = 'p';
  f->data[5] = 'p';
  return true;
}

static bool parseByte_xLongUnknownSubFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId    = 0x0B;
  f->len      = 11;
  f->data[0]  = SIMHUB_ARQ_MSG_H;
  f->data[1]  = 'X';
  f->data[2]  = 'u';
  f->data[3]  = 'n';
  f->data[4]  = 'k';
  f->data[5]  = 'n';
  f->data[6]  = 'o';
  f->data[7]  = 'w';
  f->data[8]  = 'n';
  f->data[9]  = '\n';
  f->data[10] = 0;
  return true;
}

static bool parseByte_baudRateFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x09;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '8';
  return true;
}

static bool parseByte_ledDataUnsupportedModeFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x11;
  f->len     = 3;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '6';
  f->data[2] = 0x02;
  return true;
}

static bool parseByte_ledDataMode1ShortFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x12;
  f->len     = 4;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '6';
  f->data[2] = 0x01;
  f->data[3] = 0xFF;
  return true;
}

static const uint8_t kLedRgb[] = {
  0x11, 0x22, 0x33,
  0x44, 0x55, 0x66,
  0x77, 0x88, 0x99,
};

static bool parseByte_ledDataMode1Frame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x10;
  f->len     = 2 + 1 + 3 * 3;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = '6';
  f->data[2] = 0x01;
  memcpy(&f->data[3], kLedRgb, sizeof(kLedRgb));
  return true;
}

static bool parseByte_unknownCmdFrame(SimhubArqFrame_t *f, uint8_t byte)
{
  ARG_UNUSED(byte);
  f->pktId   = 0x20;
  f->len     = 2;
  f->data[0] = SIMHUB_ARQ_MSG_H;
  f->data[1] = 0xEE;
  return true;
}

/* ---- Suite setup / before ---- */

/**
 * Test setup function.
 */
static void *util_tests_setup(void)
{
  simhubDevUtilInit(mock_tx);
  return NULL;
}

/**
 * Test before function.
 */
static void util_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);
  simhubDevUtilReset();
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
  simhubArqBuildAck_fake.custom_fake     = buildAck_real;
  simhubArqBuildByte_fake.custom_fake    = buildByte_real;
  simhubArqBuildStr_fake.custom_fake     = buildStr_real;
  simhubArqBuildStrTerm_fake.custom_fake = buildStrTerm_real;
  mock_tx_fake.custom_fake               = mock_tx_capture;
  buildStr_succeed_n  = INT_MAX;
  buildStr_call_idx   = 0;
  buildByte_succeed_n = INT_MAX;
  buildByte_call_idx  = 0;
}

/* ===========================================================================
 * simhubDevUtilInit
 * =========================================================================*/

/**
 * @test The simhubDevUtilInit function must call simhubArqFrameReset, set the
 * session state to IDLE, and return 0.
 */
ZTEST(simhubDevUtil_tests, test_init_sets_idle_state)
{
  int result = simhubDevUtilInit(mock_tx);

  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Init must call simhubArqFrameReset once");
  zassert_not_null(simhubArqFrameReset_fake.arg0_val,
                   "Init must pass a non-NULL frame to simhubArqFrameReset");
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_IDLE,
                "Init must set session state to IDLE");
  zassert_equal(result, 0, "Init must return 0");
}

/* ===========================================================================
 * simhubDevUtilReset
 * =========================================================================*/

/**
 * @test The simhubDevUtilReset function must call simhubArqFrameReset, set
 * state to IDLE, and clear the pending LED frame flag.
 */
ZTEST(simhubDevUtil_tests, test_reset_clears_state_and_frame)
{
  simhubArqParseByte_fake.custom_fake = parseByte_helloFrame;
  simhubDevUtilReceivedByte(TEST_BYTE);
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_ENUMERATING,
                "pre-condition: state must be ENUMERATING");

  RESET_FAKE(simhubArqFrameReset);
  RESET_FAKE(simhubArqParseByte);
  simhubDevUtilReset();

  struct led_rgb frame[3];
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Reset must call simhubArqFrameReset once");
  zassert_not_null(simhubArqFrameReset_fake.arg0_val,
                   "Reset must pass a non-NULL frame to simhubArqFrameReset");
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_IDLE,
                "Reset must set state to IDLE");
  zassert_false(simhubDevUtilGetLedFrame(frame),
                "Reset must clear the pending LED frame flag");
}

/* ===========================================================================
 * simhubDevUtilReceivedByte
 * =========================================================================*/

/**
 * @test The simhubDevUtilReceivedByte function must return false when
 * simhubArqParseByte reports the frame is not yet complete.
 */
ZTEST(simhubDevUtil_tests, test_received_byte_returns_false_on_incomplete_frame)
{
  bool result = simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqParseByte_fake.call_count, 1,
                "ReceivedByte must call simhubArqParseByte once");
  zassert_equal(simhubArqParseByte_fake.arg1_val, TEST_BYTE,
                "ReceivedByte must pass the received byte to simhubArqParseByte");
  zassert_false(result,
                "ReceivedByte must return false when frame is incomplete");
  zassert_equal(simhubArqFrameReset_fake.call_count, 0,
                "ReceivedByte must not call simhubArqFrameReset on incomplete frame");
  zassert_equal(mock_tx_fake.call_count, 0,
                "ReceivedByte must not call txFn on incomplete frame");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn and must
 * leave the session state as IDLE when handleHello encounters a buildAck
 * failure.
 */
ZTEST(simhubDevUtil_tests, test_hello_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_helloFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Hello must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, SIMHUB_ARQ_BCAST,
                "Hello must call simhubArqBuildAck with broadcast ID");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Hello must not call txFn when buildAck fails");
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_IDLE,
                "Hello must not change state when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleHello encounters a buildByte failure after a successful buildAck.
 */
ZTEST(simhubDevUtil_tests, test_hello_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_helloFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Hello must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, SIMHUB_ARQ_BCAST,
                "Hello must call simhubArqBuildAck with broadcast ID");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Hello must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0x6A,
                "Hello must call simhubArqBuildByte with 0x6A");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Hello must not call txFn when buildByte fails");
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_IDLE,
                "Hello must not change state when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must transition the session
 * state to ENUMERATING when a Hello frame is received.
 */
ZTEST(simhubDevUtil_tests, test_hello_transitions_to_enumerating)
{
  simhubArqParseByte_fake.custom_fake = parseByte_helloFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqParseByte_fake.call_count, 1,
                "ReceivedByte must call simhubArqParseByte once");
  zassert_equal(simhubArqParseByte_fake.arg1_val, TEST_BYTE,
                "ReceivedByte must pass the received byte to simhubArqParseByte");
  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_ENUMERATING,
                "Hello must transition state to ENUMERATING");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK(broadcast)
 * followed by BYTE(0x6A) and reset the frame when a Hello frame is received.
 */
ZTEST(simhubDevUtil_tests, test_hello_sends_ack_broadcast_and_version_byte)
{
  simhubArqParseByte_fake.custom_fake = parseByte_helloFrame;

  bool result = simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_true(result,
               "ReceivedByte must return true on a complete frame");
  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Hello must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, SIMHUB_ARQ_BCAST,
                "Hello must call simhubArqBuildAck with broadcast ID");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Hello must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0x6A,
                "Hello must call simhubArqBuildByte with 0x6A");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Hello must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.call_count, 1,
                "Hello must call txFn once");
  zassert_equal(mock_tx_fake.arg1_val, 4,
                "Hello must pass 4 bytes to txFn");
  zassert_equal(txCapBuf[0], SIMHUB_ARQ_ACK,   "byte 0: ACK");
  zassert_equal(txCapBuf[1], SIMHUB_ARQ_BCAST, "byte 1: broadcast ID");
  zassert_equal(txCapBuf[2], SIMHUB_ARQ_BYTE,  "byte 2: BYTE type");
  zassert_equal(txCapBuf[3], 0x6A,             "byte 3: 0x6A");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Features must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x00,
                "Features must call simhubArqBuildAck with pkt ID 0x00");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a failure on the first buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_first_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;
  buildStr_succeed_n                  = 0;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "Features must call simhubArqBuildStr once before failure");
  zassert_equal(simhubArqBuildStr_fake.arg0_val[0], 'N',
                "Features first buildStr must be called with 'N'");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, 1,
                "Features first buildStr must be called with length 1");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when first buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a failure on the second buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_second_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;
  buildStr_succeed_n                  = 1;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 2,
                "Features must call simhubArqBuildStr twice before failure");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[0][0], 'N',
                "Features first buildStr must be called with 'N'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[1][0], 'I',
                "Features second buildStr must be called with 'I'");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when second buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a failure on the third buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_third_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;
  buildStr_succeed_n                  = 2;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 3,
                "Features must call simhubArqBuildStr three times before failure");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[1][0], 'I',
                "Features second buildStr must be called with 'I'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[2][0], 'J',
                "Features third buildStr must be called with 'J'");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when third buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a failure on the fourth buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_fourth_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;
  buildStr_succeed_n                  = 3;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 4,
                "Features must call simhubArqBuildStr four times before failure");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[2][0], 'J',
                "Features third buildStr must be called with 'J'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[3][0], 'X',
                "Features fourth buildStr must be called with 'X'");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when fourth buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleFeatures encounters a buildStrTerm failure.
 */
ZTEST(simhubDevUtil_tests, test_features_does_not_call_txfn_when_str_term_fails)
{
  simhubArqParseByte_fake.custom_fake    = parseByte_featuresFrame;
  simhubArqBuildStrTerm_fake.custom_fake = buildStrTerm_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 4,
                "Features must call simhubArqBuildStr four times");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[3][0], 'X',
                "Features fourth buildStr must be called with 'X'");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "Features must call simhubArqBuildStrTerm once");
  zassert_not_null(simhubArqBuildStrTerm_fake.arg0_val,
                   "Features must pass a non-NULL buf to simhubArqBuildStrTerm");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Features must not call txFn when buildStrTerm fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by four
 * single-char STR frames and a STRTERM when a Features frame is received.
 */
ZTEST(simhubDevUtil_tests, test_features_sends_four_feature_chars_and_terminator)
{
  simhubArqParseByte_fake.custom_fake = parseByte_featuresFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Features must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x00,
                "Features must call simhubArqBuildAck with pkt ID 0x00");
  zassert_equal(simhubArqBuildStr_fake.call_count, 4,
                "Features must call simhubArqBuildStr four times");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[0], 1, "first STR len must be 1");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[1], 1, "second STR len must be 1");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[2], 1, "third STR len must be 1");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[3], 1, "fourth STR len must be 1");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[0][0], 'N', "first STR must be 'N'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[1][0], 'I', "second STR must be 'I'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[2][0], 'J', "third STR must be 'J'");
  zassert_equal(simhubArqBuildStr_fake.arg0_history[3][0], 'X', "fourth STR must be 'X'");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "Features must call simhubArqBuildStrTerm once");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Features must call simhubArqFrameReset after dispatch");
  /* ACK(00) + STR(1,'N') + STR(1,'I') + STR(1,'J') + STR(1,'X') + STRTERM
   * = 2 + 4 + 4 + 4 + 4 + 4 = 22 bytes */
  zassert_equal(mock_tx_fake.arg1_val, 22,
                "Features must pass 22 bytes to txFn");
  zassert_equal(txCapBuf[4],  'N', "byte 4: 'N'");
  zassert_equal(txCapBuf[8],  'I', "byte 8: 'I'");
  zassert_equal(txCapBuf[12], 'J', "byte 12: 'J'");
  zassert_equal(txCapBuf[16], 'X', "byte 16: 'X'");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleLedCount encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_led_count_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledCountFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "LED count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x01,
                "LED count must call simhubArqBuildAck with pkt ID 0x01");
  zassert_equal(mock_tx_fake.call_count, 0,
                "LED count must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleLedCount encounters a buildByte failure after a successful buildAck.
 */
ZTEST(simhubDevUtil_tests, test_led_count_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledCountFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "LED count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x01,
                "LED count must call simhubArqBuildAck with pkt ID 0x01");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "LED count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 3,
                "LED count must call simhubArqBuildByte with SIMHUB_LED_COUNT (3)");
  zassert_equal(mock_tx_fake.call_count, 0,
                "LED count must not call txFn when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by
 * BYTE(SIMHUB_LED_COUNT) when a LED count frame is received.
 */
ZTEST(simhubDevUtil_tests, test_led_count_sends_ack_and_configured_count)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledCountFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x01,
                "LED count must call simhubArqBuildAck with pkt ID 0x01");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "LED count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 3,
                "LED count must call simhubArqBuildByte with SIMHUB_LED_COUNT (3)");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "LED count must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 4, "LED count must pass 4 bytes to txFn");
  zassert_equal(txCapBuf[2], SIMHUB_ARQ_BYTE, "byte 2: BYTE type");
  zassert_equal(txCapBuf[3], 3,               "byte 3: SIMHUB_LED_COUNT");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleTm1638Count encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_tm1638_count_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_tm1638CountFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "TM1638 count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x02,
                "TM1638 count must call simhubArqBuildAck with pkt ID 0x02");
  zassert_equal(mock_tx_fake.call_count, 0,
                "TM1638 count must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleTm1638Count encounters a buildByte failure after a successful buildAck.
 */
ZTEST(simhubDevUtil_tests, test_tm1638_count_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_tm1638CountFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "TM1638 count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x02,
                "TM1638 count must call simhubArqBuildAck with pkt ID 0x02");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "TM1638 count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "TM1638 count must call simhubArqBuildByte with 0");
  zassert_equal(mock_tx_fake.call_count, 0,
                "TM1638 count must not call txFn when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by
 * BYTE(0) when a TM1638 count frame is received.
 */
ZTEST(simhubDevUtil_tests, test_tm1638_count_sends_ack_and_zero)
{
  simhubArqParseByte_fake.custom_fake = parseByte_tm1638CountFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x02,
                "TM1638 count must call simhubArqBuildAck with pkt ID 0x02");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "TM1638 count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "TM1638 count must call simhubArqBuildByte with 0");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "TM1638 count must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 4, "TM1638 count must pass 4 bytes to txFn");
  zassert_equal(txCapBuf[3], 0, "byte 3: 0");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleModulesCount encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_modules_count_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_modulesCountFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Modules count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x03,
                "Modules count must call simhubArqBuildAck with pkt ID 0x03");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Modules count must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleModulesCount encounters a buildByte failure after a successful buildAck.
 */
ZTEST(simhubDevUtil_tests, test_modules_count_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_modulesCountFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Modules count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x03,
                "Modules count must call simhubArqBuildAck with pkt ID 0x03");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Modules count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "Modules count must call simhubArqBuildByte with 0");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Modules count must not call txFn when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by
 * BYTE(0) when a modules count frame is received.
 */
ZTEST(simhubDevUtil_tests, test_modules_count_sends_ack_and_zero)
{
  simhubArqParseByte_fake.custom_fake = parseByte_modulesCountFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x03,
                "Modules count must call simhubArqBuildAck with pkt ID 0x03");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Modules count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "Modules count must call simhubArqBuildByte with 0");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Modules count must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 4, "Modules count must pass 4 bytes to txFn");
  zassert_equal(txCapBuf[3], 0, "byte 3: 0");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleName encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_name_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_nameFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Name must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x05,
                "Name must call simhubArqBuildAck with pkt ID 0x05");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Name must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleName encounters a buildStr failure.
 */
ZTEST(simhubDevUtil_tests, test_name_does_not_call_txfn_when_build_str_fails)
{
  const char *name    = CONFIG_ENYA_SIMHUB_DEVICE_NAME;
  uint8_t     nameLen = (uint8_t)strlen(name);

  simhubArqParseByte_fake.custom_fake = parseByte_nameFrame;
  buildStr_succeed_n                  = 0;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "Name must call simhubArqBuildStr once");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_val, name, nameLen,
                    "Name buildStr must be called with CONFIG_ENYA_SIMHUB_DEVICE_NAME");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, nameLen,
                "Name buildStr must be called with the correct length");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Name must not call txFn when buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleName encounters a buildStrTerm failure.
 */
ZTEST(simhubDevUtil_tests, test_name_does_not_call_txfn_when_build_str_term_fails)
{
  simhubArqParseByte_fake.custom_fake    = parseByte_nameFrame;
  simhubArqBuildStrTerm_fake.custom_fake = buildStrTerm_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "Name must call simhubArqBuildStr once");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "Name must call simhubArqBuildStrTerm once");
  zassert_not_null(simhubArqBuildStrTerm_fake.arg0_val,
                   "Name must pass a non-NULL buf to simhubArqBuildStrTerm");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Name must not call txFn when buildStrTerm fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK, STR(name), and
 * STRTERM when a Name frame is received.
 */
ZTEST(simhubDevUtil_tests, test_name_sends_configured_name_string)
{
  const char *name    = CONFIG_ENYA_SIMHUB_DEVICE_NAME;
  uint8_t     nameLen = (uint8_t)strlen(name);

  simhubArqParseByte_fake.custom_fake = parseByte_nameFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x05,
                "Name must call simhubArqBuildAck with pkt ID 0x05");
  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "Name must call simhubArqBuildStr once");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, nameLen,
                "Name must call simhubArqBuildStr with the correct name length");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_val, name, nameLen,
                    "Name must call simhubArqBuildStr with CONFIG_ENYA_SIMHUB_DEVICE_NAME");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "Name must call simhubArqBuildStrTerm once");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Name must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, (size_t)(2 + 3 + nameLen + 4),
                "Name must pass the right number of bytes to txFn");
  zassert_mem_equal(&txCapBuf[4], name, nameLen,
                    "Name bytes must match CONFIG_ENYA_SIMHUB_DEVICE_NAME");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleUniqueId encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_unique_id_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_uidFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "UID must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x06,
                "UID must call simhubArqBuildAck with pkt ID 0x06");
  zassert_equal(mock_tx_fake.call_count, 0,
                "UID must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleUniqueId encounters a buildStr failure.
 */
ZTEST(simhubDevUtil_tests, test_unique_id_does_not_call_txfn_when_build_str_fails)
{
  const char *uid    = CONFIG_ENYA_SIMHUB_DEVICE_UID;
  uint8_t     uidLen = (uint8_t)strlen(uid);

  simhubArqParseByte_fake.custom_fake = parseByte_uidFrame;
  buildStr_succeed_n                  = 0;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "UID must call simhubArqBuildStr once");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_val, uid, uidLen,
                    "UID buildStr must be called with CONFIG_ENYA_SIMHUB_DEVICE_UID");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, uidLen,
                "UID buildStr must be called with the correct length");
  zassert_equal(mock_tx_fake.call_count, 0,
                "UID must not call txFn when buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleUniqueId encounters a buildStrTerm failure.
 */
ZTEST(simhubDevUtil_tests, test_unique_id_does_not_call_txfn_when_build_str_term_fails)
{
  simhubArqParseByte_fake.custom_fake    = parseByte_uidFrame;
  simhubArqBuildStrTerm_fake.custom_fake = buildStrTerm_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "UID must call simhubArqBuildStr once");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "UID must call simhubArqBuildStrTerm once");
  zassert_not_null(simhubArqBuildStrTerm_fake.arg0_val,
                   "UID must pass a non-NULL buf to simhubArqBuildStrTerm");
  zassert_equal(mock_tx_fake.call_count, 0,
                "UID must not call txFn when buildStrTerm fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK, STR(uid), and
 * STRTERM when a Unique ID frame is received.
 */
ZTEST(simhubDevUtil_tests, test_unique_id_sends_configured_uid_string)
{
  const char *uid    = CONFIG_ENYA_SIMHUB_DEVICE_UID;
  uint8_t     uidLen = (uint8_t)strlen(uid);

  simhubArqParseByte_fake.custom_fake = parseByte_uidFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x06,
                "UID must call simhubArqBuildAck with pkt ID 0x06");
  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "UID must call simhubArqBuildStr once");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, uidLen,
                "UID must call simhubArqBuildStr with the correct UID length");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_val, uid, uidLen,
                    "UID must call simhubArqBuildStr with CONFIG_ENYA_SIMHUB_DEVICE_UID");
  zassert_equal(simhubArqBuildStrTerm_fake.call_count, 1,
                "UID must call simhubArqBuildStrTerm once");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "UID must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, (size_t)(2 + 3 + uidLen + 4),
                "UID must pass the right number of bytes to txFn");
  zassert_mem_equal(&txCapBuf[4], uid, uidLen,
                    "UID bytes must match CONFIG_ENYA_SIMHUB_DEVICE_UID");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleButtonCount encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_button_count_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_buttonCountFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Button count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x07,
                "Button count must call simhubArqBuildAck with pkt ID 0x07");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Button count must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleButtonCount encounters a buildByte failure after a successful buildAck.
 */
ZTEST(simhubDevUtil_tests, test_button_count_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_buttonCountFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Button count must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x07,
                "Button count must call simhubArqBuildAck with pkt ID 0x07");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Button count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "Button count must call simhubArqBuildByte with 0");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Button count must not call txFn when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by
 * BYTE(0) when a button count frame is received.
 */
ZTEST(simhubDevUtil_tests, test_button_count_sends_ack_and_zero)
{
  simhubArqParseByte_fake.custom_fake = parseByte_buttonCountFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x07,
                "Button count must call simhubArqBuildAck with pkt ID 0x07");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "Button count must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0,
                "Button count must call simhubArqBuildByte with 0");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Button count must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 4, "Button count must pass 4 bytes to txFn");
  zassert_equal(txCapBuf[3], 0, "byte 3: 0");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleXCmd encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_x_cmd_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xListFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "X cmd must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x04,
                "X cmd must call simhubArqBuildAck with pkt ID 0x04");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X cmd must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK only when an
 * X command frame with a sub-command shorter than 4 bytes is received.
 */
ZTEST(simhubDevUtil_tests, test_x_cmd_short_sub_sends_ack_only)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xShortSubFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "X short sub must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x0A,
                "X short sub must call simhubArqBuildAck with pkt ID 0x0A");
  zassert_equal(simhubArqBuildStr_fake.call_count, 0,
                "X short sub must not call simhubArqBuildStr");
  zassert_equal(simhubArqBuildByte_fake.call_count, 0,
                "X short sub must not call simhubArqBuildByte");
  zassert_equal(mock_tx_fake.call_count, 1,
                "X short sub must call txFn once");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "X short sub must pass 2 bytes to txFn (ACK only)");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK only when an
 * X command frame with an unrecognised sub-command shorter than 7 bytes is
 * received.
 */
ZTEST(simhubDevUtil_tests, test_x_cmd_unknown_sub_sends_ack_only)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xUnknownSubFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "X unknown sub must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x05,
                "X unknown sub must call simhubArqBuildAck with pkt ID 0x05");
  zassert_equal(simhubArqBuildStr_fake.call_count, 0,
                "X unknown sub must not call simhubArqBuildStr");
  zassert_equal(simhubArqBuildByte_fake.call_count, 0,
                "X unknown sub must not call simhubArqBuildByte");
  zassert_equal(mock_tx_fake.call_count, 1,
                "X unknown sub must call txFn once");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "X unknown sub must pass 2 bytes to txFn (ACK only)");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK only when an
 * X command frame with a sub-command of 7 or more bytes that does not match
 * "mcutype" is received.
 */
ZTEST(simhubDevUtil_tests, test_x_cmd_long_unknown_sub_sends_ack_only)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xLongUnknownSubFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "X long unknown sub must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x0B,
                "X long unknown sub must call simhubArqBuildAck with pkt ID 0x0B");
  zassert_equal(simhubArqBuildStr_fake.call_count, 0,
                "X long unknown sub must not call simhubArqBuildStr");
  zassert_equal(simhubArqBuildByte_fake.call_count, 0,
                "X long unknown sub must not call simhubArqBuildByte");
  zassert_equal(mock_tx_fake.call_count, 1,
                "X long unknown sub must call txFn once");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "X long unknown sub must pass 2 bytes to txFn (ACK only)");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-list handler encounters a failure on the first buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_x_list_does_not_call_txfn_when_first_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xListFrame;
  buildStr_succeed_n                  = 0;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 1,
                "X list must call simhubArqBuildStr once before failure");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_val, "keepalive\n", 10,
                    "X list first buildStr must be called with 'keepalive\\n'");
  zassert_equal(simhubArqBuildStr_fake.arg1_val, 10,
                "X list first buildStr must have length 10");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X list must not call txFn when first buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-list handler encounters a failure on the second buildStr call.
 */
ZTEST(simhubDevUtil_tests, test_x_list_does_not_call_txfn_when_second_str_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xListFrame;
  buildStr_succeed_n                  = 1;
  simhubArqBuildStr_fake.custom_fake  = buildStr_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 2,
                "X list must call simhubArqBuildStr twice before failure");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_history[0], "keepalive\n", 10,
                    "X list first buildStr must be 'keepalive\\n'");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_history[1], "mcutype\n", 8,
                    "X list second buildStr must be 'mcutype\\n'");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[1], 8,
                "X list second buildStr must have length 8");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X list must not call txFn when second buildStr fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-list handler encounters a buildByte failure.
 */
ZTEST(simhubDevUtil_tests, test_x_list_does_not_call_txfn_when_build_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xListFrame;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildStr_fake.call_count, 2,
                "X list must call simhubArqBuildStr twice before buildByte");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "X list must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0x0A,
                "X list must call simhubArqBuildByte with 0x0A");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X list must not call txFn when buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK,
 * STR("keepalive\\n"), STR("mcutype\\n"), and BYTE(0x0A) when an X-list
 * frame is received.
 */
ZTEST(simhubDevUtil_tests, test_x_list_sends_keepalive_mcutype_and_end_marker)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xListFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x04,
                "X list must call simhubArqBuildAck with pkt ID 0x04");
  zassert_equal(simhubArqBuildStr_fake.call_count, 2,
                "X list must call simhubArqBuildStr twice");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[0], 10,
                "first STR must have length 10 ('keepalive\\n')");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_history[0], "keepalive\n", 10,
                    "first STR must be 'keepalive\\n'");
  zassert_equal(simhubArqBuildStr_fake.arg1_history[1], 8,
                "second STR must have length 8 ('mcutype\\n')");
  zassert_mem_equal(simhubArqBuildStr_fake.arg0_history[1], "mcutype\n", 8,
                    "second STR must be 'mcutype\\n'");
  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "X list must call simhubArqBuildByte once");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0x0A,
                "X list must call simhubArqBuildByte with 0x0A");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "X list must call simhubArqFrameReset after dispatch");
  /* ACK(04) + STR(10) + STR(8) + BYTE = 2 + 13 + 11 + 2 = 28 bytes */
  zassert_equal(mock_tx_fake.arg1_val, 28, "X list must pass 28 bytes to txFn");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-mcutype handler encounters a failure on the first buildByte call.
 */
ZTEST(simhubDevUtil_tests, test_x_mcutype_does_not_call_txfn_when_first_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xMcutypeFrame;
  buildByte_succeed_n                 = 0;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildByte_fake.call_count, 1,
                "X mcutype must call simhubArqBuildByte once before failure");
  zassert_equal(simhubArqBuildByte_fake.arg0_val, 0x1E,
                "X mcutype first buildByte must be called with 0x1E");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X mcutype must not call txFn when first buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-mcutype handler encounters a failure on the second buildByte call.
 */
ZTEST(simhubDevUtil_tests, test_x_mcutype_does_not_call_txfn_when_second_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xMcutypeFrame;
  buildByte_succeed_n                 = 1;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildByte_fake.call_count, 2,
                "X mcutype must call simhubArqBuildByte twice before failure");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[0], 0x1E,
                "X mcutype first buildByte must be called with 0x1E");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[1], 0x98,
                "X mcutype second buildByte must be called with 0x98");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X mcutype must not call txFn when second buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * X-mcutype handler encounters a failure on the third buildByte call.
 */
ZTEST(simhubDevUtil_tests, test_x_mcutype_does_not_call_txfn_when_third_byte_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xMcutypeFrame;
  buildByte_succeed_n                 = 2;
  simhubArqBuildByte_fake.custom_fake = buildByte_fail_after_n;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildByte_fake.call_count, 3,
                "X mcutype must call simhubArqBuildByte three times before failure");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[1], 0x98,
                "X mcutype second buildByte must be called with 0x98");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[2], 0x01,
                "X mcutype third buildByte must be called with 0x01");
  zassert_equal(mock_tx_fake.call_count, 0,
                "X mcutype must not call txFn when third buildByte fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK followed by
 * BYTE(0x1E), BYTE(0x98), BYTE(0x01) when an X-mcutype frame is received.
 */
ZTEST(simhubDevUtil_tests, test_x_mcutype_sends_three_byte_values)
{
  simhubArqParseByte_fake.custom_fake = parseByte_xMcutypeFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x08,
                "X mcutype must call simhubArqBuildAck with pkt ID 0x08");
  zassert_equal(simhubArqBuildByte_fake.call_count, 3,
                "X mcutype must call simhubArqBuildByte three times");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[0], 0x1E,
                "first BYTE must be 0x1E");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[1], 0x98,
                "second BYTE must be 0x98");
  zassert_equal(simhubArqBuildByte_fake.arg0_history[2], 0x01,
                "third BYTE must be 0x01");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "X mcutype must call simhubArqFrameReset after dispatch");
  /* ACK(08) + BYTE(1E) + BYTE(98) + BYTE(01) = 8 bytes */
  zassert_equal(mock_tx_fake.arg1_val, 8, "X mcutype must pass 8 bytes to txFn");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleBaudRate encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_baud_rate_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_baudRateFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Baud rate must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x09,
                "Baud rate must call simhubArqBuildAck with pkt ID 0x09");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Baud rate must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK only when a
 * baud rate frame is received.
 */
ZTEST(simhubDevUtil_tests, test_baud_rate_sends_ack_only)
{
  simhubArqParseByte_fake.custom_fake = parseByte_baudRateFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x09,
                "Baud rate must call simhubArqBuildAck with pkt ID 0x09");
  zassert_equal(simhubArqBuildByte_fake.call_count, 0,
                "Baud rate must not call simhubArqBuildByte");
  zassert_equal(simhubArqBuildStr_fake.call_count, 0,
                "Baud rate must not call simhubArqBuildStr");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Baud rate must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "Baud rate must pass 2 bytes to txFn (ACK only)");
  zassert_equal(txCapBuf[0], SIMHUB_ARQ_ACK, "byte 0: ACK");
  zassert_equal(txCapBuf[1], 0x09,           "byte 1: pkt ID");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when
 * handleLedData encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_led_data_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataMode1Frame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "LED data must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x10,
                "LED data must call simhubArqBuildAck with pkt ID 0x10");
  zassert_equal(mock_tx_fake.call_count, 0,
                "LED data must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK without setting
 * a pending frame when a mode-1 LED data frame with too few payload bytes is
 * received.
 */
ZTEST(simhubDevUtil_tests, test_led_data_mode1_payload_too_short_acks_without_frame)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataMode1ShortFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "LED data short must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x12,
                "LED data short must call simhubArqBuildAck with pkt ID 0x12");
  zassert_equal(mock_tx_fake.call_count, 1,
                "LED data short must call txFn once");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "LED data short must pass 2 bytes to txFn (ACK only)");

  struct led_rgb frame[3];
  zassert_false(simhubDevUtilGetLedFrame(frame),
                "LED data short must not set a pending frame");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK without setting
 * a pending frame when an LED data frame with an unsupported mode is received.
 */
ZTEST(simhubDevUtil_tests, test_led_data_unsupported_mode_acks_without_frame)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataUnsupportedModeFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x11,
                "Unsupported LED mode must call simhubArqBuildAck with pkt ID 0x11");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Unsupported LED mode must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "Unsupported LED mode must pass 2 bytes to txFn (ACK only)");

  struct led_rgb frame[3];
  zassert_false(simhubDevUtilGetLedFrame(frame),
                "Unsupported LED mode must not set a pending frame");
}

/**
 * @test The simhubDevUtilReceivedByte function must unpack all RGB pixels into
 * the pending frame and send ACK when a mode-1 LED data frame is received.
 */
ZTEST(simhubDevUtil_tests, test_led_data_mode1_fills_all_leds_and_acks)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataMode1Frame;

  bool result = simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_true(result, "ReceivedByte must return true on a complete frame");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x10,
                "LED data must call simhubArqBuildAck with pkt ID 0x10");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "LED data must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "LED data must pass 2 bytes to txFn (ACK only)");

  struct led_rgb frame[3];
  zassert_true(simhubDevUtilGetLedFrame(frame),
               "GetLedFrame must return true after LED data");
  for(int i = 0; i < 3; i++)
  {
    zassert_equal(frame[i].r, kLedRgb[i * 3],     "pixel %d red must match",   i);
    zassert_equal(frame[i].g, kLedRgb[i * 3 + 1], "pixel %d green must match", i);
    zassert_equal(frame[i].b, kLedRgb[i * 3 + 2], "pixel %d blue must match",  i);
  }
}

/**
 * @test The simhubDevUtilReceivedByte function must transition state to
 * STREAMING when a mode-1 LED data frame is received.
 */
ZTEST(simhubDevUtil_tests, test_led_data_mode1_transitions_to_streaming)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataMode1Frame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubDevUtilGetState(), SIMHUB_ARQ_STREAMING,
                "LED data mode 1 must transition state to STREAMING");
}

/**
 * @test The simhubDevUtilReceivedByte function must not call txFn when the
 * default handler encounters a buildAck failure.
 */
ZTEST(simhubDevUtil_tests, test_unknown_command_does_not_call_txfn_when_build_ack_fails)
{
  simhubArqParseByte_fake.custom_fake = parseByte_unknownCmdFrame;
  simhubArqBuildAck_fake.custom_fake  = buildAck_fail;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Unknown command must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x20,
                "Unknown command must call simhubArqBuildAck with pkt ID 0x20");
  zassert_equal(mock_tx_fake.call_count, 0,
                "Unknown command must not call txFn when buildAck fails");
}

/**
 * @test The simhubDevUtilReceivedByte function must send ACK when an unknown
 * command byte is received.
 */
ZTEST(simhubDevUtil_tests, test_unknown_command_sends_ack)
{
  simhubArqParseByte_fake.custom_fake = parseByte_unknownCmdFrame;

  simhubDevUtilReceivedByte(TEST_BYTE);

  zassert_equal(simhubArqBuildAck_fake.call_count, 1,
                "Unknown command must call simhubArqBuildAck once");
  zassert_equal(simhubArqBuildAck_fake.arg0_val, 0x20,
                "Unknown command must call simhubArqBuildAck with pkt ID 0x20");
  zassert_equal(simhubArqBuildByte_fake.call_count, 0,
                "Unknown command must not call simhubArqBuildByte");
  zassert_equal(simhubArqBuildStr_fake.call_count, 0,
                "Unknown command must not call simhubArqBuildStr");
  zassert_equal(simhubArqFrameReset_fake.call_count, 1,
                "Unknown command must call simhubArqFrameReset after dispatch");
  zassert_equal(mock_tx_fake.arg1_val, 2,
                "Unknown command must pass 2 bytes to txFn (ACK only)");
}

/* ===========================================================================
 * simhubDevUtilGetLedFrame
 * =========================================================================*/

/**
 * @test The simhubDevUtilGetLedFrame function must return false when no LED
 * frame is pending.
 */
ZTEST(simhubDevUtil_tests, test_get_led_frame_returns_false_when_no_data)
{
  struct led_rgb frame[3];

  bool result = simhubDevUtilGetLedFrame(frame);

  zassert_false(result,
                "GetLedFrame must return false when no frame is pending");
}

/**
 * @test The simhubDevUtilGetLedFrame function must return true on the first
 * call when a frame is pending and false on the second call once the flag is
 * cleared.
 */
ZTEST(simhubDevUtil_tests, test_get_led_frame_returns_true_and_clears_flag)
{
  simhubArqParseByte_fake.custom_fake = parseByte_ledDataMode1Frame;
  simhubDevUtilReceivedByte(TEST_BYTE);

  struct led_rgb frame[3];
  bool first  = simhubDevUtilGetLedFrame(frame);
  bool second = simhubDevUtilGetLedFrame(frame);

  zassert_true(first,
               "GetLedFrame must return true when a frame is pending");
  zassert_false(second,
                "GetLedFrame must return false after the flag is cleared");
}

ZTEST_SUITE(simhubDevUtil_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
