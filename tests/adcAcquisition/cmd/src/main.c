/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-01-08
 * @brief     ADC Acquisition Command Tests
 *
 *            Unit tests for ADC acquisition shell command functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DEFINE_FFF_GLOBALS;

/* Prevent headers from being included */
#define ADC_ACQUISITION_UTIL
#define ADC_AQC_SERVICE_NAME adcAcquisition
#define SHELL_H__  /* Prevent shell.h inclusion */

/* Provide needed declarations */
struct shell;

enum shell_vt100_color {
  SHELL_NORMAL = 0,
  SHELL_INFO,
  SHELL_ERROR,
  SHELL_WARNING
};

/* Captured shell output */
static char captured_shell_output[256];
static int shell_info_call_count = 0;
static int shell_error_call_count = 0;

/* Mock for shell_fprintf (what shell_info and shell_error expand to) */
void shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
                   const char *fmt, ...)
{
  va_list args;

  ARG_UNUSED(shell);
  ARG_UNUSED(color);

  va_start(args, fmt);
  vsnprintf(captured_shell_output, sizeof(captured_shell_output), fmt, args);
  va_end(args);

  if(color == SHELL_ERROR)
  {
    shell_error_call_count++;
  }
  else if(color == SHELL_INFO)
  {
    shell_info_call_count++;
  }
}

/* Mock shell functions */
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);
FAKE_VOID_FUNC(shell_help, const struct shell *);

/* Mock utility functions */
FAKE_VALUE_FUNC(size_t, adcAcqUtilGetChanCount);
FAKE_VALUE_FUNC(int, adcAcqUtilGetRaw, size_t, uint32_t *);
FAKE_VALUE_FUNC(int, adcAcqUtilGetVolt, size_t, float *);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(shell_strtoul) \
  FAKE(shell_help) \
  FAKE(adcAcqUtilGetChanCount) \
  FAKE(adcAcqUtilGetRaw) \
  FAKE(adcAcqUtilGetVolt)

/* Define shell macros for testing */
#define shell_info(shell, fmt, ...) shell_fprintf(shell, SHELL_INFO, fmt, ##__VA_ARGS__)
#define shell_error(shell, fmt, ...) shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)
#define SHELL_CMD(...)
#define SHELL_CMD_ARG(...)
#define SHELL_SUBCMD_SET_END
#define SHELL_STATIC_SUBCMD_SET_CREATE(...)
#define SHELL_CMD_REGISTER(...)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adcAcquisition, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Include command implementation */
#include "adcAcquisitionCmd.c"

/* Test suite setup and teardown */
static void *cmd_tests_setup(void)
{
  return NULL;
}

static void cmd_tests_before(void *f)
{
  ARG_UNUSED(f);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  memset(captured_shell_output, 0, sizeof(captured_shell_output));
  shell_info_call_count = 0;
  shell_error_call_count = 0;
}

/**
 * Requirement: The execGetChanCount function must call adcAcqUtilGetChanCount
 * and print the channel count with SUCCESS prefix via shell_print.
 */
ZTEST(adc_cmd_tests, test_get_chan_count_success)
{
  const struct shell *shell;
  char *argv[] = {"get_chan_count"};
  int result;

  shell = (const struct shell *)0x1234;

  adcAcqUtilGetChanCount_fake.return_val = 4;

  result = execGetChanCount(shell, 1, argv);

  zassert_equal(adcAcqUtilGetChanCount_fake.call_count, 1,
                "adcAcqUtilGetChanCount should be called once");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output, "4") != NULL,
               "shell_info output should contain channel count 4");
  zassert_equal(result, 0,
                "execGetChanCount should return 0 on success");
}

/* Custom fake for shell_strtoul that sets error parameter */
static unsigned long shell_strtoul_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  *err = -EINVAL;
  return 0;
}

/**
 * Requirement: The execGetRaw function must return error when shell_strtoul
 * fails to parse the channel ID argument.
 */
ZTEST(adc_cmd_tests, test_get_raw_invalid_channel_arg)
{
  const struct shell *shell;
  char *argv[] = {"get_raw", "invalid"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execGetRaw(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output, "invalid channel ID argument") != NULL,
               "shell_error output should contain error message");
  zassert_equal(result, -EINVAL,
                "execGetRaw should return error from shell_strtoul");
}

/* Custom fake for adcAcqUtilGetRaw that sets raw value */
static int adcAcqUtilGetRaw_success(size_t chanId, uint32_t *rawVal)
{
  ARG_UNUSED(chanId);

  *rawVal = 1234;
  return 0;
}

/**
 * Requirement: The execGetRaw function must call adcAcqUtilGetRaw with the
 * parsed channel ID and print the raw value with SUCCESS prefix via shell_info.
 */
ZTEST(adc_cmd_tests, test_get_raw_success)
{
  const struct shell *shell;
  char *argv[] = {"get_raw", "2"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.return_val = 2;
  adcAcqUtilGetRaw_fake.custom_fake = adcAcqUtilGetRaw_success;

  result = execGetRaw(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(adcAcqUtilGetRaw_fake.call_count, 1,
                "adcAcqUtilGetRaw should be called once");
  zassert_equal(adcAcqUtilGetRaw_fake.arg0_val, 2,
                "adcAcqUtilGetRaw should be called with channel ID 2");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output, "2") != NULL,
               "shell_info output should contain channel ID 2");
  zassert_true(strstr(captured_shell_output, "1234") != NULL,
               "shell_info output should contain raw value 1234");
  zassert_equal(result, 0,
                "execGetRaw should return 0 on success");
}

/**
 * Requirement: The execGetRaw function must return error when adcAcqUtilGetRaw
 * fails to get the raw value.
 */
ZTEST(adc_cmd_tests, test_get_raw_util_fails)
{
  const struct shell *shell;
  char *argv[] = {"get_raw", "2"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.return_val = 2;
  adcAcqUtilGetRaw_fake.return_val = -EINVAL;

  result = execGetRaw(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(adcAcqUtilGetRaw_fake.call_count, 1,
                "adcAcqUtilGetRaw should be called once");
  zassert_equal(adcAcqUtilGetRaw_fake.arg0_val, 2,
                "adcAcqUtilGetRaw should be called with channel ID 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output, "unable to get the raw value") != NULL,
               "shell_error output should contain error message");
  zassert_equal(result, -EINVAL,
                "execGetRaw should return error from adcAcqUtilGetRaw");
}

/**
 * Requirement: The execGetVolt function must return error when shell_strtoul
 * fails to parse the channel ID argument.
 */
ZTEST(adc_cmd_tests, test_get_volt_invalid_channel_arg)
{
  const struct shell *shell;
  char *argv[] = {"get_volt", "invalid"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execGetVolt(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output, "invalid channel ID argument") != NULL,
               "shell_error output should contain error message");
  zassert_equal(result, -EINVAL,
                "execGetVolt should return error from shell_strtoul");
}

/* Custom fake for adcAcqUtilGetVolt that sets volt value */
static int adcAcqUtilGetVolt_success(size_t chanId, float *voltVal)
{
  ARG_UNUSED(chanId);

  *voltVal = 3.456f;
  return 0;
}

/**
 * Requirement: The execGetVolt function must call adcAcqUtilGetVolt with the
 * parsed channel ID and print the volt value with SUCCESS prefix via shell_info.
 */
ZTEST(adc_cmd_tests, test_get_volt_success)
{
  const struct shell *shell;
  char *argv[] = {"get_volt", "2"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.return_val = 2;
  adcAcqUtilGetVolt_fake.custom_fake = adcAcqUtilGetVolt_success;

  result = execGetVolt(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(adcAcqUtilGetVolt_fake.call_count, 1,
                "adcAcqUtilGetVolt should be called once");
  zassert_equal(adcAcqUtilGetVolt_fake.arg0_val, 2,
                "adcAcqUtilGetVolt should be called with channel ID 2");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output, "2") != NULL,
               "shell_info output should contain channel ID 2");
  zassert_true(strstr(captured_shell_output, "3.456") != NULL,
               "shell_info output should contain volt value 3.456");
  zassert_equal(result, 0,
                "execGetVolt should return 0 on success");
}

/**
 * Requirement: The execGetVolt function must return error when adcAcqUtilGetVolt
 * fails to get the volt value.
 */
ZTEST(adc_cmd_tests, test_get_volt_util_fails)
{
  const struct shell *shell;
  char *argv[] = {"get_volt", "2"};
  int result;

  shell = (const struct shell *)0x1234;

  shell_strtoul_fake.return_val = 2;
  adcAcqUtilGetVolt_fake.return_val = -EINVAL;

  result = execGetVolt(shell, 2, argv);

  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(adcAcqUtilGetVolt_fake.call_count, 1,
                "adcAcqUtilGetVolt should be called once");
  zassert_equal(adcAcqUtilGetVolt_fake.arg0_val, 2,
                "adcAcqUtilGetVolt should be called with channel ID 2");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output, "unable to get the volt value") != NULL,
               "shell_error output should contain error message");
  zassert_equal(result, -EINVAL,
                "execGetVolt should return error from adcAcqUtilGetVolt");
}

ZTEST_SUITE(adc_cmd_tests, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);
