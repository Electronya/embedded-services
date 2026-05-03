/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevProto.h
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Protocol
 *
 *            Per-state byte handlers for the SimHub protocol parser. Owns the
 *            parser state machine variables and the pixel data buffer.
 *
 * @ingroup  simhubDevice
 *
 * @{
 */

#ifndef SIMHUB_DEV_PROTO_H
#define SIMHUB_DEV_PROTO_H

#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>

/**
 * @brief   Maximum TX response buffer size.
 */
#define SIMHUB_PROTO_RES_MAX_SIZE 12

/**
 * @brief   Number of pixels in the LED strip, from DTS chain-length.
 */
#define SIMHUB_LED_COUNT  DT_PROP(DT_ALIAS(led_strip), chain_length)

/**
 * @brief   SimHub packet types.
 */
typedef enum
{
  SIMHUB_PKT_PROTO = 0,         /**< Protocol version query. */
  SIMHUB_PKT_LED_COUNT,         /**< LED count query. */
  SIMHUB_PKT_LED_DATA,          /**< LED data frame. */
  SIMHUB_PKT_UNLOCK,            /**< Upload unlock (silently discarded). */
  SIMHUB_PKT_TYPE_COUNT,        /**< Number of packet types. */
} SimhubProtoPktType_t;

/**
 * @brief   Parser state machine states.
 */
typedef enum
{
  SIMHUB_PROTO_EMPTY_BUFF = 0,        /**< Idle, waiting for preamble start. */
  SIMHUB_PROTO_RECEIVING_PREAMBLE,    /**< Accumulating 0xFF sync bytes. */
  SIMHUB_PROTO_RECEIVING_CMD,         /**< Accumulating ASCII command bytes. */
  SIMHUB_PROTO_RECEIVING_LED_DATA,    /**< Accumulating RGB pixel bytes. */
  SIMHUB_PROTO_RECEIVING_LED_FOOTER,  /**< Verifying 0xFF 0xFE 0xFD footer. */
  SIMHUB_PROTO_RECEIVED_PKT,          /**< Complete packet awaiting processing. */
  SIMHUB_PROTO_STATE_COUNT,
} SimhubProtoState_t;

/**
 * @brief   Reset the parser state machine to the initial state.
 */
void simhubDevProtoReset(void);

/**
 * @brief   Get the current parser state.
 *
 * @return  The current state.
 */
SimhubProtoState_t simhubDevProtoGetState(void);

/**
 * @brief   Get the type of the last fully received packet.
 *
 * @return  The packet type.
 */
SimhubProtoPktType_t simhubDevProtoGetPktType(void);

/**
 * @brief   Get a pointer to the internal pixel data buffer.
 *
 * @return  Pointer to the pixel byte buffer (SIMHUB_LED_COUNT × 3 bytes, RGB order).
 */
const uint8_t *simhubDevProtoGetPixelBuf(void);

/**
 * @brief   Process one byte in the EMPTY_BUFF state.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been detected, false otherwise.
 */
bool simhubDevProtoProcessEmptyBuff(uint8_t byte);

/**
 * @brief   Process one byte in the RECEIVING_PREAMBLE state.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been detected, false otherwise.
 */
bool simhubDevProtoProcessPreamble(uint8_t byte);

/**
 * @brief   Process one byte in the RECEIVING_CMD state.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been detected, false otherwise.
 */
bool simhubDevProtoProcessCmd(uint8_t byte);

/**
 * @brief   Process one byte in the RECEIVING_LED_DATA state.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been detected, false otherwise.
 */
bool simhubDevProtoProcessLedData(uint8_t byte);

/**
 * @brief   Process one byte in the RECEIVING_LED_FOOTER state.
 *
 * @param[in]   byte: The received byte.
 *
 * @return  true if a complete packet has been detected, false otherwise.
 */
bool simhubDevProtoProcessLedFooter(uint8_t byte);

#endif /* SIMHUB_DEV_PROTO_H */

/** @} */
