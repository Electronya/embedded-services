/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionCmd.c
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition Service Command
 *
 *            ADC acquisition CLI commands.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#include <zephyr/shell/shell.h>

#include "adcAcquisitionUtil.h"

/**
 * @brief   Execute the get channel count command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execGetChanCount(const struct shell *shell, size_t argc, char **argv)
{
  size_t chanCount;

  chanCount = adcAcqUtilGetChanCount();

  shell_print(shell, "SUCCESS: channel count: %d", chanCount);

  return 0;
}

/**
 * @brief   Execute the get raw value command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execGetRaw(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  size_t chanId;
  uint32_t rawVal;

  err = shell_strtoul(argv[1], 10, &chanId);
  if(err < 0)
  {
    shell_print(shell, "FAIL %d: invalid channel ID argument", err);
    shell_help(shell);
    return err;
  }

  err = adcAcqUtilGetRaw(chanId, &rawVal);
  if(err < 0)
  {
    shell_print(shell, "FAIL %d: unable to get the raw value of channel %d", err, chanId);
    return err;
  }

  shell_print(shell, "SUCCESS: channel %d raw value: %d", chanId, rawVal);

  return 0;
}

/**
 * @brief   Execute the get volt value command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execGetVolt(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  size_t chanId;
  float voltVal;

  err = shell_strtoul(argv[1], 10, &chanId);
  if(err < 0)
  {
    shell_print(shell, "FAIL %d: invalid channel ID argument", err);
    shell_help(shell);
    return err;
  }

  err = adcAcqUtilGetVolt(chanId, &voltVal);
  if(err < 0)
  {
    shell_print(shell, "FAIL %d: unable to get the volt value of channel %d", err, chanId);
    return err;
  }

  shell_print(shell, "SUCCESS: channel %d volt value: %.3f V", chanId, (double)voltVal);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(adcAqc_sub,
  SHELL_CMD(get_chan_count, NULL, "Get the channel count.\n\tUsage: adc_acq get_chan_count", execGetChanCount),
  SHELL_CMD_ARG(get_raw, NULL, "Get a channel raw value.\n\tUsage: adc_acq get_raw <chan ID>", execGetRaw, 2, 0),
  SHELL_CMD_ARG(get_volt, NULL, "Get a channel volt value.\n\tUsage: adc_acq get_raw <chan ID>", execGetVolt, 2, 0),
  SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(adc_acq, &adcAqc_sub, "ADC acquisition commands.",	NULL);

/** @} */
