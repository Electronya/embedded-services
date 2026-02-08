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
  FAKE(k_malloc) \
  FAKE(k_free) \
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

/* Mock kernel memory allocation functions */
FAKE_VALUE_FUNC(void *, k_malloc, size_t);
FAKE_VOID_FUNC(k_free, void *);

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
 * @test The notifyButtonSubs function must skip notifications for
 * subscriptions that match the datapointId but are paused.
 */
ZTEST(datastore_util_tests, test_notify_button_subs_paused_in_range)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  DatastoreSubEntry_t test_entry;
  int result;

  /* Set up buttonSubs with 1 entry that matches but is paused */
  buttonSubs.maxCount = 1;
  buttonSubs.activeCount = 1;
  buttonSubs.entries = &test_entry;

  /* Entry covers [5, 10), paused - datapointId=7 is in range but should be skipped */
  test_entry.datapointId = 5;
  test_entry.valCount = 5;
  test_entry.isPaused = true;
  test_entry.callback = mock_subscription_callback;

  /* Call notifyButtonSubs with datapointId=7 */
  result = notifyButtonSubs(7, pool);

  zassert_equal(result, 0,
                "notifyButtonSubs should return 0 when all matching subs are paused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called for paused subscriptions");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called for paused subscriptions");
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

/**
 * @test The isDatapointIdAndValCountValid function must return false when
 * datapointId is greater than or equal to datapointCount.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_id_out_of_bounds)
{
  bool result;

  /* datapointId >= datapointCount - should be invalid */
  result = isDatapointIdAndValCountValid(10, 1, 10);
  zassert_false(result,
                "Should return false when datapointId >= datapointCount");

  result = isDatapointIdAndValCountValid(15, 1, 10);
  zassert_false(result,
                "Should return false when datapointId > datapointCount");
}

/**
 * @test The isDatapointIdAndValCountValid function must return false when
 * datapointId + valCount exceeds datapointCount.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_range_overflow)
{
  bool result;

  /* datapointId + valCount > datapointCount - should be invalid */
  result = isDatapointIdAndValCountValid(8, 3, 10);
  zassert_false(result,
                "Should return false when datapointId + valCount > datapointCount");

  result = isDatapointIdAndValCountValid(9, 2, 10);
  zassert_false(result,
                "Should return false when range exceeds bounds by 1");
}

/**
 * @test The isDatapointIdAndValCountValid function must return false when
 * both conditions fail (datapointId out of bounds and range overflow).
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_both_fail)
{
  bool result;

  /* Both datapointId >= datapointCount AND datapointId + valCount > datapointCount */
  result = isDatapointIdAndValCountValid(15, 5, 10);
  zassert_false(result,
                "Should return false when both conditions fail");
}

/**
 * @test The isDatapointIdAndValCountValid function must return true when
 * datapointId is at the start boundary and range is valid.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_at_start)
{
  bool result;

  /* datapointId = 0, valid range */
  result = isDatapointIdAndValCountValid(0, 1, 10);
  zassert_true(result,
               "Should return true when datapointId = 0 with valid range");

  result = isDatapointIdAndValCountValid(0, 5, 10);
  zassert_true(result,
               "Should return true when datapointId = 0 with larger valid range");
}

/**
 * @test The isDatapointIdAndValCountValid function must return true when
 * datapointId is at the end boundary and range is valid.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_at_end)
{
  bool result;

  /* datapointId at end with valCount = 1 */
  result = isDatapointIdAndValCountValid(9, 1, 10);
  zassert_true(result,
               "Should return true when datapointId + valCount = datapointCount");
}

/**
 * @test The isDatapointIdAndValCountValid function must return true when
 * datapointId is in the middle and range is valid.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_in_middle)
{
  bool result;

  /* datapointId in middle with valid range */
  result = isDatapointIdAndValCountValid(5, 3, 10);
  zassert_true(result,
               "Should return true when datapointId and range are in middle");

  result = isDatapointIdAndValCountValid(3, 5, 10);
  zassert_true(result,
               "Should return true when range spans middle to near end");
}

/**
 * @test The isDatapointIdAndValCountValid function must return true when
 * valCount is 0.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_zero_count)
{
  bool result;

  /* valCount = 0 should be valid (no range to check) */
  result = isDatapointIdAndValCountValid(5, 0, 10);
  zassert_true(result,
               "Should return true when valCount = 0");

  result = isDatapointIdAndValCountValid(9, 0, 10);
  zassert_true(result,
               "Should return true when valCount = 0 at end boundary");
}

/**
 * @test The isDatapointIdAndValCountValid function must return true when
 * the range covers the entire datapoint array.
 */
ZTEST(datastore_util_tests, test_is_datapoint_id_and_val_count_valid_full_range)
{
  bool result;

  /* Full range from start to end */
  result = isDatapointIdAndValCountValid(0, 10, 10);
  zassert_true(result,
               "Should return true when range covers entire array");
}

/**
 * @test The datastoreUtilAllocateBinarySubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_binary_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateBinarySubs - should fail */
  result = datastoreUtilAllocateBinarySubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(binarySubs.entries,
                  "binarySubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateBinarySubs function must successfully allocate
 * memory and initialize binarySubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_binary_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateBinarySubs - should succeed */
  result = datastoreUtilAllocateBinarySubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(binarySubs.entries, fake_buffer,
                    "binarySubs.entries should be set to allocated memory");
  zassert_equal(binarySubs.maxCount, 5,
                "binarySubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilAllocateButtonSubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_button_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateButtonSubs - should fail */
  result = datastoreUtilAllocateButtonSubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(buttonSubs.entries,
                  "buttonSubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateButtonSubs function must successfully allocate
 * memory and initialize buttonSubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_button_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateButtonSubs - should succeed */
  result = datastoreUtilAllocateButtonSubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(buttonSubs.entries, fake_buffer,
                    "buttonSubs.entries should be set to allocated memory");
  zassert_equal(buttonSubs.maxCount, 5,
                "buttonSubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilAllocateFloatSubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_float_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateFloatSubs - should fail */
  result = datastoreUtilAllocateFloatSubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(floatSubs.entries,
                  "floatSubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateFloatSubs function must successfully allocate
 * memory and initialize floatSubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_float_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateFloatSubs - should succeed */
  result = datastoreUtilAllocateFloatSubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(floatSubs.entries, fake_buffer,
                    "floatSubs.entries should be set to allocated memory");
  zassert_equal(floatSubs.maxCount, 5,
                "floatSubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilAllocateIntSubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_int_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateIntSubs - should fail */
  result = datastoreUtilAllocateIntSubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(intSubs.entries,
                  "intSubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateIntSubs function must successfully allocate
 * memory and initialize intSubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_int_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateIntSubs - should succeed */
  result = datastoreUtilAllocateIntSubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(intSubs.entries, fake_buffer,
                    "intSubs.entries should be set to allocated memory");
  zassert_equal(intSubs.maxCount, 5,
                "intSubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilAllocateMultiStateSubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_multi_state_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateMultiStateSubs - should fail */
  result = datastoreUtilAllocateMultiStateSubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(multiStateSubs.entries,
                  "multiStateSubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateMultiStateSubs function must successfully allocate
 * memory and initialize multiStateSubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_multi_state_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateMultiStateSubs - should succeed */
  result = datastoreUtilAllocateMultiStateSubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(multiStateSubs.entries, fake_buffer,
                    "multiStateSubs.entries should be set to allocated memory");
  zassert_equal(multiStateSubs.maxCount, 5,
                "multiStateSubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilAllocateUintSubs function must return -ENOSPC
 * when k_malloc fails to allocate memory.
 */
ZTEST(datastore_util_tests, test_allocate_uint_subs_allocation_failure)
{
  int result;

  /* Configure k_malloc to return NULL (allocation failure) */
  k_malloc_fake.return_val = NULL;

  /* Call datastoreUtilAllocateUintSubs - should fail */
  result = datastoreUtilAllocateUintSubs(5);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when k_malloc fails");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_is_null(uintSubs.entries,
                  "uintSubs.entries should remain NULL");
}

/**
 * @test The datastoreUtilAllocateUintSubs function must successfully allocate
 * memory and initialize uintSubs when k_malloc succeeds.
 */
ZTEST(datastore_util_tests, test_allocate_uint_subs_success)
{
  static uint8_t fake_buffer[256];
  int result;

  /* Configure k_malloc to succeed */
  k_malloc_fake.return_val = fake_buffer;

  /* Call datastoreUtilAllocateUintSubs - should succeed */
  result = datastoreUtilAllocateUintSubs(5);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(k_malloc_fake.call_count, 1,
                "k_malloc should be called once");
  zassert_equal(k_malloc_fake.arg0_val, 5 * sizeof(DatastoreSubEntry_t),
                "k_malloc should be called with correct size");
  zassert_equal_ptr(uintSubs.entries, fake_buffer,
                    "uintSubs.entries should be set to allocated memory");
  zassert_equal(uintSubs.maxCount, 5,
                "uintSubs.maxCount should be set to maxSubCount");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return 0 when
 * all datapoint counts are zero.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_all_zero)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {0, 0, 0, 0, 0, 0};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 0,
                "Should return 0 when all counts are zero");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when the first element is the maximum.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_first_max)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {10, 5, 3, 2, 1, 0};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 10 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when the last element is the maximum.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_last_max)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {1, 2, 3, 5, 8, 15};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 15 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when a middle element is the maximum.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_middle_max)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {5, 3, 12, 7, 2, 1};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 12 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when multiple elements have the same maximum value.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_multiple_max)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {8, 3, 8, 5, 8, 2};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 8 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when all elements have the same non-zero value.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_all_same)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {7, 7, 7, 7, 7, 7};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 7 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilCalculateBufferSize function must return correct size
 * when only one element is non-zero.
 */
ZTEST(datastore_util_tests, test_calculate_buffer_size_single_nonzero)
{
  size_t counts[DATAPOINT_TYPE_COUNT] = {0, 0, 0, 20, 0, 0};
  size_t result;

  result = datastoreUtilCalculateBufferSize(counts);

  zassert_equal(result, 20 * sizeof(Datapoint_t),
                "Should return max_count * sizeof(Datapoint_t)");
}

/**
 * @test The datastoreUtilAddBinarySub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_binary_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddBinarySub - should fail due to full list */
  result = datastoreUtilAddBinarySub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(binarySubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddBinarySub function must return the error code
 * when notifyBinarySub fails.
 */
ZTEST(datastore_util_tests, test_add_binary_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyBinarySub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddBinarySub - should fail due to notification failure */
  result = datastoreUtilAddBinarySub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyBinarySub fails");
  zassert_equal(binarySubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @test The datastoreUtilAddBinarySub function must successfully add a subscription
 * and notify when everything succeeds.
 */
ZTEST(datastore_util_tests, test_add_binary_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call datastoreUtilAddBinarySub - should succeed */
  result = datastoreUtilAddBinarySub(&sub, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(binarySubs.activeCount, 3,
                "activeCount should be incremented");
  zassert_equal(binarySubs.entries[2].datapointId, 0,
                "New subscription datapointId should be added");
  zassert_equal(binarySubs.entries[2].valCount, 1,
                "New subscription valCount should be added");
  zassert_false(binarySubs.entries[2].isPaused,
                "New subscription isPaused should be added");
  zassert_equal(binarySubs.entries[2].callback, mock_subscription_callback,
                "New subscription callback should be added");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilRemoveBinarySub function must return -ESRCH when
 * the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_binary_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up binarySubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 2;
  binarySubs.entries[0].callback = mock_subscription_callback;
  binarySubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveBinarySub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(binarySubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveBinarySub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_binary_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up binarySubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 3;
  binarySubs.entries[0].callback = callback1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[1].callback = callback2;
  binarySubs.entries[1].datapointId = 1;
  binarySubs.entries[2].callback = callback3;
  binarySubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveBinarySub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(binarySubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(binarySubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(binarySubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(binarySubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetBinarySubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_binary_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetBinarySubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetBinarySubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_binary_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 2;
  binarySubs.entries[0].callback = mock_subscription_callback;
  binarySubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetBinarySubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetBinarySubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_binary_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 2;
  binarySubs.entries[0].callback = mock_subscription_callback;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  binarySubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetBinarySubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(binarySubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(binarySubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetBinarySubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_binary_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binarySubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].callback = mock_subscription_callback;
  binarySubs.entries[0].isPaused = true;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetBinarySubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(binarySubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilAddButtonSub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_button_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddButtonSub - should fail due to full list */
  result = datastoreUtilAddButtonSub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(buttonSubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddButtonSub function must return the error code
 * when notifyButtonSub fails.
 */
ZTEST(datastore_util_tests, test_add_button_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyButtonSub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddButtonSub - should fail due to notification failure */
  result = datastoreUtilAddButtonSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyButtonSub fails");
  zassert_equal(buttonSubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @test The datastoreUtilAddButtonSub function must successfully add a subscription
 * and notify when everything succeeds.
 */
ZTEST(datastore_util_tests, test_add_button_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call datastoreUtilAddButtonSub - should succeed */
  result = datastoreUtilAddButtonSub(&sub, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(buttonSubs.activeCount, 3,
                "activeCount should be incremented");
  zassert_equal(buttonSubs.entries[2].datapointId, 0,
                "New subscription datapointId should be added");
  zassert_equal(buttonSubs.entries[2].valCount, 1,
                "New subscription valCount should be added");
  zassert_false(buttonSubs.entries[2].isPaused,
                "New subscription isPaused should be added");
  zassert_equal(buttonSubs.entries[2].callback, mock_subscription_callback,
                "New subscription callback should be added");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilRemoveButtonSub function must return -ESRCH when
 * the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_button_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up buttonSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 2;
  buttonSubs.entries[0].callback = mock_subscription_callback;
  buttonSubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveButtonSub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(buttonSubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveButtonSub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_button_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up buttonSubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 3;
  buttonSubs.entries[0].callback = callback1;
  buttonSubs.entries[0].datapointId = 0;
  buttonSubs.entries[1].callback = callback2;
  buttonSubs.entries[1].datapointId = 1;
  buttonSubs.entries[2].callback = callback3;
  buttonSubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveButtonSub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(buttonSubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(buttonSubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(buttonSubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(buttonSubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetButtonSubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_button_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetButtonSubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetButtonSubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_button_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 2;
  buttonSubs.entries[0].callback = mock_subscription_callback;
  buttonSubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetButtonSubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetButtonSubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_button_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 2;
  buttonSubs.entries[0].callback = mock_subscription_callback;
  buttonSubs.entries[0].isPaused = false;
  buttonSubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  buttonSubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetButtonSubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(buttonSubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(buttonSubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetButtonSubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_button_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up buttonSubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 1;
  buttonSubs.entries[0].callback = mock_subscription_callback;
  buttonSubs.entries[0].isPaused = true;
  buttonSubs.entries[0].datapointId = 0;
  buttonSubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetButtonSubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(buttonSubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilAddFloatSub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_float_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddFloatSub - should fail due to full list */
  result = datastoreUtilAddFloatSub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(floatSubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddFloatSub function must return the error code
 * when notifyFloatSub fails.
 */
ZTEST(datastore_util_tests, test_add_float_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyFloatSub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddFloatSub - should fail due to notification failure */
  result = datastoreUtilAddFloatSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyFloatSub fails");
  zassert_equal(floatSubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @test The datastoreUtilAddFloatSub function must successfully add a subscription
 * and notify when everything succeeds.
 */
ZTEST(datastore_util_tests, test_add_float_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call datastoreUtilAddFloatSub - should succeed */
  result = datastoreUtilAddFloatSub(&sub, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(floatSubs.activeCount, 3,
                "activeCount should be incremented");
  zassert_equal(floatSubs.entries[2].datapointId, 0,
                "New subscription datapointId should be added");
  zassert_equal(floatSubs.entries[2].valCount, 1,
                "New subscription valCount should be added");
  zassert_false(floatSubs.entries[2].isPaused,
                "New subscription isPaused should be added");
  zassert_equal(floatSubs.entries[2].callback, mock_subscription_callback,
                "New subscription callback should be added");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilRemoveFloatSub function must return -ESRCH when
 * the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_float_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up floatSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 2;
  floatSubs.entries[0].callback = mock_subscription_callback;
  floatSubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveFloatSub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(floatSubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveFloatSub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_float_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up floatSubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 3;
  floatSubs.entries[0].callback = callback1;
  floatSubs.entries[0].datapointId = 0;
  floatSubs.entries[1].callback = callback2;
  floatSubs.entries[1].datapointId = 1;
  floatSubs.entries[2].callback = callback3;
  floatSubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveFloatSub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(floatSubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(floatSubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(floatSubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(floatSubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetFloatSubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_float_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetFloatSubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetFloatSubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_float_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 2;
  floatSubs.entries[0].callback = mock_subscription_callback;
  floatSubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetFloatSubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetFloatSubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_float_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 2;
  floatSubs.entries[0].callback = mock_subscription_callback;
  floatSubs.entries[0].isPaused = false;
  floatSubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  floatSubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetFloatSubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(floatSubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(floatSubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetFloatSubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_float_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up floatSubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 1;
  floatSubs.entries[0].callback = mock_subscription_callback;
  floatSubs.entries[0].isPaused = true;
  floatSubs.entries[0].datapointId = 0;
  floatSubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetFloatSubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(floatSubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilAddIntSub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_int_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddIntSub - should fail due to full list */
  result = datastoreUtilAddIntSub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(intSubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddIntSub function must return the error code
 * when notifyIntSub fails.
 */
ZTEST(datastore_util_tests, test_add_int_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyIntSub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddIntSub - should fail due to notification failure */
  result = datastoreUtilAddIntSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyIntSub fails");
  zassert_equal(intSubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @test The datastoreUtilAddIntSub function must successfully add a subscription
 * and notify when everything succeeds.
 */
ZTEST(datastore_util_tests, test_add_int_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Call datastoreUtilAddIntSub - should succeed */
  result = datastoreUtilAddIntSub(&sub, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(intSubs.activeCount, 3,
                "activeCount should be incremented");
  zassert_equal(intSubs.entries[2].datapointId, 0,
                "New subscription datapointId should be added");
  zassert_equal(intSubs.entries[2].valCount, 1,
                "New subscription valCount should be added");
  zassert_false(intSubs.entries[2].isPaused,
                "New subscription isPaused should be added");
  zassert_equal(intSubs.entries[2].callback, mock_subscription_callback,
                "New subscription callback should be added");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilRemoveIntSub function must return -ESRCH when
 * the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_int_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up intSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 2;
  intSubs.entries[0].callback = mock_subscription_callback;
  intSubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveIntSub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(intSubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveIntSub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_int_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up intSubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 3;
  intSubs.entries[0].callback = callback1;
  intSubs.entries[0].datapointId = 0;
  intSubs.entries[1].callback = callback2;
  intSubs.entries[1].datapointId = 1;
  intSubs.entries[2].callback = callback3;
  intSubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveIntSub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(intSubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(intSubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(intSubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(intSubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetIntSubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_int_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetIntSubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetIntSubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_int_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 2;
  intSubs.entries[0].callback = mock_subscription_callback;
  intSubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetIntSubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetIntSubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_int_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 2;
  intSubs.entries[0].callback = mock_subscription_callback;
  intSubs.entries[0].isPaused = false;
  intSubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  intSubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetIntSubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(intSubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(intSubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetIntSubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_int_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up intSubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 1;
  intSubs.entries[0].callback = mock_subscription_callback;
  intSubs.entries[0].isPaused = true;
  intSubs.entries[0].datapointId = 0;
  intSubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetIntSubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(intSubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilAddMultiStateSub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_multi_state_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddMultiStateSub - should fail due to full list */
  result = datastoreUtilAddMultiStateSub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(multiStateSubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddMultiStateSub function must return the error code
 * when notifyMultiStateSub fails.
 */
ZTEST(datastore_util_tests, test_add_multi_state_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyMultiStateSub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddMultiStateSub - should fail due to notification failure */
  result = datastoreUtilAddMultiStateSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyMultiStateSub fails");
  zassert_equal(multiStateSubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @brief   Test adding a multi state subscription successfully.
 */
ZTEST(datastore_util_tests, test_add_multi_state_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_msg;

  /* Call datastoreUtilAddMultiStateSub - should succeed */
  result = datastoreUtilAddMultiStateSub(&sub, pool);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(multiStateSubs.activeCount, 3, "activeCount should be incremented");
  zassert_equal(multiStateSubs.entries[2].datapointId, sub.datapointId,
                "datapointId should be set correctly");
  zassert_equal(multiStateSubs.entries[2].valCount, sub.valCount,
                "valCount should be set correctly");
  zassert_equal(multiStateSubs.entries[2].isPaused, sub.isPaused,
                "isPaused should be set correctly");
  zassert_equal(multiStateSubs.entries[2].callback, sub.callback,
                "callback should be set correctly");
}

/**
 * @test The datastoreUtilRemoveMultiStateSub function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_multi_state_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up multiStateSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 2;
  multiStateSubs.entries[0].callback = mock_subscription_callback;
  multiStateSubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveMultiStateSub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(multiStateSubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveMultiStateSub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_multi_state_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up multiStateSubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 3;
  multiStateSubs.entries[0].callback = callback1;
  multiStateSubs.entries[0].datapointId = 0;
  multiStateSubs.entries[1].callback = callback2;
  multiStateSubs.entries[1].datapointId = 1;
  multiStateSubs.entries[2].callback = callback3;
  multiStateSubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveMultiStateSub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(multiStateSubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(multiStateSubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(multiStateSubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(multiStateSubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetMultiStateSubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_multi_state_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetMultiStateSubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetMultiStateSubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_multi_state_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 2;
  multiStateSubs.entries[0].callback = mock_subscription_callback;
  multiStateSubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetMultiStateSubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetMultiStateSubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_multi_state_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 2;
  multiStateSubs.entries[0].callback = mock_subscription_callback;
  multiStateSubs.entries[0].isPaused = false;
  multiStateSubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  multiStateSubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetMultiStateSubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(multiStateSubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(multiStateSubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetMultiStateSubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_multi_state_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up multiStateSubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 1;
  multiStateSubs.entries[0].callback = mock_subscription_callback;
  multiStateSubs.entries[0].isPaused = true;
  multiStateSubs.entries[0].datapointId = 0;
  multiStateSubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetMultiStateSubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(multiStateSubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilAddUintSub function must return -ENOBUFS when
 * the subscription list is full.
 */
ZTEST(datastore_util_tests, test_add_uint_sub_list_full)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs to be full */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 4;  /* maxCount is 5, so activeCount + 1 >= maxCount */

  /* Call datastoreUtilAddUintSub - should fail due to full list */
  result = datastoreUtilAddUintSub(&sub, pool);

  zassert_equal(result, -ENOBUFS,
                "Should return -ENOBUFS when subscription list is full");
  zassert_equal(uintSubs.activeCount, 4,
                "activeCount should not be incremented");
}

/**
 * @test The datastoreUtilAddUintSub function must return the error code
 * when notifyUintSub fails.
 */
ZTEST(datastore_util_tests, test_add_uint_sub_notify_failure)
{
  static uint8_t fake_buffer[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to fail (causes notifyUintSub to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreUtilAddUintSub - should fail due to notification failure */
  result = datastoreUtilAddUintSub(&sub, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyUintSub fails");
  zassert_equal(uintSubs.activeCount, 3,
                "activeCount should be incremented even when notification fails");
}

/**
 * @test The datastoreUtilAddUintSub function must successfully add a
 * subscription when space is available and notification succeeds.
 */
ZTEST(datastore_util_tests, test_add_uint_sub_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  DatastoreSubEntry_t sub = {
    .datapointId = 0,
    .valCount = 1,
    .isPaused = false,
    .callback = mock_subscription_callback
  };
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs with available space */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 2;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_msg;

  /* Call datastoreUtilAddUintSub - should succeed */
  result = datastoreUtilAddUintSub(&sub, pool);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(uintSubs.activeCount, 3, "activeCount should be incremented");
  zassert_equal(uintSubs.entries[2].datapointId, sub.datapointId,
                "datapointId should be set correctly");
  zassert_equal(uintSubs.entries[2].valCount, sub.valCount,
                "valCount should be set correctly");
  zassert_equal(uintSubs.entries[2].isPaused, sub.isPaused,
                "isPaused should be set correctly");
  zassert_equal(uintSubs.entries[2].callback, sub.callback,
                "callback should be set correctly");
}

/**
 * @test The datastoreUtilRemoveUintSub function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_remove_uint_sub_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  int result;

  /* Set up uintSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 2;
  uintSubs.entries[0].callback = mock_subscription_callback;
  uintSubs.entries[1].callback = mock_subscription_callback;

  /* Try to remove a callback that doesn't exist */
  result = datastoreUtilRemoveUintSub(other_callback);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
  zassert_equal(uintSubs.activeCount, 2,
                "activeCount should remain unchanged");
}

/**
 * @test The datastoreUtilRemoveUintSub function must successfully remove
 * a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_remove_uint_sub_success)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t callback1 = (DatastoreSubCb_t)0x1000;
  DatastoreSubCb_t callback2 = (DatastoreSubCb_t)0x2000;
  DatastoreSubCb_t callback3 = (DatastoreSubCb_t)0x3000;
  int result;

  /* Set up uintSubs with three entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 3;
  uintSubs.entries[0].callback = callback1;
  uintSubs.entries[0].datapointId = 0;
  uintSubs.entries[1].callback = callback2;
  uintSubs.entries[1].datapointId = 1;
  uintSubs.entries[2].callback = callback3;
  uintSubs.entries[2].datapointId = 2;

  /* Remove the middle entry (callback2) */
  result = datastoreUtilRemoveUintSub(callback2);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_equal(uintSubs.activeCount, 2,
                "activeCount should be decremented");
  zassert_equal(uintSubs.entries[0].callback, callback1,
                "First entry should remain unchanged");
  zassert_equal(uintSubs.entries[1].callback, callback3,
                "Third entry should be shifted down to second position");
  zassert_equal(uintSubs.entries[1].datapointId, 2,
                "Third entry's datapointId should be preserved");
}

/**
 * @test The datastoreUtilSetUintSubPauseState function must return -EINVAL
 * when the callback is NULL.
 */
ZTEST(datastore_util_tests, test_set_uint_sub_pause_state_null_callback)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Call with NULL callback - should fail */
  result = datastoreUtilSetUintSubPauseState(NULL, true, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when callback is NULL");
}

/**
 * @test The datastoreUtilSetUintSubPauseState function must return -ESRCH
 * when the callback is not found in the subscription list.
 */
ZTEST(datastore_util_tests, test_set_uint_sub_pause_state_not_found)
{
  static uint8_t fake_buffer[256];
  DatastoreSubCb_t other_callback = (DatastoreSubCb_t)0x2000;
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 2;
  uintSubs.entries[0].callback = mock_subscription_callback;
  uintSubs.entries[1].callback = mock_subscription_callback;

  /* Try to set pause state for a callback that doesn't exist */
  result = datastoreUtilSetUintSubPauseState(other_callback, true, pool);

  zassert_equal(result, -ESRCH,
                "Should return -ESRCH when callback not found");
}

/**
 * @test The datastoreUtilSetUintSubPauseState function must successfully
 * pause a subscription when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_uint_sub_pause_state_pause_success)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs with some entries */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 2;
  uintSubs.entries[0].callback = mock_subscription_callback;
  uintSubs.entries[0].isPaused = false;
  uintSubs.entries[1].callback = (DatastoreSubCb_t)0x2000;
  uintSubs.entries[1].isPaused = false;

  /* Pause the first subscription */
  result = datastoreUtilSetUintSubPauseState(mock_subscription_callback, true, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_true(uintSubs.entries[0].isPaused,
               "Subscription should be paused");
  zassert_false(uintSubs.entries[1].isPaused,
                "Other subscriptions should remain unchanged");
}

/**
 * @test The datastoreUtilSetUintSubPauseState function must successfully
 * unpause a subscription and notify when the callback is found.
 */
ZTEST(datastore_util_tests, test_set_uint_sub_pause_state_unpause_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_pool_buffer[128];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up uintSubs with a paused entry */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 1;
  uintSubs.entries[0].callback = mock_subscription_callback;
  uintSubs.entries[0].isPaused = true;
  uintSubs.entries[0].datapointId = 0;
  uintSubs.entries[0].valCount = 1;

  /* Configure osMemoryPoolAlloc to succeed for notification */
  osMemoryPoolAlloc_fake.return_val = fake_pool_buffer;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Unpause the subscription */
  result = datastoreUtilSetUintSubPauseState(mock_subscription_callback, false, pool);

  zassert_equal(result, 0,
                "Should return 0 on success");
  zassert_false(uintSubs.entries[0].isPaused,
                "Subscription should be unpaused");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
}

/**
 * @test The datastoreUtilRead function must return -EINVAL when the
 * datapoint ID or value count is invalid.
 */
ZTEST(datastore_util_tests, test_read_invalid_datapoint)
{
  Data_t values[2];
  int result;

  /* Try to read with invalid datapoint ID (beyond range) */
  result = datastoreUtilRead(DATAPOINT_BINARY, 100, 1, values);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when datapoint ID is invalid");
}

/**
 * @test The datastoreUtilRead function must successfully read datapoint
 * values when the datapoint ID and value count are valid.
 */
ZTEST(datastore_util_tests, test_read_success)
{
  Data_t values[2];
  int result;

  /* Set known values in the datastore */
  binaries[0].value.uintVal = 1;
  binaries[1].value.uintVal = 0;

  /* Read the values */
  result = datastoreUtilRead(DATAPOINT_BINARY, 0, 2, values);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(values[0].uintVal, 1, "First value should match");
  zassert_equal(values[1].uintVal, 0, "Second value should match");
}

/**
 * @test The datastoreUtilWrite function must return -EINVAL when the
 * datapoint ID or value count is invalid.
 */
ZTEST(datastore_util_tests, test_write_invalid_datapoint)
{
  Data_t values[2] = {{.uintVal = 1}, {.uintVal = 0}};
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Try to write with invalid datapoint ID (beyond range) */
  result = datastoreUtilWrite(DATAPOINT_BINARY, 100, values, 1, pool);

  zassert_equal(result, -EINVAL,
                "Should return -EINVAL when datapoint ID is invalid");
}

/**
 * @test The datastoreUtilWrite function must return the error code when
 * notification fails.
 */
ZTEST(datastore_util_tests, test_write_notification_failure)
{
  static uint8_t fake_buffer[256];
  Data_t values[1] = {{.uintVal = 5}};
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binary subscriptions */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 1;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[0].callback = mock_subscription_callback;

  /* Set initial value different from write value to trigger notification */
  binaries[0].value.uintVal = 0;

  /* Configure osMemoryPoolAlloc to fail (causes notification to fail) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Write the value - should trigger notification which will fail */
  result = datastoreUtilWrite(DATAPOINT_BINARY, 0, values, 1, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notification fails");
}

/**
 * @test The datastoreUtilWrite function must successfully write datapoint
 * values without notification when there are no subscriptions.
 */
ZTEST(datastore_util_tests, test_write_success_no_notification)
{
  Data_t values[2] = {{.uintVal = 1}, {.uintVal = 0}};
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set initial values */
  binaries[0].value.uintVal = 0;
  binaries[1].value.uintVal = 1;

  /* No subscriptions set up */
  binarySubs.activeCount = 0;

  /* Write the values */
  result = datastoreUtilWrite(DATAPOINT_BINARY, 0, values, 2, pool);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(binaries[0].value.uintVal, 1, "First value should be updated");
  zassert_equal(binaries[1].value.uintVal, 0, "Second value should be updated");
}

/**
 * @test The datastoreUtilWrite function must not trigger notification when
 * writing the same values (no change).
 */
ZTEST(datastore_util_tests, test_write_no_change)
{
  static uint8_t fake_buffer[256];
  Data_t values[2] = {{.uintVal = 5}, {.uintVal = 10}};
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set initial values same as what we'll write */
  binaries[0].value.uintVal = 5;
  binaries[1].value.uintVal = 10;

  /* Set up subscriptions */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 2;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[0].callback = mock_subscription_callback;

  /* Write the same values */
  result = datastoreUtilWrite(DATAPOINT_BINARY, 0, values, 2, pool);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(binaries[0].value.uintVal, 5, "First value should remain unchanged");
  zassert_equal(binaries[1].value.uintVal, 10, "Second value should remain unchanged");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when no change");
  zassert_equal(mock_subscription_callback_fake.call_count, 0,
                "Callback should not be called when no change");
}

/**
 * @test The datastoreUtilWrite function must successfully write datapoint
 * values and trigger notification when subscriptions exist.
 */
ZTEST(datastore_util_tests, test_write_success_with_notification)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  Data_t values[1] = {{.uintVal = 5}};
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Set up binary subscriptions */
  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 1;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[0].callback = mock_subscription_callback;

  /* Set initial value different from write value */
  binaries[0].value.uintVal = 0;

  /* Configure osMemoryPoolAlloc to succeed */
  osMemoryPoolAlloc_fake.return_val = fake_msg;

  /* Configure callback to return success */
  mock_subscription_callback_fake.return_val = 0;

  /* Write the value - should trigger notification */
  result = datastoreUtilWrite(DATAPOINT_BINARY, 0, values, 1, pool);

  zassert_equal(result, 0, "Should return 0 on success");
  zassert_equal(binaries[0].value.uintVal, 5, "Value should be updated");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once for notification");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, pool,
                "osMemoryPoolAlloc should be called with correct pool");
  zassert_equal(mock_subscription_callback_fake.call_count, 1,
                "Callback should be called once for notification");
  zassert_not_null(mock_subscription_callback_fake.arg0_val,
                   "Callback should be called with non-NULL message");
}

/**
 * @test The datastoreUtilNotify function must return -ENOTSUP when the
 * datapoint type is invalid.
 */
ZTEST(datastore_util_tests, test_notify_invalid_type)
{
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  /* Try to notify with invalid datapoint type */
  result = datastoreUtilNotify(DATAPOINT_TYPE_COUNT, 0, pool);

  zassert_equal(result, -ENOTSUP,
                "Should return -ENOTSUP when datapoint type is invalid");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyBinarySubs fails.
 */
ZTEST(datastore_util_tests, test_notify_binary_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 1;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_BINARY, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyBinarySubs fails");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyButtonSubs fails.
 */
ZTEST(datastore_util_tests, test_notify_button_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 1;
  buttonSubs.entries[0].datapointId = 0;
  buttonSubs.entries[0].valCount = 1;
  buttonSubs.entries[0].isPaused = false;
  buttonSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_BUTTON, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyButtonSubs fails");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyFloatSubs fails.
 */
ZTEST(datastore_util_tests, test_notify_float_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 1;
  floatSubs.entries[0].datapointId = 0;
  floatSubs.entries[0].valCount = 1;
  floatSubs.entries[0].isPaused = false;
  floatSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_FLOAT, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyFloatSubs fails");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyIntSubs fails.
 */
ZTEST(datastore_util_tests, test_notify_int_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 1;
  intSubs.entries[0].datapointId = 0;
  intSubs.entries[0].valCount = 1;
  intSubs.entries[0].isPaused = false;
  intSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_INT, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyIntSubs fails");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyMultiStateSubs fails.
 */
ZTEST(datastore_util_tests, test_notify_multi_state_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 1;
  multiStateSubs.entries[0].datapointId = 0;
  multiStateSubs.entries[0].valCount = 1;
  multiStateSubs.entries[0].isPaused = false;
  multiStateSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_MULTI_STATE, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyMultiStateSubs fails");
}

/**
 * @test The datastoreUtilNotify function must return the error code when
 * notifyUintSubs fails.
 */
ZTEST(datastore_util_tests, test_notify_uint_subs_failure)
{
  static uint8_t fake_buffer[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 1;
  uintSubs.entries[0].datapointId = 0;
  uintSubs.entries[0].valCount = 1;
  uintSubs.entries[0].isPaused = false;
  uintSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = datastoreUtilNotify(DATAPOINT_UINT, 0, pool);

  zassert_equal(result, -ENOSPC,
                "Should return -ENOSPC when notifyUintSubs fails");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyBinarySubs.
 */
ZTEST(datastore_util_tests, test_util_notify_binary_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateBinarySubs(5);
  binarySubs.activeCount = 1;
  binarySubs.entries[0].datapointId = 0;
  binarySubs.entries[0].valCount = 1;
  binarySubs.entries[0].isPaused = false;
  binarySubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_BINARY, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyButtonSubs.
 */
ZTEST(datastore_util_tests, test_util_notify_button_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateButtonSubs(5);
  buttonSubs.activeCount = 1;
  buttonSubs.entries[0].datapointId = 0;
  buttonSubs.entries[0].valCount = 1;
  buttonSubs.entries[0].isPaused = false;
  buttonSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_BUTTON, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyFloatSubs.
 */
ZTEST(datastore_util_tests, test_util_notify_float_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateFloatSubs(5);
  floatSubs.activeCount = 1;
  floatSubs.entries[0].datapointId = 0;
  floatSubs.entries[0].valCount = 1;
  floatSubs.entries[0].isPaused = false;
  floatSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_FLOAT, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyIntSubs.
 */
ZTEST(datastore_util_tests, test_util_notify_int_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateIntSubs(5);
  intSubs.activeCount = 1;
  intSubs.entries[0].datapointId = 0;
  intSubs.entries[0].valCount = 1;
  intSubs.entries[0].isPaused = false;
  intSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_INT, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyMultiStateSubs.
 */
ZTEST(datastore_util_tests, test_util_notify_multi_state_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateMultiStateSubs(5);
  multiStateSubs.activeCount = 1;
  multiStateSubs.entries[0].datapointId = 0;
  multiStateSubs.entries[0].valCount = 1;
  multiStateSubs.entries[0].isPaused = false;
  multiStateSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_MULTI_STATE, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

/**
 * @test The datastoreUtilNotify function must successfully dispatch to
 * notifyUintSubs.
 */
ZTEST(datastore_util_tests, test_util_notify_uint_success)
{
  static uint8_t fake_buffer[256];
  static uint8_t fake_msg[256];
  osMemoryPoolId_t pool = (osMemoryPoolId_t)0x1000;
  int result;

  k_malloc_fake.return_val = fake_buffer;
  datastoreUtilAllocateUintSubs(5);
  uintSubs.activeCount = 1;
  uintSubs.entries[0].datapointId = 0;
  uintSubs.entries[0].valCount = 1;
  uintSubs.entries[0].isPaused = false;
  uintSubs.entries[0].callback = mock_subscription_callback;

  osMemoryPoolAlloc_fake.return_val = fake_msg;
  mock_subscription_callback_fake.return_val = 0;

  result = datastoreUtilNotify(DATAPOINT_UINT, 0, pool);

  zassert_equal(result, 0, "Should return 0 on success");
}

ZTEST_SUITE(datastore_util_tests, NULL, util_tests_setup, util_tests_before, NULL, NULL);
