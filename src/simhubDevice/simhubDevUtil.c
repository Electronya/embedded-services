/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevUtil.c
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub Device Utility
 *
 *            ARQ session state machine implementation.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <string.h>
#include <zephyr/logging/log.h>

#include "simhubArqProto.h"
#include "simhubDevUtil.h"

LOG_MODULE_DECLARE(simhubDevice, 3);

static SimhubArqFrame_t rxFrame;
static SimhubArqState_t sessionState;
static SimhubDevTxFn_t  txFn;
static struct led_rgb   pendingLedFrame[SIMHUB_LED_COUNT];
static bool             ledFrameReady;
static uint8_t          txScratch[SIMHUB_DEV_TX_BUF_SIZE];

static bool     groupActive;
static uint8_t  ledRxBuf[SIMHUB_LED_COUNT * 3];
static uint8_t  ledRxCount;
static uint8_t  ledRxStart;
static uint16_t ledRxBytes;
static uint8_t  lastButtonState;

static void handleHello(void)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(SIMHUB_ARQ_BCAST, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildByte(0x6A, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
  sessionState = SIMHUB_ARQ_ENUMERATING;
}

static void handleFeatures(uint8_t id)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("G", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("N", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("I", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("J", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("P", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr("X", 1, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStrTerm(txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleLedCount(uint8_t id)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildByte(SIMHUB_LED_COUNT, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleTm1638Count(uint8_t id)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildByte(0, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleModulesCount(uint8_t id)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildByte(0, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleName(uint8_t id)
{
  const char *name   = CONFIG_ENYA_SIMHUB_DEVICE_NAME;
  uint8_t     nameLen = (uint8_t)strlen(name);
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr(name, nameLen, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStrTerm(txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleUniqueId(uint8_t id)
{
  const char *uid   = CONFIG_ENYA_SIMHUB_DEVICE_UID;
  uint8_t     uidLen = (uint8_t)strlen(uid);
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStr(uid, uidLen, txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildStrTerm(txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleButtonCount(uint8_t id)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  n = simhubArqBuildByte(CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT,
                         txScratch + len, sizeof(txScratch) - len);
  if(n < 0)
    return;
  len += n;

  txFn(txScratch, len);
}

static void handleXCmd(uint8_t id, const uint8_t *sub, uint8_t subLen)
{
  int len = 0;
  int n;

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  len += n;

  if(subLen >= 4 && strncmp((const char *)sub, "list", 4) == 0)
  {
    n = simhubArqBuildStr("keepalive\n", 10, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;

    n = simhubArqBuildStr("mcutype\n", 8, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;

    n = simhubArqBuildByte(0x0A, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;
  }
  else if(subLen >= 7 && strncmp((const char *)sub, "mcutype", 7) == 0)
  {
    n = simhubArqBuildByte(0x1E, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;

    n = simhubArqBuildByte(0x95, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;

    n = simhubArqBuildByte(0x87, txScratch + len, sizeof(txScratch) - len);
    if(n < 0)
      return;
    len += n;
  }

  txFn(txScratch, len);
}

static void applyLedUpdate(void)
{
  for(uint8_t i = 0; i < ledRxCount && (ledRxStart + i) < SIMHUB_LED_COUNT; i++)
  {
    uint8_t ledIdx = ledRxStart + i;
    pendingLedFrame[ledIdx].r = ledRxBuf[i * 3];
    pendingLedFrame[ledIdx].g = ledRxBuf[i * 3 + 1];
    pendingLedFrame[ledIdx].b = ledRxBuf[i * 3 + 2];
  }
  ledFrameReady = true;
  sessionState  = SIMHUB_ARQ_STREAMING;
}

static void appendToLedRxBuf(const uint8_t *src, uint8_t n)
{
  uint16_t bufSize   = (uint16_t)SIMHUB_LED_COUNT * 3;
  uint16_t available = (ledRxBytes < bufSize) ? (uint16_t)(bufSize - ledRxBytes) : 0;
  if(n > available)
    n = (uint8_t)available;
  if(n > 0)
    memcpy(ledRxBuf + ledRxBytes, src, n);
  ledRxBytes += n;
}

static void handleGroupFrame(uint8_t id, const uint8_t *data, uint8_t len)
{
  int txLen = 0;
  int n;

  if(len < 8)
  {
    n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n > 0)
      txFn(txScratch, n);
    return;
  }

  ledRxStart = data[6];
  ledRxCount = data[7];
  ledRxBytes = 0;

  if(len == 16)
  {
    appendToLedRxBuf(&data[8], len - 8);
    groupActive = true;

    n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n < 0)
      return;
    txFn(txScratch, n);
  }
  else
  {
    uint8_t ledBytes = (uint8_t)(len - 8 - 1);
    lastButtonState  = data[len - 1];

    appendToLedRxBuf(&data[8], ledBytes);
    applyLedUpdate();

    n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n < 0)
      return;
    txLen += n;
    n = simhubArqBuildByte(0x15, txScratch + txLen, sizeof(txScratch) - txLen);
    if(n < 0)
      return;
    txLen += n;
    txFn(txScratch, txLen);
  }
}

static void handleGroupData(uint8_t id, const uint8_t *data, uint8_t len)
{
  int txLen = 0;
  int n;

  if(len == 16)
  {
    appendToLedRxBuf(data, 16);

    n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n < 0)
      return;
    txFn(txScratch, n);
  }
  else
  {
    uint8_t ledBytes = (len > 0) ? (uint8_t)(len - 1) : 0;
    lastButtonState  = (len > 0) ? data[len - 1] : 0;

    appendToLedRxBuf(data, ledBytes);
    groupActive = false;
    applyLedUpdate();

    n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n < 0)
      return;
    txLen += n;
    n = simhubArqBuildByte(0x15, txScratch + txLen, sizeof(txScratch) - txLen);
    if(n < 0)
      return;
    txLen += n;
    txFn(txScratch, txLen);
  }
}

static void handleBaudRate(uint8_t id)
{
  int n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  txFn(txScratch, n);
}

static void handleLedData(uint8_t id, const uint8_t *sub, uint8_t subLen)
{
  int n;

  if(subLen >= 1 + 3 * SIMHUB_LED_COUNT)
  {
    const uint8_t *rgb = &sub[1];

    for(uint32_t i = 0; i < SIMHUB_LED_COUNT; i++)
    {
      pendingLedFrame[i].r = rgb[i * 3];
      pendingLedFrame[i].g = rgb[i * 3 + 1];
      pendingLedFrame[i].b = rgb[i * 3 + 2];
    }

    ledFrameReady = true;
    sessionState  = SIMHUB_ARQ_STREAMING;
  }

  n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
  if(n < 0)
    return;
  txFn(txScratch, n);
}

static void dispatchFrame(SimhubArqFrame_t *frame)
{
  uint8_t id = frame->pktId;

  if(groupActive)
  {
    handleGroupData(id, frame->data, frame->len);
    simhubArqFrameReset(frame);
    return;
  }

  if(frame->len < 2)
  {
    int n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
    if(n > 0)
      txFn(txScratch, n);
    simhubArqFrameReset(frame);
    return;
  }

  uint8_t cmd = frame->data[1];

  switch(cmd)
  {
    case '1':
      handleHello();
      break;
    case '0':
      handleFeatures(id);
      break;
    case '4':
      handleLedCount(id);
      break;
    case '2':
      handleTm1638Count(id);
      break;
    case 'B':
      handleModulesCount(id);
      break;
    case 'N':
      handleName(id);
      break;
    case 'I':
      handleUniqueId(id);
      break;
    case 'J':
      handleButtonCount(id);
      break;
    case SIMHUB_ARQ_MSG_H:
      if(frame->len >= 4)
        handleXCmd(id, frame->data + 4, frame->len - 4);
      break;
    case 'X':
      handleXCmd(id, frame->data + 2, frame->len - 2);
      break;
    case 'G':
      handleGroupFrame(id, frame->data, frame->len);
      break;
    case '8':
      handleBaudRate(id);
      break;
    case '6':
      handleLedData(id, frame->data + 2, frame->len - 2);
      break;
    default:
    {
      LOG_WRN("unknown SimHub command 0x%02x, sending ACK", cmd);
      int n = simhubArqBuildAck(id, txScratch, sizeof(txScratch));
      if(n > 0)
        txFn(txScratch, n);
      break;
    }
  }

  simhubArqFrameReset(frame);
}

int simhubDevUtilInit(SimhubDevTxFn_t fn)
{
  txFn = fn;
  simhubDevUtilReset();
  return 0;
}

void simhubDevUtilReset(void)
{
  simhubArqFrameReset(&rxFrame);
  sessionState    = SIMHUB_ARQ_IDLE;
  ledFrameReady   = false;
  groupActive     = false;
  ledRxBytes      = 0;
  ledRxCount      = 0;
  ledRxStart      = 0;
  lastButtonState = 0;
}

bool simhubDevUtilReceivedByte(uint8_t byte)
{
  if(!simhubArqParseByte(&rxFrame, byte))
    return false;

  dispatchFrame(&rxFrame);
  return true;
}

bool simhubDevUtilGetLedFrame(struct led_rgb *frame)
{
  if(!ledFrameReady)
    return false;

  memcpy(frame, pendingLedFrame, sizeof(pendingLedFrame));
  ledFrameReady = false;
  return true;
}

uint8_t simhubDevUtilGetButtonState(void)
{
  return lastButtonState;
}

SimhubArqState_t simhubDevUtilGetState(void)
{
  return sessionState;
}

/** @} */
