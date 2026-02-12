/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreCmdUtil.c
 * @author    jbacon
 * @date      2025-02-10
 * @brief     Datastore Command Utility Functions
 *
 *            Utility functions for datastore shell commands.
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
#include "datastoreCmdUtil.h"

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

const DatapointEntry_t *getDatapointRegistry(void)
{
  return datapointRegistry;
}

size_t getDatapointRegistrySize(void)
{
  return DATAPOINT_REGISTRY_SIZE;
}

void toUpper(char *str)
{
  size_t strLength = strlen(str);

  for(size_t i = 0; i < strLength; ++i)
    str[i] = toupper(str[i]);
}

const char *getTypeName(DatapointType_t type)
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

int parseBool(const char *str, bool *value)
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

int parseButtonValue(const char *str, ButtonState_t *value)
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

const char *getButtonValueString(ButtonState_t value)
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

void printTableHeader(const struct shell *shell)
{
  shell_print(shell, "%-3s %-40s %-15s %s", "ID", "Name", "Type", "Value");
  shell_print(shell, "%-3s %-40s %-15s %s", "---", "----------------------------------------", "---------------", "--------------------");
}

void printBinaryLine(const struct shell *shell, uint32_t id, const char *name, bool value)
{
  shell_print(shell, "%-3u %-40s %-15s %s", id, name, "binary", value ? TRUE_STR : FALSE_STR);
}

void printButtonLine(const struct shell *shell, uint32_t id, const char *name, ButtonState_t value)
{
  shell_print(shell, "%-3u %-40s %-15s %s", id, name, "button", getButtonValueString(value));
}

void printFloatLine(const struct shell *shell, uint32_t id, const char *name, float value)
{
  char buffer[VALUE_STRING_MAX_LENGTH];

  snprintf(buffer, sizeof(buffer), "%.6f", (double)value);
  shell_print(shell, "%-3u %-40s %-15s %s", id, name, "float", buffer);
}

void printIntLine(const struct shell *shell, uint32_t id, const char *name, int32_t value)
{
  shell_print(shell, "%-3u %-40s %-15s %d", id, name, "int", value);
}

void printMultiStateLine(const struct shell *shell, uint32_t id, const char *name, uint32_t value)
{
  shell_print(shell, "%-3u %-40s %-15s %u", id, name, "multi_state", value);
}

void printUintLine(const struct shell *shell, uint32_t id, const char *name, uint32_t value)
{
  shell_print(shell, "%-3u %-40s %-15s %u", id, name, "uint", value);
}

void printBinaryValues(const struct shell *shell, const DatapointEntry_t *entry,
                       const Data_t *values, size_t valCount)
{
  for (size_t i = 0; i < valCount; i++)
  {
    shell_print(shell, "%s = %s", entry[i].name, ((uint8_t *)values)[i] ? TRUE_STR : FALSE_STR);
  }
}

void printButtonValues(const struct shell *shell, const DatapointEntry_t *entry,
                       const Data_t *values, size_t valCount)
{
  for (size_t i = 0; i < valCount; i++)
  {
    shell_print(shell, "%s = %s", entry[i].name, getButtonValueString((ButtonState_t)values[i].uintVal));
  }
}

void printFloatValues(const struct shell *shell, const DatapointEntry_t *entry,
                      const Data_t *values, size_t valCount)
{
  char buffer[VALUE_STRING_MAX_LENGTH];

  for (size_t i = 0; i < valCount; i++)
  {
    snprintf(buffer, sizeof(buffer), "%.6f", (double)values[i].floatVal);
    shell_print(shell, "%s = %s", entry[i].name, buffer);
  }
}

void printIntValues(const struct shell *shell, const DatapointEntry_t *entry,
                    const Data_t *values, size_t valCount)
{
  for (size_t i = 0; i < valCount; i++)
  {
    shell_print(shell, "%s = %d", entry[i].name, values[i].intVal);
  }
}

void printMultiStateValues(const struct shell *shell, const DatapointEntry_t *entry,
                           const Data_t *values, size_t valCount)
{
  for (size_t i = 0; i < valCount; i++)
  {
    shell_print(shell, "%s = %u", entry[i].name, values[i].uintVal);
  }
}

void printUintValues(const struct shell *shell, const DatapointEntry_t *entry,
                     const Data_t *values, size_t valCount)
{
  for (size_t i = 0; i < valCount; i++)
  {
    shell_print(shell, "%s = %u", entry[i].name, values[i].uintVal);
  }
}

int parseBinaryValues(char **valStrings, size_t valCount, Data_t *values)
{
  int err;
  bool boolVal;

  for (size_t i = 0; i < valCount; i++)
  {
    err = parseBool(valStrings[i], &boolVal);
    if (err < 0)
      return err;

    values[i].uintVal = boolVal ? 1 : 0;
  }
  return 0;
}

int parseButtonValues(char **valStrings, size_t valCount, Data_t *values)
{
  int err;
  ButtonState_t buttonState;

  for (size_t i = 0; i < valCount; i++)
  {
    err = parseButtonValue(valStrings[i], &buttonState);
    if (err < 0)
      return err;

    values[i].uintVal = buttonState;
  }
  return 0;
}

int parseFloatValues(char **valStrings, size_t valCount, Data_t *values)
{
  char *endptr;
  float result;

  for (size_t i = 0; i < valCount; i++)
  {
    result = strtof(valStrings[i], &endptr);
    if (endptr == valStrings[i] || *endptr != '\0')
      return -EINVAL;

    values[i].floatVal = result;
  }
  return 0;
}

int parseIntValues(char **valStrings, size_t valCount, Data_t *values)
{
  int err;

  for (size_t i = 0; i < valCount; i++)
  {
    values[i].intVal = shell_strtol(valStrings[i], 10, &err);
    if (err < 0)
      return err;
  }
  return 0;
}

int parseMultiStateValues(char **valStrings, size_t valCount, Data_t *values)
{
  int err;

  for (size_t i = 0; i < valCount; i++)
  {
    values[i].uintVal = shell_strtoul(valStrings[i], 10, &err);
    if (err < 0)
      return err;
  }
  return 0;
}

int parseUintValues(char **valStrings, size_t valCount, Data_t *values)
{
  int err;

  for (size_t i = 0; i < valCount; i++)
  {
    values[i].uintVal = shell_strtoul(valStrings[i], 10, &err);
    if (err < 0)
      return err;
  }
  return 0;
}

int findDatapointByName(const char *name, const DatapointEntry_t **entry)
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

/** @} */
