/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStripCmd.c
 * @author    jbacon
 * @date      2026-04-26
 * @brief     LED Strip Update Commands
 *
 *            LED strip updater commands.
 *
 * @ingroup   ledStrip
 *
 * @{
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "ledStrip.h"

static int execGetPixelCount(const struct shell *sh, size_t argc, char **argv)
{
  size_t pixelCount = DT_PROP(DT_ALIAS(led_strip), chain_length);

  shell_info(sh, "SUCCESS: Pixel count: %d", pixelCount);

  return 0;
}

static int execSetFrame(const struct shell *sh, size_t argc, char **argv)
{
  size_t pixelCount = DT_PROP(DT_ALIAS(led_strip), chain_length);
  struct led_rgb *frame;
  int err;
  int convErr = 0;

  frame = ledStripGetNextFramebuffer();
  if(!frame)
  {
    shell_error(sh, "FAIL: no frame buffer available");
    return -ENOMEM;
  }

  for(size_t i = 0; i < pixelCount; ++i)
    for(size_t c = 0; c < sizeof(struct led_rgb); ++c)
    {
      convErr = 0;
      uint8_t *chPtr = (uint8_t *)&frame[i];
      chPtr[c] = (uint8_t)shell_strtoul(argv[1 + i * sizeof(struct led_rgb) + c], 0, &convErr);
      if(convErr)
      {
        shell_error(sh, "FAIL: invalid channel value at argument %zu", 1 + i * sizeof(struct led_rgb) + c);
        return convErr;
      }
    }

  err = ledStripUpdateFrame(frame);
  if(err < 0)
  {
    shell_error(sh, "FAIL %d: unable to submit frame", err);
    return err;
  }

  shell_info(sh, "SUCCESS: frame submitted");

  return 0;
}

static int execSetBrightness(const struct shell *sh, size_t argc, char **argv)
{
  int err;
  int convErr = 0;
  uint8_t brightness = (uint8_t)shell_strtoul(argv[1], 0, &convErr);

  if(convErr)
  {
    shell_error(sh, "FAIL: invalid brightness value");
    return convErr;
  }

  err = ledStripSetBrightness(brightness);
  if(err < 0)
  {
    shell_error(sh, "FAIL %d: unable to set brightness", err);
    return err;
  }

  shell_info(sh, "SUCCESS: brightness set to %d", brightness);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ledStrip_sub, SHELL_CMD(pc, NULL, "Get the pixel count", execGetPixelCount),
                               SHELL_CMD_ARG(sf, NULL, "Set next frame", execSetFrame,
                                             1 + DT_PROP(DT_ALIAS(led_strip), chain_length) * sizeof(struct led_rgb),
                                             0),
                               SHELL_CMD_ARG(br, NULL, "Write datapoint value(s)", execSetBrightness, 2, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(led, &ledStrip_sub, "LED strip commands.", NULL);
/** @} */
