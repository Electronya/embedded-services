/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubArqProto.h
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub ARQ Transport Protocol
 *
 *            Binary ARQ frame parser and response builder for the SimHub
 *            Standard Firmware discovery and streaming protocol.
 *
 * @ingroup  simhubDevice
 *
 * @{
 */

#ifndef SIMHUB_ARQ_PROTO_H
#define SIMHUB_ARQ_PROTO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief   ARQ frame header bytes.
 */
#define SIMHUB_ARQ_HDR0     0x01
#define SIMHUB_ARQ_HDR1     0x01

/**
 * @brief   Broadcast packet ID used by the Hello frame; does not advance the
 *          packet sequence counter on either side.
 */
#define SIMHUB_ARQ_BCAST    0xFF

/**
 * @brief   MESSAGE_HEADER byte that always appears as data[0] in every ARQ
 *          payload.
 */
#define SIMHUB_ARQ_MSG_H    0x03

/**
 * @brief   Maximum payload (data field) length supported by the parser.
 */
#define SIMHUB_ARQ_MAX_DATA 32

/**
 * @brief   Response type prefix bytes.
 */
#define SIMHUB_ARQ_ACK      0x03  /**< Acknowledgement:  03 ID             */
#define SIMHUB_ARQ_NACK     0x04  /**< Negative ack:     04 lastId reason  */
#define SIMHUB_ARQ_STR      0x06  /**< String response:  06 len data... 20 */
#define SIMHUB_ARQ_BYTE     0x08  /**< Byte response:    08 val            */

/**
 * @brief   Frame parser state machine states.
 */
typedef enum
{
  ARQ_SYNC0 = 0,  /**< Waiting for first header byte (0x01).  */
  ARQ_SYNC1,      /**< Waiting for second header byte (0x01). */
  ARQ_PKTID,      /**< Receiving packet ID.                   */
  ARQ_LEN,        /**< Receiving payload length.              */
  ARQ_DATA,       /**< Accumulating payload bytes.            */
  ARQ_CRC,        /**< Receiving and validating CRC-8.        */
  ARQ_DONE,       /**< Complete, CRC-valid frame ready.       */
} SimhubArqParseState_t;

/**
 * @brief   ARQ frame descriptor populated incrementally by simhubArqParseByte.
 */
typedef struct
{
  SimhubArqParseState_t state;                    /**< Current parser state.   */
  uint8_t               pktId;                    /**< Frame packet ID.        */
  uint8_t               len;                      /**< Payload length (bytes). */
  uint8_t               data[SIMHUB_ARQ_MAX_DATA]; /**< Payload bytes.         */
  uint8_t               dataIdx;                  /**< Bytes received so far.  */
} SimhubArqFrame_t;

/**
 * @brief   Compute CRC-8 (polynomial 0xD5, initial value 0, no reflection).
 *
 * @param[in]   buf: Data bytes to checksum.
 * @param[in]   len: Number of bytes.
 *
 * @return  CRC-8 checksum.
 */
uint8_t simhubArqCrc8(const uint8_t *buf, size_t len);

/**
 * @brief   Reset a frame descriptor to the initial parser state.
 *
 * @param[in,out]   frame: Frame to reset.
 */
void simhubArqFrameReset(SimhubArqFrame_t *frame);

/**
 * @brief   Feed one received byte into the frame parser.
 *
 *          Returns true only when a complete, CRC-valid frame has been
 *          assembled.  On a CRC mismatch the parser resets to ARQ_SYNC0 and
 *          returns false.  The caller must call simhubArqFrameReset before
 *          feeding more bytes after a successful parse.
 *
 * @param[in,out]   frame: Frame being assembled.
 * @param[in]       byte:  Received byte.
 *
 * @return  true when a complete valid frame is ready, false otherwise.
 */
bool simhubArqParseByte(SimhubArqFrame_t *frame, uint8_t byte);

/**
 * @brief   Build an ACK response (03 ID) into buf.
 *
 * @param[in]   id:   Packet ID to acknowledge.
 * @param[out]  buf:  Output buffer.
 * @param[in]   size: Buffer capacity.
 *
 * @return  Bytes written, or -ENOMEM if buf is too small.
 */
int simhubArqBuildAck(uint8_t id, uint8_t *buf, size_t size);

/**
 * @brief   Build a single-byte response (08 val) into buf.
 *
 * @param[in]   val:  Value byte.
 * @param[out]  buf:  Output buffer.
 * @param[in]   size: Buffer capacity.
 *
 * @return  Bytes written, or -ENOMEM if buf is too small.
 */
int simhubArqBuildByte(uint8_t val, uint8_t *buf, size_t size);

/**
 * @brief   Build a string response frame (06 len str... 20) into buf.
 *          Does NOT append a newline terminator; call simhubArqBuildStrTerm
 *          separately to close a string list.
 *
 * @param[in]   str:  String data (not NUL-terminated; len bytes used).
 * @param[in]   len:  String length in bytes.
 * @param[out]  buf:  Output buffer.
 * @param[in]   size: Buffer capacity.
 *
 * @return  Bytes written, or -ENOMEM if buf is too small.
 */
int simhubArqBuildStr(const char *str, uint8_t len, uint8_t *buf, size_t size);

/**
 * @brief   Build a string-list terminator (06 01 0A 20) into buf.
 *          Sent after all simhubArqBuildStr calls to close an enumeration list.
 *
 * @param[out]  buf:  Output buffer.
 * @param[in]   size: Buffer capacity.
 *
 * @return  Bytes written, or -ENOMEM if buf is too small.
 */
int simhubArqBuildStrTerm(uint8_t *buf, size_t size);

#endif /* SIMHUB_ARQ_PROTO_H */

/** @} */
