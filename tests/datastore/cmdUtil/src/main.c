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
  /* Reset any state if needed */
}

/**
 * Test after function.
 */
static void cmdUtil_tests_after(void *fixture)
{
  /* Cleanup if needed */
}

/**
 * @brief Test getDatapointRegistrySize returns correct size.
 */
ZTEST(cmdUtil_tests, test_getDatapointRegistrySize)
{
  size_t expectedSize = 3 + 2 + 2 + 2 + 2 + 2; /* Binary + Button + Float + Int + MultiState + Uint */
  size_t actualSize = getDatapointRegistrySize();

  zassert_equal(actualSize, expectedSize,
                "Registry size mismatch: expected %zu, got %zu",
                expectedSize, actualSize);
}

ZTEST_SUITE(cmdUtil_tests, NULL, cmdUtil_tests_setup, cmdUtil_tests_before,
            cmdUtil_tests_after, NULL);
