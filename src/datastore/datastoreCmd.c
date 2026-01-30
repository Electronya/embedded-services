/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreCmd.c
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Service Command
 *
 *            Datastore service command set.
 *
 * @ingroup   datastore
 * @{
 */

#include <zephyr/shell/shell.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datastore.h"
#include "datastoreMeta.h"

/**
 * @brief   The string for true binary values.
 */
#define TRUE_STR                                                        "true"

/**
 * @brief   The string for false binary values.
 */
#define FALSE_STR                                                       "false"

/**
 * @brief   The string for the unpressed button value.
 */
#define UNPRESSED_STR                                                   "unpressed"

/**
 * @brief   The string for the short pressed button value.
 */
#define SHORT_PRESSED_STR                                               "short_pressed"

/**
 * @brief   The string for the long pressed button value.
 */
#define LONG_PRESSED_STR                                                "long_pressed"

/**
 * @brief   The max length of a value string.
 */
#define VALUE_STRING_MAX_LENGTH                                         20

/**
 * @brief   The datapoint name argument index.
 */
#define DATAPOINT_NAME_ARG_IDX                                          1

/**
 * @brief   The value count argument index.
 */
#define VALUE_COUNT_ARG_IDX                                             2

/**
 * @brief   The maximum argument count for a read.
 */
#define MAX_READ_ARG_COUNT                                              3

/**
 * @brief   The write first value index.
 */
#define WRITE_VALUE_FIRST_IDX                                           3

/**
 * @brief   The list of binary datapoint names.
 */
static char *binaryNames[BINARY_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_BINARY_DATAPOINTS
#undef X
};

/**
 * @brief   The list of button datapoint names.
 */
static char *buttonNames[BUTTON_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_BUTTON_DATAPOINTS
#undef X
};

/**
 * @brief   The list of float datapoint names.
 */
static char *floatNames[FLOAT_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_FLOAT_DATAPOINTS
#undef X
};

/**
 * @brief   The list of signed integer datapoint names.
 */
static char *intNames[INT_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_INT_DATAPOINTS
#undef X
};

/**
 * @brief   The list of multi-state datapoint names.
 */
static char *multiStateNames[MULTI_STATE_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_MULTI_STATE_DATAPOINTS
#undef X
};

/**
 * @brief   The list of unsigned integer datapoint names.
 */
static char *uintNames[UINT_DATAPOINT_COUNT] = {
#define X(name, flags, defaultVal) STRINGIFY(name),
  DATASTORE_UINT_DATAPOINTS
#undef X
};

/**
 * @brief   Datastore command response queue.
 */
K_MSGQ_DEFINE(datastoreCmdResQueue, sizeof(int), DATASTORE_MSG_COUNT, 4);

/**
 * @brief   Get the index of the string.
 *
 * @param[in]   str: The string to look for.
 * @param[in]   strList: The list of string to look in.
 * @param[in]   listSize: The string list size.
 * @param[out]  index: The found index.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int getStringIndex(char *str, char **strList, size_t listSize, uint32_t *index)
{
  char *listedStr;
  bool indexFound = false;

  *index = 0;

  while(!indexFound && *index < listSize)
  {
    listedStr = strList[*index];

    if(strcmp(str, listedStr) == 0)
      indexFound = true;
    else
      ++(*index);
  }

  if(indexFound)
    return 0;

  return -ESRCH;
}

/**
 * @brief   Convert a string to upper case.
 *
 * @param[in,out] str: The string to convert.
 */
static void toUpper(char *str)
{
  size_t strLength = strlen(str);

  for(size_t i = 0; i < strLength; ++i)
    str[i] = toupper(str[i]);
}

/**
 * @brief   Execute the list binary datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListBinary(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of binary datapoint:");

  for(size_t i = 0; i < BINARY_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", binaryNames[i]);

  return 0;
}

/**
 * @brief   Execute the read binary datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadBinary(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  bool values[BINARY_DATAPOINT_COUNT];
  char *valStr;

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], binaryNames, BINARY_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == MAX_READ_ARG_COUNT ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadBinary(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    valStr = values[i] ? TRUE_STR : FALSE_STR;
    shell_info(shell, "%s: %s", binaryNames[datapointId + i], valStr);
  }

  return 0;
}

/**
 * @brief   Execute the write binary datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteBinary(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  bool values[BINARY_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], binaryNames, BINARY_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = shell_strtobool(argv[WRITE_VALUE_FIRST_IDX + i], 10, &err);
    if(err < 0)
    {
      shell_error(shell, "FAIL: bad binary value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return err;
    }
  }

  err = datastoreWriteBinary(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", binaryNames[datapointId],
             binaryNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_binary,
  SHELL_CMD(ls, NULL, "List binary objects.\n\tUsage datastore binary_data ls", execListBinary),
  SHELL_CMD_ARG(read, NULL, "Read a binary datapoint.\n\tUsage datastore binary_data read <datapoint ID> [value count]",
                execReadBinary, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a binary datapoint.\n\tUsage datastore binary_data write <datapoint ID> <value count> <true|false> [true|false] ...",
                execWriteBinary, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

/**
 * @brief   Convert a string to button state.
 *
 * @param str
 * @param value
 * @return int
 */
int convertButtonStateStr(char *str, ButtonState_t *value)
{
  if(strcmp(str, UNPRESSED_STR) == 0)
  {
    *value = BUTTON_UNPRESSED;
    return 0;
  }

  if(strcmp(str, SHORT_PRESSED_STR) == 0)
  {
    *value = BUTTON_SHORT_PRESSED;
    return 0;
  }

  if(strcmp(str, LONG_PRESSED_STR) == 0)
  {
    *value = BUTTON_LONG_PRESSED;
    return 0;
  }

  return -EINVAL;
}

/**
 * @brief   Execute the list button datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListButton(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of button datapoint:");

  for(size_t i = 0; i < BUTTON_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", buttonNames[i]);

  return 0;
}

/**
 * @brief   Execute the read button datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadButton(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  ButtonState_t values[BUTTON_DATAPOINT_COUNT];
  char *valStr;

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], buttonNames, BUTTON_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == 3 ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadButton(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    valStr = values[i] == BUTTON_SHORT_PRESSED ? SHORT_PRESSED_STR : UNPRESSED_STR;
    valStr = values[i] == BUTTON_LONG_PRESSED ? LONG_PRESSED_STR : valStr;
    shell_info(shell, "%s: %s", buttonNames[datapointId + i], valStr);
  }

  return 0;
}

/**
 * @brief   Execute the write button datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteButton(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  ButtonState_t values[BUTTON_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], buttonNames, BUTTON_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    err = convertButtonStateStr(argv[WRITE_VALUE_FIRST_IDX + i], values + i);
    if(err < 0)
    {
      shell_error(shell, "FAIL: bad button value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return err;
    }
  }

  err = datastoreWriteButton(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", buttonNames[datapointId],
             buttonNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_button,
  SHELL_CMD(ls, NULL, "List button objects.\n\tUsage datastore button_data ls", execListButton),
  SHELL_CMD_ARG(read, NULL, "Read a button datapoint.\n\tUsage datastore button_data read <datapoint ID> [value count]",
                execReadButton, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a button datapoint.\n\tUsage datastore button_data write <datapoint ID> <value count> <unpressed|short_pressed|long_pressed> [unpressed|short_pressed|long_pressed] ...",
                execWriteButton, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

/**
 * @brief   Execute the list float datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListFloat(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of float datapoint:");

  for(size_t i = 0; i < FLOAT_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", floatNames[i]);

  return 0;
}

/**
 * @brief   Execute the read float datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadFloat(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  float values[FLOAT_DATAPOINT_COUNT];
  char valStr[VALUE_STRING_MAX_LENGTH];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], floatNames, FLOAT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == 3 ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadFloat(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    sprintf(valStr, "%f", (double)values[i]);
    shell_info(shell, "%s: %s", floatNames[datapointId + i], valStr);
  }

  return 0;
}

/**
 * @brief   Execute the write float datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteFloat(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  char *endPtr;
  float values[FLOAT_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], floatNames, FLOAT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    endPtr = NULL;
    values[i] = strtof(argv[WRITE_VALUE_FIRST_IDX + i], &endPtr);
    if(endPtr < argv[WRITE_VALUE_FIRST_IDX + i] + strlen(argv[WRITE_VALUE_FIRST_IDX + i]))
    {
      shell_error(shell, "FAIL: bad float value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return -EINVAL;
    }
  }

  err = datastoreWriteFloat(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", floatNames[datapointId],
             floatNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_float,
  SHELL_CMD(ls, NULL, "List float objects.\n\tUsage datastore float_data ls", execListFloat),
  SHELL_CMD_ARG(read, NULL, "Read a float datapoint.\n\tUsage datastore float_data read <datapoint ID> [value count]",
                execReadFloat, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a float datapoint.\n\tUsage datastore float_data write <datapoint ID> <value count> <float value> [float value] ...",
                execWriteFloat, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

/**
 * @brief   Execute the list signed integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListInt(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of int datapoint:");

  for(size_t i = 0; i < INT_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", intNames[i]);

  return 0;
}

/**
 * @brief   Execute the read signed integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadInt(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[INT_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], intNames, INT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == 3 ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadInt(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    shell_info(shell, "%s: %d", intNames[datapointId + i], values[i]);
  }

  return 0;
}

/**
 * @brief   Execute the write signed integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteInt(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[INT_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], intNames, INT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = shell_strtol(argv[WRITE_VALUE_FIRST_IDX + i], 10, &err);
    if(err < 0)
    {
      shell_error(shell, "FAIL: bad signed integer value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return err;
    }
  }

  err = datastoreWriteInt(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", intNames[datapointId],
             intNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_int,
  SHELL_CMD(ls, NULL, "List signed integer objects.\n\tUsage datastore int_data ls", execListInt),
  SHELL_CMD_ARG(read, NULL, "Read a signed integer datapoint.\n\tUsage datastore int_data read <datapoint ID> [value count]",
                execReadInt, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a signed integer datapoint.\n\tUsage datastore int_data write <datapoint ID> <value count> <int value> [int value] ...",
                execWriteInt, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

/**
 * @brief   Execute the list multi-state datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListMultiState(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of multi-state datapoint:");

  for(size_t i = 0; i < MULTI_STATE_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", multiStateNames[i]);

  return 0;
}

/**
 * @brief   Execute the read multi-state datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadMultiState(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[MULTI_STATE_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], multiStateNames, MULTI_STATE_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == 3 ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadMultiState(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    shell_info(shell, "%s: %d", multiStateNames[datapointId + i], values[i]);
  }

  return 0;
}

/**
 * @brief   Execute the write multi-state datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteMultiState(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[MULTI_STATE_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], multiStateNames, MULTI_STATE_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = shell_strtol(argv[WRITE_VALUE_FIRST_IDX + i], 10, &err);
    if(err < 0)
    {
      shell_error(shell, "FAIL: bad multi-state value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return err;
    }
  }

  err = datastoreWriteMultiState(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", multiStateNames[datapointId],
             multiStateNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_multi_state,
  SHELL_CMD(ls, NULL, "List multi-state objects.\n\tUsage datastore multi_state_data ls", execListMultiState),
  SHELL_CMD_ARG(read, NULL, "Read a multi-state datapoint.\n\tUsage datastore multi_state_data read <datapoint ID> [value count]",
                execReadMultiState, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a multi-state datapoint.\n\tUsage datastore multi_state_data write <datapoint ID> <value count> <multi-state value> [multi-state value] ...",
                execWriteMultiState, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

/**
 * @brief   Execute the list unsigned integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execListUint(const struct shell *shell, size_t argc, char **argv)
{
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_info(shell, "List of unsigned int datapoint:");

  for(size_t i = 0; i < UINT_DATAPOINT_COUNT; ++i)
    shell_info(shell, "%s", uintNames[i]);

  return 0;
}

/**
 * @brief   Execute the read unsigned integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execReadUint(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[UINT_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], uintNames, UINT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = argc == 3 ? shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to read %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  err = datastoreReadUint(datapointId, valCount, &datastoreCmdResQueue, values);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: here are the values read");

  for(size_t i = 0; i < valCount; ++i)
  {
    shell_info(shell, "%s: %d", uintNames[datapointId + i], values[i]);
  }

  return 0;
}

/**
 * @brief   Execute the write unsigned integer datapoints command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: The count of argument.
 * @param[in]   argv: The vector of argument.
 *
 * @return  0 if successful the error code otherwise.
 */
static int execWriteUint(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  uint32_t datapointId;
  size_t valCount;
  int32_t values[UINT_DATAPOINT_COUNT];

  toUpper(argv[DATAPOINT_NAME_ARG_IDX]);

  err = getStringIndex(argv[DATAPOINT_NAME_ARG_IDX], uintNames, UINT_DATAPOINT_COUNT, &datapointId);
  if(err < 0)
  {
    shell_error(shell, "FAIL: unkown datapoint %s", argv[DATAPOINT_NAME_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  valCount = shell_strtoul(argv[VALUE_COUNT_ARG_IDX], 10, &err);
  if(err < 0)
  {
    shell_error(shell, "FAIL: invalid value count to write %s", argv[VALUE_COUNT_ARG_IDX]);
    shell_help(shell);
    return err;
  }

  if(argc - WRITE_VALUE_FIRST_IDX < valCount)
  {
    shell_error(shell, "FAIL: not enough value provided (%d) for the requested value to write (%d)",
                argc - WRITE_VALUE_FIRST_IDX, valCount);
    shell_help(shell);
    return -EINVAL;
  }

  for(size_t i = 0; i < valCount; ++i)
  {
    values[i] = shell_strtol(argv[WRITE_VALUE_FIRST_IDX + i], 10, &err);
    if(err < 0)
    {
      shell_error(shell, "FAIL: bad unsigned integer value %s for value %i", argv[WRITE_VALUE_FIRST_IDX + i], i);
      shell_help(shell);
      return err;
    }
  }

  err = datastoreWriteUint(datapointId, values, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL: read operation fail with %d error code", err);
    shell_help(shell);
    return err;
  }

  shell_info(shell, "SUCCESS: write operation of %s up to %s done", uintNames[datapointId],
             uintNames[datapointId + valCount - 1]);

  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_uint,
  SHELL_CMD(ls, NULL, "List unsigned integer objects.\n\tUsage datastore uint_data ls", execListUint),
  SHELL_CMD_ARG(read, NULL, "Read a unsigned integer datapoint.\n\tUsage datastore uint_data read <datapoint ID> [value count]",
                execReadUint, 2, 1),
  SHELL_CMD_ARG(write, NULL, "Write a unsigned integer datapoint.\n\tUsage datastore uint_data write <datapoint ID> <value count> <uint value> [uint value] ...",
                execWriteUint, 3, SHELL_OPT_ARG_CHECK_SKIP),
  SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(datastore_sub,
  SHELL_CMD(binary_data, &sub_binary, "Binaries datapoint commands.", NULL),
  SHELL_CMD(button_data, &sub_button, "Button datapoint commands.", NULL),
  SHELL_CMD(float_data, &sub_float, "Float datapoint commands.", NULL),
  SHELL_CMD(int_data, &sub_int, "Signed integer datapoint commands.", NULL),
  SHELL_CMD(multi_state_data, &sub_multi_state, "Multi-state datapoint commands.", NULL),
  SHELL_CMD(uint_data, &sub_uint, "Unsigned integer datapoint commands.", NULL),
  SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(datastore, &datastore_sub, "Datastore commands.",	NULL);

/** @} */
