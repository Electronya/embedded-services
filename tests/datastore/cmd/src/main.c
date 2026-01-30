/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-01-24
 * @brief     Datastore Command Tests
 *
 *            Unit tests for datastore shell command functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DEFINE_FFF_GLOBALS;

/* Prevent headers from being included */
#define DATASTORE_SRV
#define DATASTORE_META
#define SHELL_H__

/* Provide needed declarations */
struct shell;
struct k_msgq;

enum shell_vt100_color {
  SHELL_NORMAL = 0,
  SHELL_INFO,
  SHELL_ERROR,
  SHELL_WARNING
};

/* Button state enum (from datastoreMeta.h) */
typedef enum __attribute__((mode(SI)))
{
  BUTTON_UNPRESSED = 0,
  BUTTON_SHORT_PRESSED,
  BUTTON_LONG_PRESSED,
  BUTTON_STATE_COUNT
} ButtonState_t;

/* Datapoint counts - minimal for testing */
#define BINARY_DATAPOINT_COUNT 4
#define BUTTON_DATAPOINT_COUNT 2
#define FLOAT_DATAPOINT_COUNT 2
#define INT_DATAPOINT_COUNT 2
#define MULTI_STATE_DATAPOINT_COUNT 2
#define UINT_DATAPOINT_COUNT 2

/* Message queue count */
#define DATASTORE_MSG_COUNT 10

/* Captured shell output */
#define MAX_SHELL_OUTPUT_COUNT 16
#define MAX_SHELL_OUTPUT_LEN 256
static char captured_shell_output[MAX_SHELL_OUTPUT_COUNT][MAX_SHELL_OUTPUT_LEN];
static int shell_info_call_count = 0;
static int shell_error_call_count = 0;
static int shell_output_index = 0;

/* Mock for shell_fprintf (what shell_info and shell_error expand to) */
void shell_fprintf(const struct shell *shell, enum shell_vt100_color color,
                   const char *fmt, ...)
{
  va_list args;

  ARG_UNUSED(shell);

  if(shell_output_index < MAX_SHELL_OUTPUT_COUNT)
  {
    va_start(args, fmt);
    vsnprintf(captured_shell_output[shell_output_index], MAX_SHELL_OUTPUT_LEN, fmt, args);
    va_end(args);
    shell_output_index++;
  }

  if(color == SHELL_ERROR)
    shell_error_call_count++;
  else if(color == SHELL_INFO)
    shell_info_call_count++;
}

/* Custom fake for shell_strtobool that returns different values for each call */
static bool shell_strtobool_success(const char *str, int base, int *err)
{
  ARG_UNUSED(base);

  *err = 0;

  /* Return value based on the input string */
  if(strcmp(str, "true") == 0)
    return true;
  else if(strcmp(str, "false") == 0)
    return false;

  /* Default to false */
  return false;
}

/* Captured values from datastoreWriteBinary call */
static bool captured_write_values[BINARY_DATAPOINT_COUNT];
static size_t captured_write_count;

/* Custom fake for datastoreWriteBinary that captures the values */
static int datastoreWriteBinary_capture(uint32_t datapointId, bool *values,
                                        size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_write_values[i] = values[i];

  return 0;
}

/* Custom fake for datastoreReadButton that fills in button values */
static int datastoreReadButton_success(uint32_t datapointId, size_t valCount,
                                       struct k_msgq *resQueue, ButtonState_t *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  /* Fill with different button states for testing */
  for(size_t i = 0; i < valCount; ++i)
  {
    if(i == 0)
      values[i] = BUTTON_SHORT_PRESSED;
    else if(i == 1)
      values[i] = BUTTON_LONG_PRESSED;
    else
      values[i] = BUTTON_UNPRESSED;
  }

  return 0;
}

/* Custom fake for datastoreReadFloat that fills in float values */
static int datastoreReadFloat_success(uint32_t datapointId, size_t valCount,
                                      struct k_msgq *resQueue, float *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  /* Fill with different float values for testing */
  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = 12.5f + (float)i * 10.0f;
  }

  return 0;
}

/* Custom fake for datastoreReadInt that fills in int values */
static int datastoreReadInt_success(uint32_t datapointId, size_t valCount,
                                    struct k_msgq *resQueue, int32_t *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  /* Fill with different int values for testing */
  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = 100 + (int32_t)i * 50;
  }

  return 0;
}

/* Custom fake for datastoreReadMultiState that fills in multi-state values */
static int datastoreReadMultiState_success(uint32_t datapointId, size_t valCount,
                                           struct k_msgq *resQueue, uint32_t *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  /* Fill with different multi-state values for testing */
  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = i;
  }

  return 0;
}

/* Custom fake for datastoreReadUint that fills in uint values */
static int datastoreReadUint_success(uint32_t datapointId, size_t valCount,
                                     struct k_msgq *resQueue, uint32_t *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  /* Fill with different uint values for testing */
  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = 1000 + i * 100;
  }

  return 0;
}

/* Captured values from datastoreWriteButton call */
static ButtonState_t captured_button_write_values[BUTTON_DATAPOINT_COUNT];
static size_t captured_button_write_count;

/* Custom fake for datastoreWriteButton that captures the values */
static int datastoreWriteButton_capture(uint32_t datapointId, ButtonState_t *values,
                                        size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_button_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_button_write_values[i] = values[i];

  return 0;
}

/* Captured values from datastoreWriteFloat call */
static float captured_float_write_values[FLOAT_DATAPOINT_COUNT];
static size_t captured_float_write_count;

/* Custom fake for datastoreWriteFloat that captures the values */
static int datastoreWriteFloat_capture(uint32_t datapointId, float *values,
                                       size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_float_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_float_write_values[i] = values[i];

  return 0;
}

/* Captured values from datastoreWriteInt call */
static int32_t captured_int_write_values[INT_DATAPOINT_COUNT];
static size_t captured_int_write_count;

/* Custom fake for datastoreWriteInt that captures the values */
static int datastoreWriteInt_capture(uint32_t datapointId, int32_t *values,
                                     size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_int_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_int_write_values[i] = values[i];

  return 0;
}

/* Captured values from datastoreWriteMultiState call */
static uint32_t captured_multi_state_write_values[MULTI_STATE_DATAPOINT_COUNT];
static size_t captured_multi_state_write_count;

/* Custom fake for datastoreWriteMultiState that captures the values */
static int datastoreWriteMultiState_capture(uint32_t datapointId, uint32_t *values,
                                            size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_multi_state_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_multi_state_write_values[i] = values[i];

  return 0;
}

/* Captured values from datastoreWriteUint call */
static uint32_t captured_uint_write_values[UINT_DATAPOINT_COUNT];
static size_t captured_uint_write_count;

/* Custom fake for datastoreWriteUint that captures the values */
static int datastoreWriteUint_capture(uint32_t datapointId, uint32_t *values,
                                      size_t valCount, struct k_msgq *resQueue)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  captured_uint_write_count = valCount;
  for(size_t i = 0; i < valCount; ++i)
    captured_uint_write_values[i] = values[i];

  return 0;
}

/* Mock shell functions */
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);
FAKE_VALUE_FUNC(long, shell_strtol, const char *, int, int *);
FAKE_VALUE_FUNC(bool, shell_strtobool, const char *, int, int *);
FAKE_VOID_FUNC(shell_help, const struct shell *);

/* Mock datastore API functions */
FAKE_VALUE_FUNC(int, datastoreReadBinary, uint32_t, size_t, struct k_msgq *, bool *);
FAKE_VALUE_FUNC(int, datastoreWriteBinary, uint32_t, bool *, size_t, struct k_msgq *);
FAKE_VALUE_FUNC(int, datastoreReadButton, uint32_t, size_t, struct k_msgq *, ButtonState_t *);
FAKE_VALUE_FUNC(int, datastoreWriteButton, uint32_t, ButtonState_t *, size_t, struct k_msgq *);
FAKE_VALUE_FUNC(int, datastoreReadFloat, uint32_t, size_t, struct k_msgq *, float *);
FAKE_VALUE_FUNC(int, datastoreWriteFloat, uint32_t, float *, size_t, struct k_msgq *);
FAKE_VALUE_FUNC(int, datastoreReadInt, uint32_t, size_t, struct k_msgq *, int32_t *);
FAKE_VALUE_FUNC(int, datastoreWriteInt, uint32_t, int32_t *, size_t, struct k_msgq *);
FAKE_VALUE_FUNC(int, datastoreReadMultiState, uint32_t, size_t, struct k_msgq *, uint32_t *);
FAKE_VALUE_FUNC(int, datastoreWriteMultiState, uint32_t, uint32_t *, size_t, struct k_msgq *);
FAKE_VALUE_FUNC(int, datastoreReadUint, uint32_t, size_t, struct k_msgq *, uint32_t *);
FAKE_VALUE_FUNC(int, datastoreWriteUint, uint32_t, uint32_t *, size_t, struct k_msgq *);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(shell_strtoul) \
  FAKE(shell_strtol) \
  FAKE(shell_strtobool) \
  FAKE(shell_help) \
  FAKE(datastoreReadBinary) \
  FAKE(datastoreWriteBinary) \
  FAKE(datastoreReadButton) \
  FAKE(datastoreWriteButton) \
  FAKE(datastoreReadFloat) \
  FAKE(datastoreWriteFloat) \
  FAKE(datastoreReadInt) \
  FAKE(datastoreWriteInt) \
  FAKE(datastoreReadMultiState) \
  FAKE(datastoreWriteMultiState) \
  FAKE(datastoreReadUint) \
  FAKE(datastoreWriteUint)

/* Define shell macros for testing */
#define shell_info(shell, fmt, ...) shell_fprintf(shell, SHELL_INFO, fmt, ##__VA_ARGS__)
#define shell_error(shell, fmt, ...) shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)
#define SHELL_CMD(...)
#define SHELL_CMD_ARG(...)
#define SHELL_SUBCMD_SET_END
#define SHELL_STATIC_SUBCMD_SET_CREATE(...)
#define SHELL_CMD_REGISTER(...)

#undef STRINGIFY
#define STRINGIFY(x) #x

/* Suppress K_MSGQ_DEFINE from the source file - we mock it */
#undef K_MSGQ_DEFINE
#define K_MSGQ_DEFINE(name, ...) /* mocked */

/* Define datapoint X-macros for testing */
#define DATASTORE_BINARY_DATAPOINTS \
  X(BINARY_FIRST_DATAPOINT, 0, true) \
  X(BINARY_SECOND_DATAPOINT, 0, false) \
  X(BINARY_THIRD_DATAPOINT, 0, true) \
  X(BINARY_FOURTH_DATAPOINT, 0, false)

#define DATASTORE_BUTTON_DATAPOINTS \
  X(BUTTON_FIRST_DATAPOINT, 0, BUTTON_UNPRESSED) \
  X(BUTTON_SECOND_DATAPOINT, 0, BUTTON_UNPRESSED)

#define DATASTORE_FLOAT_DATAPOINTS \
  X(FLOAT_FIRST_DATAPOINT, 0, 0.0f) \
  X(FLOAT_SECOND_DATAPOINT, 0, 0.0f)

#define DATASTORE_INT_DATAPOINTS \
  X(INT_FIRST_DATAPOINT, 0, 0) \
  X(INT_SECOND_DATAPOINT, 0, 0)

#define DATASTORE_MULTI_STATE_DATAPOINTS \
  X(MULTI_STATE_FIRST_DATAPOINT, 0, 0) \
  X(MULTI_STATE_SECOND_DATAPOINT, 0, 0)

#define DATASTORE_UINT_DATAPOINTS \
  X(UINT_FIRST_DATAPOINT, 0, 0) \
  X(UINT_SECOND_DATAPOINT, 0, 0)

/* Mock message queue */
static struct k_msgq datastoreCmdResQueue;

/* Include command implementation */
#include "datastoreCmd.c"

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
  shell_output_index = 0;

  memset(captured_write_values, 0, sizeof(captured_write_values));
  captured_write_count = 0;

  memset(captured_button_write_values, 0, sizeof(captured_button_write_values));
  captured_button_write_count = 0;

  memset(captured_float_write_values, 0, sizeof(captured_float_write_values));
  captured_float_write_count = 0;

  memset(captured_int_write_values, 0, sizeof(captured_int_write_values));
  captured_int_write_count = 0;

  memset(captured_multi_state_write_values, 0, sizeof(captured_multi_state_write_values));
  captured_multi_state_write_count = 0;

  memset(captured_uint_write_values, 0, sizeof(captured_uint_write_values));
  captured_uint_write_count = 0;
}

/**
 * @test  The getStringIndex function must return -ESRCH when the string
 *        is not found in the list.
 */
ZTEST(datastore_cmd_tests, test_get_string_index_not_found)
{
  uint32_t index;
  int result;

  result = getStringIndex("UNKNOWN_DATAPOINT", binaryNames, BINARY_DATAPOINT_COUNT, &index);

  zassert_equal(result, -ESRCH, "getStringIndex should return -ESRCH when string is not found");
}

/**
 * @test  The getStringIndex function must return 0 and set the index
 *        when the string is found in the list.
 */
ZTEST(datastore_cmd_tests, test_get_string_index_found)
{
  uint32_t index;
  int result;

  result = getStringIndex("BINARY_SECOND_DATAPOINT", binaryNames, BINARY_DATAPOINT_COUNT, &index);

  zassert_equal(result, 0, "getStringIndex should return 0 when string is found");
  zassert_equal(index, 1, "Index should be 1 for BINARY_SECOND_DATAPOINT");
}

/**
 * @test  The toUpper function must convert all characters in the string
 *        to uppercase.
 */
ZTEST(datastore_cmd_tests, test_to_upper)
{
  char str[] = "binary_first_datapoint";

  toUpper(str);

  zassert_str_equal(str, "BINARY_FIRST_DATAPOINT", "toUpper should convert to uppercase");
}

/**
 * @test  The execListBinary function must return 0 and print the header
 *        followed by all binary datapoint names.
 */
ZTEST(datastore_cmd_tests, test_exec_list_binary)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"ls"};
  int result;

  result = execListBinary(shell, 1, argv);

  zassert_equal(result, 0, "execListBinary should return 0");
  zassert_equal(shell_info_call_count, BINARY_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of binary datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "BINARY_FIRST_DATAPOINT",
                    "second shell_info output should be BINARY_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "BINARY_SECOND_DATAPOINT",
                    "third shell_info output should be BINARY_SECOND_DATAPOINT");
  zassert_str_equal(captured_shell_output[3], "BINARY_THIRD_DATAPOINT",
                    "fourth shell_info output should be BINARY_THIRD_DATAPOINT");
  zassert_str_equal(captured_shell_output[4], "BINARY_FOURTH_DATAPOINT",
                    "fifth shell_info output should be BINARY_FOURTH_DATAPOINT");
}

/**
 * @test  The execReadBinary function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_read_binary_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadBinary(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadBinary should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/* Custom fake for shell_strtoul that sets error parameter */
static unsigned long shell_strtoul_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  *err = -EINVAL;
  return 0;
}

/* Custom fake for shell_strtoul that succeeds on first call, fails on second */
static unsigned long shell_strtoul_fail_on_second(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  /* First call succeeds (for value count), second call fails (for uint value) */
  if(shell_strtoul_fake.call_count == 0)
  {
    *err = 0;
    return 2;
  }
  else
  {
    *err = -EINVAL;
    return 0;
  }
}

/* Custom fake for shell_strtol that sets error parameter */
static long shell_strtol_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  *err = -EINVAL;
  return 0;
}

/* Custom fake for shell_strtol that returns correct parsed values */
static long shell_strtol_success(const char *str, int base, int *err)
{
  ARG_UNUSED(base);

  *err = 0;

  /* Simple parsing for test values */
  if(strcmp(str, "0") == 0)
    return 0;
  else if(strcmp(str, "1") == 0)
    return 1;
  else if(strcmp(str, "100") == 0)
    return 100;
  else if(strcmp(str, "200") == 0)
    return 200;
  else if(strcmp(str, "1000") == 0)
    return 1000;
  else if(strcmp(str, "2000") == 0)
    return 2000;
  else if(strcmp(str, "-50") == 0)
    return -50;

  return 0;
}

/* Custom fake for shell_strtobool that sets error parameter on second call */
static bool shell_strtobool_with_error(const char *str, int base, int *err)
{
  ARG_UNUSED(str);
  ARG_UNUSED(base);

  /* First call succeeds, second call fails */
  if(shell_strtobool_fake.call_count == 0)
  {
    *err = 0;
    return true;
  }
  else
  {
    *err = -EINVAL;
    return false;
  }
}

/**
 * @test  The execReadBinary function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_read_binary_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadBinary(shell, 3, argv);

  zassert_equal(result, -EINVAL, "execReadBinary should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execReadBinary function must return the error code and print an error
 *        when datastoreReadBinary fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_binary_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "binary_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadBinary_fake.return_val = -EIO;

  result = execReadBinary(shell, 2, argv);

  zassert_equal(result, -EIO, "execReadBinary should return -EIO when datastore read fails");
  zassert_equal(datastoreReadBinary_fake.call_count, 1,
                "datastoreReadBinary should be called once");
  zassert_equal(datastoreReadBinary_fake.arg0_val, 0,
                "datastoreReadBinary should be called with datapoint ID 0");
  zassert_equal(datastoreReadBinary_fake.arg1_val, 1,
                "datastoreReadBinary should be called with value count 1");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
}

/* Custom fake for datastoreReadBinary that sets output values */
static int datastoreReadBinary_success(uint32_t datapointId, size_t valCount,
                                       struct k_msgq *resQueue, bool *values)
{
  ARG_UNUSED(datapointId);
  ARG_UNUSED(resQueue);

  for(size_t i = 0; i < valCount; ++i)
    values[i] = (i % 2 == 0);

  return 0;
}

/**
 * @test  The execReadBinary function must return 0 and print the values
 *        when datastoreReadBinary succeeds with multiple values.
 */
ZTEST(datastore_cmd_tests, test_exec_read_binary_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "3";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 3;
  datastoreReadBinary_fake.custom_fake = datastoreReadBinary_success;

  result = execReadBinary(shell, 3, argv);

  zassert_equal(result, 0, "execReadBinary should return 0 on success");
  zassert_equal(datastoreReadBinary_fake.call_count, 1,
                "datastoreReadBinary should be called once");
  zassert_equal(datastoreReadBinary_fake.arg0_val, 0,
                "datastoreReadBinary should be called with datapoint ID 0");
  zassert_equal(datastoreReadBinary_fake.arg1_val, 3,
                "datastoreReadBinary should be called with value count 3");
  zassert_equal(shell_info_call_count, 4,
                "shell_info should be called 4 times (header + 3 values)");
  zassert_str_equal(captured_shell_output[0], "SUCCESS: here are the values read",
                    "first shell_info output should be the success header");
  zassert_true(strstr(captured_shell_output[1], "BINARY_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain the first datapoint name");
  zassert_true(strstr(captured_shell_output[1], "true") != NULL,
               "second shell_info output should contain true");
  zassert_true(strstr(captured_shell_output[2], "BINARY_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain the second datapoint name");
  zassert_true(strstr(captured_shell_output[2], "false") != NULL,
               "third shell_info output should contain false");
  zassert_true(strstr(captured_shell_output[3], "BINARY_THIRD_DATAPOINT") != NULL,
               "fourth shell_info output should contain the third datapoint name");
  zassert_true(strstr(captured_shell_output[3], "true") != NULL,
               "fourth shell_info output should contain true");
}

/**
 * @test execReadBinary should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_binary_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "binary_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadBinary_fake.custom_fake = datastoreReadBinary_success;

  result = execReadBinary(shell, 2, argv);

  zassert_equal(result, 0, "execReadBinary should return success");
  zassert_equal(datastoreReadBinary_fake.call_count, 1,
                "datastoreReadBinary should be called once");
  zassert_equal(datastoreReadBinary_fake.arg1_val, 1,
                "datastoreReadBinary should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test  The execWriteBinary function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "true";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  result = execWriteBinary(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteBinary should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test  The execWriteBinary function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "true";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteBinary(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteBinary should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execWriteBinary function must return an error and print an error
 *        when not enough values are provided for the requested count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "3";
  char arg3[] = "true";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 3;

  result = execWriteBinary(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteBinary should return error when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough") != NULL,
               "shell_error output should contain 'not enough'");
}

/**
 * @test  The execWriteBinary function must return an error and print an error
 *        when an invalid boolean value is provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_invalid_bool_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "true";
  char arg4[] = "invalid_bool";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtobool_fake.custom_fake = shell_strtobool_with_error;

  result = execWriteBinary(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteBinary should return error for invalid boolean value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad binary value") != NULL,
               "shell_error output should contain 'bad binary value'");
}

/**
 * @test  The execWriteBinary function must return an error and print an error
 *        when datastoreWriteBinary fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "true";
  char arg4[] = "false";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtobool_fake.return_val = true;
  datastoreWriteBinary_fake.return_val = -EIO;

  result = execWriteBinary(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteBinary should return error when datastoreWriteBinary fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test  The execWriteBinary function must successfully write multiple binary values
 *        and print success message.
 */
ZTEST(datastore_cmd_tests, test_exec_write_binary_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "binary_first_datapoint";
  char arg2[] = "3";
  char arg3[] = "true";
  char arg4[] = "false";
  char arg5[] = "true";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4, arg5};
  int result;

  shell_strtoul_fake.return_val = 3;
  shell_strtobool_fake.custom_fake = shell_strtobool_success;
  datastoreWriteBinary_fake.custom_fake = datastoreWriteBinary_capture;

  result = execWriteBinary(shell, 6, argv);

  zassert_equal(result, 0, "execWriteBinary should return success");
  zassert_equal(shell_strtobool_fake.call_count, 3,
                "shell_strtobool should be called three times for three values");
  zassert_equal(datastoreWriteBinary_fake.call_count, 1,
                "datastoreWriteBinary should be called once");
  zassert_equal(datastoreWriteBinary_fake.arg0_val, 0,
                "datastoreWriteBinary should be called with datapoint ID 0");
  zassert_equal(datastoreWriteBinary_fake.arg2_val, 3,
                "datastoreWriteBinary should be called with value count 3");

  /* Verify the captured values from datastoreWriteBinary */
  zassert_equal(captured_write_count, 3, "should have captured 3 values");
  zassert_equal(captured_write_values[0], true, "first value should be true");
  zassert_equal(captured_write_values[1], false, "second value should be false");
  zassert_equal(captured_write_values[2], true, "third value should be true");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_help_fake.call_count, 0,
                "shell_help should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once for success message");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "BINARY_FIRST_DATAPOINT") != NULL,
               "shell_info output should contain first datapoint name");
  zassert_true(strstr(captured_shell_output[0], "BINARY_THIRD_DATAPOINT") != NULL,
               "shell_info output should contain last datapoint name");
}

/**
 * @test  The convertButtonStateStr function must return -EINVAL when the
 *        string is not a valid button state.
 */
ZTEST(datastore_cmd_tests, test_convert_button_state_str_invalid)
{
  ButtonState_t value;
  char str[] = "invalid_state";
  int result;

  result = convertButtonStateStr(str, &value);

  zassert_equal(result, -EINVAL, "convertButtonStateStr should return -EINVAL for invalid string");
}

/**
 * @test  The convertButtonStateStr function must return 0 and set the value
 *        to BUTTON_UNPRESSED when the string is "unpressed".
 */
ZTEST(datastore_cmd_tests, test_convert_button_state_str_unpressed)
{
  ButtonState_t value;
  char str[] = "unpressed";
  int result;

  result = convertButtonStateStr(str, &value);

  zassert_equal(result, 0, "convertButtonStateStr should return 0 for valid string");
  zassert_equal(value, BUTTON_UNPRESSED, "value should be BUTTON_UNPRESSED");
}

/**
 * @test  The convertButtonStateStr function must return 0 and set the value
 *        to BUTTON_SHORT_PRESSED when the string is "short_pressed".
 */
ZTEST(datastore_cmd_tests, test_convert_button_state_str_short_pressed)
{
  ButtonState_t value;
  char str[] = "short_pressed";
  int result;

  result = convertButtonStateStr(str, &value);

  zassert_equal(result, 0, "convertButtonStateStr should return 0 for valid string");
  zassert_equal(value, BUTTON_SHORT_PRESSED, "value should be BUTTON_SHORT_PRESSED");
}

/**
 * @test  The convertButtonStateStr function must return 0 and set the value
 *        to BUTTON_LONG_PRESSED when the string is "long_pressed".
 */
ZTEST(datastore_cmd_tests, test_convert_button_state_str_long_pressed)
{
  ButtonState_t value;
  char str[] = "long_pressed";
  int result;

  result = convertButtonStateStr(str, &value);

  zassert_equal(result, 0, "convertButtonStateStr should return 0 for valid string");
  zassert_equal(value, BUTTON_LONG_PRESSED, "value should be BUTTON_LONG_PRESSED");
}

/**
 * @test  The execListButton function must return 0 and print the header
 *        followed by all button datapoint names.
 */
ZTEST(datastore_cmd_tests, test_exec_list_button)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"ls"};
  int result;

  result = execListButton(shell, 1, argv);

  zassert_equal(result, 0, "execListButton should return 0");
  zassert_equal(shell_info_call_count, BUTTON_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of button datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "BUTTON_FIRST_DATAPOINT",
                    "second shell_info output should be BUTTON_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "BUTTON_SECOND_DATAPOINT",
                    "third shell_info output should be BUTTON_SECOND_DATAPOINT");
}

/**
 * @test  The execReadButton function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_read_button_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadButton(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadButton should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test  The execReadButton function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_read_button_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadButton(shell, 3, argv);

  zassert_equal(result, -EINVAL, "execReadButton should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execReadButton function must return an error and print an error
 *        when datastoreReadButton fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_button_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadButton_fake.return_val = -EIO;

  result = execReadButton(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadButton should return error when datastoreReadButton fails");
  zassert_equal(datastoreReadButton_fake.call_count, 1,
                "datastoreReadButton should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test  The execReadButton function must successfully read multiple button values
 *        and print success message with each value.
 */
ZTEST(datastore_cmd_tests, test_exec_read_button_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadButton_fake.custom_fake = datastoreReadButton_success;

  result = execReadButton(shell, 3, argv);

  zassert_equal(result, 0, "execReadButton should return success");
  zassert_equal(datastoreReadButton_fake.call_count, 1,
                "datastoreReadButton should be called once");
  zassert_equal(datastoreReadButton_fake.arg0_val, 0,
                "datastoreReadButton should be called with datapoint ID 0");
  zassert_equal(datastoreReadButton_fake.arg1_val, 2,
                "datastoreReadButton should be called with value count 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_help_fake.call_count, 0,
                "shell_help should not be called");
  zassert_equal(shell_info_call_count, 3,
                "shell_info should be called three times (header + 2 values)");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "first shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[1], "BUTTON_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain first datapoint name");
  zassert_true(strstr(captured_shell_output[1], "short_pressed") != NULL,
               "second shell_info output should contain short_pressed");
  zassert_true(strstr(captured_shell_output[2], "BUTTON_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain second datapoint name");
  zassert_true(strstr(captured_shell_output[2], "long_pressed") != NULL,
               "third shell_info output should contain long_pressed");
}

/**
 * @test execReadButton should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_button_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "button_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadButton_fake.custom_fake = datastoreReadButton_success;

  result = execReadButton(shell, 2, argv);

  zassert_equal(result, 0, "execReadButton should return success");
  zassert_equal(datastoreReadButton_fake.call_count, 1,
                "datastoreReadButton should be called once");
  zassert_equal(datastoreReadButton_fake.arg1_val, 1,
                "datastoreReadButton should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test  The execWriteButton function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "unpressed";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  result = execWriteButton(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteButton should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test  The execWriteButton function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "unpressed";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteButton(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteButton should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execWriteButton function must return an error and print an error
 *        when not enough values are provided for the requested count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "3";
  char arg3[] = "unpressed";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 3;

  result = execWriteButton(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteButton should return error when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough") != NULL,
               "shell_error output should contain 'not enough'");
}

/**
 * @test  The execWriteButton function must return an error and print an error
 *        when an invalid button state value is provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_invalid_button_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "unpressed";
  char arg4[] = "invalid_state";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;

  result = execWriteButton(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteButton should return error for invalid button value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad button value") != NULL,
               "shell_error output should contain 'bad button value'");
}

/**
 * @test  The execWriteButton function must return an error and print an error
 *        when datastoreWriteButton fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "unpressed";
  char arg4[] = "short_pressed";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreWriteButton_fake.return_val = -EIO;

  result = execWriteButton(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteButton should return error when datastoreWriteButton fails");
  zassert_equal(datastoreWriteButton_fake.call_count, 1,
                "datastoreWriteButton should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test  The execWriteButton function must successfully write multiple button values
 *        and print success message.
 */
ZTEST(datastore_cmd_tests, test_exec_write_button_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "button_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "short_pressed";
  char arg4[] = "long_pressed";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreWriteButton_fake.custom_fake = datastoreWriteButton_capture;

  result = execWriteButton(shell, 5, argv);

  zassert_equal(result, 0, "execWriteButton should return success");
  zassert_equal(datastoreWriteButton_fake.call_count, 1,
                "datastoreWriteButton should be called once");
  zassert_equal(datastoreWriteButton_fake.arg0_val, 0,
                "datastoreWriteButton should be called with datapoint ID 0");
  zassert_equal(datastoreWriteButton_fake.arg2_val, 2,
                "datastoreWriteButton should be called with value count 2");

  /* Verify the captured values from datastoreWriteButton */
  zassert_equal(captured_button_write_count, 2, "should have captured 2 values");
  zassert_equal(captured_button_write_values[0], BUTTON_SHORT_PRESSED, "first value should be BUTTON_SHORT_PRESSED");
  zassert_equal(captured_button_write_values[1], BUTTON_LONG_PRESSED, "second value should be BUTTON_LONG_PRESSED");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_help_fake.call_count, 0,
                "shell_help should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once for success message");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "BUTTON_FIRST_DATAPOINT") != NULL,
               "shell_info output should contain first datapoint name");
  zassert_true(strstr(captured_shell_output[0], "BUTTON_SECOND_DATAPOINT") != NULL,
               "shell_info output should contain last datapoint name");
}

/**
 * @test  The execListFloat function must return 0 and print the header
 *        followed by all float datapoint names.
 */
ZTEST(datastore_cmd_tests, test_exec_list_float)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char *argv[] = {"ls"};
  int result;

  result = execListFloat(shell, 1, argv);

  zassert_equal(result, 0, "execListFloat should return 0");
  zassert_equal(shell_info_call_count, FLOAT_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of float datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "FLOAT_FIRST_DATAPOINT",
                    "second shell_info output should be FLOAT_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "FLOAT_SECOND_DATAPOINT",
                    "third shell_info output should be FLOAT_SECOND_DATAPOINT");
}

/**
 * @test  The execReadFloat function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_read_float_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadFloat(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadFloat should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test  The execReadFloat function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_read_float_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadFloat(shell, 3, argv);

  zassert_equal(result, -EINVAL, "execReadFloat should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execReadFloat function must return an error and print an error
 *        when datastoreReadFloat fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_float_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadFloat_fake.return_val = -EIO;

  result = execReadFloat(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadFloat should return error when datastoreReadFloat fails");
  zassert_equal(datastoreReadFloat_fake.call_count, 1,
                "datastoreReadFloat should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test  The execReadFloat function must successfully read multiple float values
 *        and print success message with each value.
 */
ZTEST(datastore_cmd_tests, test_exec_read_float_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadFloat_fake.custom_fake = datastoreReadFloat_success;

  result = execReadFloat(shell, 3, argv);

  zassert_equal(result, 0, "execReadFloat should return success");
  zassert_equal(datastoreReadFloat_fake.call_count, 1,
                "datastoreReadFloat should be called once");
  zassert_equal(datastoreReadFloat_fake.arg0_val, 0,
                "datastoreReadFloat should be called with datapoint ID 0");
  zassert_equal(datastoreReadFloat_fake.arg1_val, 2,
                "datastoreReadFloat should be called with value count 2");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_help_fake.call_count, 0,
                "shell_help should not be called");
  zassert_equal(shell_info_call_count, 3,
                "shell_info should be called three times (header + 2 values)");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "first shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[1], "FLOAT_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain first datapoint name");
  zassert_true(strstr(captured_shell_output[1], "12.5") != NULL,
               "second shell_info output should contain the float value");
  zassert_true(strstr(captured_shell_output[2], "FLOAT_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain second datapoint name");
  zassert_true(strstr(captured_shell_output[2], "22.5") != NULL,
               "third shell_info output should contain the float value");
}

/**
 * @test execReadFloat should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_float_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "float_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadFloat_fake.custom_fake = datastoreReadFloat_success;

  result = execReadFloat(shell, 2, argv);

  zassert_equal(result, 0, "execReadFloat should return success");
  zassert_equal(datastoreReadFloat_fake.call_count, 1,
                "datastoreReadFloat should be called once");
  zassert_equal(datastoreReadFloat_fake.arg1_val, 1,
                "datastoreReadFloat should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test  The execWriteFloat function must return -ESRCH and print an error
 *        when the datapoint name is not found.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "12.5";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  result = execWriteFloat(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteFloat should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test  The execWriteFloat function must return -EINVAL and print an error
 *        when the value count argument is invalid.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "12.5";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteFloat(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteFloat should return -EINVAL for invalid value count");
  zassert_equal(shell_strtoul_fake.call_count, 1,
                "shell_strtoul should be called once");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid") != NULL,
               "shell_error output should contain the invalid argument");
}

/**
 * @test  The execWriteFloat function must return an error and print an error
 *        when not enough values are provided for the requested count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "3";
  char arg3[] = "12.5";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 3;

  result = execWriteFloat(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteFloat should return error when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough") != NULL,
               "shell_error output should contain 'not enough'");
}

/**
 * @test  The execWriteFloat function must return an error and print an error
 *        when an invalid float value is provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_invalid_float_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "12.5";
  char arg4[] = "invalid_float";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;

  result = execWriteFloat(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteFloat should return error for invalid float value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad float value") != NULL,
               "shell_error output should contain 'bad float value'");
}

/**
 * @test execWriteFloat should return error when datastore write fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "1";
  char arg3[] = "12.5";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreWriteFloat_fake.return_val = -EINVAL;

  result = execWriteFloat(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteFloat should return error when datastore write fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test execWriteFloat should successfully write multiple float values.
 */
ZTEST(datastore_cmd_tests, test_exec_write_float_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "float_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "12.5";
  char arg4[] = "22.75";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreWriteFloat_fake.custom_fake = datastoreWriteFloat_capture;

  result = execWriteFloat(shell, 5, argv);

  zassert_equal(result, 0, "execWriteFloat should return 0 on success");
  zassert_equal(datastoreWriteFloat_fake.call_count, 1,
                "datastoreWriteFloat should be called once");

  /* Verify the captured values from datastoreWriteFloat */
  zassert_equal(captured_float_write_count, 2, "should have captured 2 values");
  zassert_equal(captured_float_write_values[0], 12.5f, "first value should be 12.5");
  zassert_equal(captured_float_write_values[1], 22.75f, "second value should be 22.75");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "write operation") != NULL,
               "shell_info output should contain 'write operation'");
}

/**
 * @test execListInt should list all int datapoints.
 */
ZTEST(datastore_cmd_tests, test_exec_list_int)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "list";
  char *argv[] = {arg0};
  int result;

  result = execListInt(shell, 1, argv);

  zassert_equal(result, 0, "execListInt should return 0");
  zassert_equal(shell_info_call_count, INT_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of int datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "INT_FIRST_DATAPOINT",
                    "second shell_info output should be INT_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "INT_SECOND_DATAPOINT",
                    "third shell_info output should be INT_SECOND_DATAPOINT");
}

/**
 * @test execReadInt should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_read_int_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadInt(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadInt should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execReadInt should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_read_int_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadInt(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadInt should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execReadInt should return error when datastore read fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_int_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "1";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreReadInt_fake.return_val = -EINVAL;

  result = execReadInt(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadInt should return error when datastore read fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "read operation fail") != NULL,
               "shell_error output should contain 'read operation fail'");
}

/**
 * @test execReadInt should successfully read multiple int values.
 */
ZTEST(datastore_cmd_tests, test_exec_read_int_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadInt_fake.custom_fake = datastoreReadInt_success;

  result = execReadInt(shell, 3, argv);

  zassert_equal(result, 0, "execReadInt should return success");
  zassert_equal(datastoreReadInt_fake.call_count, 1,
                "datastoreReadInt should be called once");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 3,
                "shell_info should be called 3 times (header + 2 values)");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "first shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "here are the values read") != NULL,
               "first shell_info output should contain 'here are the values read'");
  zassert_true(strstr(captured_shell_output[1], "INT_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[1], "100") != NULL,
               "second shell_info output should contain value 100");
  zassert_true(strstr(captured_shell_output[2], "INT_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[2], "150") != NULL,
               "third shell_info output should contain value 150");
}

/**
 * @test execReadInt should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_int_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "int_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadInt_fake.custom_fake = datastoreReadInt_success;

  result = execReadInt(shell, 2, argv);

  zassert_equal(result, 0, "execReadInt should return success");
  zassert_equal(datastoreReadInt_fake.call_count, 1,
                "datastoreReadInt should be called once");
  zassert_equal(datastoreReadInt_fake.arg1_val, 1,
                "datastoreReadInt should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test execWriteInt should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "100";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;

  result = execWriteInt(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteInt should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execWriteInt should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "100";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteInt(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteInt should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execWriteInt should return error when not enough values provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "100";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 2;

  result = execWriteInt(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteInt should return -EINVAL when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough value provided") != NULL,
               "shell_error output should contain 'not enough value provided'");
}

/**
 * @test execWriteInt should return error for invalid int value.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_invalid_int_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "100";
  char arg4[] = "invalid_int";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_with_error;

  result = execWriteInt(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteInt should return error for invalid int value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad signed integer value") != NULL,
               "shell_error output should contain 'bad signed integer value'");
}

/**
 * @test execWriteInt should return error when datastore write fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "1";
  char arg3[] = "100";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreWriteInt_fake.return_val = -EINVAL;

  result = execWriteInt(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteInt should return error when datastore write fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test execWriteInt should successfully write multiple int values.
 */
ZTEST(datastore_cmd_tests, test_exec_write_int_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "int_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "100";
  char arg4[] = "200";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_success;
  datastoreWriteInt_fake.custom_fake = datastoreWriteInt_capture;

  result = execWriteInt(shell, 5, argv);

  zassert_equal(result, 0, "execWriteInt should return 0 on success");
  zassert_equal(datastoreWriteInt_fake.call_count, 1,
                "datastoreWriteInt should be called once");

  /* Verify the captured values from datastoreWriteInt */
  zassert_equal(captured_int_write_count, 2, "should have captured 2 values");
  zassert_equal(captured_int_write_values[0], 100, "first value should be 100");
  zassert_equal(captured_int_write_values[1], 200, "second value should be 200");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "write operation") != NULL,
               "shell_info output should contain 'write operation'");
}

/**
 * @test execListMultiState should list all multi-state datapoints.
 */
ZTEST(datastore_cmd_tests, test_exec_list_multi_state)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "list";
  char *argv[] = {arg0};
  int result;

  result = execListMultiState(shell, 1, argv);

  zassert_equal(result, 0, "execListMultiState should return 0");
  zassert_equal(shell_info_call_count, MULTI_STATE_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of multi-state datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "MULTI_STATE_FIRST_DATAPOINT",
                    "second shell_info output should be MULTI_STATE_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "MULTI_STATE_SECOND_DATAPOINT",
                    "third shell_info output should be MULTI_STATE_SECOND_DATAPOINT");
}

/**
 * @test execReadMultiState should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_read_multi_state_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadMultiState(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadMultiState should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execReadMultiState should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_read_multi_state_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadMultiState(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadMultiState should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execReadMultiState should return error when datastore read fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_multi_state_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "1";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreReadMultiState_fake.return_val = -EINVAL;

  result = execReadMultiState(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadMultiState should return error when datastore read fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "read operation fail") != NULL,
               "shell_error output should contain 'read operation fail'");
}

/**
 * @test execReadMultiState should successfully read multiple multi-state values.
 */
ZTEST(datastore_cmd_tests, test_exec_read_multi_state_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadMultiState_fake.custom_fake = datastoreReadMultiState_success;

  result = execReadMultiState(shell, 3, argv);

  zassert_equal(result, 0, "execReadMultiState should return success");
  zassert_equal(datastoreReadMultiState_fake.call_count, 1,
                "datastoreReadMultiState should be called once");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 3,
                "shell_info should be called 3 times (header + 2 values)");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "first shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "here are the values read") != NULL,
               "first shell_info output should contain 'here are the values read'");
  zassert_true(strstr(captured_shell_output[1], "MULTI_STATE_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[1], "0") != NULL,
               "second shell_info output should contain value 0");
  zassert_true(strstr(captured_shell_output[2], "MULTI_STATE_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[2], "1") != NULL,
               "third shell_info output should contain value 1");
}

/**
 * @test execReadMultiState should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_multi_state_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "multi_state_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadMultiState_fake.custom_fake = datastoreReadMultiState_success;

  result = execReadMultiState(shell, 2, argv);

  zassert_equal(result, 0, "execReadMultiState should return success");
  zassert_equal(datastoreReadMultiState_fake.call_count, 1,
                "datastoreReadMultiState should be called once");
  zassert_equal(datastoreReadMultiState_fake.arg1_val, 1,
                "datastoreReadMultiState should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test execWriteMultiState should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "0";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;

  result = execWriteMultiState(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteMultiState should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execWriteMultiState should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "0";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteMultiState(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteMultiState should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execWriteMultiState should return error when not enough values provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "0";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 2;

  result = execWriteMultiState(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteMultiState should return -EINVAL when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough value provided") != NULL,
               "shell_error output should contain 'not enough value provided'");
}

/**
 * @test execWriteMultiState should return error for invalid multi-state value.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_invalid_multi_state_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "0";
  char arg4[] = "invalid_value";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_with_error;

  result = execWriteMultiState(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteMultiState should return error for invalid multi-state value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad multi-state value") != NULL,
               "shell_error output should contain 'bad multi-state value'");
}

/**
 * @test execWriteMultiState should return error when datastore write fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "1";
  char arg3[] = "0";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreWriteMultiState_fake.return_val = -EINVAL;

  result = execWriteMultiState(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteMultiState should return error when datastore write fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test execWriteMultiState should successfully write multiple multi-state values.
 */
ZTEST(datastore_cmd_tests, test_exec_write_multi_state_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "multi_state_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "0";
  char arg4[] = "1";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_success;
  datastoreWriteMultiState_fake.custom_fake = datastoreWriteMultiState_capture;

  result = execWriteMultiState(shell, 5, argv);

  zassert_equal(result, 0, "execWriteMultiState should return 0 on success");
  zassert_equal(datastoreWriteMultiState_fake.call_count, 1,
                "datastoreWriteMultiState should be called once");

  /* Verify the captured values from datastoreWriteMultiState */
  zassert_equal(captured_multi_state_write_count, 2, "should have captured 2 values");
  zassert_equal(captured_multi_state_write_values[0], 0, "first value should be 0");
  zassert_equal(captured_multi_state_write_values[1], 1, "second value should be 1");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "write operation") != NULL,
               "shell_info output should contain 'write operation'");
}

/**
 * @test execListUint should list all uint datapoints.
 */
ZTEST(datastore_cmd_tests, test_exec_list_uint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "list";
  char *argv[] = {arg0};
  int result;

  result = execListUint(shell, 1, argv);

  zassert_equal(result, 0, "execListUint should return 0");
  zassert_equal(shell_info_call_count, UINT_DATAPOINT_COUNT + 1,
                "shell_info should be called for header + each datapoint");
  zassert_str_equal(captured_shell_output[0], "List of unsigned int datapoint:",
                    "first shell_info output should be the header");
  zassert_str_equal(captured_shell_output[1], "UINT_FIRST_DATAPOINT",
                    "second shell_info output should be UINT_FIRST_DATAPOINT");
  zassert_str_equal(captured_shell_output[2], "UINT_SECOND_DATAPOINT",
                    "third shell_info output should be UINT_SECOND_DATAPOINT");
}

/**
 * @test execReadUint should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_read_uint_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "unknown_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  result = execReadUint(shell, 2, argv);

  zassert_equal(result, -ESRCH, "execReadUint should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execReadUint should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_read_uint_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "invalid";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execReadUint(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadUint should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execReadUint should return error when datastore read fails.
 */
ZTEST(datastore_cmd_tests, test_exec_read_uint_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "1";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreReadUint_fake.return_val = -EINVAL;

  result = execReadUint(shell, 3, argv);

  zassert_not_equal(result, 0, "execReadUint should return error when datastore read fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "read operation fail") != NULL,
               "shell_error output should contain 'read operation fail'");
}

/**
 * @test execReadUint should successfully read multiple uint values.
 */
ZTEST(datastore_cmd_tests, test_exec_read_uint_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "2";
  char *argv[] = {arg0, arg1, arg2};
  int result;

  shell_strtoul_fake.return_val = 2;
  datastoreReadUint_fake.custom_fake = datastoreReadUint_success;

  result = execReadUint(shell, 3, argv);

  zassert_equal(result, 0, "execReadUint should return success");
  zassert_equal(datastoreReadUint_fake.call_count, 1,
                "datastoreReadUint should be called once");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 3,
                "shell_info should be called 3 times (header + 2 values)");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "first shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "here are the values read") != NULL,
               "first shell_info output should contain 'here are the values read'");
  zassert_true(strstr(captured_shell_output[1], "UINT_FIRST_DATAPOINT") != NULL,
               "second shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[1], "1000") != NULL,
               "second shell_info output should contain value 1000");
  zassert_true(strstr(captured_shell_output[2], "UINT_SECOND_DATAPOINT") != NULL,
               "third shell_info output should contain datapoint name");
  zassert_true(strstr(captured_shell_output[2], "1100") != NULL,
               "third shell_info output should contain value 1100");
}

/**
 * @test execReadUint should successfully read with default value count of 1.
 */
ZTEST(datastore_cmd_tests, test_exec_read_uint_default_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "read";
  char arg1[] = "uint_first_datapoint";
  char *argv[] = {arg0, arg1};
  int result;

  datastoreReadUint_fake.custom_fake = datastoreReadUint_success;

  result = execReadUint(shell, 2, argv);

  zassert_equal(result, 0, "execReadUint should return success");
  zassert_equal(datastoreReadUint_fake.call_count, 1,
                "datastoreReadUint should be called once");
  zassert_equal(datastoreReadUint_fake.arg1_val, 1,
                "datastoreReadUint should be called with default value count 1");
  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 2,
                "shell_info should be called 2 times (header + 1 value)");
}

/**
 * @test execWriteUint should return error for unknown datapoint.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_unknown_datapoint)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "unknown_datapoint";
  char arg2[] = "1";
  char arg3[] = "1000";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;

  result = execWriteUint(shell, 4, argv);

  zassert_equal(result, -ESRCH, "execWriteUint should return -ESRCH for unknown datapoint");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "UNKNOWN_DATAPOINT") != NULL,
               "shell_error output should contain the datapoint name");
}

/**
 * @test execWriteUint should return error for invalid value count.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_invalid_value_count)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "invalid";
  char arg3[] = "1000";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_with_error;

  result = execWriteUint(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteUint should return error for invalid value count");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "invalid value count") != NULL,
               "shell_error output should contain 'invalid value count'");
}

/**
 * @test execWriteUint should return error when not enough values provided.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_not_enough_values)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "1000";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 2;

  result = execWriteUint(shell, 4, argv);

  zassert_equal(result, -EINVAL, "execWriteUint should return -EINVAL when not enough values provided");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "not enough value provided") != NULL,
               "shell_error output should contain 'not enough value provided'");
}

/**
 * @test execWriteUint should return error for invalid uint value.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_invalid_uint_value)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "1000";
  char arg4[] = "invalid_uint";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_with_error;

  result = execWriteUint(shell, 5, argv);

  zassert_not_equal(result, 0, "execWriteUint should return error for invalid uint value");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "bad unsigned integer value") != NULL,
               "shell_error output should contain 'bad unsigned integer value'");
}

/**
 * @test execWriteUint should return error when datastore write fails.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_datastore_write_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "1";
  char arg3[] = "1000";
  char *argv[] = {arg0, arg1, arg2, arg3};
  int result;

  shell_strtoul_fake.return_val = 1;
  datastoreWriteUint_fake.return_val = -EINVAL;

  result = execWriteUint(shell, 4, argv);

  zassert_not_equal(result, 0, "execWriteUint should return error when datastore write fails");
  zassert_equal(shell_error_call_count, 1,
                "shell_error should be called once");
  zassert_equal(shell_help_fake.call_count, 1,
                "shell_help should be called once");
  zassert_true(strstr(captured_shell_output[0], "FAIL") == captured_shell_output[0],
               "shell_error output should start with FAIL");
  zassert_true(strstr(captured_shell_output[0], "operation fail") != NULL,
               "shell_error output should contain 'operation fail'");
}

/**
 * @test execWriteUint should successfully write multiple uint values.
 */
ZTEST(datastore_cmd_tests, test_exec_write_uint_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "write";
  char arg1[] = "uint_first_datapoint";
  char arg2[] = "2";
  char arg3[] = "1000";
  char arg4[] = "2000";
  char *argv[] = {arg0, arg1, arg2, arg3, arg4};
  int result;

  shell_strtoul_fake.return_val = 2;
  shell_strtol_fake.custom_fake = shell_strtol_success;
  datastoreWriteUint_fake.custom_fake = datastoreWriteUint_capture;

  result = execWriteUint(shell, 5, argv);

  zassert_equal(result, 0, "execWriteUint should return 0 on success");
  zassert_equal(datastoreWriteUint_fake.call_count, 1,
                "datastoreWriteUint should be called once");

  /* Verify the captured values from datastoreWriteUint */
  zassert_equal(captured_uint_write_count, 2, "should have captured 2 values");
  zassert_equal(captured_uint_write_values[0], 1000, "first value should be 1000");
  zassert_equal(captured_uint_write_values[1], 2000, "second value should be 2000");

  zassert_equal(shell_error_call_count, 0,
                "shell_error should not be called");
  zassert_equal(shell_info_call_count, 1,
                "shell_info should be called once");
  zassert_true(strstr(captured_shell_output[0], "SUCCESS") == captured_shell_output[0],
               "shell_info output should start with SUCCESS");
  zassert_true(strstr(captured_shell_output[0], "write operation") != NULL,
               "shell_info output should contain 'write operation'");
}

ZTEST_SUITE(datastore_cmd_tests, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);
