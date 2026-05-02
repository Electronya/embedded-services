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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>

#include "datastore.h"
#include "datastoreCmdUtil.h"
#include "datastoreMeta.h"
#include "datastoreTypes.h"

/**
 * @brief   Datastore command response queue.
 */
K_MSGQ_DEFINE(datastoreCmdResQueue, sizeof(int), DATASTORE_MSG_COUNT, 4);

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
  const DatapointEntry_t *registry = getDatapointRegistry();
  size_t registrySize              = getDatapointRegistrySize();

  printTableHeader(shell);

  for(size_t i = 0; i < registrySize; i++)
  {
    const DatapointEntry_t *entry = &registry[i];

    err = datastoreRead(entry->type, entry->id, 1, &datastoreCmdResQueue, &value);
    if(err < 0)
    {
      shell_print(shell, "%-3u %-40s %-15s %s", entry->id, entry->name, getTypeName(entry->type), "ERROR");
      continue;
    }

    switch(entry->type)
    {
      case DATAPOINT_BINARY:
        printBinaryLine(shell, entry->id, entry->name, ((uint8_t *)&value)[0]);
        break;

      case DATAPOINT_BUTTON:
        printButtonLine(shell, entry->id, entry->name, ((ButtonState_t *)&value)[0]);
        break;

      case DATAPOINT_FLOAT:
        printFloatLine(shell, entry->id, entry->name, ((float *)&value)[0]);
        break;

      case DATAPOINT_INT:
        printIntLine(shell, entry->id, entry->name, ((int32_t *)&value)[0]);
        break;

      case DATAPOINT_MULTI_STATE:
        printMultiStateLine(shell, entry->id, entry->name, ((uint32_t *)&value)[0]);
        break;

      case DATAPOINT_UINT:
        printUintLine(shell, entry->id, entry->name, ((uint32_t *)&value)[0]);
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

  /* Find datapoint by name (argv[0] is the subcommand name) */
  toUpper(argv[0]);
  err = findDatapointByName(argv[0], &entry);
  if(err < 0)
  {
    shell_error(shell, "FAIL: datapoint '%s' not found", argv[0]);
    return -ESRCH;
  }

  /* Parse value count (default to 1 if not provided) */
  valCount = (argc >= 2) ? shell_strtoul(argv[1], 10, &err) : 1;
  if(err < 0)
  {
    shell_error(shell, "FAIL %d: invalid value count '%s'", err, argv[1]);
    return err;
  }

  /* Read values based on datapoint type */
  err = datastoreRead(entry->type, entry->id, valCount, &datastoreCmdResQueue, valueStorage);
  if(err < 0)
  {
    shell_error(shell, "FAIL %d: read operation failed", err);
    return err;
  }

  shell_info(shell, "SUCCESS: read %zu value(s) from %s", valCount, entry->name);

  /* Print values in format: DATAPOINT_NAME = value */
  switch(entry->type)
  {
    case DATAPOINT_BINARY:
      printBinaryValues(shell, entry, valueStorage, valCount);
      break;

    case DATAPOINT_BUTTON:
      printButtonValues(shell, entry, valueStorage, valCount);
      break;

    case DATAPOINT_FLOAT:
      printFloatValues(shell, entry, valueStorage, valCount);
      break;

    case DATAPOINT_INT:
      printIntValues(shell, entry, valueStorage, valCount);
      break;

    case DATAPOINT_MULTI_STATE:
      printMultiStateValues(shell, entry, valueStorage, valCount);
      break;

    case DATAPOINT_UINT:
      printUintValues(shell, entry, valueStorage, valCount);
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
  if(err < 0)
  {
    shell_error(shell, "FAIL: datapoint '%s' not found", argv[0]);
    return -ESRCH;
  }

  /* Calculate value count */
  valCount = argc - 1;
  if(valCount == 0)
  {
    shell_error(shell, "FAIL: no values provided");
    return -EINVAL;
  }

  /* Parse and write values based on datapoint type */
  switch(entry->type)
  {
    case DATAPOINT_BINARY:
      err = parseBinaryValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid binary value");
        return err;
      }
      break;

    case DATAPOINT_BUTTON:
      err = parseButtonValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid button value");
        return err;
      }
      break;

    case DATAPOINT_FLOAT:
      err = parseFloatValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid float value");
        return err;
      }
      break;

    case DATAPOINT_INT:
      err = parseIntValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid int value");
        return err;
      }
      break;

    case DATAPOINT_MULTI_STATE:
      err = parseMultiStateValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid multi-state value");
        return err;
      }
      break;

    case DATAPOINT_UINT:
      err = parseUintValues(argv + 1, valCount, valueStorage);
      if(err < 0)
      {
        shell_error(shell, "FAIL: invalid uint value");
        return err;
      }
      break;

    default:
      shell_error(shell, "FAIL: unsupported datapoint type");
      return -ENOTSUP;
  }

  /* Write to datastore */
  err = datastoreWrite(entry->type, entry->id, valueStorage, valCount, &datastoreCmdResQueue);
  if(err < 0)
  {
    shell_error(shell, "FAIL %d: write operation failed", err);
    return err;
  }

  shell_info(shell, "SUCCESS: wrote %zu value(s) to %s", valCount, entry->name);
  return 0;
}

/* Generate read subcommands using X-macros */
#define X(name, flags, defaultVal) SHELL_CMD_ARG(name, NULL, "Read " STRINGIFY(name) " [count]", execRead, 1, 1),

SHELL_STATIC_SUBCMD_SET_CREATE(read_sub, DATASTORE_BINARY_DATAPOINTS DATASTORE_BUTTON_DATAPOINTS DATASTORE_FLOAT_DATAPOINTS
                                           DATASTORE_INT_DATAPOINTS DATASTORE_MULTI_STATE_DATAPOINTS DATASTORE_UINT_DATAPOINTS
                                             SHELL_SUBCMD_SET_END);

#undef X

/* Generate write subcommands using X-macros */
#define X(name, flags, defaultVal)                                                                                                 \
  SHELL_CMD_ARG(name, NULL, "Write " STRINGIFY(name) " <value> [value...]", execWrite, 2, SHELL_OPT_ARG_CHECK_SKIP),

SHELL_STATIC_SUBCMD_SET_CREATE(write_sub, DATASTORE_BINARY_DATAPOINTS DATASTORE_BUTTON_DATAPOINTS DATASTORE_FLOAT_DATAPOINTS
                                            DATASTORE_INT_DATAPOINTS DATASTORE_MULTI_STATE_DATAPOINTS DATASTORE_UINT_DATAPOINTS
                                              SHELL_SUBCMD_SET_END);

#undef X

/* Main datastore command */
SHELL_STATIC_SUBCMD_SET_CREATE(datastore_sub, SHELL_CMD(ls, NULL, "List all datapoints", execList),
                               SHELL_CMD(read, &read_sub, "Read datapoint value(s)", NULL),
                               SHELL_CMD(write, &write_sub, "Write datapoint value(s)", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ds, &datastore_sub, "Datastore commands.", NULL);

/** @} */
