/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevUtil.h
 * @author    jbacon
 * @date      2026-07-04
 * @brief     SimHub Device Utility
 *
 *            ARQ session state machine. Owns protocol session state, dispatches
 *            received ARQ frames to command handlers, and drives the TX function
 *            to send responses.
 *
 * @ingroup  simhubDevice
 *
 * @{
 */

#ifndef SIMHUB_DEV_UTIL_H
#define SIMHUB_DEV_UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>

/**
 * @brief   Number of LEDs on the connected strip, read from devicetree.
 */
#define SIMHUB_LED_COUNT DT_PROP(DT_ALIAS(led_strip), chain_length)

/**
 * @brief   Size of the TX scratch buffer used for building responses.
 */
#define SIMHUB_DEV_TX_BUF_SIZE 64

/**
 * @brief   ARQ session states.
 */
typedef enum
{
  SIMHUB_ARQ_IDLE,        /**< No active session.          */
  SIMHUB_ARQ_ENUMERATING, /**< Enumeration in progress.    */
  SIMHUB_ARQ_STREAMING,   /**< LED streaming active.       */
} SimhubArqState_t;

/**
 * @brief   Callback type for sending bytes over the UART TX path.
 */
typedef int (*SimhubDevTxFn_t)(const uint8_t *buf, size_t len);

/**
 * @brief   Initialize the ARQ session state machine.
 *
 * @param[in]   txFn: Function to call when sending a response.
 *
 * @return  0 if successful, the error code otherwise.
 */
int simhubDevUtilInit(SimhubDevTxFn_t txFn);

/**
 * @brief   Reset the session back to IDLE and clear all state.
 */
void simhubDevUtilReset(void);

/**
 * @brief   Feed one received byte into the ARQ frame parser.
 *
 *          Returns true when a complete frame has been received and dispatched.
 *
 * @param[in]   byte: Received byte.
 *
 * @return  true if a complete frame was processed, false otherwise.
 */
bool simhubDevUtilReceivedByte(uint8_t byte);

/**
 * @brief   Retrieve the pending LED frame if one is ready.
 *
 * @param[out]  frame: Buffer of SIMHUB_LED_COUNT led_rgb entries to fill.
 *
 * @return  true if a frame was ready and copied, false otherwise.
 */
bool simhubDevUtilGetLedFrame(struct led_rgb *frame);

/**
 * @brief   Return the button state byte from the most recent LED group frame.
 *
 *          Each bit corresponds to one button (bit 0 = button 0). Returns 0
 *          until the first complete group frame has been received.
 *
 * @return  Last received button state bitmask.
 */
uint8_t simhubDevUtilGetButtonState(void);

/**
 * @brief   Return the current ARQ session state.
 *
 * @return  Current SimhubArqState_t value.
 */
SimhubArqState_t simhubDevUtilGetState(void);

#endif /* SIMHUB_DEV_UTIL_H */

/** @} */
