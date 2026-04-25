/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStripUtil.c
 * @author    jbacon
 * @date      2026-04-25
 * @brief     LED Strip Updater Utilities
 *
 *            LED strip updater utilities.
 *
 * @ingroup   ledStrip
 *
 * @{
 */

#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>
#include <zephyr/portability/cmsis_os2.h>

#include "ledStripUtil.h"

LOG_MODULE_DECLARE(LED_STIP_LOGGER_NAME, CONFIG_ENYA_LED_STRIP_LOG_LEVEL);

#define FRAMEBUFFER_COUNT 2

static const struct device *ledStrip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const uint32_t pixelCount     = DT_PROP(DT_ALIAS(led_strip), chain_length);

static osMemoryPoolId_t frameBuffPool = NULL;

int ledStripUtilInitStrip(void)
{
  int err;

  if(!device_is_ready(ledStrip)) {
    err = -EBUSY;
    LOG_ERR("ERROR %d: LED strip device %s not ready", err, ledStrip->name);
    return err;
  }

  return 0;
}

int ledStripUtilInitFramebuffers(void)
{
  int err;

  frameBuffPool = osMemoryPoolNew(2, pixelCount * CONFIG_ENYA_LED_STRIP_NUM_CHANNELS, NULL);
  if(!frameBuffPool) {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: uanble to allocate framebuffer pool", err);
    return err;
  }

  return 0;
}

/** @} */
