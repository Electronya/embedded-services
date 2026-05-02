/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-04-26
 * @brief     LED Strip Command Tests
 *
 *            Unit tests for LED strip shell command functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DEFINE_FFF_GLOBALS;

/* Mock Kconfig options */
#define CONFIG_ENYA_LED_STRIP             1

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

/* Include ledStrip.h to get struct led_rgb before mock declarations */
#include "ledStrip.h"

/* Mock shell conversion function */
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);

/* Mock ledStrip public API */
FAKE_VALUE_FUNC(struct led_rgb *, ledStripGetNextFramebuffer);
FAKE_VALUE_FUNC(int, ledStripUpdateFrame, struct led_rgb *);
FAKE_VALUE_FUNC(int, ledStripSetBrightness, uint8_t);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(shell_strtoul) \
  FAKE(ledStripGetNextFramebuffer) \
  FAKE(ledStripUpdateFrame) \
  FAKE(ledStripSetBrightness)

/* Shell output macros */
#define shell_info(sh, fmt, ...)  shell_fprintf(sh, SHELL_INFO,  fmt, ##__VA_ARGS__)
#define shell_error(sh, fmt, ...) shell_fprintf(sh, SHELL_ERROR, fmt, ##__VA_ARGS__)

/* Null out shell registration macros */
#define SHELL_CMD(...)
#define SHELL_CMD_ARG(...)
#define SHELL_SUBCMD_SET_END
#define SHELL_STATIC_SUBCMD_SET_CREATE(...)
#define SHELL_CMD_REGISTER(...)

/* Mock DTS */
#undef DT_ALIAS
#define DT_ALIAS(name) DT_N_NODELABEL_ws2812
#define DT_N_NODELABEL_ws2812_P_chain_length 3

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ledStrip, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Include command implementation */
#include "ledStripCmd.c"

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

/**
 * @test execGetPixelCount must print the pixel count from DTS with a SUCCESS prefix.
 */
ZTEST(ledStripCmd, test_execGetPixelCount_success)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"pc"};
  int result;

  result = execGetPixelCount(sh, 1, argv);

  zassert_equal(result, 0, "execGetPixelCount should return 0");
  zassert_equal(shell_info_call_count, 1, "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "output should start with SUCCESS");
  zassert_not_null(strstr(captured_shell_output, "3"),
                   "output should contain the pixel count");
}

/* Custom fake for shell_strtoul that sets the error parameter */
static unsigned long shell_strtoul_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);
  *err = -EINVAL;
  return 0;
}

/**
 * @test execSetFrame must return -ENOMEM and print FAIL when no frame buffer is available.
 */
ZTEST(ledStripCmd, test_execSetFrame_allocFails)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"sf"};
  int result;

  ledStripGetNextFramebuffer_fake.return_val = NULL;

  result = execSetFrame(sh, 1, argv);

  zassert_equal(result, -ENOMEM, "execSetFrame should return -ENOMEM when allocation fails");
  zassert_equal(ledStripGetNextFramebuffer_fake.call_count, 1,
                "ledStripGetNextFramebuffer should be called once");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 0,
                "ledStripUpdateFrame should not be called when allocation fails");
  zassert_equal(shell_error_call_count, 1, "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "output should start with FAIL");
}

/**
 * @test execSetFrame must return error and print FAIL when a channel value is invalid.
 */
ZTEST(ledStripCmd, test_execSetFrame_invalidChannelValue)
{
  const struct shell *sh = (const struct shell *)0x1234;
  struct led_rgb mockFrame[3];
  char *argv[] = {"sf", "10", "20", "invalid", "40", "50", "60", "70", "80", "90"};
  int result;

  ledStripGetNextFramebuffer_fake.return_val = mockFrame;
  shell_strtoul_fake.custom_fake            = shell_strtoul_with_error;

  result = execSetFrame(sh, 10, argv);

  zassert_equal(result, -EINVAL, "execSetFrame should return error from shell_strtoul");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 0,
                "ledStripUpdateFrame should not be called on invalid channel value");
  zassert_equal(shell_error_call_count, 1, "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "output should start with FAIL");
}

/**
 * @test execSetFrame must return error and print FAIL when submitting the frame fails.
 */
ZTEST(ledStripCmd, test_execSetFrame_updateFrameFails)
{
  const struct shell *sh = (const struct shell *)0x1234;
  struct led_rgb mockFrame[3];
  char *argv[] = {"sf", "10", "20", "30", "40", "50", "60", "70", "80", "90"};
  int result;

  ledStripGetNextFramebuffer_fake.return_val = mockFrame;
  ledStripUpdateFrame_fake.return_val        = -EAGAIN;

  result = execSetFrame(sh, 10, argv);

  zassert_equal(result, -EAGAIN, "execSetFrame should return error from ledStripUpdateFrame");
  zassert_equal(ledStripGetNextFramebuffer_fake.call_count, 1,
                "ledStripGetNextFramebuffer should be called once");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 1,
                "ledStripUpdateFrame should be called once");
  zassert_equal(shell_error_call_count, 1, "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "output should start with FAIL");
}

/**
 * @test execSetFrame must fill the frame, submit it and print SUCCESS on success.
 */
ZTEST(ledStripCmd, test_execSetFrame_success)
{
  const struct shell *sh = (const struct shell *)0x1234;
  struct led_rgb mockFrame[3];
  char *argv[] = {"sf", "10", "20", "30", "40", "50", "60", "70", "80", "90"};
  int result;

  ledStripGetNextFramebuffer_fake.return_val = mockFrame;

  result = execSetFrame(sh, 10, argv);

  zassert_equal(result, 0, "execSetFrame should return 0");
  zassert_equal(ledStripGetNextFramebuffer_fake.call_count, 1,
                "ledStripGetNextFramebuffer should be called once");
  zassert_equal(shell_strtoul_fake.call_count,
                3 * sizeof(struct led_rgb),
                "shell_strtoul should be called once per channel value");
  zassert_equal(ledStripUpdateFrame_fake.call_count, 1,
                "ledStripUpdateFrame should be called once");
  zassert_equal(ledStripUpdateFrame_fake.arg0_val, mockFrame,
                "ledStripUpdateFrame should be called with the allocated frame");
  zassert_equal(shell_info_call_count, 1, "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "output should start with SUCCESS");
}

/**
 * @test execSetBrightness must return error and print FAIL when the brightness value is invalid.
 */
ZTEST(ledStripCmd, test_execSetBrightness_invalidValue)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"br", "invalid"};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execSetBrightness(sh, 2, argv);

  zassert_equal(result, -EINVAL, "execSetBrightness should return error from shell_strtoul");
  zassert_equal(ledStripSetBrightness_fake.call_count, 0,
                "ledStripSetBrightness should not be called on invalid value");
  zassert_equal(shell_error_call_count, 1, "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "output should start with FAIL");
}

/**
 * @test execSetBrightness must return error and print FAIL when setting the brightness fails.
 */
ZTEST(ledStripCmd, test_execSetBrightness_setBrightnessFails)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"br", "128"};
  int result;

  ledStripSetBrightness_fake.return_val = -EAGAIN;

  result = execSetBrightness(sh, 2, argv);

  zassert_equal(result, -EAGAIN, "execSetBrightness should return error from ledStripSetBrightness");
  zassert_equal(ledStripSetBrightness_fake.call_count, 1,
                "ledStripSetBrightness should be called once");
  zassert_equal(shell_error_call_count, 1, "shell_error should be called once");
  zassert_true(strstr(captured_shell_output, "FAIL") == captured_shell_output,
               "output should start with FAIL");
}

/**
 * @test execSetBrightness must set the brightness and print SUCCESS on success.
 */
ZTEST(ledStripCmd, test_execSetBrightness_success)
{
  const struct shell *sh = (const struct shell *)0x1234;
  char *argv[]           = {"br", "128"};
  int result;

  shell_strtoul_fake.return_val = 128;

  result = execSetBrightness(sh, 2, argv);

  zassert_equal(result, 0, "execSetBrightness should return 0");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(ledStripSetBrightness_fake.call_count, 1,
                "ledStripSetBrightness should be called once");
  zassert_equal(ledStripSetBrightness_fake.arg0_val, 128,
                "ledStripSetBrightness should be called with the parsed brightness value");
  zassert_equal(shell_info_call_count, 1, "shell_info should be called once");
  zassert_true(strstr(captured_shell_output, "SUCCESS") == captured_shell_output,
               "output should start with SUCCESS");
}

ZTEST_SUITE(ledStripCmd, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);
