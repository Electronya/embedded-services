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
#include <zephyr/portability/cmsis_os2.h>

typedef union
{
  uint8_t ch[CONFIG_ENYA_LED_STRIP_NUM_CHANNELS];
  struct
  {
    uint8_t ch0;
    uint8_t ch1;
    uint8_t ch2;
#if CONFIG_ENYA_LED_STRIP_NUM_CHANNELS == 4
    uint8_t ch3;
#endif
  };
} LedPixel_t;

/**
 * @brief   Initalize the service.
 *
 * @return  0 if successful, the error code otherwise.
 */
int ledStripInit(void);

#endif /* LEDSTRIP_H */

/** @} */
