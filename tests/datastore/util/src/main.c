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

/* Mock osMemoryPool functions */
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(int, osMemoryPoolFree, osMemoryPoolId_t, void *);

/* Include utility implementation - this will define SrvMsgPayload_t */
#include "datastoreUtil.c"

/* Mock subscription callback - SrvMsgPayload_t is now defined */
FAKE_VALUE_FUNC(int, mock_subscription_callback, SrvMsgPayload_t *, size_t);

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

  /* Reset binary subscriptions if entries are allocated */
  if(binarySubs.entries != NULL && binarySubs.maxCount > 0)
  {
    memset(binarySubs.entries, 0, sizeof(DatastoreSubEntry_t) * binarySubs.maxCount);
  }
  binarySubs.activeCount = 0;
  binarySubs.entries = NULL;
  binarySubs.maxCount = 0;

  /* Reset button subscriptions if entries are allocated */
  if(buttonSubs.entries != NULL && buttonSubs.maxCount > 0)
  {
    memset(buttonSubs.entries, 0, sizeof(DatastoreSubEntry_t) * buttonSubs.maxCount);
  }
  buttonSubs.activeCount = 0;
  buttonSubs.entries = NULL;
  buttonSubs.maxCount = 0;

  /* Reset float subscriptions if entries are allocated */
  if(floatSubs.entries != NULL && floatSubs.maxCount > 0)
  {
    memset(floatSubs.entries, 0, sizeof(DatastoreSubEntry_t) * floatSubs.maxCount);
  }
  floatSubs.activeCount = 0;
  floatSubs.entries = NULL;
  floatSubs.maxCount = 0;

  /* Reset int subscriptions if entries are allocated */
  if(intSubs.entries != NULL && intSubs.maxCount > 0)
  {
    memset(intSubs.entries, 0, sizeof(DatastoreSubEntry_t) * intSubs.maxCount);
  }
  intSubs.activeCount = 0;
  intSubs.entries = NULL;
  intSubs.maxCount = 0;

  /* Reset multi-state subscriptions if entries are allocated */
  if(multiStateSubs.entries != NULL && multiStateSubs.maxCount > 0)
  {
    memset(multiStateSubs.entries, 0, sizeof(DatastoreSubEntry_t) * multiStateSubs.maxCount);
  }
  multiStateSubs.activeCount = 0;
  multiStateSubs.entries = NULL;
  multiStateSubs.maxCount = 0;

  /* Reset uint subscriptions if entries are allocated */
  if(uintSubs.entries != NULL && uintSubs.maxCount > 0)
  {
    memset(uintSubs.entries, 0, sizeof(DatastoreSubEntry_t) * uintSubs.maxCount);
  }
  uintSubs.activeCount = 0;
  uintSubs.entries = NULL;
  uintSubs.maxCount = 0;
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

/**
 * @test The notifyBinarySub function must return -ENOSPC when
 * memory pool allocation fails.
 *
 * When osMemoryPoolAlloc returns NULL, the function should return -ENOSPC
 * without calling the subscription callback.
 */
ZTEST(datastore_util_tests, test_notify_binary_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyBinarySub - should fail due to allocation failure */
  result = notifyBinarySub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyBinarySub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyBinarySub function must return the error code when
 * the subscription callback fails.
 *
 * When the callback returns an error, that error should be propagated
 * back to the caller.
 */
ZTEST(datastore_util_tests, test_notify_binary_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyBinarySub - should fail due to callback error */
  result = notifyBinarySub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyBinarySub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, (SrvMsgPayload_t *)fake_buffer,
                "Callback should be called with the allocated buffer");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyBinarySub function must successfully notify the subscription
 * when all operations succeed.
 *
 * The function should allocate a buffer, populate it with binary datapoint values,
 * call the subscription callback, and return the callback's return value (0 on success).
 */
ZTEST(datastore_util_tests, test_notify_binary_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyBinarySub - should succeed */
  result = notifyBinarySub(&sub, pool);

  zassert_equal(result, 0,
                "notifyBinarySub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyBinarySubs function must return an error and stop processing
 * when notifyBinarySub fails for a subscription.
 *
 * When notifyBinarySub returns an error (e.g., allocation failure or callback error),
 * notifyBinarySubs should stop iterating and return that error.
 */
ZTEST(datastore_util_tests, test_notify_binary_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up binarySubs with two active subscriptions */
  binarySubs.entries = test_entries;
  binarySubs.maxCount = 2;
  binarySubs.activeCount = 2;

  /* First subscription covers datapointId [0, 10) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 10;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [5, 10) */
  test_entries[1].datapointId = 5;
  test_entries[1].valCount = 5;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyBinarySubs with datapointId=5 - should fail on first matching subscription */
  result = notifyBinarySubs(5, pool);

  zassert_equal(result, -ENOSPC,
                "notifyBinarySubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyBinarySubs function must successfully notify all matching,
 * non-paused subscriptions and skip paused or non-matching subscriptions.
 *
 * The function should iterate through all subscriptions and notify only those
 * that match the datapointId and are not paused.
 */
ZTEST(datastore_util_tests, test_notify_binary_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[4];
  static uint8_t fake_buffer[128];
  int result;

  /* Initialize and set up binarySubs with four subscriptions */
  binarySubs.entries = test_entries;
  binarySubs.maxCount = 4;
  binarySubs.activeCount = 4;

  /* Subscription 0: covers [0, 10), not paused - SHOULD be notified for datapointId=5 */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 10;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Subscription 1: covers [5, 10), PAUSED - should NOT be notified */
  test_entries[1].datapointId = 5;
  test_entries[1].valCount = 5;
  test_entries[1].isPaused = true;
  test_entries[1].callback = mock_subscription_callback;

  /* Subscription 2: covers [20, 30), not paused - should NOT be notified (doesn't match) */
  test_entries[2].datapointId = 20;
  test_entries[2].valCount = 10;
  test_entries[2].isPaused = false;
  test_entries[2].callback = mock_subscription_callback;

  /* Subscription 3: covers [3, 8), not paused - SHOULD be notified for datapointId=5 */
  test_entries[3].datapointId = 3;
  test_entries[3].valCount = 5;
  test_entries[3].isPaused = false;
  test_entries[3].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyBinarySubs with datapointId=5 - should notify subscriptions 0 and 3 */
  result = notifyBinarySubs(5, pool);

  zassert_equal(result, 0,
                "notifyBinarySubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 2,
                "osMemoryPoolAlloc should be called twice (for subscriptions 0 and 3)");
  zassert_equal(mock_subscription_callback_fake.call_count, 2,
                "Callback should be called twice (for subscriptions 0 and 3)");
}

/**
 * @test The isButtonDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (first datapoint in range [10, 15)) */
  result = isButtonDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isButtonDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must return true when
 * datapointId is the last datapoint in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 should be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last datapoint in range [10, 15)) */
  result = isButtonDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must return false when
 * datapointId is one past the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (one past end of range [10, 15)) */
  result = isButtonDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isButtonDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isButtonDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isButtonDatapointInSubRange function must handle a subscription
 * to a single datapoint correctly.
 *
 * Subscription covers [42, 43), so only datapointId=42 should be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 42,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 42 (the only datapoint in range [42, 43)) */
  result = isButtonDatapointInSubRange(42, &sub);

  zassert_true(result,
               "datapointId 42 should be included in single datapoint subscription [42, 43)");
}

/**
 * @test The isButtonDatapointInSubRange function must return false for
 * datapointId outside a single datapoint subscription.
 *
 * Subscription covers [42, 43), so datapointId=43 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 42,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 43 (outside range [42, 43)) */
  result = isButtonDatapointInSubRange(43, &sub);

  zassert_false(result,
                "datapointId 43 should NOT be included in single datapoint subscription [42, 43)");
}

/**
 * @test The isButtonDatapointInSubRange function must handle datapointId 0
 * correctly when it's included in the subscription.
 *
 * Subscription covers [0, 10), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 10,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (first datapoint in range [0, 10)) */
  result = isButtonDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 10)");
}

/**
 * @test The isButtonDatapointInSubRange function must handle datapointId 0
 * correctly when it's NOT included in the subscription.
 *
 * Subscription covers [5, 15), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_button_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 10,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [5, 15)) */
  result = isButtonDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [5, 15)");
}

/**
 * @test The notifyButtonSub function must return -ENOSPC when
 * memory pool allocation fails.
 *
 * When osMemoryPoolAlloc returns NULL, the function should return -ENOSPC
 * without calling the subscription callback.
 */
ZTEST(datastore_util_tests, test_notify_button_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyButtonSub - should fail due to allocation failure */
  result = notifyButtonSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyButtonSub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyButtonSub function must return the error code when
 * the subscription callback fails.
 *
 * When the callback returns an error, that error should be propagated
 * back to the caller.
 */
ZTEST(datastore_util_tests, test_notify_button_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyButtonSub - should fail due to callback error */
  result = notifyButtonSub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyButtonSub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, (SrvMsgPayload_t *)fake_buffer,
                "Callback should be called with the allocated buffer");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyButtonSub function must successfully notify the subscription
 * when all operations succeed.
 *
 * The function should allocate a buffer, populate it with button datapoint values,
 * call the subscription callback, and return the callback's return value (0 on success).
 */
ZTEST(datastore_util_tests, test_notify_button_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyButtonSub - should succeed */
  result = notifyButtonSub(&sub, pool);

  zassert_equal(result, 0,
                "notifyButtonSub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyButtonSubs function must return an error and stop processing
 * when notifyButtonSub fails for a subscription.
 *
 * When notifyButtonSub returns an error (e.g., allocation failure or callback error),
 * notifyButtonSubs should stop iterating and return that error.
 */
ZTEST(datastore_util_tests, test_notify_button_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up buttonSubs with two active subscriptions */
  buttonSubs.entries = test_entries;
  buttonSubs.maxCount = 2;
  buttonSubs.activeCount = 2;

  /* First subscription covers datapointId [0, 10) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 10;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [5, 10) */
  test_entries[1].datapointId = 5;
  test_entries[1].valCount = 5;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyButtonSubs with datapointId=5 - should fail on first matching subscription */
  result = notifyButtonSubs(5, pool);

  zassert_equal(result, -ENOSPC,
                "notifyButtonSubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyButtonSubs function must successfully notify all matching
 * subscriptions when allocations succeed.
 */
ZTEST(datastore_util_tests, test_notify_button_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x12345678;
  DatastoreSubEntry_t test_entries[3];
  ButtonState_t test_buffer = BUTTON_SHORT_PRESSED;
  int result;

  /* Set up buttonSubs with 3 entries */
  buttonSubs.maxCount = 3;
  buttonSubs.activeCount = 3;
  buttonSubs.entries = test_entries;

  /* Entry 0: Covers [0, 5), not paused */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 5;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Entry 1: Covers [5, 10), not paused - should match datapointId=5 */
  test_entries[1].datapointId = 5;
  test_entries[1].valCount = 5;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Entry 2: Covers [10, 15), paused - should match but be skipped */
  test_entries[2].datapointId = 10;
  test_entries[2].valCount = 5;
  test_entries[2].isPaused = true;
  test_entries[2].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = &test_buffer;

  /* Call notifyButtonSubs with datapointId=5 - should match entry 1 only */
  result = notifyButtonSubs(5, pool);

  zassert_equal(result, 0,
                "notifyButtonSubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for matching subscription");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for the matching subscription");
}

/**
 * @test The isFloatDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (start of range [10, 15)) */
  result = isFloatDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isFloatDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must return true when
 * datapointId equals the last valid datapoint ID in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 (last included) should be true.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last in range [10, 15)) */
  result = isFloatDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must return false when
 * datapointId equals the subscription ending boundary (exclusive).
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (end boundary of range [10, 15), exclusive) */
  result = isFloatDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isFloatDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isFloatDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isFloatDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId matches.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=5 should be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 5 (single datapoint range [5, 6)) */
  result = isFloatDatapointInSubRange(5, &sub);

  zassert_true(result,
               "datapointId 5 should be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isFloatDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId does not match.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=6 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 6 (outside single datapoint range [5, 6)) */
  result = isFloatDatapointInSubRange(6, &sub);

  zassert_false(result,
                "datapointId 6 should NOT be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isFloatDatapointInSubRange function must correctly handle
 * datapointId 0 when it is included in the subscription range.
 *
 * Subscription covers datapoints [0, 3), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 3,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (start of range [0, 3)) */
  result = isFloatDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 3)");
}

/**
 * @test The isFloatDatapointInSubRange function must correctly handle
 * datapointId 0 when it is not included in the subscription range.
 *
 * Subscription covers datapoints [1, 3), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_float_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 1,
    .valCount = 2,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [1, 3)) */
  result = isFloatDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [1, 3)");
}

/**
 * @test The notifyFloatSub function must return -ENOSPC when memory pool
 * allocation fails.
 */
ZTEST(datastore_util_tests, test_notify_float_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyFloatSub - should fail with allocation error */
  result = notifyFloatSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyFloatSub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyFloatSub function must return the callback error code
 * when the callback function fails.
 */
ZTEST(datastore_util_tests, test_notify_float_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyFloatSub - should fail due to callback error */
  result = notifyFloatSub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyFloatSub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
}

/**
 * @test The notifyFloatSub function must successfully notify when allocation
 * and callback succeed.
 */
ZTEST(datastore_util_tests, test_notify_float_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyFloatSub - should succeed */
  result = notifyFloatSub(&sub, pool);

  zassert_equal(result, 0,
                "notifyFloatSub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyFloatSubs function must return an error and stop processing
 * when notifyFloatSub fails for a subscription.
 */
ZTEST(datastore_util_tests, test_notify_float_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up floatSubs with two active subscriptions */
  floatSubs.entries = test_entries;
  floatSubs.maxCount = 2;
  floatSubs.activeCount = 2;

  /* First subscription covers datapointId [0, 1) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 1;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [1, 2) */
  test_entries[1].datapointId = 1;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyFloatSubs with datapointId=0 - should fail on first matching subscription */
  result = notifyFloatSubs(0, pool);

  zassert_equal(result, -ENOSPC,
                "notifyFloatSubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyFloatSubs function must successfully notify all matching,
 * non-paused subscriptions and skip paused or non-matching subscriptions.
 */
ZTEST(datastore_util_tests, test_notify_float_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[3];
  static uint8_t fake_buffer[128];
  int result;

  /* Initialize and set up floatSubs with three subscriptions */
  floatSubs.entries = test_entries;
  floatSubs.maxCount = 3;
  floatSubs.activeCount = 3;

  /* Subscription 0: covers [0, 2), not paused - SHOULD be notified for datapointId=0 */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 2;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Subscription 1: covers [0, 1), PAUSED - should NOT be notified */
  test_entries[1].datapointId = 0;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = true;
  test_entries[1].callback = mock_subscription_callback;

  /* Subscription 2: covers [1, 2), not paused - should NOT be notified (doesn't match datapointId=0) */
  test_entries[2].datapointId = 1;
  test_entries[2].valCount = 1;
  test_entries[2].isPaused = false;
  test_entries[2].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyFloatSubs with datapointId=0 - should notify only subscription 0 */
  result = notifyFloatSubs(0, pool);

  zassert_equal(result, 0,
                "notifyFloatSubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for subscription 0");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for subscription 0");
}

/**
 * @test The isIntDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (start of range [10, 15)) */
  result = isIntDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isIntDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must return true when
 * datapointId equals the last valid datapoint ID in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 (last included) should be true.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last in range [10, 15)) */
  result = isIntDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must return false when
 * datapointId equals the subscription ending boundary (exclusive).
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (end boundary of range [10, 15), exclusive) */
  result = isIntDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isIntDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isIntDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isIntDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId matches.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=5 should be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 5 (single datapoint range [5, 6)) */
  result = isIntDatapointInSubRange(5, &sub);

  zassert_true(result,
               "datapointId 5 should be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isIntDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId does not match.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=6 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 6 (outside single datapoint range [5, 6)) */
  result = isIntDatapointInSubRange(6, &sub);

  zassert_false(result,
                "datapointId 6 should NOT be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isIntDatapointInSubRange function must correctly handle
 * datapointId 0 when it is included in the subscription range.
 *
 * Subscription covers datapoints [0, 3), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 3,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (start of range [0, 3)) */
  result = isIntDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 3)");
}

/**
 * @test The isIntDatapointInSubRange function must correctly handle
 * datapointId 0 when it is not included in the subscription range.
 *
 * Subscription covers datapoints [1, 3), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_int_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 1,
    .valCount = 2,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [1, 3)) */
  result = isIntDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [1, 3)");
}

/**
 * @test The notifyIntSub function must return -ENOSPC when memory pool
 * allocation fails.
 */
ZTEST(datastore_util_tests, test_notify_int_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyIntSub - should fail with allocation error */
  result = notifyIntSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyIntSub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyIntSub function must return the callback error code
 * when the callback function fails.
 */
ZTEST(datastore_util_tests, test_notify_int_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyIntSub - should fail due to callback error */
  result = notifyIntSub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyIntSub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
}

/**
 * @test The notifyIntSub function must successfully notify when allocation
 * and callback succeed.
 */
ZTEST(datastore_util_tests, test_notify_int_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyIntSub - should succeed */
  result = notifyIntSub(&sub, pool);

  zassert_equal(result, 0,
                "notifyIntSub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyIntSubs function must return an error and stop processing
 * when notifyIntSub fails for a subscription.
 */
ZTEST(datastore_util_tests, test_notify_int_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up intSubs with two active subscriptions */
  intSubs.entries = test_entries;
  intSubs.maxCount = 2;
  intSubs.activeCount = 2;

  /* First subscription covers datapointId [0, 1) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 1;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [1, 2) */
  test_entries[1].datapointId = 1;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyIntSubs with datapointId=0 - should fail on first matching subscription */
  result = notifyIntSubs(0, pool);

  zassert_equal(result, -ENOSPC,
                "notifyIntSubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyIntSubs function must successfully notify all matching,
 * non-paused subscriptions and skip paused or non-matching subscriptions.
 */
ZTEST(datastore_util_tests, test_notify_int_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[3];
  static uint8_t fake_buffer[128];
  int result;

  /* Initialize and set up intSubs with three subscriptions */
  intSubs.entries = test_entries;
  intSubs.maxCount = 3;
  intSubs.activeCount = 3;

  /* Subscription 0: covers [0, 2), not paused - SHOULD be notified for datapointId=0 */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 2;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Subscription 1: covers [0, 1), PAUSED - should NOT be notified */
  test_entries[1].datapointId = 0;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = true;
  test_entries[1].callback = mock_subscription_callback;

  /* Subscription 2: covers [1, 2), not paused - should NOT be notified (doesn't match datapointId=0) */
  test_entries[2].datapointId = 1;
  test_entries[2].valCount = 1;
  test_entries[2].isPaused = false;
  test_entries[2].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyIntSubs with datapointId=0 - should notify only subscription 0 */
  result = notifyIntSubs(0, pool);

  zassert_equal(result, 0,
                "notifyIntSubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for subscription 0");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for subscription 0");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (start of range [10, 15)) */
  result = isMultiStateDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isMultiStateDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return true when
 * datapointId equals the last valid datapoint ID in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 (last included) should be true.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last in range [10, 15)) */
  result = isMultiStateDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return false when
 * datapointId equals the subscription ending boundary (exclusive).
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (end boundary of range [10, 15), exclusive) */
  result = isMultiStateDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isMultiStateDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isMultiStateDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId matches.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=5 should be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 5 (single datapoint range [5, 6)) */
  result = isMultiStateDatapointInSubRange(5, &sub);

  zassert_true(result,
               "datapointId 5 should be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId does not match.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=6 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 6 (outside single datapoint range [5, 6)) */
  result = isMultiStateDatapointInSubRange(6, &sub);

  zassert_false(result,
                "datapointId 6 should NOT be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must correctly handle
 * datapointId 0 when it is included in the subscription range.
 *
 * Subscription covers datapoints [0, 3), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 3,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (start of range [0, 3)) */
  result = isMultiStateDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 3)");
}

/**
 * @test The isMultiStateDatapointInSubRange function must correctly handle
 * datapointId 0 when it is not included in the subscription range.
 *
 * Subscription covers datapoints [1, 3), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_multi_state_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 1,
    .valCount = 2,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [1, 3)) */
  result = isMultiStateDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [1, 3)");
}

/**
 * @test The notifyMultiStateSub function must return -ENOSPC when memory pool
 * allocation fails.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyMultiStateSub - should fail with allocation error */
  result = notifyMultiStateSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyMultiStateSub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyMultiStateSub function must return the callback error code
 * when the callback function fails.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyMultiStateSub - should fail due to callback error */
  result = notifyMultiStateSub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyMultiStateSub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
}

/**
 * @test The notifyMultiStateSub function must successfully notify when allocation
 * and callback succeed.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyMultiStateSub - should succeed */
  result = notifyMultiStateSub(&sub, pool);

  zassert_equal(result, 0,
                "notifyMultiStateSub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyMultiStateSubs function must return an error and stop processing
 * when notifyMultiStateSub fails for a subscription.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up multiStateSubs with two active subscriptions */
  multiStateSubs.entries = test_entries;
  multiStateSubs.maxCount = 2;
  multiStateSubs.activeCount = 2;

  /* First subscription covers datapointId [0, 1) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 1;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [1, 2) */
  test_entries[1].datapointId = 1;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyMultiStateSubs with datapointId=0 - should fail on first matching subscription */
  result = notifyMultiStateSubs(0, pool);

  zassert_equal(result, -ENOSPC,
                "notifyMultiStateSubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyMultiStateSubs function must successfully notify all matching,
 * non-paused subscriptions and skip paused or non-matching subscriptions.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[3];
  static uint8_t fake_buffer[128];
  int result;

  /* Initialize and set up multiStateSubs with three subscriptions */
  multiStateSubs.entries = test_entries;
  multiStateSubs.maxCount = 3;
  multiStateSubs.activeCount = 3;

  /* Subscription 0: covers [0, 2), not paused - SHOULD be notified for datapointId=0 */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 2;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Subscription 1: covers [0, 1), PAUSED - should NOT be notified */
  test_entries[1].datapointId = 0;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = true;
  test_entries[1].callback = mock_subscription_callback;

  /* Subscription 2: covers [1, 2), not paused - should NOT be notified (doesn't match datapointId=0) */
  test_entries[2].datapointId = 1;
  test_entries[2].valCount = 1;
  test_entries[2].isPaused = false;
  test_entries[2].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyMultiStateSubs with datapointId=0 - should notify only subscription 0 */
  result = notifyMultiStateSubs(0, pool);

  zassert_equal(result, 0,
                "notifyMultiStateSubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for subscription 0");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for subscription 0");
}

/**
 * @test The isUintDatapointInSubRange function must return true when
 * datapointId equals the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=10 should be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_at_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 10 (start of range [10, 15)) */
  result = isUintDatapointInSubRange(10, &sub);

  zassert_true(result,
               "datapointId 10 should be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must return true when
 * datapointId is in the middle of the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=12 should be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_middle)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 12 (middle of range [10, 15)) */
  result = isUintDatapointInSubRange(12, &sub);

  zassert_true(result,
               "datapointId 12 should be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must return true when
 * datapointId equals the last valid datapoint ID in the subscription range.
 *
 * Subscription covers datapoints [10, 15), so datapointId=14 (last included) should be true.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_last)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 14 (last in range [10, 15)) */
  result = isUintDatapointInSubRange(14, &sub);

  zassert_true(result,
               "datapointId 14 should be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must return false when
 * datapointId equals the subscription ending boundary (exclusive).
 *
 * Subscription covers datapoints [10, 15), so datapointId=15 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_at_end_boundary)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 15 (end boundary of range [10, 15), exclusive) */
  result = isUintDatapointInSubRange(15, &sub);

  zassert_false(result,
                "datapointId 15 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must return false when
 * datapointId is less than the subscription starting datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=9 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_before_start)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 9 (before range [10, 15)) */
  result = isUintDatapointInSubRange(9, &sub);

  zassert_false(result,
                "datapointId 9 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must return false when
 * datapointId is greater than the subscription ending datapoint ID.
 *
 * Subscription covers datapoints [10, 15), so datapointId=20 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_after_end)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 10,
    .valCount = 5,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 20 (well after range [10, 15)) */
  result = isUintDatapointInSubRange(20, &sub);

  zassert_false(result,
                "datapointId 20 should NOT be included in subscription range [10, 15)");
}

/**
 * @test The isUintDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId matches.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=5 should be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_single_datapoint_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 5 (single datapoint range [5, 6)) */
  result = isUintDatapointInSubRange(5, &sub);

  zassert_true(result,
               "datapointId 5 should be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isUintDatapointInSubRange function must handle a subscription
 * with a single datapoint correctly when the datapointId does not match.
 *
 * Subscription covers only datapoint [5, 6), so datapointId=6 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_single_datapoint_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 5,
    .valCount = 1,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 6 (outside single datapoint range [5, 6)) */
  result = isUintDatapointInSubRange(6, &sub);

  zassert_false(result,
                "datapointId 6 should NOT be included in single datapoint subscription [5, 6)");
}

/**
 * @test The isUintDatapointInSubRange function must correctly handle
 * datapointId 0 when it is included in the subscription range.
 *
 * Subscription covers datapoints [0, 3), so datapointId=0 should be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_zero_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 3,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (start of range [0, 3)) */
  result = isUintDatapointInSubRange(0, &sub);

  zassert_true(result,
               "datapointId 0 should be included in subscription range [0, 3)");
}

/**
 * @test The isUintDatapointInSubRange function must correctly handle
 * datapointId 0 when it is not included in the subscription range.
 *
 * Subscription covers datapoints [1, 3), so datapointId=0 should NOT be included.
 */
ZTEST(datastore_util_tests, test_is_uint_datapoint_in_range_zero_not_included)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 1,
    .valCount = 2,
    .isPaused = false,
    .callback = NULL
  };
  bool result;

  /* Test with datapointId = 0 (before range [1, 3)) */
  result = isUintDatapointInSubRange(0, &sub);

  zassert_false(result,
                "datapointId 0 should NOT be included in subscription range [1, 3)");
}

/**
 * @test The notifyUintSub function must return -ENOSPC when memory pool
 * allocation fails.
 */
ZTEST(datastore_util_tests, test_notify_uint_sub_allocation_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyUintSub - should fail with allocation error */
  result = notifyUintSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "notifyUintSub should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyUintSub function must return the callback error code
 * when the callback function fails.
 */
ZTEST(datastore_util_tests, test_notify_uint_sub_callback_failure)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return error */
  mock_subscription_callback_fake.return_val = -EIO;

  /* Call notifyUintSub - should fail due to callback error */
  result = notifyUintSub(&sub, pool);

  zassert_equal(result, -EIO,
                "notifyUintSub should return -EIO when callback fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
}

/**
 * @test The notifyUintSub function must successfully notify when allocation
 * and callback succeed.
 */
ZTEST(datastore_util_tests, test_notify_uint_sub_success)
{
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 2,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static uint8_t fake_buffer[128];
  SrvMsgPayload_t *payload;
  int result;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyUintSub - should succeed */
  result = notifyUintSub(&sub, pool);

  zassert_equal(result, 0,
                "notifyUintSub should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with the correct pool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify payload was populated correctly */
  payload = (SrvMsgPayload_t *)fake_buffer;
  zassert_equal(payload->poolId, pool,
                "Payload poolId should be set to the pool");
  zassert_equal(payload->dataLen, 2 * sizeof(Data_t),
                "Payload dataLen should be valCount * sizeof(Data_t)");

  /* Verify callback was called with correct arguments */
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once");
  zassert_equal(mock_subscription_callback_fake.arg0_val, payload,
                "Callback should be called with the populated payload");
  zassert_equal(mock_subscription_callback_fake.arg1_val, 2,
                "Callback should be called with valCount=2");
}

/**
 * @test The notifyUintSubs function must return an error and stop processing
 * when notifyUintSub fails for a subscription.
 */
ZTEST(datastore_util_tests, test_notify_uint_subs_notification_failure)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[2];
  int result;

  /* Initialize and set up uintSubs with two active subscriptions */
  uintSubs.entries = test_entries;
  uintSubs.maxCount = 2;
  uintSubs.activeCount = 2;

  /* First subscription covers datapointId [0, 1) */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 1;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Second subscription covers datapointId [1, 2) */
  test_entries[1].datapointId = 1;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = false;
  test_entries[1].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to return NULL (allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call notifyUintSubs with datapointId=0 - should fail on first matching subscription */
  result = notifyUintSubs(0, pool);

  zassert_equal(result, -ENOSPC,
                "notifyUintSubs should return -ENOSPC when allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once before stopping");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when allocation fails");
}

/**
 * @test The notifyUintSubs function must successfully notify all matching,
 * non-paused subscriptions and skip paused or non-matching subscriptions.
 */
ZTEST(datastore_util_tests, test_notify_uint_subs_success)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  static DatastoreSubEntry_t test_entries[3];
  static uint8_t fake_buffer[128];
  int result;

  /* Initialize and set up uintSubs with three subscriptions */
  uintSubs.entries = test_entries;
  uintSubs.maxCount = 3;
  uintSubs.activeCount = 3;

  /* Subscription 0: covers [0, 2), not paused - SHOULD be notified for datapointId=0 */
  test_entries[0].datapointId = 0;
  test_entries[0].valCount = 2;
  test_entries[0].isPaused = false;
  test_entries[0].callback = mock_subscription_callback;

  /* Subscription 1: covers [0, 1), PAUSED - should NOT be notified */
  test_entries[1].datapointId = 0;
  test_entries[1].valCount = 1;
  test_entries[1].isPaused = true;
  test_entries[1].callback = mock_subscription_callback;

  /* Subscription 2: covers [1, 2), not paused - should NOT be notified (doesn't match datapointId=0) */
  test_entries[2].datapointId = 1;
  test_entries[2].valCount = 1;
  test_entries[2].isPaused = false;
  test_entries[2].callback = mock_subscription_callback;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call notifyUintSubs with datapointId=0 - should notify only subscription 0 */
  result = notifyUintSubs(0, pool);

  zassert_equal(result, 0,
                "notifyUintSubs should return 0 on success");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for subscription 0");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for subscription 0");
}

ZTEST_SUITE(datastore_util_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
