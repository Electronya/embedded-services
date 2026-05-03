/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevUtil.c
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Utility
 *
 *            Protocol dispatch and packet builder implementation.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "simhubDevUtil.h"

LOG_MODULE_DECLARE(simhubDevice, 3);

#define SIMHUB_PROTO_PROTO_RESPONSE     "SIMHUB_1.0\r\n"

int simhubDevUtilInit(void)
{
  simhubDevUtilReset();
  return 0;
}

void simhubDevUtilReset(void)
{
  simhubDevProtoReset();
}

bool simhubDevUtilReceivedPkt(uint8_t byte)
{
  switch(simhubDevProtoGetState())
  {
    case SIMHUB_PROTO_EMPTY_BUFF:
      return simhubDevProtoProcessEmptyBuff(byte);

    case SIMHUB_PROTO_RECEIVING_PREAMBLE:
      return simhubDevProtoProcessPreamble(byte);

    case SIMHUB_PROTO_RECEIVING_CMD:
      return simhubDevProtoProcessCmd(byte);

    case SIMHUB_PROTO_RECEIVING_LED_DATA:
      return simhubDevProtoProcessLedData(byte);

    case SIMHUB_PROTO_RECEIVING_LED_FOOTER:
      return simhubDevProtoProcessLedFooter(byte);

    case SIMHUB_PROTO_RECEIVED_PKT:
      LOG_WRN("byte received while packet pending, resetting");
      simhubDevProtoReset();
      break;

    default:
      LOG_WRN("byte received in unknown state, resetting");
      simhubDevProtoReset();
      break;
  }

  return false;
}

SimhubProtoPktType_t simhubDevUtilGetPktType(void)
{
  return simhubDevProtoGetPktType();
}

int simhubDevUtilProcessProto(uint8_t *txBuf, size_t size)
{
  static const char resp[] = SIMHUB_PROTO_PROTO_RESPONSE;
  size_t respLen = sizeof(resp) - 1;

  if(size < respLen)
  {
    LOG_ERR("ERROR: TX buffer too small for proto response");
    simhubDevProtoReset();
    return -ENOMEM;
  }

  memcpy(txBuf, resp, respLen);
  simhubDevProtoReset();
  return (int)respLen;
}

int simhubDevUtilProcessLedCount(uint8_t *txBuf, size_t size)
{
  int len;

  len = snprintf((char *)txBuf, size, "%d\r\n", SIMHUB_LED_COUNT);
  if(len < 0 || (size_t)len >= size)
  {
    LOG_ERR("ERROR: TX buffer too small for LED count response");
    simhubDevProtoReset();
    return -ENOMEM;
  }

  simhubDevProtoReset();
  return len;
}

int simhubDevUtilProcessUnlock(void)
{
  LOG_DBG("unlock command discarded");
  simhubDevProtoReset();
  return 0;
}

int simhubDevUtilProcessLedData(struct led_rgb *frame)
{
  const uint8_t *buf = simhubDevProtoGetPixelBuf();

  for(uint32_t i = 0; i < SIMHUB_LED_COUNT; i++)
  {
    frame[i].r = buf[i * 3];
    frame[i].g = buf[i * 3 + 1];
    frame[i].b = buf[i * 3 + 2];
  }

  simhubDevProtoReset();
  return 0;
}

/** @} */
