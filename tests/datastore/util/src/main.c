/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-01-29
 * @brief     Datastore Util Tests
 *
 *            Unit tests for datastore utility functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Mock osMemoryPool type */
typedef void *osMemoryPoolId_t;

/* Prevent CMSIS OS2 header */
#define CMSIS_OS2_H_

/* Mock Kconfig options */
#define CONFIG_ENYA_DATASTORE 1

#define FFF_FAKES_LIST(FAKE) \
  FAKE(osMemoryPoolAlloc) \
  FAKE(osMemoryPoolFree) \
  FAKE(mock_subscription_callback)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(datastore, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Redefine LOG_ERR to avoid dereferencing invalid pointers in error messages */
#undef LOG_ERR
#define LOG_ERR(...) do {} while (0)

/* Define types from datastore.h before including source */
typedef enum
{
  BINARY_DATAPOINT,
  BUTTON_DATAPOINT,
  FLOAT_DATAPOINT,
  INT_DATAPOINT,
  MULTI_STATE_DATAPOINT,
  UINT_DATAPOINT,
  DATAPOINT_TYPE_COUNT
} DatapointType_t;

typedef union
{
  bool binaryVal;
  uint32_t buttonVal;
  float floatVal;
  int32_t intVal;
  uint32_t multiStateVal;
  uint32_t uintVal;
} Data_t;

typedef struct SrvMsgPayload SrvMsgPayload_t;

struct SrvMsgPayload {
  osMemoryPoolId_t poolId;
  size_t dataLen;
  uint8_t data[];
};

typedef int (*DatastoreSubCb_t)(SrvMsgPayload_t *payload, size_t valCount);

typedef struct
{
  uint32_t datapointId;
  size_t valCount;
  bool isPaused;
  DatastoreSubCb_t callback;
} DatastoreSubEntry_t;

/* Mock osMemoryPool functions */
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(int, osMemoryPoolFree, osMemoryPoolId_t, void *);

/* Mock subscription callback */
FAKE_VALUE_FUNC(int, mock_subscription_callback, SrvMsgPayload_t *, size_t);

/* Copy the function under test to expose it for testing */
static inline bool isBinaryDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * Test setup function.
 */
static void *util_tests_setup(void)
{
  return NULL;
}

/**
 * Test before function.
 */
static void util_tests_before(void *fixture)
{
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * @test The isBinaryDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (first datapoint in range [10, 15)) */
  result = isBinaryDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isBinaryDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return true when
 * datapointId is the last datapoint in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 should be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last datapoint in range [10, 15)) */
  result = isBinaryDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return false when
 * datapointId is one past the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (one past end of range [10, 15)) */
  result = isBinaryDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isBinaryDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isBinaryDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isBinaryDatapointInSubRange function must handle a subscription
 * to a single datapoint correctly.
 *
 * Subscription covers [42, 43), so only datapointId=42 should be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 42,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 42 (the only datapoint in range [42, 43)) */
  result = isBinaryDatapointInSubRange(42, &sub);

  zassert_true(result,
               "datapointId 42 should be included in single datapoint subscription [42, 43)");
}

/**
 * @test The isBinaryDatapointInSubRange function must return false for
 * datapointId outside a single datapoint subscription.
 *
 * Subscription covers [42, 43), so datapointId=43 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 42,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 43 (outside range [42, 43)) */
  result = isBinaryDatapointInSubRange(43, &sub);

  zassert_false(result,
                "datapointId 43 should NOT be included in single datapoint subscription [42, 43)");
}

/**
 * @test The isBinaryDatapointInSubRange function must handle datapointId 0
 * correctly when it's included in the subscription.
 *
 * Subscription covers [0, 10), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 10,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (first datapoint in range [0, 10)) */
  result = isBinaryDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 10)");
}

/**
 * @test The isBinaryDatapointInSubRange function must handle datapointId 0
 * correctly when it's NOT included in the subscription.
 *
 * Subscription covers [5, 15), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_binary_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 10,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [5, 15)) */
  result = isBinaryDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [5, 15)");
}

ZTEST_SUITE(datastore_util_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
