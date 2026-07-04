/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubArqProto.c
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub ARQ Transport Protocol
 *
 *            CRC-8 calculation, ARQ frame parser state machine, and
 *            response frame builders.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <errno.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>

#include "simhubArqProto.h"

LOG_MODULE_DECLARE(simhubDevice, 3);

uint8_t simhubArqCrc8(const uint8_t *buf, size_t len)
{
  return crc8(buf, len, 0xD5, 0, false);
}

void simhubArqFrameReset(SimhubArqFrame_t *frame)
{
  frame->state   = ARQ_SYNC0;
  frame->pktId   = 0;
  frame->len     = 0;
  frame->dataIdx = 0;
}

bool simhubArqParseByte(SimhubArqFrame_t *frame, uint8_t byte)
{
  switch(frame->state)
  {
    case ARQ_SYNC0:
      if(byte == SIMHUB_ARQ_HDR0)
        frame->state = ARQ_SYNC1;
      break;

    case ARQ_SYNC1:
      if(byte == SIMHUB_ARQ_HDR1)
        frame->state = ARQ_PKTID;
      else
        frame->state = ARQ_SYNC0;
      break;

    case ARQ_PKTID:
      frame->pktId = byte;
      frame->state = ARQ_LEN;
      break;

    case ARQ_LEN:
      if(byte == 0 || byte > SIMHUB_ARQ_MAX_DATA)
      {
        LOG_WRN("invalid ARQ frame length %d, resetting", byte);
        simhubArqFrameReset(frame);
        break;
      }
      frame->len     = byte;
      frame->dataIdx = 0;
      frame->state   = ARQ_DATA;
      break;

    case ARQ_DATA:
      frame->data[frame->dataIdx++] = byte;
      if(frame->dataIdx == frame->len)
        frame->state = ARQ_CRC;
      break;

    case ARQ_CRC:
    {
      uint8_t crcBuf[2 + SIMHUB_ARQ_MAX_DATA];
      crcBuf[0] = frame->pktId;
      crcBuf[1] = frame->len;
      memcpy(&crcBuf[2], frame->data, frame->len);
      uint8_t expected = simhubArqCrc8(crcBuf, (size_t)(2 + frame->len));

      if(byte != expected)
      {
        LOG_WRN("ARQ CRC mismatch: got 0x%02x expected 0x%02x", byte, expected);
        simhubArqFrameReset(frame);
        break;
      }
      frame->state = ARQ_DONE;
      return true;
    }

    case ARQ_DONE:
      LOG_WRN("byte received in ARQ_DONE state, resetting");
      simhubArqFrameReset(frame);
      break;

    default:
      simhubArqFrameReset(frame);
      break;
  }

  return false;
}

int simhubArqBuildAck(uint8_t id, uint8_t *buf, size_t size)
{
  if(size < 2)
    return -ENOMEM;

  buf[0] = SIMHUB_ARQ_ACK;
  buf[1] = id;
  return 2;
}

int simhubArqBuildByte(uint8_t val, uint8_t *buf, size_t size)
{
  if(size < 2)
    return -ENOMEM;

  buf[0] = SIMHUB_ARQ_BYTE;
  buf[1] = val;
  return 2;
}

int simhubArqBuildStr(const char *str, uint8_t len, uint8_t *buf, size_t size)
{
  if(size < (size_t)(3 + len))
    return -ENOMEM;

  buf[0] = SIMHUB_ARQ_STR;
  buf[1] = len;
  memcpy(&buf[2], str, len);
  buf[2 + len] = 0x20;
  return 3 + len;
}

int simhubArqBuildStrTerm(uint8_t *buf, size_t size)
{
  if(size < 4)
    return -ENOMEM;

  buf[0] = SIMHUB_ARQ_STR;
  buf[1] = 0x01;
  buf[2] = 0x0A;
  buf[3] = 0x20;
  return 4;
}

/** @} */
