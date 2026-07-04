/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub ARQ Transport Protocol Tests
 *
 *            Unit tests for simhubArqProto: CRC-8 wrapper, frame parser, and
 *            response builders.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Mock Zephyr CRC-8 to remove the external dependency. */
#include <zephyr/sys/crc.h>
FAKE_VALUE_FUNC(uint8_t, crc8, const uint8_t *, size_t, uint8_t, uint8_t, bool);

#include "simhubArqProto.c"

/* ---- helpers ---- */

static bool feedPacket(SimhubArqFrame_t *f, const uint8_t *pkt, size_t len)
{
  bool result = false;
  for(size_t i = 0; i < len; i++)
    result = simhubArqParseByte(f, pkt[i]);
  return result;
}

/* Features-query frame: 01 01 00 02 03 30 38
 * CRC covers [ID=00, LEN=02, DATA=03 30] → 0x38 */
static const uint8_t kFeaturesFrame[] = {0x01, 0x01, 0x00, 0x02, 0x03, 0x30, 0x38};
#define FEATURES_FRAME_CRC      0x38
#define FEATURES_FRAME_CRC_LEN  4   /* 2 header bytes + LEN=2 */

/* Hello frame: 01 01 FF 03 03 31 10 6A
 * CRC covers [ID=FF, LEN=03, DATA=03 31 10] → 0x6A */
static const uint8_t kHelloFrame[] = {0x01, 0x01, 0xFF, 0x03, 0x03, 0x31, 0x10, 0x6A};
#define HELLO_FRAME_CRC      0x6A
#define HELLO_FRAME_CRC_LEN  5   /* 2 header bytes + LEN=3 */

static void *arq_proto_setup(void)
{
  return NULL;
}

static void arq_proto_before(void *fixture)
{
  ARG_UNUSED(fixture);
  RESET_FAKE(crc8);
}

/* ===========================================================================
 * simhubArqCrc8
 * =========================================================================*/

/**
 * @test simhubArqCrc8 must call Zephyr crc8 with the input buffer, the input
 * length, polynomial 0xD5, seed 0, and reflected=false.
 */
ZTEST(simhubArqProto_tests, test_crc8_passes_correct_params_to_zephyr_crc8)
{
  const uint8_t input[] = {0xFF, 0x03, 0x03, 0x31, 0x10};
  crc8_fake.return_val = HELLO_FRAME_CRC;

  simhubArqCrc8(input, sizeof(input));

  zassert_equal(crc8_fake.call_count, 1,
                "crc8 must be called exactly once");
  zassert_equal(crc8_fake.arg0_val, input,
                "crc8 must receive the input pointer");
  zassert_equal(crc8_fake.arg1_val, sizeof(input),
                "crc8 must receive the correct length");
  zassert_equal(crc8_fake.arg2_val, 0xD5,
                "crc8 must use polynomial 0xD5");
  zassert_equal(crc8_fake.arg3_val, 0,
                "crc8 must use seed 0");
  zassert_equal(crc8_fake.arg4_val, false,
                "crc8 must not use reflection");
}

/**
 * @test simhubArqCrc8 must return the value produced by Zephyr crc8 unchanged.
 */
ZTEST(simhubArqProto_tests, test_crc8_returns_zephyr_crc8_result)
{
  const uint8_t input[] = {0x00, 0x02, 0x03, 0x30};
  crc8_fake.return_val = FEATURES_FRAME_CRC;

  uint8_t result = simhubArqCrc8(input, sizeof(input));

  zassert_equal(crc8_fake.call_count, 1,
                "crc8 must be called exactly once");
  zassert_equal(crc8_fake.arg0_val, input,
                "crc8 must receive the input pointer");
  zassert_equal(crc8_fake.arg1_val, sizeof(input),
                "crc8 must receive the correct length");
  zassert_equal(crc8_fake.arg2_val, 0xD5,
                "crc8 must use polynomial 0xD5");
  zassert_equal(crc8_fake.arg3_val, 0,
                "crc8 must use seed 0");
  zassert_equal(crc8_fake.arg4_val, false,
                "crc8 must not use reflection");
  zassert_equal(result, FEATURES_FRAME_CRC,
                "simhubArqCrc8 must return the crc8 result unchanged");
}

/* ===========================================================================
 * simhubArqFrameReset
 * =========================================================================*/

/**
 * @test simhubArqFrameReset must set the parser state to ARQ_SYNC0.
 */
ZTEST(simhubArqProto_tests, test_frameReset_sets_state_to_sync0)
{
  SimhubArqFrame_t frame;
  memset(&frame, 0xFF, sizeof(frame));

  simhubArqFrameReset(&frame);

  zassert_equal(frame.state, ARQ_SYNC0,
                "state must be ARQ_SYNC0 after reset");
}

/**
 * @test simhubArqFrameReset must clear pktId, len, and dataIdx to 0.
 */
ZTEST(simhubArqProto_tests, test_frameReset_clears_pktId_len_dataIdx)
{
  SimhubArqFrame_t frame;
  memset(&frame, 0xFF, sizeof(frame));

  simhubArqFrameReset(&frame);

  zassert_equal(frame.pktId,   0, "pktId must be 0 after reset");
  zassert_equal(frame.len,     0, "len must be 0 after reset");
  zassert_equal(frame.dataIdx, 0, "dataIdx must be 0 after reset");
}

/* ===========================================================================
 * simhubArqParseByte
 * =========================================================================*/

/**
 * @test simhubArqParseByte must return false and reset to ARQ_SYNC0 when the
 * frame is in an unrecognized state.
 */
ZTEST(simhubArqProto_tests, test_parseByte_resets_on_unknown_state)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  frame.state = (SimhubArqParseState_t)99;

  bool result = simhubArqParseByte(&frame, 0x01);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called on an unknown state");
  zassert_false(result,
                "parseByte must return false on an unknown state");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on an unknown state");
}

/**
 * @test simhubArqParseByte must return false and stay in ARQ_SYNC0 when the
 * first byte is not 0x01.
 */
ZTEST(simhubArqProto_tests, test_parseByte_ignores_non_01_first_byte)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);

  bool result = simhubArqParseByte(&frame, 0xAA);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called before the CRC state");
  zassert_false(result,
                "parseByte must return false on a non-header first byte");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must remain ARQ_SYNC0 on a non-header first byte");
}

/**
 * @test simhubArqParseByte must return false and reset to ARQ_SYNC0 when the
 * second byte is not 0x01.
 */
ZTEST(simhubArqProto_tests, test_parseByte_resets_on_bad_second_byte)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  simhubArqParseByte(&frame, 0x01);

  bool result = simhubArqParseByte(&frame, 0xAA);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called before the CRC state");
  zassert_false(result,
                "parseByte must return false on a bad second header byte");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on a bad second header byte");
}

/**
 * @test simhubArqParseByte must return false and reset to ARQ_SYNC0 when the
 * length field is zero.
 */
ZTEST(simhubArqProto_tests, test_parseByte_rejects_zero_length)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  simhubArqParseByte(&frame, 0x01);
  simhubArqParseByte(&frame, 0x01);
  simhubArqParseByte(&frame, 0x05);

  bool result = simhubArqParseByte(&frame, 0x00);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called before the CRC state");
  zassert_false(result,
                "parseByte must return false on zero length");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on zero length");
}

/**
 * @test simhubArqParseByte must return false and reset to ARQ_SYNC0 when the
 * length field exceeds SIMHUB_ARQ_MAX_DATA.
 */
ZTEST(simhubArqProto_tests, test_parseByte_rejects_overlength)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  simhubArqParseByte(&frame, 0x01);
  simhubArqParseByte(&frame, 0x01);
  simhubArqParseByte(&frame, 0x05);

  bool result = simhubArqParseByte(&frame, SIMHUB_ARQ_MAX_DATA + 1);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called before the CRC state");
  zassert_false(result,
                "parseByte must return false when length exceeds MAX_DATA");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on overlength");
}

/**
 * @test simhubArqParseByte must return false for every byte up to and
 * including the last data byte of a valid frame.
 */
ZTEST(simhubArqProto_tests, test_parseByte_returns_false_while_accumulating)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);

  for(size_t i = 0; i < sizeof(kFeaturesFrame) - 1; i++)
  {
    bool result = simhubArqParseByte(&frame, kFeaturesFrame[i]);
    zassert_false(result,
                  "parseByte must return false before the CRC byte");
  }

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called before the CRC byte");
}

/**
 * @test simhubArqParseByte must return false and reset to ARQ_SYNC0 on a CRC
 * mismatch.
 */
ZTEST(simhubArqProto_tests, test_parseByte_rejects_mismatched_crc_and_resets)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  for(size_t i = 0; i < sizeof(kFeaturesFrame) - 1; i++)
    simhubArqParseByte(&frame, kFeaturesFrame[i]);
  crc8_fake.return_val = FEATURES_FRAME_CRC;

  bool result = simhubArqParseByte(&frame, 0xFF);  /* wrong CRC byte */

  zassert_equal(crc8_fake.call_count, 1,
                "crc8 must be called once when the CRC byte is received");
  zassert_not_null((void *)crc8_fake.arg0_val,
                   "crc8 must receive a non-NULL buffer");
  zassert_equal(crc8_fake.arg1_val, FEATURES_FRAME_CRC_LEN,
                "crc8 must receive pktId + len + data length");
  zassert_equal(crc8_fake.arg2_val, 0xD5,
                "crc8 must use polynomial 0xD5");
  zassert_equal(crc8_fake.arg3_val, 0,
                "crc8 must use seed 0");
  zassert_equal(crc8_fake.arg4_val, false,
                "crc8 must not use reflection");
  zassert_false(result,
                "parseByte must return false on CRC mismatch");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on CRC mismatch");
}

/**
 * @test simhubArqParseByte must return false and reset when a byte is fed
 * while already in ARQ_DONE.
 */
ZTEST(simhubArqProto_tests, test_parseByte_resets_on_byte_in_done_state)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  crc8_fake.return_val = FEATURES_FRAME_CRC;
  feedPacket(&frame, kFeaturesFrame, sizeof(kFeaturesFrame));
  zassert_equal(frame.state, ARQ_DONE, "pre-condition: frame must be in ARQ_DONE");
  RESET_FAKE(crc8);

  bool result = simhubArqParseByte(&frame, 0x00);

  zassert_equal(crc8_fake.call_count, 0,
                "crc8 must not be called when resetting from ARQ_DONE");
  zassert_false(result,
                "parseByte must return false when a byte is fed in ARQ_DONE");
  zassert_equal(frame.state, ARQ_SYNC0,
                "state must reset to ARQ_SYNC0 on byte received in ARQ_DONE");
}

/**
 * @test simhubArqParseByte must return true and populate the frame correctly
 * when a complete, CRC-valid Features-query frame is received.
 */
ZTEST(simhubArqProto_tests, test_parseByte_returns_true_on_valid_features_frame)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  crc8_fake.return_val = FEATURES_FRAME_CRC;

  bool result = feedPacket(&frame, kFeaturesFrame, sizeof(kFeaturesFrame));

  zassert_equal(crc8_fake.call_count, 1,
                "crc8 must be called exactly once for the CRC check");
  zassert_not_null((void *)crc8_fake.arg0_val,
                   "crc8 must receive a non-NULL buffer");
  zassert_equal(crc8_fake.arg1_val, FEATURES_FRAME_CRC_LEN,
                "crc8 must receive pktId + len + data length");
  zassert_equal(crc8_fake.arg2_val, 0xD5,
                "crc8 must use polynomial 0xD5");
  zassert_equal(crc8_fake.arg3_val, 0,
                "crc8 must use seed 0");
  zassert_equal(crc8_fake.arg4_val, false,
                "crc8 must not use reflection");
  zassert_true(result,
               "parseByte must return true on the CRC byte of a valid frame");
  zassert_equal(frame.state,   ARQ_DONE, "state must be ARQ_DONE");
  zassert_equal(frame.pktId,   0x00,     "pktId must be 0x00");
  zassert_equal(frame.len,     0x02,     "len must be 2");
  zassert_equal(frame.data[0], 0x03,     "data[0] must be MESSAGE_HEADER");
  zassert_equal(frame.data[1], 0x30,     "data[1] must be '0' (features cmd)");
}

/**
 * @test simhubArqParseByte must return true for the Hello broadcast frame
 * (ID=0xFF).
 */
ZTEST(simhubArqProto_tests, test_parseByte_returns_true_on_valid_hello_frame)
{
  SimhubArqFrame_t frame;
  simhubArqFrameReset(&frame);
  crc8_fake.return_val = HELLO_FRAME_CRC;

  bool result = feedPacket(&frame, kHelloFrame, sizeof(kHelloFrame));

  zassert_equal(crc8_fake.call_count, 1,
                "crc8 must be called exactly once for the CRC check");
  zassert_not_null((void *)crc8_fake.arg0_val,
                   "crc8 must receive a non-NULL buffer");
  zassert_equal(crc8_fake.arg1_val, HELLO_FRAME_CRC_LEN,
                "crc8 must receive pktId + len + data length");
  zassert_equal(crc8_fake.arg2_val, 0xD5,
                "crc8 must use polynomial 0xD5");
  zassert_equal(crc8_fake.arg3_val, 0,
                "crc8 must use seed 0");
  zassert_equal(crc8_fake.arg4_val, false,
                "crc8 must not use reflection");
  zassert_true(result,
               "parseByte must return true for a valid Hello frame");
  zassert_equal(frame.pktId,   0xFF, "pktId must be broadcast 0xFF");
  zassert_equal(frame.len,     0x03, "len must be 3");
  zassert_equal(frame.data[1], '1',  "data[1] must be '1' (Hello cmd)");
}

/* ===========================================================================
 * simhubArqBuildAck
 * =========================================================================*/

/**
 * @test simhubArqBuildAck must return -ENOMEM when the buffer is too small.
 */
ZTEST(simhubArqProto_tests, test_buildAck_returns_enomem_when_buffer_too_small)
{
  uint8_t buf[1];

  int result = simhubArqBuildAck(0x05, buf, sizeof(buf));

  zassert_equal(result, -ENOMEM,
                "buildAck must return -ENOMEM for a 1-byte buffer");
}

/**
 * @test simhubArqBuildAck must write 0x03 followed by the packet ID and
 * return 2.
 */
ZTEST(simhubArqProto_tests, test_buildAck_writes_03_and_id)
{
  uint8_t buf[4];
  memset(buf, 0, sizeof(buf));

  int result = simhubArqBuildAck(0xAB, buf, sizeof(buf));

  zassert_equal(result, 2,    "buildAck must return 2");
  zassert_equal(buf[0], 0x03, "buf[0] must be 0x03 (ACK)");
  zassert_equal(buf[1], 0xAB, "buf[1] must be the packet ID");
}

/* ===========================================================================
 * simhubArqBuildByte
 * =========================================================================*/

/**
 * @test simhubArqBuildByte must return -ENOMEM when the buffer is too small.
 */
ZTEST(simhubArqProto_tests, test_buildByte_returns_enomem_when_buffer_too_small)
{
  uint8_t buf[1];

  int result = simhubArqBuildByte(0x42, buf, sizeof(buf));

  zassert_equal(result, -ENOMEM,
                "buildByte must return -ENOMEM for a 1-byte buffer");
}

/**
 * @test simhubArqBuildByte must write 0x08 followed by the value and return 2.
 */
ZTEST(simhubArqProto_tests, test_buildByte_writes_08_and_val)
{
  uint8_t buf[4];
  memset(buf, 0, sizeof(buf));

  int result = simhubArqBuildByte(0x42, buf, sizeof(buf));

  zassert_equal(result, 2,    "buildByte must return 2");
  zassert_equal(buf[0], 0x08, "buf[0] must be 0x08 (BYTE)");
  zassert_equal(buf[1], 0x42, "buf[1] must be the value");
}

/* ===========================================================================
 * simhubArqBuildStr
 * =========================================================================*/

/**
 * @test simhubArqBuildStr must return -ENOMEM when the buffer is too small to
 * hold 0x06 + length byte + string + 0x20.
 */
ZTEST(simhubArqProto_tests, test_buildStr_returns_enomem_when_buffer_too_small)
{
  uint8_t buf[4];  /* 3 + 2 = 5 bytes needed for a 2-char string */

  int result = simhubArqBuildStr("hi", 2, buf, sizeof(buf));

  zassert_equal(result, -ENOMEM,
                "buildStr must return -ENOMEM when buffer is too small");
}

/**
 * @test simhubArqBuildStr must write 0x06, the length, the string bytes, and
 * 0x20, returning (3 + len).
 */
ZTEST(simhubArqProto_tests, test_buildStr_writes_06_len_str_20)
{
  uint8_t buf[8];
  memset(buf, 0, sizeof(buf));

  int result = simhubArqBuildStr("AB", 2, buf, sizeof(buf));

  zassert_equal(result, 5,    "buildStr must return 3 + len");
  zassert_equal(buf[0], 0x06, "buf[0] must be 0x06 (STR)");
  zassert_equal(buf[1], 0x02, "buf[1] must be the string length");
  zassert_equal(buf[2], 'A',  "buf[2] must be the first string byte");
  zassert_equal(buf[3], 'B',  "buf[3] must be the second string byte");
  zassert_equal(buf[4], 0x20, "buf[4] must be 0x20");
}

/* ===========================================================================
 * simhubArqBuildStrTerm
 * =========================================================================*/

/**
 * @test simhubArqBuildStrTerm must return -ENOMEM when the buffer is too small
 * to hold the 4-byte terminator.
 */
ZTEST(simhubArqProto_tests, test_buildStrTerm_returns_enomem_when_buffer_too_small)
{
  uint8_t buf[3];

  int result = simhubArqBuildStrTerm(buf, sizeof(buf));

  zassert_equal(result, -ENOMEM,
                "buildStrTerm must return -ENOMEM for a 3-byte buffer");
}

/**
 * @test simhubArqBuildStrTerm must write 06 01 0A 20 and return 4.
 */
ZTEST(simhubArqProto_tests, test_buildStrTerm_writes_06_01_0A_20)
{
  uint8_t buf[8];
  memset(buf, 0, sizeof(buf));

  int result = simhubArqBuildStrTerm(buf, sizeof(buf));

  zassert_equal(result, 4,    "buildStrTerm must return 4");
  zassert_equal(buf[0], 0x06, "buf[0] must be 0x06 (STR)");
  zassert_equal(buf[1], 0x01, "buf[1] must be 0x01 (length=1)");
  zassert_equal(buf[2], 0x0A, "buf[2] must be 0x0A (newline)");
  zassert_equal(buf[3], 0x20, "buf[3] must be 0x20");
}

ZTEST_SUITE(simhubArqProto_tests, NULL, arq_proto_setup, arq_proto_before, NULL, NULL);
