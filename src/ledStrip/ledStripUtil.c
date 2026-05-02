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

LOG_MODULE_DECLARE(LED_STRIP_LOGGER_NAME, CONFIG_ENYA_LED_STRIP_LOG_LEVEL);

#define FRAMEBUFFER_COUNT 2

#define FRAMEBUFFER_ALLOC_TIMEOUT 5

static const struct device *ledStrip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static const uint32_t pixelCount = DT_PROP(DT_ALIAS(led_strip), chain_length);

static osMemoryPoolId_t framebufferPool = NULL;
static struct led_rgb *activeFrame;
static uint8_t brightness = 255;

/**
 * @brief   Apply global brightness.
 *
 * @param[in]   frame: The frame.
 */
void applyGlobalBrightness(struct led_rgb *frame)
{
  if(!frame)
    return;

  for(size_t i = 0; i < pixelCount; ++i)
  {
    frame[i].r = frame[i].r * brightness / 255;
    frame[i].g = frame[i].g * brightness / 255;
    frame[i].b = frame[i].b * brightness / 255;
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

  framebufferPool = osMemoryPoolNew(2, pixelCount * sizeof(struct led_rgb), NULL);
  if(!framebufferPool)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: uanble to allocate framebuffer pool", err);
    return err;
  }

  activeFrame = osMemoryPoolAlloc(framebufferPool, FRAMEBUFFER_ALLOC_TIMEOUT);
  if(!activeFrame)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to get the first framebuffer", err);
    return err;
  }

  memset(activeFrame, 0, pixelCount * sizeof(struct led_rgb));

  return 0;
}

struct led_rgb *ledStripUtilGetNextFramebuffer(void)
{
  return osMemoryPoolAlloc(framebufferPool, FRAMEBUFFER_ALLOC_TIMEOUT);
}

void ledStripUtilActivateFrame(struct led_rgb *frame)
{
  if(activeFrame)
    osMemoryPoolFree(framebufferPool, activeFrame);
  activeFrame = frame;
  applyGlobalBrightness(activeFrame);
}

int ledStripUtilPushFrame(void)
{
  int err;

  if(!activeFrame)
    return 0;

  err = led_strip_update_rgb(ledStrip, activeFrame, pixelCount);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to update LED string channels", err);

  return err;
}

void ledStripUtilSetBrightness(uint8_t newbrightness)
{
  brightness = newbrightness;
  applyGlobalBrightness(activeFrame);
}

/** @} */
