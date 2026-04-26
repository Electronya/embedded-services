/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStripUtil.h
 * @author    jbacon
 * @date      2026-04-25
 * @brief     LED Strip Updater Utilities
 *
 *            Led strip updater untilities.
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
 * @brief   Intialize the LED strip device.
 *
 * @return  0 if successful, the error code otherwiese.
 */
int ledStripUtilInitStrip(void);

/**
 * @brief   Intialize the famebuffers.
 *
 * @return  0 if successful, the error code otherwiese.
 */
int ledStripUtilInitFramebuffers(void);

/**
 * @brief   Get the next framebuffer.
 *
 * @retunr  The framebuffer handle if successful, NULL otherwise.
 */
LedPixel_t *ledStripUtilGetNextFramebuffer(void);

/**
 * @brief   Activate a new frame.
 *
 * @param[in]   newFrame: The new frame to activate.
 *
 * @return  0 if successful, the error code otherwiese.
 */
int ledStripUtilActivateFrame(LedPixel_t *frame);

/**
 * @brief   Push the active frame.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripUtilPushFrame(void);

/**
 * @brief   Set global brightness.
 *
 * @param[in]   birghtness: The global brightness.
 */
void ledStripUtilSetBrightness(uint8_t brightness);

#endif /* LEDSTRIPUTIL_H */

/** @} */
