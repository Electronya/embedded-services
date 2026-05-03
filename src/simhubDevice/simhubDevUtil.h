/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevUtil.h
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Utility
 *
 *            Protocol dispatch and packet builder. Drives the per-state parser
 *            and builds TX responses into caller-provided buffers.
 *
 * @ingroup  simhubDevice
 *
 * @{
 */

#ifndef SIMHUB_DEV_UTIL_H
#define SIMHUB_DEV_UTIL_H

#include "simhubDevProto.h"

/**
 * @brief   Initialize the protocol parser.
 *
 * @return  0 if successful, the error code otherwise.
 */
int simhubDevUtilInit(void);

/**
 * @brief   Reset the protocol parser to the initial state.
 */
void simhubDevUtilReset(void);

/**
 * @brief   Push a new byte into the protocol parser.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been received, false otherwise.
 */
bool simhubDevUtilReceivedPkt(uint8_t byte);

/**
 * @brief   Get the type of the last received packet.
 *
 * @return  The packet type.
 */
SimhubProtoPktType_t simhubDevUtilGetPktType(void);

/**
 * @brief   Build the protocol version response packet.
 *
 * @param[out]  txBuf: The transmit buffer.
 * @param[in]   size:  The transmit buffer size.
 *
 * @return  The number of bytes written if successful, the error code otherwise.
 */
int simhubDevUtilProcessProto(uint8_t *txBuf, size_t size);

/**
 * @brief   Build the LED count response packet.
 *
 * @param[out]  txBuf: The transmit buffer.
 * @param[in]   size:  The transmit buffer size.
 *
 * @return  The number of bytes written if successful, the error code otherwise.
 */
int simhubDevUtilProcessLedCount(uint8_t *txBuf, size_t size);

/**
 * @brief   Process an unlock packet (silently discarded).
 *
 * @return  0 if successful, the error code otherwise.
 */
int simhubDevUtilProcessUnlock(void);

/**
 * @brief   Extract pixel data from the received LED data packet into a frame.
 *
 * @param[out]  frame: The LED strip framebuffer to fill.
 *
 * @return  0 if successful, the error code otherwise.
 */
int simhubDevUtilProcessLedData(struct led_rgb *frame);

#endif /* SIMHUB_DEV_UTIL_H */

/** @} */
