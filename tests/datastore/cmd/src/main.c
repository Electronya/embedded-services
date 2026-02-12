/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-02-11
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

/* Include application metadata */
#include "datastoreMeta.h"
#include "datastoreTypes.h"
#include "datastoreCmdUtil.h"

/* Generate datapoint ID enums from X-macros */
enum BinaryDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_BINARY_DATAPOINTS
#undef X
  BINARY_DATAPOINT_COUNT,
};

enum ButtonDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_BUTTON_DATAPOINTS
#undef X
  BUTTON_DATAPOINT_COUNT,
};

enum FloatDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_FLOAT_DATAPOINTS
#undef X
  FLOAT_DATAPOINT_COUNT,
};

enum IntDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_INT_DATAPOINTS
#undef X
  INT_DATAPOINT_COUNT,
};

enum MultiStateDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_MULTI_STATE_DATAPOINTS
#undef X
  MULTI_STATE_DATAPOINT_COUNT,
};

enum UintDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_UINT_DATAPOINTS
#undef X
  UINT_DATAPOINT_COUNT,
};

/* Captured shell output */
#define MAX_SHELL_OUTPUT_COUNT 32
#define MAX_SHELL_OUTPUT_LEN 256
static char captured_shell_output[MAX_SHELL_OUTPUT_COUNT][MAX_SHELL_OUTPUT_LEN];
static int shell_info_call_count = 0;
static int shell_error_call_count = 0;
static int shell_print_call_count = 0;
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

/* Mock for shell_print */
void shell_print(const struct shell *shell, const char *fmt, ...)
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

  shell_print_call_count++;
}

/* FFF mock declarations for datastore service */
FAKE_VALUE_FUNC(int, datastoreRead, DatapointType_t, uint32_t, size_t, struct k_msgq *, Data_t *);
FAKE_VALUE_FUNC(int, datastoreWrite, DatapointType_t, uint32_t, Data_t *, size_t, struct k_msgq *);

/* FFF mock declarations for shell functions */
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);
FAKE_VALUE_FUNC(long, shell_strtol, const char *, int, int *);
FAKE_VALUE_FUNC(bool, shell_strtobool, const char *, int, int *);

/* FFF mock declarations for datastoreCmdUtil functions */
FAKE_VALUE_FUNC(const DatapointEntry_t *, getDatapointRegistry);
FAKE_VALUE_FUNC(size_t, getDatapointRegistrySize);
FAKE_VOID_FUNC(printTableHeader, const struct shell *);
FAKE_VALUE_FUNC(const char *, getTypeName, DatapointType_t);
FAKE_VOID_FUNC(printBinaryLine, const struct shell *, uint32_t, const char *, bool);
FAKE_VOID_FUNC(printButtonLine, const struct shell *, uint32_t, const char *, ButtonState_t);
FAKE_VOID_FUNC(printFloatLine, const struct shell *, uint32_t, const char *, float);
FAKE_VOID_FUNC(printIntLine, const struct shell *, uint32_t, const char *, int32_t);
FAKE_VOID_FUNC(printMultiStateLine, const struct shell *, uint32_t, const char *, uint32_t);
FAKE_VOID_FUNC(printUintLine, const struct shell *, uint32_t, const char *, uint32_t);
FAKE_VOID_FUNC(toUpper, char *);
FAKE_VALUE_FUNC(int, findDatapointByName, const char *, const DatapointEntry_t **);
FAKE_VOID_FUNC(printBinaryValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VOID_FUNC(printButtonValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VOID_FUNC(printFloatValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VOID_FUNC(printIntValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VOID_FUNC(printMultiStateValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VOID_FUNC(printUintValues, const struct shell *, const DatapointEntry_t *, const Data_t *, size_t);
FAKE_VALUE_FUNC(int, parseBinaryValues, char **, size_t, Data_t *);
FAKE_VALUE_FUNC(int, parseButtonValues, char **, size_t, Data_t *);
FAKE_VALUE_FUNC(int, parseFloatValues, char **, size_t, Data_t *);
FAKE_VALUE_FUNC(int, parseIntValues, char **, size_t, Data_t *);
FAKE_VALUE_FUNC(int, parseMultiStateValues, char **, size_t, Data_t *);
FAKE_VALUE_FUNC(int, parseUintValues, char **, size_t, Data_t *);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(datastoreRead) \
  FAKE(datastoreWrite) \
  FAKE(shell_strtoul) \
  FAKE(shell_strtol) \
  FAKE(shell_strtobool) \
  FAKE(getDatapointRegistry) \
  FAKE(getDatapointRegistrySize) \
  FAKE(printTableHeader) \
  FAKE(getTypeName) \
  FAKE(printBinaryLine) \
  FAKE(printButtonLine) \
  FAKE(printFloatLine) \
  FAKE(printIntLine) \
  FAKE(printMultiStateLine) \
  FAKE(printUintLine) \
  FAKE(toUpper) \
  FAKE(findDatapointByName) \
  FAKE(printBinaryValues) \
  FAKE(printButtonValues) \
  FAKE(printFloatValues) \
  FAKE(printIntValues) \
  FAKE(printMultiStateValues) \
  FAKE(printUintValues) \
  FAKE(parseBinaryValues) \
  FAKE(parseButtonValues) \
  FAKE(parseFloatValues) \
  FAKE(parseIntValues) \
  FAKE(parseMultiStateValues) \
  FAKE(parseUintValues)

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

/* Mock CONFIG_ENYA_DATASTORE_BUFFER_SIZE */
#define CONFIG_ENYA_DATASTORE_BUFFER_SIZE 32

/* Suppress K_MSGQ_DEFINE from the source file - we mock it */
#undef K_MSGQ_DEFINE
#define K_MSGQ_DEFINE(name, ...) /* mocked */

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
  shell_print_call_count = 0;
  shell_output_index = 0;
}

/* Test datapoint registry */
static const DatapointEntry_t testRegistry[] = {
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_BINARY, name},
  DATASTORE_BINARY_DATAPOINTS
#undef X
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_BUTTON, name},
  DATASTORE_BUTTON_DATAPOINTS
#undef X
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_FLOAT, name},
  DATASTORE_FLOAT_DATAPOINTS
#undef X
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_INT, name},
  DATASTORE_INT_DATAPOINTS
#undef X
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_MULTI_STATE, name},
  DATASTORE_MULTI_STATE_DATAPOINTS
#undef X
#define X(name, flags, defaultVal) {STRINGIFY(name), DATAPOINT_UINT, name},
  DATASTORE_UINT_DATAPOINTS
#undef X
};

static const size_t testRegistrySize = BINARY_DATAPOINT_COUNT + BUTTON_DATAPOINT_COUNT +
                                        FLOAT_DATAPOINT_COUNT + INT_DATAPOINT_COUNT +
                                        MULTI_STATE_DATAPOINT_COUNT + UINT_DATAPOINT_COUNT;

/**
 * @test execList should handle datastoreRead failure and continue.
 */
ZTEST(datastore_cmd_tests, test_exec_list_datastore_read_fails)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "list";
  char *argv[] = {arg0};
  int result;

  /* Setup mocks */
  getDatapointRegistry_fake.return_val = testRegistry;
  getDatapointRegistrySize_fake.return_val = testRegistrySize;
  datastoreRead_fake.return_val = -EINVAL;  /* All reads fail */

  result = execList(shell, 1, argv);

  zassert_equal(result, 0, "execList should return 0 even when datastoreRead fails");
  zassert_equal(datastoreRead_fake.call_count, testRegistrySize,
                "datastoreRead should be called for each datapoint");
  zassert_equal(getDatapointRegistry_fake.call_count, 1,
                "getDatapointRegistry should be called once");
  zassert_equal(getDatapointRegistrySize_fake.call_count, 1,
                "getDatapointRegistrySize should be called once");
  zassert_equal(printTableHeader_fake.call_count, 1,
                "printTableHeader should be called once");

  /* Verify datastoreRead was called with correct parameters for each datapoint */
  for (size_t i = 0; i < testRegistrySize; i++)
  {
    zassert_equal(datastoreRead_fake.arg0_history[i], testRegistry[i].type,
                  "datastoreRead call %zu: incorrect type", i);
    zassert_equal(datastoreRead_fake.arg1_history[i], testRegistry[i].id,
                  "datastoreRead call %zu: incorrect ID", i);
    zassert_equal(datastoreRead_fake.arg2_history[i], 1,
                  "datastoreRead call %zu: incorrect value count", i);
  }
}

/**
 * @test execList should successfully list all datapoints.
 */
ZTEST(datastore_cmd_tests, test_exec_list_success)
{
  const struct shell *shell = (const struct shell *)0x1234;
  char arg0[] = "list";
  char *argv[] = {arg0};
  int result;

  /* Setup mocks */
  getDatapointRegistry_fake.return_val = testRegistry;
  getDatapointRegistrySize_fake.return_val = testRegistrySize;
  datastoreRead_fake.return_val = 0;  /* All reads succeed */

  result = execList(shell, 1, argv);

  zassert_equal(result, 0, "execList should return 0 on success");
  zassert_equal(datastoreRead_fake.call_count, testRegistrySize,
                "datastoreRead should be called for each datapoint");
  zassert_equal(getDatapointRegistry_fake.call_count, 1,
                "getDatapointRegistry should be called once");
  zassert_equal(getDatapointRegistrySize_fake.call_count, 1,
                "getDatapointRegistrySize should be called once");
  zassert_equal(printTableHeader_fake.call_count, 1,
                "printTableHeader should be called once");
  zassert_equal(printTableHeader_fake.arg0_val, shell,
                "printTableHeader should be called with correct shell");

  /* Verify datastoreRead parameters for each call */
  for (size_t i = 0; i < testRegistrySize; i++)
  {
    zassert_equal(datastoreRead_fake.arg0_history[i], testRegistry[i].type,
                  "datastoreRead call %zu: incorrect type", i);
    zassert_equal(datastoreRead_fake.arg1_history[i], testRegistry[i].id,
                  "datastoreRead call %zu: incorrect ID", i);
    zassert_equal(datastoreRead_fake.arg2_history[i], 1,
                  "datastoreRead call %zu: incorrect value count", i);
  }

  /* Verify printBinaryLine calls */
  zassert_equal(printBinaryLine_fake.call_count, BINARY_DATAPOINT_COUNT,
                "printBinaryLine should be called for each binary datapoint");
  for (size_t i = 0; i < BINARY_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printBinaryLine_fake.arg0_history[i], shell,
                  "printBinaryLine call %zu: incorrect shell", i);
    zassert_equal(printBinaryLine_fake.arg1_history[i], testRegistry[i].id,
                  "printBinaryLine call %zu: incorrect ID", i);
    zassert_equal(printBinaryLine_fake.arg2_history[i], testRegistry[i].name,
                  "printBinaryLine call %zu: incorrect name", i);
  }

  /* Verify printButtonLine calls */
  size_t buttonIndex = BINARY_DATAPOINT_COUNT;
  zassert_equal(printButtonLine_fake.call_count, BUTTON_DATAPOINT_COUNT,
                "printButtonLine should be called for each button datapoint");
  for (size_t i = 0; i < BUTTON_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printButtonLine_fake.arg0_history[i], shell,
                  "printButtonLine call %zu: incorrect shell", i);
    zassert_equal(printButtonLine_fake.arg1_history[i], testRegistry[buttonIndex + i].id,
                  "printButtonLine call %zu: incorrect ID", i);
    zassert_equal(printButtonLine_fake.arg2_history[i], testRegistry[buttonIndex + i].name,
                  "printButtonLine call %zu: incorrect name", i);
  }

  /* Verify printFloatLine calls */
  size_t floatIndex = BINARY_DATAPOINT_COUNT + BUTTON_DATAPOINT_COUNT;
  zassert_equal(printFloatLine_fake.call_count, FLOAT_DATAPOINT_COUNT,
                "printFloatLine should be called for each float datapoint");
  for (size_t i = 0; i < FLOAT_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printFloatLine_fake.arg0_history[i], shell,
                  "printFloatLine call %zu: incorrect shell", i);
    zassert_equal(printFloatLine_fake.arg1_history[i], testRegistry[floatIndex + i].id,
                  "printFloatLine call %zu: incorrect ID", i);
    zassert_equal(printFloatLine_fake.arg2_history[i], testRegistry[floatIndex + i].name,
                  "printFloatLine call %zu: incorrect name", i);
  }

  /* Verify printIntLine calls */
  size_t intIndex = BINARY_DATAPOINT_COUNT + BUTTON_DATAPOINT_COUNT + FLOAT_DATAPOINT_COUNT;
  zassert_equal(printIntLine_fake.call_count, INT_DATAPOINT_COUNT,
                "printIntLine should be called for each int datapoint");
  for (size_t i = 0; i < INT_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printIntLine_fake.arg0_history[i], shell,
                  "printIntLine call %zu: incorrect shell", i);
    zassert_equal(printIntLine_fake.arg1_history[i], testRegistry[intIndex + i].id,
                  "printIntLine call %zu: incorrect ID", i);
    zassert_equal(printIntLine_fake.arg2_history[i], testRegistry[intIndex + i].name,
                  "printIntLine call %zu: incorrect name", i);
  }

  /* Verify printMultiStateLine calls */
  size_t multiStateIndex = intIndex + INT_DATAPOINT_COUNT;
  zassert_equal(printMultiStateLine_fake.call_count, MULTI_STATE_DATAPOINT_COUNT,
                "printMultiStateLine should be called for each multi-state datapoint");
  for (size_t i = 0; i < MULTI_STATE_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printMultiStateLine_fake.arg0_history[i], shell,
                  "printMultiStateLine call %zu: incorrect shell", i);
    zassert_equal(printMultiStateLine_fake.arg1_history[i], testRegistry[multiStateIndex + i].id,
                  "printMultiStateLine call %zu: incorrect ID", i);
    zassert_equal(printMultiStateLine_fake.arg2_history[i], testRegistry[multiStateIndex + i].name,
                  "printMultiStateLine call %zu: incorrect name", i);
  }

  /* Verify printUintLine calls */
  size_t uintIndex = multiStateIndex + MULTI_STATE_DATAPOINT_COUNT;
  zassert_equal(printUintLine_fake.call_count, UINT_DATAPOINT_COUNT,
                "printUintLine should be called for each uint datapoint");
  for (size_t i = 0; i < UINT_DATAPOINT_COUNT; i++)
  {
    zassert_equal(printUintLine_fake.arg0_history[i], shell,
                  "printUintLine call %zu: incorrect shell", i);
    zassert_equal(printUintLine_fake.arg1_history[i], testRegistry[uintIndex + i].id,
                  "printUintLine call %zu: incorrect ID", i);
    zassert_equal(printUintLine_fake.arg2_history[i], testRegistry[uintIndex + i].name,
                  "printUintLine call %zu: incorrect name", i);
  }
}

ZTEST_SUITE(datastore_cmd_tests, NULL, cmd_tests_setup, cmd_tests_before, NULL, NULL);
