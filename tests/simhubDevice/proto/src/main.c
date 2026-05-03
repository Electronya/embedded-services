/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Protocol Tests
 *
 *            Unit tests for the SimHub protocol per-state byte handlers.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <string.h>

/* Prevent led_strip driver header — define only the type we need. */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_
struct led_rgb { uint8_t r; uint8_t g; uint8_t b; };

/* Setup logging before including the module under test. */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Override DT macros so SIMHUB_LED_COUNT resolves to 3 without real DTS. */
#undef DT_ALIAS
#define DT_ALIAS(name) DT_N_NODELABEL_test_led_strip

#define DT_N_NODELABEL_test_led_strip_P_chain_length 3

#include "simhubDevProto.c"

static void *proto_tests_setup(void)
{
  return NULL;
}

static void proto_tests_before(void *fixture)
{
  /* Corrupt the context so reset tests are not relying on zero-init. */
  memset(&ctx, 0xFF, sizeof(ctx));
}

/**
 * @test simhubDevProtoReset must set the parser state to SIMHUB_PROTO_EMPTY_BUFF.
 */
ZTEST(simhubDevProto_tests, test_reset_sets_empty_buff_state)
{
  simhubDevProtoReset();

  zassert_equal(ctx.state, SIMHUB_PROTO_EMPTY_BUFF,
                "state must be EMPTY_BUFF after reset");
}

/**
 * @test simhubDevProtoReset must set the synchronization counter to 0.
 */
ZTEST(simhubDevProto_tests, test_reset_clears_sync_count)
{
  simhubDevProtoReset();

  zassert_equal(ctx.syncCount, 0,
                "syncCount must be 0 after reset");
}

/**
 * @test simhubDevProtoReset must set the command length to 0.
 */
ZTEST(simhubDevProto_tests, test_reset_clears_cmd_len)
{
  simhubDevProtoReset();

  zassert_equal(ctx.cmdLen, 0,
                "cmdLen must be 0 after reset");
}

/**
 * @test simhubDevProtoReset must set the packet type to SIMHUB_PKT_TYPE_COUNT.
 */
ZTEST(simhubDevProto_tests, test_reset_sets_pkt_type_to_sentinel)
{
  simhubDevProtoReset();

  zassert_equal(ctx.pktType, SIMHUB_PKT_TYPE_COUNT,
                "pktType must be SIMHUB_PKT_TYPE_COUNT after reset");
}

/**
 * @test simhubDevProtoReset must set the data index to 0.
 */
ZTEST(simhubDevProto_tests, test_reset_clears_data_idx)
{
  simhubDevProtoReset();

  zassert_equal(ctx.dataIdx, 0,
                "dataIdx must be 0 after reset");
}

/**
 * @test simhubDevProtoReset must set the footer index to 0.
 */
ZTEST(simhubDevProto_tests, test_reset_clears_footer_idx)
{
  simhubDevProtoReset();

  zassert_equal(ctx.footerIdx, 0,
                "footerIdx must be 0 after reset");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_EMPTY_BUFF after reset.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_empty_buff_after_reset)
{
  simhubDevProtoReset();

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "GetState must return EMPTY_BUFF after reset");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_RECEIVING_PREAMBLE
 * after a 0xFF byte is processed in the EMPTY_BUFF state.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_receiving_preamble)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_PREAMBLE,
                "GetState must return RECEIVING_PREAMBLE after first preamble byte");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_RECEIVING_CMD after
 * a complete 6-byte preamble has been received.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_receiving_cmd)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_CMD,
                "GetState must return RECEIVING_CMD after complete preamble");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_RECEIVING_LED_DATA
 * after the "sleds" command has been received.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_receiving_led_data)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_DATA,
                "GetState must return RECEIVING_LED_DATA after 'sleds' command");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_RECEIVING_LED_FOOTER
 * after all pixel buffer bytes have been received.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_receiving_led_footer)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_FOOTER,
                "GetState must return RECEIVING_LED_FOOTER after all pixel buffer bytes");
}

/**
 * @test simhubDevProtoGetState must return SIMHUB_PROTO_RECEIVED_PKT after
 * a complete packet has been parsed.
 */
ZTEST(simhubDevProto_tests, test_get_state_returns_received_pkt)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'p', 'r', 'o', 't', 'o'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVED_PKT,
                "GetState must return RECEIVED_PKT after a complete packet");
}

/**
 * @test simhubDevProtoGetPktType must return SIMHUB_PKT_TYPE_COUNT after reset.
 */
ZTEST(simhubDevProto_tests, test_get_pkt_type_returns_sentinel_after_reset)
{
  simhubDevProtoReset();

  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_TYPE_COUNT,
                "GetPktType must return SIMHUB_PKT_TYPE_COUNT after reset");
}

/**
 * @test simhubDevProtoGetPktType must return SIMHUB_PKT_PROTO after the
 * "proto" command has been received.
 */
ZTEST(simhubDevProto_tests, test_get_pkt_type_returns_proto)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'p', 'r', 'o', 't', 'o'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_PROTO,
                "GetPktType must return SIMHUB_PKT_PROTO after 'proto' command");
}

/**
 * @test simhubDevProtoGetPktType must return SIMHUB_PKT_LED_COUNT after the
 * "ledsc" command has been received.
 */
ZTEST(simhubDevProto_tests, test_get_pkt_type_returns_led_count)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'l', 'e', 'd', 's', 'c'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_LED_COUNT,
                "GetPktType must return SIMHUB_PKT_LED_COUNT after 'ledsc' command");
}

/**
 * @test simhubDevProtoGetPktType must return SIMHUB_PKT_LED_DATA after the
 * "sleds" command has been received.
 */
ZTEST(simhubDevProto_tests, test_get_pkt_type_returns_led_data)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_LED_DATA,
                "GetPktType must return SIMHUB_PKT_LED_DATA after 'sleds' command");
}

/**
 * @test simhubDevProtoGetPktType must return SIMHUB_PKT_UNLOCK after the
 * "unlock" command has been received.
 */
ZTEST(simhubDevProto_tests, test_get_pkt_type_returns_unlock)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'u', 'n', 'l', 'o', 'c', 'k'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_UNLOCK,
                "GetPktType must return SIMHUB_PKT_UNLOCK after 'unlock' command");
}

/**
 * @test simhubDevProtoGetPixelBuf must always return a pointer to the
 * internal pixel data buffer regardless of the current parser state.
 */
ZTEST(simhubDevProto_tests, test_get_pixel_buf_returns_valid_pointer)
{
  simhubDevProtoReset();

  zassert_not_null(simhubDevProtoGetPixelBuf(),
                   "GetPixelBuf must return a non-NULL pointer");
}

/**
 * @test simhubDevProtoGetPixelBuf must return a pointer to the buffer
 * holding the bytes received via simhubDevProtoProcessLedData in order.
 */
ZTEST(simhubDevProto_tests, test_get_pixel_buf_reflects_received_data)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  uint8_t expected[SIMHUB_LED_COUNT * 3];
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
  {
    expected[i] = (uint8_t)(i + 10);
    simhubDevProtoProcessLedData(expected[i]);
  }

  zassert_mem_equal(simhubDevProtoGetPixelBuf(), expected, SIMHUB_LED_COUNT * 3,
                    "GetPixelBuf must return a pointer to the received pixel data");
}

/**
 * @test simhubDevProtoProcessEmptyBuff must return false and keep the state
 * SIMHUB_PROTO_EMPTY_BUFF when the received byte is not 0xFF.
 */
ZTEST(simhubDevProto_tests, test_process_empty_buff_non_preamble_byte_ignored)
{
  simhubDevProtoReset();

  bool result = simhubDevProtoProcessEmptyBuff(0x00);

  zassert_false(result,
                "ProcessEmptyBuff must return false for a non-preamble byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "state must remain EMPTY_BUFF on a non-preamble byte");
}

/**
 * @test simhubDevProtoProcessEmptyBuff must return false and advance the state
 * to SIMHUB_PROTO_RECEIVING_PREAMBLE when the received byte is 0xFF.
 */
ZTEST(simhubDevProto_tests, test_process_empty_buff_preamble_byte_advances_state)
{
  simhubDevProtoReset();

  bool result = simhubDevProtoProcessEmptyBuff(0xFF);

  zassert_false(result,
                "ProcessEmptyBuff must return false on the first preamble byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_PREAMBLE,
                "state must advance to RECEIVING_PREAMBLE on first 0xFF byte");
}

/**
 * @test simhubDevProtoProcessEmptyBuff must set the synchronization counter
 * to 1 when the received byte is 0xFF.
 */
ZTEST(simhubDevProto_tests, test_process_empty_buff_preamble_byte_sets_sync_count)
{
  simhubDevProtoReset();

  simhubDevProtoProcessEmptyBuff(0xFF);

  zassert_equal(ctx.syncCount, 1,
                "syncCount must be set to 1 on first 0xFF byte");
}

/**
 * @test simhubDevProtoProcessPreamble must return false, reset the state to
 * SIMHUB_PROTO_EMPTY_BUFF, and clear the synchronization counter when the
 * received byte is not 0xFF.
 */
ZTEST(simhubDevProto_tests, test_process_preamble_non_preamble_byte_resets_state)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);

  bool result = simhubDevProtoProcessPreamble(0xAA);

  zassert_false(result,
                "ProcessPreamble must return false on a non-preamble byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "state must reset to EMPTY_BUFF on a non-preamble byte");
  zassert_equal(ctx.syncCount, 0,
                "syncCount must be cleared on a non-preamble byte");
}

/**
 * @test simhubDevProtoProcessPreamble must return false, keep the state
 * SIMHUB_PROTO_RECEIVING_PREAMBLE, and increment the synchronization counter
 * when a 0xFF byte is received and the preamble is not yet complete.
 */
ZTEST(simhubDevProto_tests, test_process_preamble_increments_sync_count)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);

  bool result = simhubDevProtoProcessPreamble(0xFF);

  zassert_false(result,
                "ProcessPreamble must return false while preamble is incomplete");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_PREAMBLE,
                "state must remain RECEIVING_PREAMBLE while preamble is incomplete");
  zassert_equal(ctx.syncCount, 2,
                "syncCount must be incremented on each 0xFF byte");
}

/**
 * @test simhubDevProtoProcessPreamble must return false, advance the state to
 * SIMHUB_PROTO_RECEIVING_CMD, and clear the command length when exactly 6
 * consecutive 0xFF bytes have been received.
 */
ZTEST(simhubDevProto_tests, test_process_preamble_complete_advances_state)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 4; i++)
    simhubDevProtoProcessPreamble(0xFF);

  bool result = simhubDevProtoProcessPreamble(0xFF);

  zassert_false(result,
                "ProcessPreamble must return false even on preamble completion");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_CMD,
                "state must advance to RECEIVING_CMD after 6 preamble bytes");
  zassert_equal(ctx.cmdLen, 0,
                "cmdLen must be cleared when advancing to RECEIVING_CMD");
}

/**
 * @test simhubDevProtoProcessCmd must return false, keep the state
 * SIMHUB_PROTO_RECEIVING_CMD, and accumulate the received bytes in the
 * command buffer while fewer than 5 bytes have been received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_accumulates_bytes)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t partial[] = {'p', 'r', 'o', 't'};
  for(size_t i = 0; i < sizeof(partial); i++)
  {
    bool result = simhubDevProtoProcessCmd(partial[i]);

    zassert_false(result,
                  "ProcessCmd must return false while command is incomplete");
    zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_CMD,
                  "state must remain RECEIVING_CMD while command is incomplete");
    zassert_equal(ctx.cmdBuf[i], partial[i],
                  "cmdBuf must hold the accumulated command bytes");
  }
}

/**
 * @test simhubDevProtoProcessCmd must return true, set the packet type to
 * SIMHUB_PKT_PROTO, and advance the state to SIMHUB_PROTO_RECEIVED_PKT when
 * the command "proto" is received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_proto_completes_packet)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t cmd[] = {'p', 'r', 'o', 't', 'o'};
  bool result = false;
  for(size_t i = 0; i < sizeof(cmd); i++)
    result = simhubDevProtoProcessCmd(cmd[i]);

  zassert_true(result,
               "ProcessCmd must return true for 'proto'");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_PROTO,
                "pkt type must be SIMHUB_PKT_PROTO for 'proto'");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVED_PKT,
                "state must be RECEIVED_PKT after 'proto'");
}

/**
 * @test simhubDevProtoProcessCmd must return true, set the packet type to
 * SIMHUB_PKT_LED_COUNT, and advance the state to SIMHUB_PROTO_RECEIVED_PKT
 * when the command "ledsc" is received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_ledsc_completes_packet)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t cmd[] = {'l', 'e', 'd', 's', 'c'};
  bool result = false;
  for(size_t i = 0; i < sizeof(cmd); i++)
    result = simhubDevProtoProcessCmd(cmd[i]);

  zassert_true(result,
               "ProcessCmd must return true for 'ledsc'");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_LED_COUNT,
                "pkt type must be SIMHUB_PKT_LED_COUNT for 'ledsc'");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVED_PKT,
                "state must be RECEIVED_PKT after 'ledsc'");
}

/**
 * @test simhubDevProtoProcessCmd must return false, set the packet type to
 * SIMHUB_PKT_LED_DATA, reset the data index to 0, and advance the state to
 * SIMHUB_PROTO_RECEIVING_LED_DATA when the command "sleds" is received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_sleds_opens_led_data)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  bool result = false;
  for(size_t i = 0; i < sizeof(cmd); i++)
    result = simhubDevProtoProcessCmd(cmd[i]);

  zassert_false(result,
                "ProcessCmd must return false for 'sleds' (data follows)");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_LED_DATA,
                "pkt type must be SIMHUB_PKT_LED_DATA for 'sleds'");
  zassert_equal(ctx.dataIdx, 0,
                "dataIdx must be reset to 0 when entering RECEIVING_LED_DATA");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_DATA,
                "state must advance to RECEIVING_LED_DATA after 'sleds'");
}

/**
 * @test simhubDevProtoProcessCmd must return true, set the packet type to
 * SIMHUB_PKT_UNLOCK, and advance the state to SIMHUB_PROTO_RECEIVED_PKT when
 * the command "unlock" is received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_unlock_completes_packet)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t cmd[] = {'u', 'n', 'l', 'o', 'c', 'k'};
  bool result = false;
  for(size_t i = 0; i < sizeof(cmd); i++)
    result = simhubDevProtoProcessCmd(cmd[i]);

  zassert_true(result,
               "ProcessCmd must return true for 'unlock'");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_UNLOCK,
                "pkt type must be SIMHUB_PKT_UNLOCK for 'unlock'");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVED_PKT,
                "state must be RECEIVED_PKT after 'unlock'");
}

/**
 * @test simhubDevProtoProcessCmd must return false and reset the parser to
 * the initial state when an unknown 6-byte command is received.
 */
ZTEST(simhubDevProto_tests, test_process_cmd_unknown_resets_parser)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);

  const uint8_t cmd[] = {'b', 'a', 'd', 'c', 'm', 'd'};
  bool result = false;
  for(size_t i = 0; i < sizeof(cmd); i++)
    result = simhubDevProtoProcessCmd(cmd[i]);

  zassert_false(result,
                "ProcessCmd must return false for an unknown command");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "state must reset to EMPTY_BUFF on an unknown command");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_TYPE_COUNT,
                "pkt type must reset to SIMHUB_PKT_TYPE_COUNT on an unknown command");
}

/**
 * @test simhubDevProtoProcessLedData must return false, keep the state
 * SIMHUB_PROTO_RECEIVING_LED_DATA, and store the received byte at the current
 * data index while the pixel buffer is not yet full.
 */
ZTEST(simhubDevProto_tests, test_process_led_data_accumulates_bytes)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);

  for(int i = 0; i < SIMHUB_LED_COUNT * 3 - 1; i++)
  {
    bool result = simhubDevProtoProcessLedData((uint8_t)i);

    zassert_false(result,
                  "ProcessLedData must return false before the pixel buffer is full");
    zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_DATA,
                  "state must remain RECEIVING_LED_DATA before the pixel buffer is full");
    zassert_equal(ctx.pixelBuf[i], (uint8_t)i,
                  "received byte must be stored at the current data index");
  }
}

/**
 * @test simhubDevProtoProcessLedData must return false and advance the state
 * to SIMHUB_PROTO_RECEIVING_LED_FOOTER when the last pixel buffer byte is
 * received.
 */
ZTEST(simhubDevProto_tests, test_process_led_data_advances_state_on_last_byte)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3 - 1; i++)
    simhubDevProtoProcessLedData((uint8_t)i);

  bool result = simhubDevProtoProcessLedData(0xAA);

  zassert_false(result,
                "ProcessLedData must return false even on the last pixel buffer byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_FOOTER,
                "state must advance to RECEIVING_LED_FOOTER on the last pixel buffer byte");
}

/**
 * @test simhubDevProtoProcessLedData must reset the footer index to 0 when
 * advancing to the SIMHUB_PROTO_RECEIVING_LED_FOOTER state.
 */
ZTEST(simhubDevProto_tests, test_process_led_data_resets_footer_idx)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);

  zassert_equal(ctx.footerIdx, 0,
                "footerIdx must be reset to 0 when entering RECEIVING_LED_FOOTER");
}

/**
 * @test simhubDevProtoProcessLedFooter must return false and reset the parser
 * to the initial state when the first footer byte is incorrect.
 */
ZTEST(simhubDevProto_tests, test_process_led_footer_wrong_first_byte_resets_parser)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);

  bool result = simhubDevProtoProcessLedFooter(0x00);

  zassert_false(result,
                "ProcessLedFooter must return false on a wrong first footer byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "state must reset to EMPTY_BUFF on a wrong first footer byte");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_TYPE_COUNT,
                "pkt type must reset to SIMHUB_PKT_TYPE_COUNT on a wrong first footer byte");
}

/**
 * @test simhubDevProtoProcessLedFooter must return false, keep the state
 * SIMHUB_PROTO_RECEIVING_LED_FOOTER, and increment the footer index when a
 * correct footer byte is received and the footer is not yet complete.
 */
ZTEST(simhubDevProto_tests, test_process_led_footer_increments_footer_idx)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);

  bool result = simhubDevProtoProcessLedFooter(0xFF);

  zassert_false(result,
                "ProcessLedFooter must return false while footer is incomplete");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVING_LED_FOOTER,
                "state must remain RECEIVING_LED_FOOTER while footer is incomplete");
  zassert_equal(ctx.footerIdx, 1,
                "footerIdx must be incremented on a correct footer byte");
}

/**
 * @test simhubDevProtoProcessLedFooter must return false and reset the parser
 * to the initial state when an incorrect byte is received mid-footer.
 */
ZTEST(simhubDevProto_tests, test_process_led_footer_wrong_mid_byte_resets_parser)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);
  simhubDevProtoProcessLedFooter(0xFF);

  bool result = simhubDevProtoProcessLedFooter(0x00);

  zassert_false(result,
                "ProcessLedFooter must return false on a wrong mid-footer byte");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_EMPTY_BUFF,
                "state must reset to EMPTY_BUFF on a wrong mid-footer byte");
  zassert_equal(simhubDevProtoGetPktType(), SIMHUB_PKT_TYPE_COUNT,
                "pkt type must reset to SIMHUB_PKT_TYPE_COUNT on a wrong mid-footer byte");
}

/**
 * @test simhubDevProtoProcessLedFooter must return true and advance the state
 * to SIMHUB_PROTO_RECEIVED_PKT when all three footer bytes (0xFF, 0xFE, 0xFD)
 * are received in order.
 */
ZTEST(simhubDevProto_tests, test_process_led_footer_complete_advances_state)
{
  simhubDevProtoReset();
  simhubDevProtoProcessEmptyBuff(0xFF);
  for(int i = 0; i < 5; i++)
    simhubDevProtoProcessPreamble(0xFF);
  const uint8_t cmd[] = {'s', 'l', 'e', 'd', 's'};
  for(size_t i = 0; i < sizeof(cmd); i++)
    simhubDevProtoProcessCmd(cmd[i]);
  for(int i = 0; i < SIMHUB_LED_COUNT * 3; i++)
    simhubDevProtoProcessLedData((uint8_t)i);
  simhubDevProtoProcessLedFooter(0xFF);
  simhubDevProtoProcessLedFooter(0xFE);

  bool result = simhubDevProtoProcessLedFooter(0xFD);

  zassert_true(result,
               "ProcessLedFooter must return true after all 3 footer bytes");
  zassert_equal(simhubDevProtoGetState(), SIMHUB_PROTO_RECEIVED_PKT,
                "state must advance to RECEIVED_PKT after complete footer");
}

ZTEST_SUITE(simhubDevProto_tests, NULL, proto_tests_setup, proto_tests_before, NULL, NULL);
