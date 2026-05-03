/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevProto.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Protocol
 *
 *            Per-state byte handlers for the SimHub protocol parser. Owns the
 *            parser state machine variables and the pixel data buffer.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <string.h>
#include <zephyr/logging/log.h>

#include "simhubDevProto.h"

LOG_MODULE_DECLARE(simhubDevice, 3);

#define SIMHUB_PROTO_PREAMBLE_SIZE 6
#define SIMHUB_PROTO_PREAMBLE_BYTE 0xFF

#define SIMHUB_PROTO_PROTO_CMD    "proto"
#define SIMHUB_PROTO_LED_CNT_CMD  "ledsc"
#define SIMHUB_PROTO_LED_DATA_CMD "sleds"
#define SIMHUB_PROTO_UNLOCK_CMD   "unlock"
#define SIMHUB_PROTO_MAX_CMD_SIZE 6

#define SIMHUB_PROTO_LED_FOOTER_SIZE 3

static const uint8_t ledDataFooter[SIMHUB_PROTO_LED_FOOTER_SIZE] = {0xFF, 0xFE, 0xFD};

typedef struct
{
  SimhubProtoState_t state;
  uint8_t syncCount;
  uint8_t cmdBuf[SIMHUB_PROTO_MAX_CMD_SIZE];
  uint8_t cmdLen;
  SimhubProtoPktType_t pktType;
  uint8_t pixelBuf[SIMHUB_LED_COUNT * 3];
  uint32_t dataIdx;
  uint8_t footerIdx;
} SimhubDevProtoCtx_t;

static SimhubDevProtoCtx_t ctx;

void simhubDevProtoReset(void)
{
  ctx.state = SIMHUB_PROTO_EMPTY_BUFF;
  ctx.syncCount = 0;
  ctx.cmdLen = 0;
  ctx.pktType = SIMHUB_PKT_TYPE_COUNT;
  ctx.dataIdx = 0;
  ctx.footerIdx = 0;
}

SimhubProtoState_t simhubDevProtoGetState(void)
{
  return ctx.state;
}

SimhubProtoPktType_t simhubDevProtoGetPktType(void)
{
  return ctx.pktType;
}

const uint8_t *simhubDevProtoGetPixelBuf(void)
{
  return ctx.pixelBuf;
}

bool simhubDevProtoProcessEmptyBuff(uint8_t byte)
{
  if(byte == SIMHUB_PROTO_PREAMBLE_BYTE)
  {
    ctx.syncCount = 1;
    ctx.state = SIMHUB_PROTO_RECEIVING_PREAMBLE;
  }
  return false;
}

bool simhubDevProtoProcessPreamble(uint8_t byte)
{
  if(byte == SIMHUB_PROTO_PREAMBLE_BYTE)
  {
    ctx.syncCount++;
    if(ctx.syncCount == SIMHUB_PROTO_PREAMBLE_SIZE)
    {
      ctx.cmdLen = 0;
      ctx.state = SIMHUB_PROTO_RECEIVING_CMD;
    }
  } else
  {
    ctx.syncCount = 0;
    ctx.state = SIMHUB_PROTO_EMPTY_BUFF;
  }
  return false;
}

bool simhubDevProtoProcessCmd(uint8_t byte)
{
  ctx.cmdBuf[ctx.cmdLen++] = byte;

  if(ctx.cmdLen == 5)
  {
    if(memcmp(ctx.cmdBuf, SIMHUB_PROTO_PROTO_CMD, 5) == 0)
    {
      ctx.pktType = SIMHUB_PKT_PROTO;
      ctx.state = SIMHUB_PROTO_RECEIVED_PKT;
      return true;
    }
    if(memcmp(ctx.cmdBuf, SIMHUB_PROTO_LED_CNT_CMD, 5) == 0)
    {
      ctx.pktType = SIMHUB_PKT_LED_COUNT;
      ctx.state = SIMHUB_PROTO_RECEIVED_PKT;
      return true;
    }
    if(memcmp(ctx.cmdBuf, SIMHUB_PROTO_LED_DATA_CMD, 5) == 0)
    {
      ctx.pktType = SIMHUB_PKT_LED_DATA;
      ctx.dataIdx = 0;
      ctx.state = SIMHUB_PROTO_RECEIVING_LED_DATA;
    }
  } else if(ctx.cmdLen == SIMHUB_PROTO_MAX_CMD_SIZE)
  {
    if(memcmp(ctx.cmdBuf, SIMHUB_PROTO_UNLOCK_CMD, SIMHUB_PROTO_MAX_CMD_SIZE) == 0)
    {
      ctx.pktType = SIMHUB_PKT_UNLOCK;
      ctx.state = SIMHUB_PROTO_RECEIVED_PKT;
      return true;
    }
    LOG_WRN("unknown command received");
    simhubDevProtoReset();
  }

  return false;
}

bool simhubDevProtoProcessLedData(uint8_t byte)
{
  ctx.pixelBuf[ctx.dataIdx++] = byte;
  if(ctx.dataIdx == SIMHUB_LED_COUNT * 3)
  {
    ctx.footerIdx = 0;
    ctx.state = SIMHUB_PROTO_RECEIVING_LED_FOOTER;
  }
  return false;
}

bool simhubDevProtoProcessLedFooter(uint8_t byte)
{
  if(byte == ledDataFooter[ctx.footerIdx])
  {
    ctx.footerIdx++;
    if(ctx.footerIdx == SIMHUB_PROTO_LED_FOOTER_SIZE)
    {
      ctx.state = SIMHUB_PROTO_RECEIVED_PKT;
      return true;
    }
  } else
  {
    LOG_WRN("bad LED data footer byte at index %d: 0x%02x", ctx.footerIdx, byte);
    simhubDevProtoReset();
  }
  return false;
}

/** @} */
