/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreCmdUtil.h
 * @author    jbacon
 * @date      2025-02-10
 * @brief     Datastore Command Utility Functions
 *
 *            Utility functions for datastore shell commands.
 *
 * @ingroup   datastore
 * @{
 */

#ifndef DATASTORE_CMD_UTIL_H
#define DATASTORE_CMD_UTIL_H

#include <zephyr/shell/shell.h>

#include "datastoreTypes.h"
#include "serviceCommon.h"

/**
 * @brief   The string for true binary values.
 */
#define TRUE_STR                                                        "true"

/**
 * @brief   The string for false binary values.
 */
#define FALSE_STR                                                       "false"

/**
 * @brief   Datapoint registry entry.
 */
typedef struct
{
  const char *name;         /**< Datapoint name */
  DatapointType_t type;     /**< Datapoint type */
  uint32_t id;              /**< Datapoint ID within its type */
} DatapointEntry_t;

/**
 * @brief   Get the datapoint registry.
 *
 * @return  Pointer to the datapoint registry.
 */
const DatapointEntry_t *getDatapointRegistry(void);

/**
 * @brief   Get the datapoint registry size.
 *
 * @return  Number of entries in the registry.
 */
size_t getDatapointRegistrySize(void);

/**
 * @brief   Convert string to uppercase.
 *
 * @param[in,out]   str: The string to convert.
 */
void toUpper(char *str);

/**
 * @brief   Get datapoint type name string.
 *
 * @param[in]   type: The datapoint type.
 *
 * @return  Type name string.
 */
const char *getTypeName(DatapointType_t type);

/**
 * @brief   Parse a boolean value from string.
 *
 * @param[in]   str: The string to parse.
 * @param[out]  value: Pointer to store the parsed boolean value.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseBool(const char *str, bool *value);

/**
 * @brief   Parse a button state value from string.
 *
 * @param[in]   str: The string to parse.
 * @param[out]  value: Pointer to store the parsed button state.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseButtonValue(const char *str, ButtonState_t *value);

/**
 * @brief   Get button state as string.
 *
 * @param[in]   value: The button state value.
 *
 * @return  Button state string.
 */
const char *getButtonValueString(ButtonState_t value);

/**
 * @brief   Print the datapoint table header.
 *
 * @param[in]   shell: The shell handle.
 */
void printTableHeader(const struct shell *shell);

/**
 * @brief   Print a binary datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The binary value.
 */
void printBinaryLine(const struct shell *shell, uint32_t id, const char *name, bool value);

/**
 * @brief   Print a button datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The button value.
 */
void printButtonLine(const struct shell *shell, uint32_t id, const char *name, ButtonState_t value);

/**
 * @brief   Print a float datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The float value.
 */
void printFloatLine(const struct shell *shell, uint32_t id, const char *name, float value);

/**
 * @brief   Print an int datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The int value.
 */
void printIntLine(const struct shell *shell, uint32_t id, const char *name, int32_t value);

/**
 * @brief   Print a multi-state datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The multi-state value.
 */
void printMultiStateLine(const struct shell *shell, uint32_t id, const char *name, uint32_t value);

/**
 * @brief   Print a uint datapoint table line.
 *
 * @param[in]   shell: The shell handle.
 * @param[in]   id: The datapoint ID.
 * @param[in]   name: The datapoint name.
 * @param[in]   value: The uint value.
 */
void printUintLine(const struct shell *shell, uint32_t id, const char *name, uint32_t value);

/**
 * @brief   Print binary datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printBinaryValues(const struct shell *shell, const DatapointEntry_t *entry,
                       const Data_t *values, size_t valCount);

/**
 * @brief   Print button datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printButtonValues(const struct shell *shell, const DatapointEntry_t *entry,
                       const Data_t *values, size_t valCount);

/**
 * @brief   Print float datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printFloatValues(const struct shell *shell, const DatapointEntry_t *entry,
                      const Data_t *values, size_t valCount);

/**
 * @brief   Print int datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printIntValues(const struct shell *shell, const DatapointEntry_t *entry,
                    const Data_t *values, size_t valCount);

/**
 * @brief   Print multi-state datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printMultiStateValues(const struct shell *shell, const DatapointEntry_t *entry,
                           const Data_t *values, size_t valCount);

/**
 * @brief   Print uint datapoint values.
 *
 * @param[in]   shell: The shell instance.
 * @param[in]   entry: The datapoint entry array.
 * @param[in]   values: The value storage array.
 * @param[in]   valCount: Number of values to print.
 */
void printUintValues(const struct shell *shell, const DatapointEntry_t *entry,
                     const Data_t *values, size_t valCount);

/**
 * @brief   Parse binary values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseBinaryValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Parse button values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseButtonValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Parse float values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseFloatValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Parse int values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseIntValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Parse multi-state values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseMultiStateValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Parse uint values from command arguments.
 *
 * @param[in]   valStrings: Array of value strings to parse.
 * @param[in]   valCount: Number of values to parse.
 * @param[out]  values: Destination for parsed values.
 *
 * @return  0 if successful, error code otherwise.
 */
int parseUintValues(char **valStrings, size_t valCount, Data_t *values);

/**
 * @brief   Find datapoint entry by name.
 *
 * @param[in]   name: The datapoint name to search for.
 * @param[out]  entry: Pointer to store the found entry.
 *
 * @return  0 if successful, error code otherwise.
 */
int findDatapointByName(const char *name, const DatapointEntry_t **entry);

#endif /* DATASTORE_CMD_UTIL_H */

/** @} */
