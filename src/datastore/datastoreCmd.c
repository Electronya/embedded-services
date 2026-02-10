/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreCmd.c
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Service Command
 *
 *            Datastore service command set with X-macro generated subcommands.
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
#include "datastoreTypes.h"

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
 * @brief   Datapoint registry entry.
 */
typedef struct
{
  const char *name;         /**< Datapoint name */
  DatapointType_t type;     /**< Datapoint type */
  uint32_t id;              /**< Datapoint ID within its type */
} DatapointEntry_t;

/* Build unified datapoint registry using X-macros */
static const DatapointEntry_t datapointRegistry[] = {
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

#define DATAPOINT_REGISTRY_SIZE ARRAY_SIZE(datapointRegistry)

/**
 * @brief   Datastore command response queue.
 */
K_MSGQ_DEFINE(datastoreCmdResQueue, sizeof(int), DATASTORE_MSG_COUNT, 4);

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
 * @brief   Get type name string.
 *
 * @param[in]   type: The datapoint type.
 *
 * @return  Type name string.
 */
static const char *getTypeName(DatapointType_t type)
{
  switch (type)
  {
    case DATAPOINT_BINARY: return "binary";
    case DATAPOINT_BUTTON: return "button";
    case DATAPOINT_FLOAT: return "float";
    case DATAPOINT_INT: return "int";
    case DATAPOINT_MULTI_STATE: return "multi_state";
    case DATAPOINT_UINT: return "uint";
    default: return "unknown";
  }
}

/**
 * @brief   Parse a boolean value from string.
 *
 * @param[in]   str: The string to parse.
 * @param[out]  value: Pointer to store the parsed boolean value.
 *
 * @return  0 if successful, error code otherwise.
 */
static int parseBool(const char *str, bool *value)
{
  if (strcmp(str, TRUE_STR) == 0 || strcmp(str, "1") == 0)
  {
    *value = true;
    return 0;
  }
  else if (strcmp(str, FALSE_STR) == 0 || strcmp(str, "0") == 0)
  {
    *value = false;
    return 0;
  }
  return -EINVAL;
}

/**
 * @brief   Parse a button value from string.
 *
 * @param[in]   str: The string to parse.
 * @param[out]  value: Pointer to store the parsed button value.
 *
 * @return  0 if successful, error code otherwise.
 */
static int parseButtonValue(const char *str, ButtonState_t *value)
{
  if (strcmp(str, UNPRESSED_STR) == 0)
  {
    *value = BUTTON_UNPRESSED;
    return 0;
  }
  else if (strcmp(str, SHORT_PRESSED_STR) == 0)
  {
    *value = BUTTON_SHORT_PRESSED;
    return 0;
  }
  else if (strcmp(str, LONG_PRESSED_STR) == 0)
  {
    *value = BUTTON_LONG_PRESSED;
    return 0;
  }
  return -EINVAL;
}

/**
 * @brief   Get button value as string.
 *
 * @param[in]   value: The button value.
 *
 * @return  String representation of the button value.
 */
static const char *getButtonValueString(ButtonState_t value)
{
  switch (value)
  {
    case BUTTON_UNPRESSED:
      return UNPRESSED_STR;
    case BUTTON_SHORT_PRESSED:
      return SHORT_PRESSED_STR;
    case BUTTON_LONG_PRESSED:
      return LONG_PRESSED_STR;
    default:
      return "unknown";
  }
}

/**
 * @brief   Parse a float value from string.
 *
 * @param[in]   str: The string to parse.
 * @param[out]  value: Pointer to store the parsed float value.
 *
 * @return  0 if successful, error code otherwise.
 */
static int parseFloat(const char *str, float *value)
{
  char *endptr;
  float result;

  result = strtof(str, &endptr);

  if (endptr == str || *endptr != '\0')
  {
    return -EINVAL;
  }

  *value = result;
  return 0;
}

/**
 * @brief   Get float value as string.
 *
 * @param[in]   value: The float value.
 * @param[out]  buffer: Buffer to store the string representation.
 * @param[in]   bufferSize: Size of the buffer.
 */
static void getFloatValueString(float value, char *buffer, size_t bufferSize)
{
  snprintf(buffer, bufferSize, "%.6f", (double)value);
}

/**
 * @brief   Print the datapoint table header.
 *
 * @param[in]   shell: The shell handle.
 */
static void printTableHeader(const struct shell *shell)
{
  shell_print(shell, "%-3s %-40s %-15s %s", "ID", "Name", "Type", "Value");
  shell_print(shell, "%-3s %-40s %-15s %s", "---", "----------------------------------------", "---------------", "--------------------");
}

/**
 * @brief   Print a button datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The button value.
 */
static void printButtonLine(const struct shell *shell, uint32_t id, const char *name, ButtonState_t value)
{
  shell_print(shell, "%-3u %-40s %-15s %s", id, name, "button", getButtonValueString(value));
}

/**
 * @brief   Print a float datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The float value.
 */
static void printFloatLine(const struct shell *shell, uint32_t id, const char *name, float value)
{
  char buffer[VALUE_STRING_MAX_LENGTH];

  getFloatValueString(value, buffer, sizeof(buffer));
  shell_print(shell, "%-3u %-40s %-15s %s", id, name, "float", buffer);
}

/**
 * @brief   Find datapoint entry by name.
 *
 * @param[in]   name: The datapoint name to find.
 * @param[out]  entry: Pointer to store the found entry.
 *
 * @return  0 if found, -ENOENT otherwise.
 */
static int findDatapointByName(const char *name, const DatapointEntry_t **entry)
{
  for (size_t i = 0; i < DATAPOINT_REGISTRY_SIZE; i++)
  {
    if (strcmp(datapointRegistry[i].name, name) == 0)
    {
      *entry = &datapointRegistry[i];
      return 0;
    }
  }
  return -ENOENT;
}

/**
 * @brief   Execute the list command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector.
 *
 * @return  0 if successful, error code otherwise.
 */
static int execList(const struct shell *shell, size_t argc, char **argv)
{
  int err;
  Data_t value;

  printTableHeader(shell);

  for (size_t i = 0; i < DATAPOINT_REGISTRY_SIZE; i++)
  {
    const DatapointEntry_t *entry = &datapointRegistry[i];

    err = datastoreRead(entry->type, entry->id, &value, 1, &datastoreCmdResQueue);
    if (err < 0)
    {
      shell_print(shell, "%-3u %-40s %-15s %s", entry->id, entry->name, getTypeName(entry->type), "ERROR");
      continue;
    }

    switch (entry->type)
    {
      case DATAPOINT_BINARY:
        shell_print(shell, "%-3u %-40s %-15s %s", entry->id, entry->name, "binary",
                    ((uint8_t *)&value)[0] ? TRUE_STR : FALSE_STR);
        break;

      case DATAPOINT_BUTTON:
        printButtonLine(shell, entry->id, entry->name, ((ButtonState_t *)&value)[0]);
        break;

      case DATAPOINT_FLOAT:
        printFloatLine(shell, entry->id, entry->name, ((float *)&value)[0]);
        break;

      case DATAPOINT_INT:
        shell_print(shell, "%-3u %-40s %-15s %d", entry->id, entry->name, "int",
                    ((int32_t *)&value)[0]);
        break;

      case DATAPOINT_MULTI_STATE:
        shell_print(shell, "%-3u %-40s %-15s %u", entry->id, entry->name, "multi_state",
                    ((uint8_t *)&value)[0]);
        break;

      case DATAPOINT_UINT:
        shell_print(shell, "%-3u %-40s %-15s %u", entry->id, entry->name, "uint",
                    ((uint32_t *)&value)[0]);
        break;

      default:
        shell_print(shell, "%-3u %-40s %-15s %s", entry->id, entry->name, getTypeName(entry->type), "UNKNOWN TYPE");
        break;
    }
  }

  return 0;
}

/**
 * @brief   Execute the read command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector (argv[0] is datapoint name, argv[1] is optional count).
 *
 * @return  0 if successful, error code otherwise.
 */
static int execRead(const struct shell *shell, size_t argc, char **argv)
{
  const DatapointEntry_t *entry;
  int err;
  size_t valCount;
  Data_t valueStorage[CONFIG_ENYA_DATASTORE_BUFFER_SIZE];
  char buffer[VALUE_STRING_MAX_LENGTH];

  /* Find datapoint by name (argv[0] is the subcommand name) */
  toUpper(argv[0]);
  err = findDatapointByName(argv[0], &entry);
  if (err < 0)
  {
    shell_error(shell, "FAIL: datapoint '%s' not found", argv[0]);
    return -ESRCH;
  }

  /* Parse value count (default to 1 if not provided) */
  valCount = (argc >= 2) ? shell_strtoul(argv[1], 10, &err) : 1;
  if (err < 0)
  {
    shell_error(shell, "FAIL %d: invalid value count '%s'", err, argv[1]);
    return err;
  }

  /* Read values based on datapoint type */
  err = datastoreRead(entry->type, entry->id, valCount, &datastoreCmdResQueue, valueStorage);
  if (err < 0)
  {
    shell_error(shell, "FAIL %d: read operation failed", err);
    return err;
  }

  /* Print values in format: DATAPOINT_NAME = value */
  switch (entry->type)
  {
    case DATAPOINT_BINARY:
      for (size_t i = 0; i < valCount; i++)
      {
        shell_print(shell, "%s = %s", entry->name, ((uint8_t *)valueStorage)[i] ? TRUE_STR : FALSE_STR);
      }
      break;

    case DATAPOINT_BUTTON:
      for (size_t i = 0; i < valCount; i++)
      {
        shell_print(shell, "%s = %s", entry->name, getButtonValueString(((ButtonState_t *)valueStorage)[i]));
      }
      break;

    case DATAPOINT_FLOAT:
      for (size_t i = 0; i < valCount; i++)
      {
        getFloatValueString(((float *)valueStorage)[i], buffer, sizeof(buffer));
        shell_print(shell, "%s = %s", entry->name, buffer);
      }
      break;

    case DATAPOINT_INT:
      for (size_t i = 0; i < valCount; i++)
      {
        shell_print(shell, "%s = %d", entry->name, ((int32_t *)valueStorage)[i]);
      }
      break;

    case DATAPOINT_MULTI_STATE:
      for (size_t i = 0; i < valCount; i++)
      {
        shell_print(shell, "%s = %u", entry->name, ((uint8_t *)valueStorage)[i]);
      }
      break;

    case DATAPOINT_UINT:
      for (size_t i = 0; i < valCount; i++)
      {
        shell_print(shell, "%s = %u", entry->name, ((uint32_t *)valueStorage)[i]);
      }
      break;

    default:
      shell_error(shell, "FAIL: unsupported datapoint type");
      return -ENOTSUP;
  }

  return 0;
}

/**
 * @brief   Execute the write command.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   argc: Argument count.
 * @param[in]   argv: Argument vector (argv[0] is datapoint name, argv[1+] are values).
 *
 * @return  0 if successful, error code otherwise.
 */
static int execWrite(const struct shell *shell, size_t argc, char **argv)
{
  const DatapointEntry_t *entry;
  int err;
  size_t valCount;
  Data_t valueStorage[CONFIG_ENYA_DATASTORE_BUFFER_SIZE];

  /* Find datapoint by name */
  toUpper(argv[0]);
  err = findDatapointByName(argv[0], &entry);
  if (err < 0)
  {
    shell_error(shell, "FAIL: datapoint '%s' not found", argv[0]);
    return -ESRCH;
  }

  /* Calculate value count */
  valCount = argc - 1;
  if (valCount == 0)
  {
    shell_error(shell, "FAIL: no values provided");
    return -EINVAL;
  }

  /* Parse and write values based on datapoint type */
  switch (entry->type)
  {
    case DATAPOINT_BINARY:
    {
      uint8_t *values = (uint8_t *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        bool boolVal;
        err = parseBool(argv[i + 1], &boolVal);
        if (err < 0)
        {
          shell_error(shell, "FAIL: invalid binary value '%s'", argv[i + 1]);
          return err;
        }
        values[i] = boolVal ? 1 : 0;
      }
      break;
    }

    case DATAPOINT_BUTTON:
    {
      ButtonState_t *values = (ButtonState_t *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        err = parseButtonValue(argv[i + 1], &values[i]);
        if (err < 0)
        {
          shell_error(shell, "FAIL: invalid button value '%s'", argv[i + 1]);
          return err;
        }
      }
      break;
    }

    case DATAPOINT_FLOAT:
    {
      float *values = (float *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        err = parseFloat(argv[i + 1], &values[i]);
        if (err < 0)
        {
          shell_error(shell, "FAIL: invalid float value '%s'", argv[i + 1]);
          return err;
        }
      }
      break;
    }

    case DATAPOINT_INT:
    {
      int32_t *values = (int32_t *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        values[i] = shell_strtol(argv[i + 1], 10, &err);
        if (err < 0)
        {
          shell_error(shell, "FAIL %d: invalid int value '%s'", err, argv[i + 1]);
          return err;
        }
      }
      break;
    }

    case DATAPOINT_MULTI_STATE:
    {
      uint8_t *values = (uint8_t *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        uint32_t val = shell_strtoul(argv[i + 1], 10, &err);
        if (err < 0 || val > 255)
        {
          shell_error(shell, "FAIL: invalid multi-state value '%s'", argv[i + 1]);
          return err < 0 ? err : -EINVAL;
        }
        values[i] = (uint8_t)val;
      }
      break;
    }

    case DATAPOINT_UINT:
    {
      uint32_t *values = (uint32_t *)valueStorage;
      for (size_t i = 0; i < valCount; i++)
      {
        values[i] = shell_strtoul(argv[i + 1], 10, &err);
        if (err < 0)
        {
          shell_error(shell, "FAIL %d: invalid uint value '%s'", err, argv[i + 1]);
          return err;
        }
      }
      break;
    }

    default:
      shell_error(shell, "FAIL: unsupported datapoint type");
      return -ENOTSUP;
  }

  /* Write to datastore */
  err = datastoreWrite(entry->type, entry->id, valueStorage, valCount, &datastoreCmdResQueue);
  if (err < 0)
  {
    shell_error(shell, "FAIL %d: write operation failed", err);
    return err;
  }

  shell_print(shell, "SUCCESS: wrote %zu value(s) to %s", valCount, entry->name);
  return 0;
}

/* Generate read subcommands using X-macros */
#define X(name, flags, defaultVal) \
  SHELL_CMD_ARG(STRINGIFY(name), NULL, "Read " STRINGIFY(name) " [count]", execRead, 1, 1),

SHELL_STATIC_SUBCMD_SET_CREATE(read_sub,
  DATASTORE_BINARY_DATAPOINTS
  DATASTORE_BUTTON_DATAPOINTS
  DATASTORE_FLOAT_DATAPOINTS
  DATASTORE_INT_DATAPOINTS
  DATASTORE_MULTI_STATE_DATAPOINTS
  DATASTORE_UINT_DATAPOINTS
  SHELL_SUBCMD_SET_END);

#undef X

/* Generate write subcommands using X-macros */
#define X(name, flags, defaultVal) \
  SHELL_CMD_ARG(STRINGIFY(name), NULL, "Write " STRINGIFY(name) " <value> [value...]", execWrite, 2, SHELL_OPT_ARG_CHECK_SKIP),

SHELL_STATIC_SUBCMD_SET_CREATE(write_sub,
  DATASTORE_BINARY_DATAPOINTS
  DATASTORE_BUTTON_DATAPOINTS
  DATASTORE_FLOAT_DATAPOINTS
  DATASTORE_INT_DATAPOINTS
  DATASTORE_MULTI_STATE_DATAPOINTS
  DATASTORE_UINT_DATAPOINTS
  SHELL_SUBCMD_SET_END);

#undef X

/* Main datastore command */
SHELL_STATIC_SUBCMD_SET_CREATE(datastore_sub,
  SHELL_CMD(ls, NULL, "List all datapoints", execList),
  SHELL_CMD(read, &read_sub, "Read datapoint value(s)", NULL),
  SHELL_CMD(write, &write_sub, "Write datapoint value(s)", NULL),
  SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(datastore, &datastore_sub, "Datastore commands.", NULL);

/** @} */
