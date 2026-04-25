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

#endif /* LEDSTRIPUTIL_H */

/** @} */
