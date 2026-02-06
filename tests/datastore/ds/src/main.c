/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-02-05
 * @brief     Datastore Tests
 *
 *            Unit tests for datastore module functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Include serviceCommon for Data_t and SrvMsgPayload_t */
#include "serviceCommon.h"

/* Include datastore header for types and API */
#include "datastore.h"

/* Define config values needed by datastore.c */
#define CONFIG_ENYA_DATASTORE_LOG_LEVEL LOG_LEVEL_DBG
#define DATASTORE_BUFFER_ALLOC_TIMEOUT 4

/* Prevent util header from being included */
#define DATASTORE_SRV_UTIL

/* Mock function declarations */
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(osStatus_t, osMemoryPoolFree, osMemoryPoolId_t, void *);
FAKE_VALUE_FUNC(osMemoryPoolId_t, osMemoryPoolNew, uint32_t, uint32_t, const osMemoryPoolAttr_t *);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateBinarySubs, size_t);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateButtonSubs, size_t);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateFloatSubs, size_t);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateIntSubs, size_t);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateMultiStateSubs, size_t);
FAKE_VALUE_FUNC(int, datastoreUtilAllocateUintSubs, size_t);
FAKE_VALUE_FUNC(size_t, datastoreUtilCalculateBufferSize, size_t *);
FAKE_VALUE_FUNC(int, datastoreUtilRead, DatapointType_t, uint32_t, size_t, Data_t *);
FAKE_VALUE_FUNC(int, datastoreUtilWrite, DatapointType_t, uint32_t, Data_t *, size_t, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddBinarySub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveBinarySub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetBinarySubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddButtonSub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveButtonSub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetButtonSubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddFloatSub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveFloatSub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetFloatSubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddIntSub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveIntSub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetIntSubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddMultiStateSub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveMultiStateSub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetMultiStateSubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilAddUintSub, DatastoreSubEntry_t *, osMemoryPoolId_t);
FAKE_VALUE_FUNC(int, datastoreUtilRemoveUintSub, DatastoreSubCb_t);
FAKE_VALUE_FUNC(int, datastoreUtilSetUintSubPauseState, DatastoreSubCb_t, bool, osMemoryPoolId_t);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(osMemoryPoolAlloc) \
  FAKE(osMemoryPoolFree) \
  FAKE(osMemoryPoolNew) \
  FAKE(datastoreUtilAllocateBinarySubs) \
  FAKE(datastoreUtilAllocateButtonSubs) \
  FAKE(datastoreUtilAllocateFloatSubs) \
  FAKE(datastoreUtilAllocateIntSubs) \
  FAKE(datastoreUtilAllocateMultiStateSubs) \
  FAKE(datastoreUtilAllocateUintSubs) \
  FAKE(datastoreUtilCalculateBufferSize) \
  FAKE(datastoreUtilRead) \
  FAKE(datastoreUtilWrite) \
  FAKE(datastoreUtilAddBinarySub) \
  FAKE(datastoreUtilRemoveBinarySub) \
  FAKE(datastoreUtilSetBinarySubPauseState) \
  FAKE(datastoreUtilAddButtonSub) \
  FAKE(datastoreUtilRemoveButtonSub) \
  FAKE(datastoreUtilSetButtonSubPauseState) \
  FAKE(datastoreUtilAddFloatSub) \
  FAKE(datastoreUtilRemoveFloatSub) \
  FAKE(datastoreUtilSetFloatSubPauseState) \
  FAKE(datastoreUtilAddIntSub) \
  FAKE(datastoreUtilRemoveIntSub) \
  FAKE(datastoreUtilSetIntSubPauseState) \
  FAKE(datastoreUtilAddMultiStateSub) \
  FAKE(datastoreUtilRemoveMultiStateSub) \
  FAKE(datastoreUtilSetMultiStateSubPauseState) \
  FAKE(datastoreUtilAddUintSub) \
  FAKE(datastoreUtilRemoveUintSub) \
  FAKE(datastoreUtilSetUintSubPauseState)

/* Include the module under test */
#include "datastore.c"

static void *datastore_tests_setup(void)
{
  return NULL;
}

static void datastore_tests_before(void *f)
{
  ARG_UNUSED(f);
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateBinarySubs fails.
 */
ZTEST(datastore_tests, test_init_binary_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure binary subs allocation to fail */
  datastoreUtilAllocateBinarySubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when binary subs allocation fails");

  /* Verify datastoreUtilAllocateBinarySubs was called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");

  /* Verify no other util allocation functions were called */
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 0,
                "datastoreUtilAllocateButtonSubs should not be called when binary subs allocation fails");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 0,
                "datastoreUtilAllocateFloatSubs should not be called when binary subs allocation fails");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 0,
                "datastoreUtilAllocateIntSubs should not be called when binary subs allocation fails");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 0,
                "datastoreUtilAllocateMultiStateSubs should not be called when binary subs allocation fails");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 0,
                "datastoreUtilAllocateUintSubs should not be called when binary subs allocation fails");
}

ZTEST_SUITE(datastore_tests, NULL, datastore_tests_setup, datastore_tests_before, NULL, NULL);
