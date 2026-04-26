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

#define FRAMEBUFFER_ALLOC_TIMEOUT 5

static const struct device *ledStrip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const uint32_t pixelCount     = DT_PROP(DT_ALIAS(led_strip), chain_length);

static osMemoryPoolId_t framebufferPool = NULL;
static LedPixel_t *activeFrame;
static uint8_t brightness = 255;

/**
 * @brief   Apply global brightness.
 *
 * @param[in]   frame: The frame.
 */
void applyGlobalBrightness(LedPixel_t *frame)
{
  for(size_t i = 0; i < pixelCount; ++i)
  {
    frame[i].ch0 = frame[i].ch0 * brightness / 255;
    frame[i].ch1 = frame[i].ch1 * brightness / 255;
    frame[i].ch2 = frame[i].ch2 * brightness / 255;
#if CONFIG_ENYA_LED_STRIP_NUM_CHANNELS == 4
    frame[i].ch3 = frame[i].ch3 * brightness / 255;
#endif
  }
}

int ledStripUtilInitStrip(void)
{
  int err;

  if(!device_is_ready(ledStrip))
  {
    err = -EBUSY;
    LOG_ERR("ERROR %d: LED strip device %s not ready", err, ledStrip->name);
    return err;
  }

  return 0;
}

int ledStripUtilInitFramebuffers(void)
{
  int err;

  framebufferPool = osMemoryPoolNew(2, pixelCount * CONFIG_ENYA_LED_STRIP_NUM_CHANNELS, NULL);
  if(!framebufferPool)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: uanble to allocate framebuffer pool", err);
    return err;
  }

  return 0;
}

LedPixel_t *ledStripUtilGetNextFramebuffer(void)
{
  return osMemoryPoolAlloc(framebufferPool, FRAMEBUFFER_ALLOC_TIMEOUT);
}

void ledStripUtilActivateFrame(LedPixel_t *frame)
{
  osMemoryPoolFree(framebufferPool, activeFrame);
  activeFrame = frame;
  applyGlobalBrightness(activeFrame);
}

int ledStripUtilPushFrame(void)
{
  int err;

  err = led_strip_update_channels(ledStrip, (uint8_t *)activeFrame, CONFIG_ENYA_LED_STRIP_NUM_CHANNELS);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to update LED string channels", err);

  return err;
}

void ledStripUtilSetBrightness(uint8_t newbrightness)
{
  brightness = newbrightness;
}

/** @} */
