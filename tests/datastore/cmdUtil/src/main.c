/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2025-02-11
 * @brief     Datastore CmdUtil Tests
 *
 *            Unit tests for datastore command utility functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/fff.h>

/* Mock Kconfig options */
#define CONFIG_ENYA_DATASTORE 1

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(datastore, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Redefine LOG_ERR to avoid dereferencing invalid pointers in error messages */
#undef LOG_ERR
#define LOG_ERR(...) do {} while (0)

/* Mock datastore.h to avoid pulling in the full datastore service */
#ifndef DATASTORE_SRV
#define DATASTORE_SRV

#include "datastoreTypes.h"
#include "datastoreMeta.h"

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

#endif /* DATASTORE_SRV */

/* Mock shell functions */
FAKE_VOID_FUNC_VARARG(shell_fprintf_normal, const struct shell *, const char *, ...);
FAKE_VALUE_FUNC(long, shell_strtol, const char *, int, int *);
FAKE_VALUE_FUNC(unsigned long, shell_strtoul, const char *, int, int *);

/* Define the list of fakes for easy reset */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(shell_fprintf_normal) \
  FAKE(shell_strtol) \
  FAKE(shell_strtoul)

DEFINE_FFF_GLOBALS;

/* Include cmdUtil implementation */
#include "datastoreCmdUtil.c"

/**
 * Test setup function.
 */
static void *cmdUtil_tests_setup(void)
{
  return NULL;
}

/**
 * Test before function.
 */
static void cmdUtil_tests_before(void *fixture)
{
  /* Reset all FFF fakes */
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * Test after function.
 */
static void cmdUtil_tests_after(void *fixture)
{
  /* Cleanup if needed */
}

/**
 * @test getDatapointRegistrySize must return the total count of all datapoint types.
 */
ZTEST(cmdUtil_tests, test_getDatapointRegistrySize)
{
  size_t expectedSize = 3 + 2 + 2 + 2 + 2 + 2; /* Binary + Button + Float + Int + MultiState + Uint */
  size_t actualSize = getDatapointRegistrySize();

  zassert_equal(actualSize, expectedSize,
                "Registry size mismatch: expected %zu, got %zu",
                expectedSize, actualSize);
}

/**
 * @test toUpper must convert all lowercase characters to uppercase.
 */
ZTEST(cmdUtil_tests, test_toUpper_lowercase)
{
  char str[] = "hello";

  toUpper(str);

  zassert_str_equal(str, "HELLO", "String should be converted to uppercase");
}

/**
 * @test toUpper must convert mixed case string to uppercase.
 */
ZTEST(cmdUtil_tests, test_toUpper_mixed_case)
{
  char str[] = "HeLLo_WoRLd";

  toUpper(str);

  zassert_str_equal(str, "HELLO_WORLD", "Mixed case string should be converted to uppercase");
}

/**
 * @test toUpper must not modify already uppercase string.
 */
ZTEST(cmdUtil_tests, test_toUpper_already_uppercase)
{
  char str[] = "ALREADY_UPPERCASE";

  toUpper(str);

  zassert_str_equal(str, "ALREADY_UPPERCASE", "Uppercase string should remain unchanged");
}

/**
 * @test toUpper must preserve numbers and special characters.
 */
ZTEST(cmdUtil_tests, test_toUpper_with_numbers_and_special_chars)
{
  char str[] = "test123_name-456";

  toUpper(str);

  zassert_str_equal(str, "TEST123_NAME-456", "Numbers and special chars should remain unchanged");
}

/**
 * @test toUpper must handle empty string.
 */
ZTEST(cmdUtil_tests, test_toUpper_empty_string)
{
  char str[] = "";

  toUpper(str);

  zassert_str_equal(str, "", "Empty string should remain empty");
}

/**
 * @test toUpper must preserve spaces while converting letters.
 */
ZTEST(cmdUtil_tests, test_toUpper_with_spaces)
{
  char str[] = "hello world";

  toUpper(str);

  zassert_str_equal(str, "HELLO WORLD", "Spaces should be preserved");
}

/**
 * @test toUpper must convert single lowercase character.
 */
ZTEST(cmdUtil_tests, test_toUpper_single_char_lowercase)
{
  char str[] = "a";

  toUpper(str);

  zassert_str_equal(str, "A", "Single lowercase char should be converted");
}

/**
 * @test toUpper must not modify single uppercase character.
 */
ZTEST(cmdUtil_tests, test_toUpper_single_char_uppercase)
{
  char str[] = "Z";

  toUpper(str);

  zassert_str_equal(str, "Z", "Single uppercase char should remain unchanged");
}

/**
 * @test getTypeName must return "unknown" for invalid datapoint type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_unknown)
{
  const char *typeName = getTypeName(99);  /* Invalid type */

  zassert_str_equal(typeName, "unknown", "Type name for invalid type must be 'unknown'");
}

/**
 * @test getTypeName must return "binary" for DATAPOINT_BINARY type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_binary)
{
  const char *typeName = getTypeName(DATAPOINT_BINARY);

  zassert_str_equal(typeName, "binary", "Type name for DATAPOINT_BINARY must be 'binary'");
}

/**
 * @test getTypeName must return "button" for DATAPOINT_BUTTON type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_button)
{
  const char *typeName = getTypeName(DATAPOINT_BUTTON);

  zassert_str_equal(typeName, "button", "Type name for DATAPOINT_BUTTON must be 'button'");
}

/**
 * @test getTypeName must return "float" for DATAPOINT_FLOAT type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_float)
{
  const char *typeName = getTypeName(DATAPOINT_FLOAT);

  zassert_str_equal(typeName, "float", "Type name for DATAPOINT_FLOAT must be 'float'");
}

/**
 * @test getTypeName must return "int" for DATAPOINT_INT type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_int)
{
  const char *typeName = getTypeName(DATAPOINT_INT);

  zassert_str_equal(typeName, "int", "Type name for DATAPOINT_INT must be 'int'");
}

/**
 * @test getTypeName must return "multi_state" for DATAPOINT_MULTI_STATE type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_multi_state)
{
  const char *typeName = getTypeName(DATAPOINT_MULTI_STATE);

  zassert_str_equal(typeName, "multi_state", "Type name for DATAPOINT_MULTI_STATE must be 'multi_state'");
}

/**
 * @test getTypeName must return "uint" for DATAPOINT_UINT type.
 */
ZTEST(cmdUtil_tests, test_getTypeName_uint)
{
  const char *typeName = getTypeName(DATAPOINT_UINT);

  zassert_str_equal(typeName, "uint", "Type name for DATAPOINT_UINT must be 'uint'");
}

/**
 * @test parseBool must parse "true" and set value to true.
 */
ZTEST(cmdUtil_tests, test_parseBool_true_string)
{
  bool value = false;
  int result = parseBool("true", &value);

  zassert_equal(result, 0, "parseBool must return 0 for 'true'");
  zassert_true(value, "parseBool must set value to true for 'true'");
}

/**
 * @test parseBool must parse "1" and set value to true.
 */
ZTEST(cmdUtil_tests, test_parseBool_one_string)
{
  bool value = false;
  int result = parseBool("1", &value);

  zassert_equal(result, 0, "parseBool must return 0 for '1'");
  zassert_true(value, "parseBool must set value to true for '1'");
}

/**
 * @test parseBool must parse "false" and set value to false.
 */
ZTEST(cmdUtil_tests, test_parseBool_false_string)
{
  bool value = true;
  int result = parseBool("false", &value);

  zassert_equal(result, 0, "parseBool must return 0 for 'false'");
  zassert_false(value, "parseBool must set value to false for 'false'");
}

/**
 * @test parseBool must parse "0" and set value to false.
 */
ZTEST(cmdUtil_tests, test_parseBool_zero_string)
{
  bool value = true;
  int result = parseBool("0", &value);

  zassert_equal(result, 0, "parseBool must return 0 for '0'");
  zassert_false(value, "parseBool must set value to false for '0'");
}

/**
 * @test parseBool must return error for invalid string.
 */
ZTEST(cmdUtil_tests, test_parseBool_invalid_string)
{
  bool value;
  int result = parseBool("invalid", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for invalid string");
}

/**
 * @test parseBool must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseBool_empty_string)
{
  bool value;
  int result = parseBool("", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for empty string");
}

/**
 * @test parseBool must return error for uppercase TRUE.
 */
ZTEST(cmdUtil_tests, test_parseBool_uppercase_true)
{
  bool value;
  int result = parseBool("TRUE", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for uppercase TRUE");
}

/**
 * @test parseBool must return error for uppercase FALSE.
 */
ZTEST(cmdUtil_tests, test_parseBool_uppercase_false)
{
  bool value;
  int result = parseBool("FALSE", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for uppercase FALSE");
}

/**
 * @test parseBool must return error for mixed case true.
 */
ZTEST(cmdUtil_tests, test_parseBool_mixed_case_true)
{
  bool value;
  int result = parseBool("True", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for mixed case True");
}

/**
 * @test parseBool must return error for numbers other than 0 or 1.
 */
ZTEST(cmdUtil_tests, test_parseBool_invalid_number)
{
  bool value;
  int result = parseBool("2", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for number other than 0 or 1");
}

/**
 * @test parseBool must return error for partial match.
 */
ZTEST(cmdUtil_tests, test_parseBool_partial_match)
{
  bool value;
  int result = parseBool("tru", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for partial match");
}

/**
 * @test parseBool must return error for string with whitespace.
 */
ZTEST(cmdUtil_tests, test_parseBool_with_whitespace)
{
  bool value;
  int result = parseBool(" true", &value);

  zassert_equal(result, -EINVAL, "parseBool must return -EINVAL for string with leading whitespace");
}

/**
 * @test getButtonValueString must return "unknown" for invalid button state.
 */
ZTEST(cmdUtil_tests, test_getButtonValueString_unknown)
{
  const char *valueString = getButtonValueString(99);  /* Invalid state */

  zassert_str_equal(valueString, "unknown", "Value string for invalid button state must be 'unknown'");
}

/**
 * @test getButtonValueString must return "unpressed" for BUTTON_UNPRESSED state.
 */
ZTEST(cmdUtil_tests, test_getButtonValueString_unpressed)
{
  const char *valueString = getButtonValueString(BUTTON_UNPRESSED);

  zassert_str_equal(valueString, "unpressed", "Value string for BUTTON_UNPRESSED must be 'unpressed'");
}

/**
 * @test getButtonValueString must return "short_pressed" for BUTTON_SHORT_PRESSED state.
 */
ZTEST(cmdUtil_tests, test_getButtonValueString_short_pressed)
{
  const char *valueString = getButtonValueString(BUTTON_SHORT_PRESSED);

  zassert_str_equal(valueString, "short_pressed", "Value string for BUTTON_SHORT_PRESSED must be 'short_pressed'");
}

/**
 * @test getButtonValueString must return "long_pressed" for BUTTON_LONG_PRESSED state.
 */
ZTEST(cmdUtil_tests, test_getButtonValueString_long_pressed)
{
  const char *valueString = getButtonValueString(BUTTON_LONG_PRESSED);

  zassert_str_equal(valueString, "long_pressed", "Value string for BUTTON_LONG_PRESSED must be 'long_pressed'");
}

/**
 * @test printTableHeader must call shell_fprintf_normal twice to print header and separator.
 */
ZTEST(cmdUtil_tests, test_printTableHeader)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printTableHeader(mock_shell);

  /* Verify shell_fprintf_normal is called twice (once for header, once for separator) */
  zassert_equal(shell_fprintf_normal_fake.call_count, 2,
                "printTableHeader must call shell_fprintf_normal twice");

  /* Verify both calls use the same shell pointer */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "First call must use correct shell pointer");
  zassert_equal(shell_fprintf_normal_fake.arg0_history[1], mock_shell,
                "Second call must use correct shell pointer");
}

/**
 * @test printBinaryLine must call shell_fprintf_normal once with true value.
 */
ZTEST(cmdUtil_tests, test_printBinaryLine_true)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printBinaryLine(mock_shell, 5, "TEST_BINARY", true);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printBinaryLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printBinaryLine must use correct shell pointer");
}

/**
 * @test printBinaryLine must call shell_fprintf_normal once with false value.
 */
ZTEST(cmdUtil_tests, test_printBinaryLine_false)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printBinaryLine(mock_shell, 10, "TEST_BINARY_FALSE", false);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printBinaryLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printBinaryLine must use correct shell pointer");
}

/**
 * @test printButtonLine must call shell_fprintf_normal once with BUTTON_UNPRESSED.
 */
ZTEST(cmdUtil_tests, test_printButtonLine_unpressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printButtonLine(mock_shell, 3, "TEST_BUTTON", BUTTON_UNPRESSED);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonLine must use correct shell pointer");
}

/**
 * @test printButtonLine must call shell_fprintf_normal once with BUTTON_SHORT_PRESSED.
 */
ZTEST(cmdUtil_tests, test_printButtonLine_short_pressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printButtonLine(mock_shell, 7, "TEST_BUTTON_SHORT", BUTTON_SHORT_PRESSED);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonLine must use correct shell pointer");
}

/**
 * @test printButtonLine must call shell_fprintf_normal once with BUTTON_LONG_PRESSED.
 */
ZTEST(cmdUtil_tests, test_printButtonLine_long_pressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printButtonLine(mock_shell, 12, "TEST_BUTTON_LONG", BUTTON_LONG_PRESSED);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonLine must use correct shell pointer");
}

/**
 * @test printFloatLine must call shell_fprintf_normal once with positive float value.
 */
ZTEST(cmdUtil_tests, test_printFloatLine_positive)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printFloatLine(mock_shell, 4, "TEST_FLOAT", 123.456789f);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatLine must use correct shell pointer");
}

/**
 * @test printFloatLine must call shell_fprintf_normal once with negative float value.
 */
ZTEST(cmdUtil_tests, test_printFloatLine_negative)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printFloatLine(mock_shell, 8, "TEST_FLOAT_NEG", -45.67f);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatLine must use correct shell pointer");
}

/**
 * @test printFloatLine must call shell_fprintf_normal once with zero value.
 */
ZTEST(cmdUtil_tests, test_printFloatLine_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printFloatLine(mock_shell, 15, "TEST_FLOAT_ZERO", 0.0f);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatLine must use correct shell pointer");
}

/**
 * @test printIntLine must call shell_fprintf_normal once with positive int value.
 */
ZTEST(cmdUtil_tests, test_printIntLine_positive)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printIntLine(mock_shell, 6, "TEST_INT", 42);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntLine must use correct shell pointer");
}

/**
 * @test printIntLine must call shell_fprintf_normal once with negative int value.
 */
ZTEST(cmdUtil_tests, test_printIntLine_negative)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printIntLine(mock_shell, 9, "TEST_INT_NEG", -1234);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntLine must use correct shell pointer");
}

/**
 * @test printIntLine must call shell_fprintf_normal once with zero value.
 */
ZTEST(cmdUtil_tests, test_printIntLine_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printIntLine(mock_shell, 11, "TEST_INT_ZERO", 0);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntLine must use correct shell pointer");
}

/**
 * @test printMultiStateLine must call shell_fprintf_normal once with small value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateLine_small)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printMultiStateLine(mock_shell, 2, "TEST_MULTI_STATE", 5);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateLine must use correct shell pointer");
}

/**
 * @test printMultiStateLine must call shell_fprintf_normal once with large value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateLine_large)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printMultiStateLine(mock_shell, 13, "TEST_MULTI_STATE_LARGE", 999999);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateLine must use correct shell pointer");
}

/**
 * @test printMultiStateLine must call shell_fprintf_normal once with zero value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateLine_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printMultiStateLine(mock_shell, 16, "TEST_MULTI_STATE_ZERO", 0);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateLine must use correct shell pointer");
}

/**
 * @test printUintLine must call shell_fprintf_normal once with small value.
 */
ZTEST(cmdUtil_tests, test_printUintLine_small)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printUintLine(mock_shell, 1, "TEST_UINT", 10);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintLine must use correct shell pointer");
}

/**
 * @test printUintLine must call shell_fprintf_normal once with large value.
 */
ZTEST(cmdUtil_tests, test_printUintLine_large)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printUintLine(mock_shell, 14, "TEST_UINT_LARGE", 4294967295U);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintLine must use correct shell pointer");
}

/**
 * @test printUintLine must call shell_fprintf_normal once with zero value.
 */
ZTEST(cmdUtil_tests, test_printUintLine_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */

  printUintLine(mock_shell, 17, "TEST_UINT_ZERO", 0);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintLine must call shell_fprintf_normal once");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintLine must use correct shell pointer");
}

/**
 * @test printBinaryValues must call shell_fprintf_normal once for single true value.
 */
ZTEST(cmdUtil_tests, test_printBinaryValues_single_true)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BINARY_TEST", DATAPOINT_BINARY, 0}};
  uint8_t values[] = {1};

  printBinaryValues(mock_shell, entries, (Data_t *)values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printBinaryValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printBinaryValues must use correct shell pointer");
}

/**
 * @test printBinaryValues must call shell_fprintf_normal once for single false value.
 */
ZTEST(cmdUtil_tests, test_printBinaryValues_single_false)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BINARY_FALSE", DATAPOINT_BINARY, 0}};
  uint8_t values[] = {0};

  printBinaryValues(mock_shell, entries, (Data_t *)values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printBinaryValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printBinaryValues must use correct shell pointer");
}

/**
 * @test printBinaryValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printBinaryValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"BINARY_1", DATAPOINT_BINARY, 0},
    {"BINARY_2", DATAPOINT_BINARY, 1},
    {"BINARY_3", DATAPOINT_BINARY, 2}
  };
  uint8_t values[] = {1, 0, 1};

  printBinaryValues(mock_shell, entries, (Data_t *)values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printBinaryValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printBinaryValues must use correct shell pointer");
}

/**
 * @test printBinaryValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printBinaryValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BINARY_TEST", DATAPOINT_BINARY, 0}};
  uint8_t values[] = {1};

  printBinaryValues(mock_shell, entries, (Data_t *)values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printBinaryValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test printButtonValues must call shell_fprintf_normal once for single unpressed value.
 */
ZTEST(cmdUtil_tests, test_printButtonValues_single_unpressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BUTTON_TEST", DATAPOINT_BUTTON, 0}};
  Data_t values[] = {{.uintVal = BUTTON_UNPRESSED}};

  printButtonValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonValues must use correct shell pointer");
}

/**
 * @test printButtonValues must call shell_fprintf_normal once for single short pressed value.
 */
ZTEST(cmdUtil_tests, test_printButtonValues_single_short_pressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BUTTON_SHORT", DATAPOINT_BUTTON, 0}};
  Data_t values[] = {{.uintVal = BUTTON_SHORT_PRESSED}};

  printButtonValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonValues must use correct shell pointer");
}

/**
 * @test printButtonValues must call shell_fprintf_normal once for single long pressed value.
 */
ZTEST(cmdUtil_tests, test_printButtonValues_single_long_pressed)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BUTTON_LONG", DATAPOINT_BUTTON, 0}};
  Data_t values[] = {{.uintVal = BUTTON_LONG_PRESSED}};

  printButtonValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printButtonValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonValues must use correct shell pointer");
}

/**
 * @test printButtonValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printButtonValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"BUTTON_1", DATAPOINT_BUTTON, 0},
    {"BUTTON_2", DATAPOINT_BUTTON, 1},
    {"BUTTON_3", DATAPOINT_BUTTON, 2}
  };
  Data_t values[] = {
    {.uintVal = BUTTON_UNPRESSED},
    {.uintVal = BUTTON_SHORT_PRESSED},
    {.uintVal = BUTTON_LONG_PRESSED}
  };

  printButtonValues(mock_shell, entries, values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printButtonValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printButtonValues must use correct shell pointer");
}

/**
 * @test printButtonValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printButtonValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"BUTTON_TEST", DATAPOINT_BUTTON, 0}};
  Data_t values[] = {{.uintVal = BUTTON_UNPRESSED}};

  printButtonValues(mock_shell, entries, values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printButtonValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test printFloatValues must call shell_fprintf_normal once for single positive value.
 */
ZTEST(cmdUtil_tests, test_printFloatValues_single_positive)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"FLOAT_TEST", DATAPOINT_FLOAT, 0}};
  Data_t values[] = {{.floatVal = 123.456f}};

  printFloatValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatValues must use correct shell pointer");
}

/**
 * @test printFloatValues must call shell_fprintf_normal once for single negative value.
 */
ZTEST(cmdUtil_tests, test_printFloatValues_single_negative)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"FLOAT_NEG", DATAPOINT_FLOAT, 0}};
  Data_t values[] = {{.floatVal = -45.67f}};

  printFloatValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatValues must use correct shell pointer");
}

/**
 * @test printFloatValues must call shell_fprintf_normal once for single zero value.
 */
ZTEST(cmdUtil_tests, test_printFloatValues_single_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"FLOAT_ZERO", DATAPOINT_FLOAT, 0}};
  Data_t values[] = {{.floatVal = 0.0f}};

  printFloatValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printFloatValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatValues must use correct shell pointer");
}

/**
 * @test printFloatValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printFloatValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"FLOAT_1", DATAPOINT_FLOAT, 0},
    {"FLOAT_2", DATAPOINT_FLOAT, 1},
    {"FLOAT_3", DATAPOINT_FLOAT, 2}
  };
  Data_t values[] = {
    {.floatVal = 1.23f},
    {.floatVal = -4.56f},
    {.floatVal = 0.0f}
  };

  printFloatValues(mock_shell, entries, values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printFloatValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printFloatValues must use correct shell pointer");
}

/**
 * @test printFloatValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printFloatValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"FLOAT_TEST", DATAPOINT_FLOAT, 0}};
  Data_t values[] = {{.floatVal = 1.23f}};

  printFloatValues(mock_shell, entries, values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printFloatValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test printIntValues must call shell_fprintf_normal once for single positive value.
 */
ZTEST(cmdUtil_tests, test_printIntValues_single_positive)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"INT_TEST", DATAPOINT_INT, 0}};
  Data_t values[] = {{.intVal = 42}};

  printIntValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntValues must use correct shell pointer");
}

/**
 * @test printIntValues must call shell_fprintf_normal once for single negative value.
 */
ZTEST(cmdUtil_tests, test_printIntValues_single_negative)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"INT_NEG", DATAPOINT_INT, 0}};
  Data_t values[] = {{.intVal = -1234}};

  printIntValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntValues must use correct shell pointer");
}

/**
 * @test printIntValues must call shell_fprintf_normal once for single zero value.
 */
ZTEST(cmdUtil_tests, test_printIntValues_single_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"INT_ZERO", DATAPOINT_INT, 0}};
  Data_t values[] = {{.intVal = 0}};

  printIntValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printIntValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntValues must use correct shell pointer");
}

/**
 * @test printIntValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printIntValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"INT_1", DATAPOINT_INT, 0},
    {"INT_2", DATAPOINT_INT, 1},
    {"INT_3", DATAPOINT_INT, 2}
  };
  Data_t values[] = {
    {.intVal = 100},
    {.intVal = -200},
    {.intVal = 0}
  };

  printIntValues(mock_shell, entries, values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printIntValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printIntValues must use correct shell pointer");
}

/**
 * @test printIntValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printIntValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"INT_TEST", DATAPOINT_INT, 0}};
  Data_t values[] = {{.intVal = 42}};

  printIntValues(mock_shell, entries, values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printIntValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test printMultiStateValues must call shell_fprintf_normal once for single small value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateValues_single_small)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"MULTI_STATE_TEST", DATAPOINT_MULTI_STATE, 0}};
  Data_t values[] = {{.uintVal = 5}};

  printMultiStateValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateValues must use correct shell pointer");
}

/**
 * @test printMultiStateValues must call shell_fprintf_normal once for single large value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateValues_single_large)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"MULTI_STATE_LARGE", DATAPOINT_MULTI_STATE, 0}};
  Data_t values[] = {{.uintVal = 999999}};

  printMultiStateValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateValues must use correct shell pointer");
}

/**
 * @test printMultiStateValues must call shell_fprintf_normal once for single zero value.
 */
ZTEST(cmdUtil_tests, test_printMultiStateValues_single_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"MULTI_STATE_ZERO", DATAPOINT_MULTI_STATE, 0}};
  Data_t values[] = {{.uintVal = 0}};

  printMultiStateValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printMultiStateValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateValues must use correct shell pointer");
}

/**
 * @test printMultiStateValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printMultiStateValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"MULTI_STATE_1", DATAPOINT_MULTI_STATE, 0},
    {"MULTI_STATE_2", DATAPOINT_MULTI_STATE, 1},
    {"MULTI_STATE_3", DATAPOINT_MULTI_STATE, 2}
  };
  Data_t values[] = {
    {.uintVal = 1},
    {.uintVal = 2},
    {.uintVal = 3}
  };

  printMultiStateValues(mock_shell, entries, values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printMultiStateValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printMultiStateValues must use correct shell pointer");
}

/**
 * @test printMultiStateValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printMultiStateValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"MULTI_STATE_TEST", DATAPOINT_MULTI_STATE, 0}};
  Data_t values[] = {{.uintVal = 5}};

  printMultiStateValues(mock_shell, entries, values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printMultiStateValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test printUintValues must call shell_fprintf_normal once for single small value.
 */
ZTEST(cmdUtil_tests, test_printUintValues_single_small)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"UINT_TEST", DATAPOINT_UINT, 0}};
  Data_t values[] = {{.uintVal = 10}};

  printUintValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintValues must use correct shell pointer");
}

/**
 * @test printUintValues must call shell_fprintf_normal once for single large value.
 */
ZTEST(cmdUtil_tests, test_printUintValues_single_large)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"UINT_LARGE", DATAPOINT_UINT, 0}};
  Data_t values[] = {{.uintVal = 4294967295U}};

  printUintValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintValues must use correct shell pointer");
}

/**
 * @test printUintValues must call shell_fprintf_normal once for single zero value.
 */
ZTEST(cmdUtil_tests, test_printUintValues_single_zero)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"UINT_ZERO", DATAPOINT_UINT, 0}};
  Data_t values[] = {{.uintVal = 0}};

  printUintValues(mock_shell, entries, values, 1);

  /* Verify shell_fprintf_normal is called once */
  zassert_equal(shell_fprintf_normal_fake.call_count, 1,
                "printUintValues must call shell_fprintf_normal once for single value");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintValues must use correct shell pointer");
}

/**
 * @test printUintValues must call shell_fprintf_normal multiple times for multiple values.
 */
ZTEST(cmdUtil_tests, test_printUintValues_multiple)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {
    {"UINT_1", DATAPOINT_UINT, 0},
    {"UINT_2", DATAPOINT_UINT, 1},
    {"UINT_3", DATAPOINT_UINT, 2}
  };
  Data_t values[] = {
    {.uintVal = 100},
    {.uintVal = 200},
    {.uintVal = 300}
  };

  printUintValues(mock_shell, entries, values, 3);

  /* Verify shell_fprintf_normal is called three times */
  zassert_equal(shell_fprintf_normal_fake.call_count, 3,
                "printUintValues must call shell_fprintf_normal three times for three values");

  /* Verify correct shell pointer is used */
  zassert_equal(shell_fprintf_normal_fake.arg0_val, mock_shell,
                "printUintValues must use correct shell pointer");
}

/**
 * @test printUintValues must not call shell_fprintf_normal for zero values.
 */
ZTEST(cmdUtil_tests, test_printUintValues_zero_count)
{
  const struct shell *mock_shell = (const struct shell *)0x1234;  /* Mock shell pointer */
  DatapointEntry_t entries[] = {{"UINT_TEST", DATAPOINT_UINT, 0}};
  Data_t values[] = {{.uintVal = 10}};

  printUintValues(mock_shell, entries, values, 0);

  /* Verify shell_fprintf_normal is not called */
  zassert_equal(shell_fprintf_normal_fake.call_count, 0,
                "printUintValues must not call shell_fprintf_normal for zero count");
}

/**
 * @test parseBinaryValues must parse single true value successfully.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_single_true)
{
  char *valStrings[] = {"true"};
  Data_t values[1];
  int result;

  result = parseBinaryValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseBinaryValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 1, "parseBinaryValues must set value to 1 for 'true'");
}

/**
 * @test parseBinaryValues must parse single false value successfully.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_single_false)
{
  char *valStrings[] = {"false"};
  Data_t values[1];
  int result;

  result = parseBinaryValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseBinaryValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 0, "parseBinaryValues must set value to 0 for 'false'");
}

/**
 * @test parseBinaryValues must parse single numeric true value successfully.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_single_one)
{
  char *valStrings[] = {"1"};
  Data_t values[1];
  int result;

  result = parseBinaryValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseBinaryValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 1, "parseBinaryValues must set value to 1 for '1'");
}

/**
 * @test parseBinaryValues must parse single numeric false value successfully.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_single_zero)
{
  char *valStrings[] = {"0"};
  Data_t values[1];
  int result;

  result = parseBinaryValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseBinaryValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 0, "parseBinaryValues must set value to 0 for '0'");
}

/**
 * @test parseBinaryValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_multiple_mixed)
{
  char *valStrings[] = {"true", "false", "1", "0"};
  Data_t values[4];
  int result;

  result = parseBinaryValues(valStrings, 4, values);

  zassert_equal(result, 0, "parseBinaryValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 1, "First value must be 1 for 'true'");
  zassert_equal(values[1].uintVal, 0, "Second value must be 0 for 'false'");
  zassert_equal(values[2].uintVal, 1, "Third value must be 1 for '1'");
  zassert_equal(values[3].uintVal, 0, "Fourth value must be 0 for '0'");
}

/**
 * @test parseBinaryValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_error_first_position)
{
  char *valStrings[] = {"invalid", "true", "false"};
  Data_t values[3];
  int result;

  result = parseBinaryValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseBinaryValues must return -EINVAL for invalid first value");
}

/**
 * @test parseBinaryValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_error_middle_position)
{
  char *valStrings[] = {"true", "invalid", "false"};
  Data_t values[3];
  int result;

  result = parseBinaryValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseBinaryValues must return -EINVAL for invalid middle value");
}

/**
 * @test parseBinaryValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_error_last_position)
{
  char *valStrings[] = {"true", "false", "invalid"};
  Data_t values[3];
  int result;

  result = parseBinaryValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseBinaryValues must return -EINVAL for invalid last value");
}

/**
 * @test parseBinaryValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseBinaryValues_error_empty_string)
{
  char *valStrings[] = {"true", "", "false"};
  Data_t values[3];
  int result;

  result = parseBinaryValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseBinaryValues must return -EINVAL for empty string");
}

/**
 * @test parseButtonValues must parse single unpressed value successfully.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_single_unpressed)
{
  char *valStrings[] = {"unpressed"};
  Data_t values[1];
  int result;

  result = parseButtonValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseButtonValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, BUTTON_UNPRESSED,
                "parseButtonValues must set value to BUTTON_UNPRESSED for 'unpressed'");
}

/**
 * @test parseButtonValues must parse single short_pressed value successfully.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_single_short_pressed)
{
  char *valStrings[] = {"short_pressed"};
  Data_t values[1];
  int result;

  result = parseButtonValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseButtonValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, BUTTON_SHORT_PRESSED,
                "parseButtonValues must set value to BUTTON_SHORT_PRESSED for 'short_pressed'");
}

/**
 * @test parseButtonValues must parse single long_pressed value successfully.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_single_long_pressed)
{
  char *valStrings[] = {"long_pressed"};
  Data_t values[1];
  int result;

  result = parseButtonValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseButtonValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, BUTTON_LONG_PRESSED,
                "parseButtonValues must set value to BUTTON_LONG_PRESSED for 'long_pressed'");
}

/**
 * @test parseButtonValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_multiple_mixed)
{
  char *valStrings[] = {"unpressed", "short_pressed", "long_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, 0, "parseButtonValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, BUTTON_UNPRESSED, "First value must be BUTTON_UNPRESSED");
  zassert_equal(values[1].uintVal, BUTTON_SHORT_PRESSED, "Second value must be BUTTON_SHORT_PRESSED");
  zassert_equal(values[2].uintVal, BUTTON_LONG_PRESSED, "Third value must be BUTTON_LONG_PRESSED");
}

/**
 * @test parseButtonValues must parse multiple same values successfully.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_multiple_same)
{
  char *valStrings[] = {"unpressed", "unpressed", "unpressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, 0, "parseButtonValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, BUTTON_UNPRESSED, "First value must be BUTTON_UNPRESSED");
  zassert_equal(values[1].uintVal, BUTTON_UNPRESSED, "Second value must be BUTTON_UNPRESSED");
  zassert_equal(values[2].uintVal, BUTTON_UNPRESSED, "Third value must be BUTTON_UNPRESSED");
}

/**
 * @test parseButtonValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_first_position)
{
  char *valStrings[] = {"invalid", "unpressed", "short_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for invalid first value");
}

/**
 * @test parseButtonValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_middle_position)
{
  char *valStrings[] = {"unpressed", "invalid", "short_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for invalid middle value");
}

/**
 * @test parseButtonValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_last_position)
{
  char *valStrings[] = {"unpressed", "short_pressed", "invalid"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for invalid last value");
}

/**
 * @test parseButtonValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_empty_string)
{
  char *valStrings[] = {"unpressed", "", "short_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for empty string");
}

/**
 * @test parseButtonValues must return error for uppercase string.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_uppercase)
{
  char *valStrings[] = {"UNPRESSED", "short_pressed", "long_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for uppercase string");
}

/**
 * @test parseButtonValues must return error for partial match.
 */
ZTEST(cmdUtil_tests, test_parseButtonValues_error_partial_match)
{
  char *valStrings[] = {"short", "unpressed", "long_pressed"};
  Data_t values[3];
  int result;

  result = parseButtonValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseButtonValues must return -EINVAL for partial match");
}

/**
 * @test parseFloatValues must parse single positive value successfully.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_single_positive)
{
  char *valStrings[] = {"123.456"};
  Data_t values[1];
  int result;

  result = parseFloatValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseFloatValues must return 0 for valid input");
  zassert_within(values[0].floatVal, 123.456f, 0.001f,
                 "parseFloatValues must set value to 123.456");
}

/**
 * @test parseFloatValues must parse single negative value successfully.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_single_negative)
{
  char *valStrings[] = {"-45.67"};
  Data_t values[1];
  int result;

  result = parseFloatValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseFloatValues must return 0 for valid input");
  zassert_within(values[0].floatVal, -45.67f, 0.001f,
                 "parseFloatValues must set value to -45.67");
}

/**
 * @test parseFloatValues must parse single zero value successfully.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_single_zero)
{
  char *valStrings[] = {"0.0"};
  Data_t values[1];
  int result;

  result = parseFloatValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseFloatValues must return 0 for valid input");
  zassert_within(values[0].floatVal, 0.0f, 0.001f,
                 "parseFloatValues must set value to 0.0");
}

/**
 * @test parseFloatValues must parse integer as float successfully.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_integer_format)
{
  char *valStrings[] = {"42"};
  Data_t values[1];
  int result;

  result = parseFloatValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseFloatValues must return 0 for integer format");
  zassert_within(values[0].floatVal, 42.0f, 0.001f,
                 "parseFloatValues must set value to 42.0");
}

/**
 * @test parseFloatValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_multiple_mixed)
{
  char *valStrings[] = {"1.23", "-4.56", "0.0", "789"};
  Data_t values[4];
  int result;

  result = parseFloatValues(valStrings, 4, values);

  zassert_equal(result, 0, "parseFloatValues must return 0 for valid input");
  zassert_within(values[0].floatVal, 1.23f, 0.001f, "First value must be 1.23");
  zassert_within(values[1].floatVal, -4.56f, 0.001f, "Second value must be -4.56");
  zassert_within(values[2].floatVal, 0.0f, 0.001f, "Third value must be 0.0");
  zassert_within(values[3].floatVal, 789.0f, 0.001f, "Fourth value must be 789.0");
}

/**
 * @test parseFloatValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_first_position)
{
  char *valStrings[] = {"invalid", "1.23", "4.56"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for invalid first value");
}

/**
 * @test parseFloatValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_middle_position)
{
  char *valStrings[] = {"1.23", "invalid", "4.56"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for invalid middle value");
}

/**
 * @test parseFloatValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_last_position)
{
  char *valStrings[] = {"1.23", "4.56", "invalid"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for invalid last value");
}

/**
 * @test parseFloatValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_empty_string)
{
  char *valStrings[] = {"1.23", "", "4.56"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for empty string");
}

/**
 * @test parseFloatValues must return error for string with trailing characters.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_trailing_chars)
{
  char *valStrings[] = {"1.23abc", "4.56", "7.89"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for trailing characters");
}

/**
 * @test parseFloatValues must return error for string with leading non-numeric.
 */
ZTEST(cmdUtil_tests, test_parseFloatValues_error_leading_chars)
{
  char *valStrings[] = {"abc1.23", "4.56", "7.89"};
  Data_t values[3];
  int result;

  result = parseFloatValues(valStrings, 3, values);

  zassert_equal(result, -EINVAL, "parseFloatValues must return -EINVAL for leading non-numeric characters");
}

/* Custom fake for shell_strtol that returns error on first call */
static long shell_strtol_error_first(const char *str, int base, int *err)
{
  if (shell_strtol_fake.call_count == 1) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtol(str, NULL, base);
}

/* Custom fake for shell_strtol that returns error on second call */
static long shell_strtol_error_second(const char *str, int base, int *err)
{
  if (shell_strtol_fake.call_count == 2) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtol(str, NULL, base);
}

/* Custom fake for shell_strtol that returns error on third call */
static long shell_strtol_error_third(const char *str, int base, int *err)
{
  if (shell_strtol_fake.call_count == 3) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtol(str, NULL, base);
}

/* Custom fake for shell_strtol that parses strings normally */
static long shell_strtol_success(const char *str, int base, int *err)
{
  char *endptr;
  long result = strtol(str, &endptr, base);
  if (endptr == str || *endptr != '\0') {
    *err = -EINVAL;
  } else {
    *err = 0;
  }
  return result;
}

/**
 * @test parseIntValues must parse single positive value successfully.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_single_positive)
{
  char *valStrings[] = {"42"};
  Data_t values[1];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_success;

  result = parseIntValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseIntValues must return 0 for valid input");
  zassert_equal(values[0].intVal, 42, "parseIntValues must set value to 42");
}

/**
 * @test parseIntValues must parse single negative value successfully.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_single_negative)
{
  char *valStrings[] = {"-1234"};
  Data_t values[1];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_success;

  result = parseIntValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseIntValues must return 0 for valid input");
  zassert_equal(values[0].intVal, -1234, "parseIntValues must set value to -1234");
}

/**
 * @test parseIntValues must parse single zero value successfully.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_single_zero)
{
  char *valStrings[] = {"0"};
  Data_t values[1];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_success;

  result = parseIntValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseIntValues must return 0 for valid input");
  zassert_equal(values[0].intVal, 0, "parseIntValues must set value to 0");
}

/**
 * @test parseIntValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_multiple_mixed)
{
  char *valStrings[] = {"100", "-200", "0", "42"};
  Data_t values[4];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_success;

  result = parseIntValues(valStrings, 4, values);

  zassert_equal(result, 0, "parseIntValues must return 0 for valid input");
  zassert_equal(values[0].intVal, 100, "First value must be 100");
  zassert_equal(values[1].intVal, -200, "Second value must be -200");
  zassert_equal(values[2].intVal, 0, "Third value must be 0");
  zassert_equal(values[3].intVal, 42, "Fourth value must be 42");
}

/**
 * @test parseIntValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_first_position)
{
  char *valStrings[] = {"invalid", "123", "456"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_first;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for invalid first value");
}

/**
 * @test parseIntValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_middle_position)
{
  char *valStrings[] = {"123", "invalid", "456"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_second;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for invalid middle value");
}

/**
 * @test parseIntValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_last_position)
{
  char *valStrings[] = {"123", "456", "invalid"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_third;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for invalid last value");
}

/**
 * @test parseIntValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_empty_string)
{
  char *valStrings[] = {"123", "", "456"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_second;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for empty string");
}

/**
 * @test parseIntValues must return error for string with trailing characters.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_trailing_chars)
{
  char *valStrings[] = {"123abc", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_first;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for trailing characters");
}

/**
 * @test parseIntValues must return error for float format.
 */
ZTEST(cmdUtil_tests, test_parseIntValues_error_float_format)
{
  char *valStrings[] = {"123.456", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtol_fake.custom_fake = shell_strtol_error_first;

  result = parseIntValues(valStrings, 3, values);

  zassert_true(result < 0, "parseIntValues must return error for float format");
}

/* Custom fake for shell_strtoul that returns error on first call */
static unsigned long shell_strtoul_error_first(const char *str, int base, int *err)
{
  if (shell_strtoul_fake.call_count == 1) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtoul(str, NULL, base);
}

/* Custom fake for shell_strtoul that returns error on second call */
static unsigned long shell_strtoul_error_second(const char *str, int base, int *err)
{
  if (shell_strtoul_fake.call_count == 2) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtoul(str, NULL, base);
}

/* Custom fake for shell_strtoul that returns error on third call */
static unsigned long shell_strtoul_error_third(const char *str, int base, int *err)
{
  if (shell_strtoul_fake.call_count == 3) {
    *err = -EINVAL;
    return 0;
  }
  *err = 0;
  return strtoul(str, NULL, base);
}

/* Custom fake for shell_strtoul that parses strings normally */
static unsigned long shell_strtoul_success(const char *str, int base, int *err)
{
  char *endptr;
  unsigned long result = strtoul(str, &endptr, base);
  if (endptr == str || *endptr != '\0') {
    *err = -EINVAL;
  } else {
    *err = 0;
  }
  return result;
}

/**
 * @test parseMultiStateValues must parse single small value successfully.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_single_small)
{
  char *valStrings[] = {"5"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseMultiStateValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseMultiStateValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 5, "parseMultiStateValues must set value to 5");
}

/**
 * @test parseMultiStateValues must parse single large value successfully.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_single_large)
{
  char *valStrings[] = {"999999"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseMultiStateValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseMultiStateValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 999999, "parseMultiStateValues must set value to 999999");
}

/**
 * @test parseMultiStateValues must parse single zero value successfully.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_single_zero)
{
  char *valStrings[] = {"0"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseMultiStateValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseMultiStateValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 0, "parseMultiStateValues must set value to 0");
}

/**
 * @test parseMultiStateValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_multiple_mixed)
{
  char *valStrings[] = {"1", "2", "3", "100"};
  Data_t values[4];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseMultiStateValues(valStrings, 4, values);

  zassert_equal(result, 0, "parseMultiStateValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 1, "First value must be 1");
  zassert_equal(values[1].uintVal, 2, "Second value must be 2");
  zassert_equal(values[2].uintVal, 3, "Third value must be 3");
  zassert_equal(values[3].uintVal, 100, "Fourth value must be 100");
}

/**
 * @test parseMultiStateValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_first_position)
{
  char *valStrings[] = {"invalid", "123", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for invalid first value");
}

/**
 * @test parseMultiStateValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_middle_position)
{
  char *valStrings[] = {"123", "invalid", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_second;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for invalid middle value");
}

/**
 * @test parseMultiStateValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_last_position)
{
  char *valStrings[] = {"123", "456", "invalid"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_third;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for invalid last value");
}

/**
 * @test parseMultiStateValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_empty_string)
{
  char *valStrings[] = {"123", "", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_second;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for empty string");
}

/**
 * @test parseMultiStateValues must return error for string with trailing characters.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_trailing_chars)
{
  char *valStrings[] = {"123abc", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for trailing characters");
}

/**
 * @test parseMultiStateValues must return error for negative number.
 */
ZTEST(cmdUtil_tests, test_parseMultiStateValues_error_negative)
{
  char *valStrings[] = {"-123", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseMultiStateValues(valStrings, 3, values);

  zassert_true(result < 0, "parseMultiStateValues must return error for negative number");
}

/**
 * @test parseUintValues must parse single small value successfully.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_single_small)
{
  char *valStrings[] = {"10"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseUintValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseUintValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 10, "parseUintValues must set value to 10");
}

/**
 * @test parseUintValues must parse single large value successfully.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_single_large)
{
  char *valStrings[] = {"4294967295"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseUintValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseUintValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 4294967295U, "parseUintValues must set value to 4294967295");
}

/**
 * @test parseUintValues must parse single zero value successfully.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_single_zero)
{
  char *valStrings[] = {"0"};
  Data_t values[1];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseUintValues(valStrings, 1, values);

  zassert_equal(result, 0, "parseUintValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 0, "parseUintValues must set value to 0");
}

/**
 * @test parseUintValues must parse multiple mixed values successfully.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_multiple_mixed)
{
  char *valStrings[] = {"100", "200", "300", "0"};
  Data_t values[4];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_success;

  result = parseUintValues(valStrings, 4, values);

  zassert_equal(result, 0, "parseUintValues must return 0 for valid input");
  zassert_equal(values[0].uintVal, 100, "First value must be 100");
  zassert_equal(values[1].uintVal, 200, "Second value must be 200");
  zassert_equal(values[2].uintVal, 300, "Third value must be 300");
  zassert_equal(values[3].uintVal, 0, "Fourth value must be 0");
}

/**
 * @test parseUintValues must return error for invalid string in first position.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_first_position)
{
  char *valStrings[] = {"invalid", "123", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for invalid first value");
}

/**
 * @test parseUintValues must return error for invalid string in middle position.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_middle_position)
{
  char *valStrings[] = {"123", "invalid", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_second;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for invalid middle value");
}

/**
 * @test parseUintValues must return error for invalid string in last position.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_last_position)
{
  char *valStrings[] = {"123", "456", "invalid"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_third;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for invalid last value");
}

/**
 * @test parseUintValues must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_empty_string)
{
  char *valStrings[] = {"123", "", "456"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_second;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for empty string");
}

/**
 * @test parseUintValues must return error for string with trailing characters.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_trailing_chars)
{
  char *valStrings[] = {"123abc", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for trailing characters");
}

/**
 * @test parseUintValues must return error for negative number.
 */
ZTEST(cmdUtil_tests, test_parseUintValues_error_negative)
{
  char *valStrings[] = {"-123", "456", "789"};
  Data_t values[3];
  int result;

  shell_strtoul_fake.custom_fake = shell_strtoul_error_first;

  result = parseUintValues(valStrings, 3, values);

  zassert_true(result < 0, "parseUintValues must return error for negative number");
}

/**
 * @test findDatapointByName must find first datapoint in registry successfully.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_success_first)
{
  const DatapointEntry_t *entry = NULL;
  const DatapointEntry_t *registry = getDatapointRegistry();
  int result;

  /* Search for the first datapoint by its name */
  result = findDatapointByName(registry[0].name, &entry);

  zassert_equal(result, 0, "findDatapointByName must return 0 for valid name");
  zassert_not_null(entry, "Entry pointer must not be NULL when found");
  zassert_equal(entry, &registry[0], "Entry pointer must point to the correct registry entry");
  zassert_str_equal(entry->name, registry[0].name, "Entry name must match search name");
}

/**
 * @test findDatapointByName must find middle datapoint in registry successfully.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_success_middle)
{
  const DatapointEntry_t *entry = NULL;
  const DatapointEntry_t *registry = getDatapointRegistry();
  size_t registry_size = getDatapointRegistrySize();
  size_t middle_index = registry_size / 2;
  int result;

  /* Search for a middle datapoint by its name */
  result = findDatapointByName(registry[middle_index].name, &entry);

  zassert_equal(result, 0, "findDatapointByName must return 0 for valid name");
  zassert_not_null(entry, "Entry pointer must not be NULL when found");
  zassert_equal(entry, &registry[middle_index], "Entry pointer must point to the correct registry entry");
  zassert_str_equal(entry->name, registry[middle_index].name, "Entry name must match search name");
}

/**
 * @test findDatapointByName must find last datapoint in registry successfully.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_success_last)
{
  const DatapointEntry_t *entry = NULL;
  const DatapointEntry_t *registry = getDatapointRegistry();
  size_t registry_size = getDatapointRegistrySize();
  size_t last_index = registry_size - 1;
  int result;

  /* Search for the last datapoint by its name */
  result = findDatapointByName(registry[last_index].name, &entry);

  zassert_equal(result, 0, "findDatapointByName must return 0 for valid name");
  zassert_not_null(entry, "Entry pointer must not be NULL when found");
  zassert_equal(entry, &registry[last_index], "Entry pointer must point to the correct registry entry");
  zassert_str_equal(entry->name, registry[last_index].name, "Entry name must match search name");
}

/**
 * @test findDatapointByName must find datapoint and preserve type information.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_success_type_preserved)
{
  const DatapointEntry_t *entry = NULL;
  const DatapointEntry_t *registry = getDatapointRegistry();
  int result;

  /* Search for the first datapoint */
  result = findDatapointByName(registry[0].name, &entry);

  zassert_equal(result, 0, "findDatapointByName must return 0 for valid name");
  zassert_not_null(entry, "Entry pointer must not be NULL when found");
  zassert_equal(entry->type, registry[0].type, "Entry type must be preserved");
  zassert_equal(entry->id, registry[0].id, "Entry id must be preserved");
}

/**
 * @test findDatapointByName must return error for non-existent name.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_error_not_found)
{
  const DatapointEntry_t *entry = NULL;
  int result;

  result = findDatapointByName("NONEXISTENT_DATAPOINT", &entry);

  zassert_equal(result, -ENOENT, "findDatapointByName must return -ENOENT for non-existent name");
  zassert_is_null(entry, "Entry pointer should remain NULL when not found");
}

/**
 * @test findDatapointByName must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_error_empty_string)
{
  const DatapointEntry_t *entry = NULL;
  int result;

  result = findDatapointByName("", &entry);

  zassert_equal(result, -ENOENT, "findDatapointByName must return -ENOENT for empty string");
}

/**
 * @test findDatapointByName must return error for partial match.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_error_partial_match)
{
  const DatapointEntry_t *entry = NULL;
  int result;

  /* Assuming there's a datapoint like "BINARY_VALUE_1", test with just "BINARY" */
  result = findDatapointByName("BINARY_VALUE", &entry);

  zassert_equal(result, -ENOENT, "findDatapointByName must return -ENOENT for partial match");
}

/**
 * @test findDatapointByName must return error for case mismatch.
 */
ZTEST(cmdUtil_tests, test_findDatapointByName_error_case_mismatch)
{
  const DatapointEntry_t *entry = NULL;
  int result;

  /* Assuming datapoint names are uppercase, test with lowercase */
  result = findDatapointByName("binary_value_1", &entry);

  zassert_equal(result, -ENOENT, "findDatapointByName must return -ENOENT for case mismatch");
}

/**
 * @test parseButtonValue must parse "unpressed" and set value to BUTTON_UNPRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_unpressed_string)
{
  ButtonState_t value = BUTTON_SHORT_PRESSED;
  int result = parseButtonValue("unpressed", &value);

  zassert_equal(result, 0, "parseButtonValue must return 0 for 'unpressed'");
  zassert_equal(value, BUTTON_UNPRESSED, "parseButtonValue must set value to BUTTON_UNPRESSED for 'unpressed'");
}

/**
 * @test parseButtonValue must parse "short_pressed" and set value to BUTTON_SHORT_PRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_short_pressed_string)
{
  ButtonState_t value = BUTTON_UNPRESSED;
  int result = parseButtonValue("short_pressed", &value);

  zassert_equal(result, 0, "parseButtonValue must return 0 for 'short_pressed'");
  zassert_equal(value, BUTTON_SHORT_PRESSED, "parseButtonValue must set value to BUTTON_SHORT_PRESSED for 'short_pressed'");
}

/**
 * @test parseButtonValue must parse "long_pressed" and set value to BUTTON_LONG_PRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_long_pressed_string)
{
  ButtonState_t value = BUTTON_UNPRESSED;
  int result = parseButtonValue("long_pressed", &value);

  zassert_equal(result, 0, "parseButtonValue must return 0 for 'long_pressed'");
  zassert_equal(value, BUTTON_LONG_PRESSED, "parseButtonValue must set value to BUTTON_LONG_PRESSED for 'long_pressed'");
}

/**
 * @test parseButtonValue must return error for invalid string.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_invalid_string)
{
  ButtonState_t value;
  int result = parseButtonValue("invalid", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for invalid string");
}

/**
 * @test parseButtonValue must return error for empty string.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_empty_string)
{
  ButtonState_t value;
  int result = parseButtonValue("", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for empty string");
}

/**
 * @test parseButtonValue must return error for uppercase UNPRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_uppercase_unpressed)
{
  ButtonState_t value;
  int result = parseButtonValue("UNPRESSED", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for uppercase UNPRESSED");
}

/**
 * @test parseButtonValue must return error for uppercase SHORT_PRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_uppercase_short_pressed)
{
  ButtonState_t value;
  int result = parseButtonValue("SHORT_PRESSED", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for uppercase SHORT_PRESSED");
}

/**
 * @test parseButtonValue must return error for uppercase LONG_PRESSED.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_uppercase_long_pressed)
{
  ButtonState_t value;
  int result = parseButtonValue("LONG_PRESSED", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for uppercase LONG_PRESSED");
}

/**
 * @test parseButtonValue must return error for mixed case Unpressed.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_mixed_case_unpressed)
{
  ButtonState_t value;
  int result = parseButtonValue("Unpressed", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for mixed case Unpressed");
}

/**
 * @test parseButtonValue must return error for partial match.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_partial_match)
{
  ButtonState_t value;
  int result = parseButtonValue("short", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for partial match");
}

/**
 * @test parseButtonValue must return error for string with whitespace.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_with_whitespace)
{
  ButtonState_t value;
  int result = parseButtonValue(" unpressed", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for string with leading whitespace");
}

/**
 * @test parseButtonValue must return error for numeric string.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_numeric_string)
{
  ButtonState_t value;
  int result = parseButtonValue("0", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for numeric string");
}

/**
 * @test parseButtonValue must return error for string with spaces instead of underscores.
 */
ZTEST(cmdUtil_tests, test_parseButtonValue_spaces_instead_of_underscores)
{
  ButtonState_t value;
  int result = parseButtonValue("short pressed", &value);

  zassert_equal(result, -EINVAL, "parseButtonValue must return -EINVAL for string with spaces instead of underscores");
}

ZTEST_SUITE(cmdUtil_tests, NULL, cmdUtil_tests_setup, cmdUtil_tests_before,
            cmdUtil_tests_after, NULL);
