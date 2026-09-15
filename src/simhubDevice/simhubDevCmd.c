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

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include "simhubDevUtil.h"

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

SHELL_STATIC_SUBCMD_SET_CREATE(simhubDevice_sub,
                               SHELL_CMD(status, NULL, "Session state and device identity", execStatus),
                               SHELL_CMD(reset, NULL, "Reset the session to idle", execReset),
                               SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(simhub, &simhubDevice_sub, "SimHub device commands.", NULL);
/** @} */
