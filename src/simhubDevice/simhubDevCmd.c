/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevCmd.c
 * @author    jbacon
 * @date      2026-09-15
 * @brief     SimHub Device Shell Commands
 *
 *            Shell command implementations for the SimHub device service.
 *
 * @ingroup   simhubDevice
 *
 * @{
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "simhubDevUtil.h"

/**
 * @brief   Max chars formatted per LED entry in the "led" command output
 *          (e.g. "255:255,255,255 " — index, r, g, b, and a separator).
 */
#define SIMHUB_DEV_CMD_LED_ENTRY_LEN 20

/**
 * @brief   Max chars formatted per button entry in the "buttons" command
 *          output (e.g. "255:released " — index, state, and a separator).
 */
#define SIMHUB_DEV_CMD_BUTTON_ENTRY_LEN 16

/**
 * @brief   Session state display strings, indexed by SimhubArqState_t.
 */
static const char *simhubStateStr[] = {
  [SIMHUB_ARQ_IDLE]        = "idle",
  [SIMHUB_ARQ_ENUMERATING] = "enumerating",
  [SIMHUB_ARQ_STREAMING]   = "streaming",
};

/**
 * @brief   Shell command: print the session state, static device identity
 *          reported to SimHub during enumeration, and link health counters.
 *
 * @param[in]   sh:   Shell instance.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector.
 *
 * @return  0 always.
 */
static int execStatus(const struct shell *sh, size_t argc, char **argv)
{
  SimhubArqState_t state = simhubDevUtilGetState();

  shell_info(sh,
            "SUCCESS: state=%s name=%s uid=%s led_count=%d button_count=%d "
            "frames=%u crc_errors=%u last_cmd=0x%02x",
            simhubStateStr[state],
            CONFIG_ENYA_SIMHUB_DEVICE_NAME,
            CONFIG_ENYA_SIMHUB_DEVICE_UID,
            SIMHUB_LED_COUNT,
            CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT,
            simhubDevUtilGetFrameCount(),
            simhubDevUtilGetCrcErrorCount(),
            simhubDevUtilGetLastCmd());

  return 0;
}

/**
 * @brief   Shell command: force the session back to IDLE.
 *
 * @param[in]   sh:   Shell instance.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector.
 *
 * @return  0 always.
 */
static int execReset(const struct shell *sh, size_t argc, char **argv)
{
  simhubDevUtilReset();

  shell_info(sh, "SUCCESS: session reset to idle");

  return 0;
}

/**
 * @brief   Shell command: dump the current pending LED frame, one
 *          "index:r,g,b" entry per LED, without consuming it.
 *
 * @param[in]   sh:   Shell instance.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector.
 *
 * @return  0 always.
 */
static int execLed(const struct shell *sh, size_t argc, char **argv)
{
  struct led_rgb frame[SIMHUB_LED_COUNT];
  char           line[SIMHUB_LED_COUNT * SIMHUB_DEV_CMD_LED_ENTRY_LEN + 1];
  size_t         offset = 0;

  simhubDevUtilPeekLedFrame(frame);

  for(uint32_t i = 0; i < SIMHUB_LED_COUNT; i++)
    offset += (size_t)snprintf(&line[offset], sizeof(line) - offset, "%s%u:%u,%u,%u", (i == 0) ? "" : " ", i,
                               frame[i].r, frame[i].g, frame[i].b);

  shell_info(sh, "SUCCESS: led_count=%d leds=%s", SIMHUB_LED_COUNT, line);

  return 0;
}

/**
 * @brief   Shell command: dump the last received button state, one
 *          "index:pressed|released" entry per button.
 *
 * @param[in]   sh:   Shell instance.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector.
 *
 * @return  0 always.
 */
static int execButtons(const struct shell *sh, size_t argc, char **argv)
{
  uint8_t buttonState = simhubDevUtilGetButtonState();
  char    line[CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT * SIMHUB_DEV_CMD_BUTTON_ENTRY_LEN + 1] = "";
  size_t  offset = 0;

  for(uint32_t i = 0; i < CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT; i++)
    offset += (size_t)snprintf(&line[offset], sizeof(line) - offset, "%s%u:%s", (i == 0) ? "" : " ", i,
                               ((buttonState >> i) & 0x01) ? "pressed" : "released");

  shell_info(sh, "SUCCESS: button_count=%d buttons=0x%02x %s", CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT,
            buttonState, line);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(simhubDevice_sub,
                               SHELL_CMD(status, NULL, "Session state and device identity", execStatus),
                               SHELL_CMD(reset, NULL, "Reset the session to idle", execReset),
                               SHELL_CMD(led, NULL, "Dump the current pending LED frame", execLed),
                               SHELL_CMD(buttons, NULL, "Dump the last received button state", execButtons),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(simhub, &simhubDevice_sub, "SimHub device commands.", NULL);
/** @} */
