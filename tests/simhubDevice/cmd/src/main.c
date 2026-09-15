/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-09-15
 * @brief     SimHub Device Command Tests
 *
 *            Unit tests for SimHub device shell command functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DEFINE_FFF_GLOBALS;

/* Prevent shell.h */
#define SHELL_H__

/* Provide shell types */
struct shell;

enum shell_vt100_color
{
  SHELL_NORMAL = 0,
  SHELL_INFO,
  SHELL_ERROR,
  SHELL_WARNING
};

/* Captured shell output */
static char captured_shell_output[256];
static int shell_info_call_count  = 0;
static int shell_error_call_count = 0;

void shell_fprintf(const struct shell *sh, enum shell_vt100_color color,
                   const char *fmt, ...)
{
  va_list args;

  ARG_UNUSED(sh);
  ARG_UNUSED(color);

  va_start(args, fmt);
  vsnprintf(captured_shell_output, sizeof(captured_shell_output), fmt, args);
  va_end(args);

  if(color == SHELL_ERROR)
    shell_error_call_count++;
  else if(color == SHELL_INFO)
    shell_info_call_count++;
}

/* Shell output macros */
#define shell_info(sh, fmt, ...)  shell_fprintf(sh, SHELL_INFO,  fmt, ##__VA_ARGS__)
#define shell_error(sh, fmt, ...) shell_fprintf(sh, SHELL_ERROR, fmt, ##__VA_ARGS__)

/* Null out shell registration macros */
#define SHELL_CMD(...)
#define SHELL_CMD_ARG(...)
#define SHELL_SUBCMD_SET_END
#define SHELL_STATIC_SUBCMD_SET_CREATE(...)
#define SHELL_CMD_REGISTER(...)

/* Prevent simhubDevUtil.h — provide only what simhubDevCmd.c needs. */
#define SIMHUB_DEV_UTIL_H
#define SIMHUB_LED_COUNT 3
typedef enum
{
  SIMHUB_ARQ_IDLE,
  SIMHUB_ARQ_ENUMERATING,
  SIMHUB_ARQ_STREAMING,
} SimhubArqState_t;

/* Kconfig values normally supplied by the build system. */
#define CONFIG_ENYA_SIMHUB_DEVICE_NAME         "Electronya LED"
#define CONFIG_ENYA_SIMHUB_DEVICE_UID          "ENYA001"
#define CONFIG_ENYA_SIMHUB_DEVICE_BUTTON_COUNT 2

/* Mock simhubDevUtil public API used by the shell commands */
FAKE_VALUE_FUNC(SimhubArqState_t, simhubDevUtilGetState);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(simhubDevUtilGetState)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simhubDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Include command implementation */
#include "simhubDevCmd.c"

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *cmd_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void cmd_tests_before(void *f)
{
  ARG_UNUSED(f);

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  memset(captured_shell_output, 0, sizeof(captured_shell_output));
  shell_info_call_count  = 0;
  shell_error_call_count = 0;
}

/* ===========================================================================
 * execStatus
 * =========================================================================*/

/**
 * @test execStatus must print the idle state and device identity with a
 *       SUCCESS prefix.
 */
ZTEST(simhubDevCmd, test_execStatus_idle)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"status"};
  int result;

  simhubDevUtilGetState_fake.return_val = SIMHUB_ARQ_IDLE;

  result = execStatus(sh, 1, argv);

  zassert_equal(result, 0, "execStatus should return 0");
  zassert_equal(simhubDevUtilGetState_fake.call_count, 1,
                "simhubDevUtilGetState should be called once");
  zassert_equal(shell_info_call_count, 1, "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "output should start with SUCCESS");
  zassert_not_null(strstr(captured_shell_output, "idle"),
                   "output should contain the idle state");
  zassert_not_null(strstr(captured_shell_output, "Electronya LED"),
                   "output should contain the configured device name");
  zassert_not_null(strstr(captured_shell_output, "ENYA001"),
                   "output should contain the configured UID");
  zassert_not_null(strstr(captured_shell_output, "led_count=3"),
                   "output should contain the LED count");
  zassert_not_null(strstr(captured_shell_output, "button_count=2"),
                   "output should contain the button count");
}

/**
 * @test execStatus must print the enumerating state.
 */
ZTEST(simhubDevCmd, test_execStatus_enumerating)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"status"};

  simhubDevUtilGetState_fake.return_val = SIMHUB_ARQ_ENUMERATING;

  execStatus(sh, 1, argv);

  zassert_not_null(strstr(captured_shell_output, "enumerating"),
                   "output should contain the enumerating state");
}

/**
 * @test execStatus must print the streaming state.
 */
ZTEST(simhubDevCmd, test_execStatus_streaming)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"status"};

  simhubDevUtilGetState_fake.return_val = SIMHUB_ARQ_STREAMING;

  execStatus(sh, 1, argv);

  zassert_not_null(strstr(captured_shell_output, "streaming"),
                   "output should contain the streaming state");
}

ZTEST_SUITE(simhubDevCmd, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);
