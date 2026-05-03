/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStripUtil.h
 * @author    jbacon
 * @date      2026-04-25
 * @brief     LED Strip Updater Utilities
 *
 *            LED strip updater utilities.
 *
 * @ingroup  ledStrip
 *
 * @{
 */

#ifndef LEDSTRIPUTIL_H
#define LEDSTRIPUTIL_H

#include "ledStrip.h"

/**
 * @brief   LED strip service logger name
 */
#define LED_STRIP_LOGGER_NAME ledStrip

/**
 * @brief   Initialize the LED strip device.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripUtilInitStrip(void);

/**
 * @brief   Initialize the framebuffers.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripUtilInitFramebuffers(void);

/**
 * @brief   Get the next framebuffer.
 *
 * @return  The framebuffer handle if successful, NULL otherwise.
 */
struct led_rgb *ledStripUtilGetNextFramebuffer(void);

/**
 * @brief   Activate a new frame.
 *
 * @param[in]   frame: The new frame to activate.
 */
void ledStripUtilActivateFrame(struct led_rgb *frame);

/**
 * @brief   Push the active frame.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripUtilPushFrame(void);

/**
 * @brief   Set global brightness.
 *
 * @param[in]   newBrightness: The global brightness.
 */
void ledStripUtilSetBrightness(uint8_t newBrightness);

#endif /* LEDSTRIPUTIL_H */

/** @} */
