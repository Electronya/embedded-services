/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStrip.h
 * @author    jbacon
 * @date      2026-04-24
 * @brief     LED Strip Updater
 *
 *            LED strip updater service.
 *
 * @defgroup ledStrip LED Strip Updater
 *
 * @{
 */

#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>

/**
 * @brief   Initialize the service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripInit(void);

/**
 * @brief   Get the next framebuffer.
 *
 * @return  The next framebuffer if successful, NULL otherwise.
 */
struct led_rgb *ledStripGetNextFramebuffer(void);

/**
 * @brief   Update frame.
 *
 * @param[in]   frame: The frame.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripUpdateFrame(struct led_rgb *frame);

/**
 * @brief   Set the brightness.
 *
 * @param[in]   brightness: The brightness.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripSetBrightness(uint8_t brightness);

#endif /* LEDSTRIP_H */

/** @} */
