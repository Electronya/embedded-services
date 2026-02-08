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
#define CONFIG_ENYA_DATASTORE_MSGQ_TIMEOUT 1
#define CONFIG_ENYA_DATASTORE_STACK_SIZE 512
#define DATASTORE_BUFFER_ALLOC_TIMEOUT 4

/* Prevent util header from being included */
#define DATASTORE_SRV_UTIL

/* Wrap k_thread_create to use mock */
#define k_thread_create k_thread_create_mock

/* Wrap k_thread_name_set to use mock */
#define k_thread_name_set k_thread_name_set_mock

/* Mock function declarations */
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(osStatus_t, osMemoryPoolFree, osMemoryPoolId_t, void *);
FAKE_VALUE_FUNC(osMemoryPoolId_t, osMemoryPoolNew, uint32_t, uint32_t, const osMemoryPoolAttr_t *);
FAKE_VALUE_FUNC(k_tid_t, k_thread_create_mock, struct k_thread *, k_thread_stack_t *, size_t,
                k_thread_entry_t, void *, void *, void *, int, uint32_t, k_timeout_t);
FAKE_VALUE_FUNC(int, k_thread_name_set_mock, k_tid_t, const char *);
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
  FAKE(k_thread_create_mock) \
  FAKE(k_thread_name_set_mock) \
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

  /* Purge the datastore queue to ensure clean state between tests */
  k_msgq_purge(&datastoreQueue);
}

/**
 * @test  The run function must handle an unsupported message type gracefully
 *        by logging a warning and not calling any datastore util functions.
 */
ZTEST(datastore_tests, test_run_unsupported_msgtype)
{
  DatastoreMsg_t msg;
  int ret;

  /* Setup message with unsupported message type */
  msg.msgType = DATASTORE_MSG_TYPE_COUNT;  /* Invalid message type */
  msg.datapointType = DATAPOINT_BINARY;
  msg.datapointId = 1;
  msg.valCount = 1;
  msg.payload = NULL;
  msg.response = NULL;

  /* Put message in queue */
  ret = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put message in queue");

  /* Call run function - it will process one message */
  run(NULL, NULL, NULL);

  /* Verify that none of the util functions were called for unsupported message type */
  zassert_equal(datastoreUtilRead_fake.call_count, 0,
                "datastoreUtilRead should not be called for unsupported message type");
  zassert_equal(datastoreUtilWrite_fake.call_count, 0,
                "datastoreUtilWrite should not be called for unsupported message type");
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called for unsupported message type");
}

/**
 * @test  The run function must handle k_msgq_put failure when attempting to
 *        send a response, log an error, and continue processing.
 */
ZTEST(datastore_tests, test_run_response_put_failure)
{
  DatastoreMsg_t msg;
  SrvMsgPayload_t payload;
  int ret;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];

  /* Initialize response queue with capacity of 1 */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Fill the response queue to make k_msgq_put fail */
  int dummy = 0;
  ret = k_msgq_put(&responseQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to fill response queue");

  /* Configure datastoreUtilRead to succeed */
  datastoreUtilRead_fake.return_val = 0;

  /* Setup READ message with response queue */
  msg.msgType = DATASTORE_READ;
  msg.datapointType = DATAPOINT_UINT;
  msg.datapointId = 3;
  msg.valCount = 1;
  msg.payload = &payload;
  msg.response = &responseQueue;

  /* Put message in queue */
  ret = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put message in queue");

  /* Call run function - it will process one message */
  run(NULL, NULL, NULL);

  /* Verify datastoreUtilRead was called (operation succeeded) */
  zassert_equal(datastoreUtilRead_fake.call_count, 1,
                "datastoreUtilRead should be called even if response fails");

  /* Response queue should still be full with the dummy value */
  int response;
  ret = k_msgq_get(&responseQueue, &response, K_NO_WAIT);
  zassert_equal(ret, 0, "Response queue should still have the dummy value");
  zassert_equal(response, dummy, "Response queue should contain original dummy value");

  /* Verify queue is now empty (the run function's response was not added) */
  ret = k_msgq_get(&responseQueue, &response, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "Response queue should be empty after getting dummy value");
}

/**
 * @test  The run function must handle k_msgq_get timeout gracefully when the
 *        queue is empty, continue processing, and must not call any datastore
 *        util functions.
 */
ZTEST(datastore_tests, test_run_kmsgq_get_timeout)
{
  /* Call run function directly with empty queue
   * k_msgq_get will timeout naturally (CONFIG_ENYA_DATASTORE_MSGQ_TIMEOUT=1ms)
   * and return -EAGAIN, testing the timeout handling path */
  run(NULL, NULL, NULL);

  /* Verify that none of the util functions were called when k_msgq_get timed out */
  zassert_equal(datastoreUtilRead_fake.call_count, 0,
                "datastoreUtilRead should not be called on k_msgq_get timeout");
  zassert_equal(datastoreUtilWrite_fake.call_count, 0,
                "datastoreUtilWrite should not be called on k_msgq_get timeout");

  /* Verify osMemoryPoolFree was not called when k_msgq_get timed out */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called on k_msgq_get timeout");
}

/**
 * @test  The run function must successfully process a READ message and call
 *        datastoreUtilRead with the correct parameters.
 */
ZTEST(datastore_tests, test_run_read_success)
{
  DatastoreMsg_t msg;
  SrvMsgPayload_t payload;
  int ret;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure datastoreUtilRead to succeed */
  datastoreUtilRead_fake.return_val = 0;

  /* Setup READ message */
  msg.msgType = DATASTORE_READ;
  msg.datapointType = DATAPOINT_BINARY;
  msg.datapointId = 5;
  msg.valCount = 1;
  msg.payload = &payload;
  msg.response = &responseQueue;

  /* Put message in queue */
  ret = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put message in queue");

  /* Call run function - it will process one message */
  run(NULL, NULL, NULL);

  /* Verify datastoreUtilRead was called with correct parameters */
  zassert_equal(datastoreUtilRead_fake.call_count, 1,
                "datastoreUtilRead should be called once");
  zassert_equal(datastoreUtilRead_fake.arg0_val, DATAPOINT_BINARY,
                "datastoreUtilRead called with wrong datapoint type");
  zassert_equal(datastoreUtilRead_fake.arg1_val, 5,
                "datastoreUtilRead called with wrong datapoint ID");
  zassert_equal(datastoreUtilRead_fake.arg2_val, 1,
                "datastoreUtilRead called with wrong value count");
  zassert_equal(datastoreUtilRead_fake.arg3_val, payload.data,
                "datastoreUtilRead called with wrong data pointer");

  /* Verify response was sent */
  ret = k_msgq_get(&responseQueue, &ret, K_NO_WAIT);
  zassert_equal(ret, 0, "Response should be available in response queue");

  /* Verify osMemoryPoolFree was not called for READ operations */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called for READ operations");
}

/**
 * @test  The run function must successfully process a WRITE message, call
 *        datastoreUtilWrite with the correct parameters, and free the payload
 *        memory pool.
 */
ZTEST(datastore_tests, test_run_write_success)
{
  DatastoreMsg_t msg;
  SrvMsgPayload_t payload;
  int ret;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  osMemoryPoolId_t mockPoolId = (osMemoryPoolId_t)0x12345678;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure datastoreUtilWrite to succeed */
  datastoreUtilWrite_fake.return_val = 0;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup WRITE message */
  msg.msgType = DATASTORE_WRITE;
  msg.datapointType = DATAPOINT_FLOAT;
  msg.datapointId = 10;
  msg.valCount = 2;
  msg.payload = &payload;
  msg.payload->poolId = mockPoolId;
  msg.response = &responseQueue;

  /* Put message in queue */
  ret = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put message in queue");

  /* Call run function - it will process one message */
  run(NULL, NULL, NULL);

  /* Verify datastoreUtilWrite was called with correct parameters */
  zassert_equal(datastoreUtilWrite_fake.call_count, 1,
                "datastoreUtilWrite should be called once");
  zassert_equal(datastoreUtilWrite_fake.arg0_val, DATAPOINT_FLOAT,
                "datastoreUtilWrite called with wrong datapoint type");
  zassert_equal(datastoreUtilWrite_fake.arg1_val, 10,
                "datastoreUtilWrite called with wrong datapoint ID");
  zassert_equal(datastoreUtilWrite_fake.arg2_val, payload.data,
                "datastoreUtilWrite called with wrong data pointer");
  zassert_equal(datastoreUtilWrite_fake.arg3_val, 2,
                "datastoreUtilWrite called with wrong value count");
  zassert_equal(datastoreUtilWrite_fake.arg4_val, bufferPool,
                "datastoreUtilWrite called with wrong buffer pool");

  /* Verify osMemoryPoolFree was called to free the payload */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once for WRITE operations");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, mockPoolId,
                "osMemoryPoolFree called with wrong pool ID");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, &payload,
                "osMemoryPoolFree called with wrong payload pointer");

  /* Verify response was sent */
  ret = k_msgq_get(&responseQueue, &ret, K_NO_WAIT);
  zassert_equal(ret, 0, "Response should be available in response queue");
}

/**
 * @test  The run function must successfully process a WRITE message without a
 *        response queue, call datastoreUtilWrite, and free the payload memory
 *        pool without attempting to send a response.
 */
ZTEST(datastore_tests, test_run_write_no_response)
{
  DatastoreMsg_t msg;
  SrvMsgPayload_t payload;
  int ret;
  osMemoryPoolId_t mockPoolId = (osMemoryPoolId_t)0x87654321;

  /* Configure datastoreUtilWrite to succeed */
  datastoreUtilWrite_fake.return_val = 0;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup WRITE message without response queue */
  msg.msgType = DATASTORE_WRITE;
  msg.datapointType = DATAPOINT_INT;
  msg.datapointId = 7;
  msg.valCount = 1;
  msg.payload = &payload;
  msg.payload->poolId = mockPoolId;
  msg.response = NULL;  /* No response queue */

  /* Put message in queue */
  ret = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put message in queue");

  /* Call run function - it will process one message */
  run(NULL, NULL, NULL);

  /* Verify datastoreUtilWrite was called with correct parameters */
  zassert_equal(datastoreUtilWrite_fake.call_count, 1,
                "datastoreUtilWrite should be called once");
  zassert_equal(datastoreUtilWrite_fake.arg0_val, DATAPOINT_INT,
                "datastoreUtilWrite called with wrong datapoint type");
  zassert_equal(datastoreUtilWrite_fake.arg1_val, 7,
                "datastoreUtilWrite called with wrong datapoint ID");
  zassert_equal(datastoreUtilWrite_fake.arg2_val, payload.data,
                "datastoreUtilWrite called with wrong data pointer");
  zassert_equal(datastoreUtilWrite_fake.arg3_val, 1,
                "datastoreUtilWrite called with wrong value count");
  zassert_equal(datastoreUtilWrite_fake.arg4_val, bufferPool,
                "datastoreUtilWrite called with wrong buffer pool");

  /* Verify osMemoryPoolFree was called to free the payload */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once for WRITE operations");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, mockPoolId,
                "osMemoryPoolFree called with wrong pool ID");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, &payload,
                "osMemoryPoolFree called with wrong payload pointer");

  /* No response should be sent since msg.response is NULL */
  /* This is implicitly verified by the test completing successfully */
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

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when binary subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when binary subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when binary subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateButtonSubs fails.
 */
ZTEST(datastore_tests, test_init_button_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure binary subs allocation to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;

  /* Configure button subs allocation to fail */
  datastoreUtilAllocateButtonSubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when button subs allocation fails");

  /* Verify datastoreUtilAllocateBinarySubs was called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");

  /* Verify datastoreUtilAllocateButtonSubs was called */
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");

  /* Verify subsequent allocation functions were not called */
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 0,
                "datastoreUtilAllocateFloatSubs should not be called when button subs allocation fails");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 0,
                "datastoreUtilAllocateIntSubs should not be called when button subs allocation fails");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 0,
                "datastoreUtilAllocateMultiStateSubs should not be called when button subs allocation fails");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 0,
                "datastoreUtilAllocateUintSubs should not be called when button subs allocation fails");

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when button subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when button subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when button subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateFloatSubs fails.
 */
ZTEST(datastore_tests, test_init_float_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure binary and button subs allocation to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;

  /* Configure float subs allocation to fail */
  datastoreUtilAllocateFloatSubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when float subs allocation fails");

  /* Verify previous allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");

  /* Verify datastoreUtilAllocateFloatSubs was called */
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");

  /* Verify subsequent allocation functions were not called */
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 0,
                "datastoreUtilAllocateIntSubs should not be called when float subs allocation fails");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 0,
                "datastoreUtilAllocateMultiStateSubs should not be called when float subs allocation fails");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 0,
                "datastoreUtilAllocateUintSubs should not be called when float subs allocation fails");

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when float subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when float subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when float subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateIntSubs fails.
 */
ZTEST(datastore_tests, test_init_int_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure binary, button, and float subs allocation to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;

  /* Configure int subs allocation to fail */
  datastoreUtilAllocateIntSubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when int subs allocation fails");

  /* Verify previous allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");

  /* Verify datastoreUtilAllocateIntSubs was called */
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");

  /* Verify subsequent allocation functions were not called */
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 0,
                "datastoreUtilAllocateMultiStateSubs should not be called when int subs allocation fails");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 0,
                "datastoreUtilAllocateUintSubs should not be called when int subs allocation fails");

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when int subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when int subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when int subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateMultiStateSubs fails.
 */
ZTEST(datastore_tests, test_init_multi_state_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure binary, button, float, and int subs allocation to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;
  datastoreUtilAllocateIntSubs_fake.return_val = 0;

  /* Configure multi-state subs allocation to fail */
  datastoreUtilAllocateMultiStateSubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when multi-state subs allocation fails");

  /* Verify previous allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");

  /* Verify datastoreUtilAllocateMultiStateSubs was called */
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 1,
                "datastoreUtilAllocateMultiStateSubs should be called once");

  /* Verify subsequent allocation functions were not called */
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 0,
                "datastoreUtilAllocateUintSubs should not be called when multi-state subs allocation fails");

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when multi-state subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when multi-state subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when multi-state subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        datastoreUtilAllocateUintSubs fails.
 */
ZTEST(datastore_tests, test_init_uint_subs_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};

  /* Configure all previous subs allocation to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;
  datastoreUtilAllocateIntSubs_fake.return_val = 0;
  datastoreUtilAllocateMultiStateSubs_fake.return_val = 0;

  /* Configure uint subs allocation to fail */
  datastoreUtilAllocateUintSubs_fake.return_val = -ENOMEM;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed */
  zassert_equal(ret, -ENOMEM, "datastoreInit should return -ENOMEM when uint subs allocation fails");

  /* Verify all allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 1,
                "datastoreUtilAllocateMultiStateSubs should be called once");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 1,
                "datastoreUtilAllocateUintSubs should be called once");

  /* Verify buffer pool and thread were not created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 0,
                "osMemoryPoolNew should not be called when uint subs allocation fails");
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when uint subs allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when uint subs allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        osMemoryPoolNew fails to create the buffer pool.
 */
ZTEST(datastore_tests, test_init_buffer_pool_alloc_failure)
{
  k_tid_t threadId;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};
  size_t expectedBufferSize = 100;

  /* Configure all subscription allocations to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;
  datastoreUtilAllocateIntSubs_fake.return_val = 0;
  datastoreUtilAllocateMultiStateSubs_fake.return_val = 0;
  datastoreUtilAllocateUintSubs_fake.return_val = 0;

  /* Configure buffer size calculation to succeed */
  datastoreUtilCalculateBufferSize_fake.return_val = expectedBufferSize;

  /* Configure osMemoryPoolNew to fail (return NULL) */
  osMemoryPoolNew_fake.return_val = NULL;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed with -ENOSPC */
  zassert_equal(ret, -ENOSPC, "datastoreInit should return -ENOSPC when buffer pool allocation fails");

  /* Verify all subscription allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 1,
                "datastoreUtilAllocateMultiStateSubs should be called once");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 1,
                "datastoreUtilAllocateUintSubs should be called once");

  /* Verify osMemoryPoolNew was called */
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
  zassert_equal(osMemoryPoolNew_fake.arg1_val, expectedBufferSize,
                "osMemoryPoolNew should be called with correct buffer size");

  /* Verify thread creation and naming were not called */
  zassert_equal(k_thread_create_mock_fake.call_count, 0,
                "k_thread_create should not be called when buffer pool allocation fails");
  zassert_equal(k_thread_name_set_mock_fake.call_count, 0,
                "k_thread_name_set should not be called when buffer pool allocation fails");
}

/**
 * @test  The datastoreInit function must return an error when
 *        k_thread_name_set fails.
 */
ZTEST(datastore_tests, test_init_thread_name_set_failure)
{
  k_tid_t threadId;
  k_tid_t mockThreadId = (k_tid_t)0x12345678;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 1, 1, 1, 1, 1};
  size_t expectedBufferSize = 100;

  /* Configure all subscription allocations to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;
  datastoreUtilAllocateIntSubs_fake.return_val = 0;
  datastoreUtilAllocateMultiStateSubs_fake.return_val = 0;
  datastoreUtilAllocateUintSubs_fake.return_val = 0;

  /* Configure buffer size calculation and pool creation to succeed */
  datastoreUtilCalculateBufferSize_fake.return_val = expectedBufferSize;
  osMemoryPoolNew_fake.return_val = (osMemoryPoolId_t)0xABCDEF00;

  /* Configure thread creation to succeed */
  k_thread_create_mock_fake.return_val = mockThreadId;

  /* Configure thread name set to fail */
  k_thread_name_set_mock_fake.return_val = -EINVAL;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, 5, &threadId);

  /* Verify initialization failed with k_thread_name_set error */
  zassert_equal(ret, -EINVAL, "datastoreInit should return -EINVAL when k_thread_name_set fails");

  /* Verify all allocation functions were called */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 1,
                "datastoreUtilAllocateMultiStateSubs should be called once");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 1,
                "datastoreUtilAllocateUintSubs should be called once");

  /* Verify buffer pool was created */
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
  zassert_equal(osMemoryPoolNew_fake.arg1_val, expectedBufferSize,
                "osMemoryPoolNew should be called with correct buffer size");

  /* Verify thread was created */
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");

  /* Verify k_thread_name_set was called and failed */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, mockThreadId,
                "k_thread_name_set should be called with the thread ID returned by k_thread_create");
}

/**
 * @test  The datastoreInit function must successfully initialize when all
 *        operations succeed.
 */
ZTEST(datastore_tests, test_init_success)
{
  k_tid_t threadId;
  k_tid_t mockThreadId = (k_tid_t)0x12345678;
  osMemoryPoolId_t mockPoolId = (osMemoryPoolId_t)0xABCDEF00;
  int ret;
  size_t maxSubs[DATAPOINT_TYPE_COUNT] = {1, 2, 3, 4, 5, 6};
  size_t expectedBufferSize = 256;
  uint32_t priority = 7;

  /* Configure all subscription allocations to succeed */
  datastoreUtilAllocateBinarySubs_fake.return_val = 0;
  datastoreUtilAllocateButtonSubs_fake.return_val = 0;
  datastoreUtilAllocateFloatSubs_fake.return_val = 0;
  datastoreUtilAllocateIntSubs_fake.return_val = 0;
  datastoreUtilAllocateMultiStateSubs_fake.return_val = 0;
  datastoreUtilAllocateUintSubs_fake.return_val = 0;

  /* Configure buffer size calculation and pool creation to succeed */
  datastoreUtilCalculateBufferSize_fake.return_val = expectedBufferSize;
  osMemoryPoolNew_fake.return_val = mockPoolId;

  /* Configure thread creation and naming to succeed */
  k_thread_create_mock_fake.return_val = mockThreadId;
  k_thread_name_set_mock_fake.return_val = 0;

  /* Initialize the datastore */
  ret = datastoreInit(maxSubs, priority, &threadId);

  /* Verify initialization succeeded */
  zassert_equal(ret, 0, "datastoreInit should return 0 on success");
  zassert_equal(threadId, mockThreadId, "threadId should be set to the value returned by k_thread_create");

  /* Verify all subscription allocations were called with correct parameters */
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.call_count, 1,
                "datastoreUtilAllocateBinarySubs should be called once");
  zassert_equal(datastoreUtilAllocateBinarySubs_fake.arg0_val, maxSubs[DATAPOINT_BINARY],
                "datastoreUtilAllocateBinarySubs should be called with correct maxSubs");

  zassert_equal(datastoreUtilAllocateButtonSubs_fake.call_count, 1,
                "datastoreUtilAllocateButtonSubs should be called once");
  zassert_equal(datastoreUtilAllocateButtonSubs_fake.arg0_val, maxSubs[DATAPOINT_BUTTON],
                "datastoreUtilAllocateButtonSubs should be called with correct maxSubs");

  zassert_equal(datastoreUtilAllocateFloatSubs_fake.call_count, 1,
                "datastoreUtilAllocateFloatSubs should be called once");
  zassert_equal(datastoreUtilAllocateFloatSubs_fake.arg0_val, maxSubs[DATAPOINT_FLOAT],
                "datastoreUtilAllocateFloatSubs should be called with correct maxSubs");

  zassert_equal(datastoreUtilAllocateIntSubs_fake.call_count, 1,
                "datastoreUtilAllocateIntSubs should be called once");
  zassert_equal(datastoreUtilAllocateIntSubs_fake.arg0_val, maxSubs[DATAPOINT_INT],
                "datastoreUtilAllocateIntSubs should be called with correct maxSubs");

  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.call_count, 1,
                "datastoreUtilAllocateMultiStateSubs should be called once");
  zassert_equal(datastoreUtilAllocateMultiStateSubs_fake.arg0_val, maxSubs[DATAPOINT_MULTI_STATE],
                "datastoreUtilAllocateMultiStateSubs should be called with correct maxSubs");

  zassert_equal(datastoreUtilAllocateUintSubs_fake.call_count, 1,
                "datastoreUtilAllocateUintSubs should be called once");
  zassert_equal(datastoreUtilAllocateUintSubs_fake.arg0_val, maxSubs[DATAPOINT_UINT],
                "datastoreUtilAllocateUintSubs should be called with correct maxSubs");

  /* Verify buffer pool was created with correct parameters */
  zassert_equal(datastoreUtilCalculateBufferSize_fake.call_count, 1,
                "datastoreUtilCalculateBufferSize should be called once");
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
  zassert_equal(osMemoryPoolNew_fake.arg0_val, DATASTORE_BUFFER_COUNT,
                "osMemoryPoolNew should be called with DATASTORE_BUFFER_COUNT");
  zassert_equal(osMemoryPoolNew_fake.arg1_val, expectedBufferSize,
                "osMemoryPoolNew should be called with buffer size from datastoreUtilCalculateBufferSize");

  /* Verify thread was created with correct parameters */
  zassert_equal(k_thread_create_mock_fake.call_count, 1,
                "k_thread_create should be called once");
  zassert_equal(k_thread_create_mock_fake.arg2_val, CONFIG_ENYA_DATASTORE_STACK_SIZE,
                "k_thread_create should be called with CONFIG_ENYA_DATASTORE_STACK_SIZE");
  zassert_equal(k_thread_create_mock_fake.arg3_val, run,
                "k_thread_create should be called with run function");

  /* Verify thread name was set */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, mockThreadId,
                "k_thread_name_set should be called with thread ID from k_thread_create");
}

/**
 * @test  The datastoreRead function must return an error when buffer allocation
 *        fails (osMemoryPoolAlloc returns NULL).
 */
ZTEST(datastore_tests, test_read_buffer_alloc_failure)
{
  DatapointType_t datapointType = DATAPOINT_FLOAT;
  uint32_t datapointId = 5;
  size_t valCount = 2;
  Data_t values[2];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreRead */
  ret = datastoreRead(datapointType, datapointId, valCount, &responseQueue, values);

  /* Verify function returned -ENOSPC */
  zassert_equal(ret, -ENOSPC, "datastoreRead should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, bufferPool,
                "osMemoryPoolAlloc should be called with bufferPool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");

  /* Verify osMemoryPoolFree was not called */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when buffer allocation fails");
}

/**
 * @test  The datastoreRead function must return an error when k_msgq_put fails
 *        (datastore queue is full) and free the allocated buffer.
 */
ZTEST(datastore_tests, test_read_msgq_put_failure)
{
  DatapointType_t datapointType = DATAPOINT_INT;
  uint32_t datapointId = 3;
  size_t valCount = 1;
  Data_t values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  SrvMsgPayload_t mockPayload;
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = &mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Fill the datastore queue to make k_msgq_put fail */
  DatastoreMsg_t dummyMsg;
  for (int i = 0; i < DATASTORE_MSG_COUNT; i++) {
    ret = k_msgq_put(&datastoreQueue, &dummyMsg, K_NO_WAIT);
    zassert_equal(ret, 0, "Failed to fill datastore queue");
  }

  /* Call datastoreRead - k_msgq_put should fail */
  ret = datastoreRead(datapointType, datapointId, valCount, &responseQueue, values);

  /* Verify function returned error from k_msgq_put */
  zassert_true(ret < 0, "datastoreRead should return error when k_msgq_put fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify osMemoryPoolFree was called to clean up the allocated buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer when k_msgq_put fails");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, &mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreRead function must return an error when k_msgq_get times
 *        out waiting for a response and free the allocated buffer.
 */
ZTEST(datastore_tests, test_read_response_timeout)
{
  DatapointType_t datapointType = DATAPOINT_BINARY;
  uint32_t datapointId = 1;
  size_t valCount = 1;
  Data_t values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  SrvMsgPayload_t mockPayload;
  int ret;

  /* Initialize response queue (empty - no response will be sent) */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = &mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Call datastoreRead - k_msgq_get should timeout on empty response queue */
  ret = datastoreRead(datapointType, datapointId, valCount, &responseQueue, values);

  /* Verify function returned timeout error from k_msgq_get */
  zassert_equal(ret, -EAGAIN, "datastoreRead should return -EAGAIN when response times out");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_READ, "Message type should be DATASTORE_READ");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");

  /* Verify osMemoryPoolFree was called to clean up the allocated buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer when response times out");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, &mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreRead function must handle operation failure gracefully
 *        when communication succeeds but the datastore operation returns an error.
 */
ZTEST(datastore_tests, test_read_operation_failure)
{
  DatapointType_t datapointType = DATAPOINT_UINT;
  uint32_t datapointId = 8;
  size_t valCount = 3;
  Data_t values[3] = {{.uintVal = 0xFFFFFFFF}, {.uintVal = 0xFFFFFFFF}, {.uintVal = 0xFFFFFFFF}};  /* Initialize to detect if copied */
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  SrvMsgPayload_t mockPayload;
  int ret;
  int errorStatus = -EINVAL;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = &mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put error status in response queue (simulating operation failure) */
  ret = k_msgq_put(&responseQueue, &errorStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put error status in response queue");

  /* Call datastoreRead - communication succeeds but operation fails */
  ret = datastoreRead(datapointType, datapointId, valCount, &responseQueue, values);

  /* Verify function returned the error status from operation */
  zassert_equal(ret, errorStatus, "datastoreRead should return error status from operation");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");

  /* Verify values were NOT copied (because resStatus != 0) */
  zassert_equal(values[0].uintVal, 0xFFFFFFFF, "values should not be modified when operation fails");
  zassert_equal(values[1].uintVal, 0xFFFFFFFF, "values should not be modified when operation fails");
  zassert_equal(values[2].uintVal, 0xFFFFFFFF, "values should not be modified when operation fails");

  /* Verify osMemoryPoolFree was called to free the buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, &mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreRead function must successfully read data when all
 *        operations succeed.
 */
ZTEST(datastore_tests, test_read_success)
{
  DatapointType_t datapointType = DATAPOINT_UINT;
  uint32_t datapointId = 12;
  size_t valCount = 2;
  Data_t values[2] = {{.uintVal = 0}, {.uintVal = 0}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  /* Allocate buffer with space for payload header + data array */
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].uintVal = 0x12345678;
  payloadData[1].uintVal = 0xABCDEF00;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreRead - should succeed */
  ret = datastoreRead(datapointType, datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreRead should return 0 on success, got %d", ret);

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_READ, "Message type should be DATASTORE_READ");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");

  /* Verify data was copied to values array */
  zassert_equal(values[0].uintVal, 0x12345678, "First value should be copied from payload, got 0x%08x", values[0].uintVal);
  zassert_equal(values[1].uintVal, 0xABCDEF00, "Second value should be copied from payload, got 0x%08x", values[1].uintVal);

  /* Verify osMemoryPoolFree was called to free the buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreWrite function must return an error when buffer allocation
 *        fails (osMemoryPoolAlloc returns NULL).
 */
ZTEST(datastore_tests, test_write_buffer_alloc_failure)
{
  DatapointType_t datapointType = DATAPOINT_UINT;
  uint32_t datapointId = 10;
  size_t valCount = 2;
  Data_t values[2] = {{.uintVal = 0x11111111}, {.uintVal = 0x22222222}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWrite */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, &responseQueue);

  /* Verify function returned -ENOSPC */
  zassert_equal(ret, -ENOSPC, "datastoreWrite should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg0_val, bufferPool,
                "osMemoryPoolAlloc should be called with bufferPool");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, DATASTORE_BUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with DATASTORE_BUFFER_ALLOC_TIMEOUT");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");

  /* Verify osMemoryPoolFree was not called */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when buffer allocation fails");
}

/**
 * @test  The datastoreWrite function must return an error when k_msgq_put fails
 *        (datastore queue is full) and free the allocated buffer.
 */
ZTEST(datastore_tests, test_write_msgq_put_failure)
{
  DatapointType_t datapointType = DATAPOINT_INT;
  uint32_t datapointId = 7;
  size_t valCount = 1;
  Data_t values[1] = {{.intVal = -42}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Fill the datastore queue to make k_msgq_put fail */
  DatastoreMsg_t dummyMsg;
  for (int i = 0; i < DATASTORE_MSG_COUNT; i++) {
    ret = k_msgq_put(&datastoreQueue, &dummyMsg, K_NO_WAIT);
    zassert_equal(ret, 0, "Failed to fill datastore queue");
  }

  /* Call datastoreWrite - k_msgq_put should fail */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, &responseQueue);

  /* Verify function returned error from k_msgq_put */
  zassert_true(ret < 0, "datastoreWrite should return error when k_msgq_put fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify osMemoryPoolFree was called to clean up the allocated buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer when k_msgq_put fails");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreWrite function must return an error when k_msgq_get times
 *        out waiting for a response and free the allocated buffer.
 */
ZTEST(datastore_tests, test_write_response_timeout)
{
  DatapointType_t datapointType = DATAPOINT_FLOAT;
  uint32_t datapointId = 15;
  size_t valCount = 2;
  Data_t values[2] = {{.floatVal = 3.14f}, {.floatVal = 2.71f}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  int ret;

  /* Initialize response queue (empty - no response will be sent) */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Call datastoreWrite - k_msgq_get should timeout on empty response queue */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, &responseQueue);

  /* Verify function returned timeout error from k_msgq_get */
  zassert_equal(ret, -EAGAIN, "datastoreWrite should return -EAGAIN when response times out");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");

  /* Verify osMemoryPoolFree was called to clean up the allocated buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer when response times out");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, bufferPool,
                "osMemoryPoolFree should be called with bufferPool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, mockPayload,
                "osMemoryPoolFree should be called with the allocated payload");
}

/**
 * @test  The datastoreWrite function must successfully write data when no
 *        response is expected (response queue is NULL).
 */
ZTEST(datastore_tests, test_write_success_no_response)
{
  DatapointType_t datapointType = DATAPOINT_UINT;
  uint32_t datapointId = 20;
  size_t valCount = 3;
  Data_t values[3] = {{.uintVal = 0xAAAAAAAA}, {.uintVal = 0xBBBBBBBB}, {.uintVal = 0xCCCCCCCC}};
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Call datastoreWrite with NULL response queue */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, NULL);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWrite should return 0 on success with no response");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, NULL, "Response queue should be NULL");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].uintVal, 0xAAAAAAAA, "First value should be copied to payload");
  zassert_equal(payloadData[1].uintVal, 0xBBBBBBBB, "Second value should be copied to payload");
  zassert_equal(payloadData[2].uintVal, 0xCCCCCCCC, "Third value should be copied to payload");

  /* Verify osMemoryPoolFree was NOT called (buffer ownership transferred to queue) */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when no response is expected");
}

/**
 * @test  The datastoreWrite function must handle operation failure gracefully
 *        when communication succeeds but the datastore operation returns an error.
 */
ZTEST(datastore_tests, test_write_operation_failure)
{
  DatapointType_t datapointType = DATAPOINT_INT;
  uint32_t datapointId = 25;
  size_t valCount = 2;
  Data_t values[2] = {{.intVal = -100}, {.intVal = 200}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int errorStatus = -EINVAL;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put error status in response queue (simulating operation failure) */
  ret = k_msgq_put(&responseQueue, &errorStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put error status in response queue");

  /* Call datastoreWrite - communication succeeds but operation fails */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, &responseQueue);

  /* Verify function returned the error status from operation */
  zassert_equal(ret, errorStatus, "datastoreWrite should return error status from operation");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].intVal, -100, "First value should be copied to payload");
  zassert_equal(payloadData[1].intVal, 200, "Second value should be copied to payload");

  /* Verify osMemoryPoolFree was NOT called (no free in datastoreWrite when response expected) */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called in datastoreWrite when operation fails with response");
}

/**
 * @test  The datastoreWrite function must successfully write data when a
 *        response is expected and the operation succeeds.
 */
ZTEST(datastore_tests, test_write_success)
{
  DatapointType_t datapointType = DATAPOINT_FLOAT;
  uint32_t datapointId = 30;
  size_t valCount = 2;
  Data_t values[2] = {{.floatVal = 1.23f}, {.floatVal = 4.56f}};
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWrite - should succeed */
  ret = datastoreWrite(datapointType, datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWrite should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, datapointType, "Message should have correct datapoint type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_within(payloadData[0].floatVal, 1.23f, 0.001f, "First value should be copied to payload");
  zassert_within(payloadData[1].floatVal, 4.56f, 0.001f, "Second value should be copied to payload");

  /* Verify osMemoryPoolFree was NOT called (buffer ownership transferred to queue) */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called in datastoreWrite when operation succeeds with response");
}

/**
 * @test  The datastoreSubscribeBinary function must return an error when
 *        datastoreUtilAddBinarySub fails.
 */
ZTEST(datastore_tests, test_subscribe_binary_failure)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddBinarySub to fail */
  datastoreUtilAddBinarySub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeBinary */
  ret = datastoreSubscribeBinary(&sub);

  /* Verify function returned the error from datastoreUtilAddBinarySub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeBinary should return error from datastoreUtilAddBinarySub");

  /* Verify datastoreUtilAddBinarySub was called with correct parameters */
  zassert_equal(datastoreUtilAddBinarySub_fake.call_count, 1,
                "datastoreUtilAddBinarySub should be called once");
  zassert_equal(datastoreUtilAddBinarySub_fake.arg0_val, &sub,
                "datastoreUtilAddBinarySub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddBinarySub_fake.arg1_val, bufferPool,
                "datastoreUtilAddBinarySub should be called with bufferPool");
}

/**
 * @test  The datastoreSubscribeBinary function must successfully subscribe
 *        when datastoreUtilAddBinarySub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_binary_success)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddBinarySub to succeed */
  datastoreUtilAddBinarySub_fake.return_val = 0;

  /* Call datastoreSubscribeBinary */
  ret = datastoreSubscribeBinary(&sub);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeBinary should return 0 on success");

  /* Verify datastoreUtilAddBinarySub was called with correct parameters */
  zassert_equal(datastoreUtilAddBinarySub_fake.call_count, 1,
                "datastoreUtilAddBinarySub should be called once");
  zassert_equal(datastoreUtilAddBinarySub_fake.arg0_val, &sub,
                "datastoreUtilAddBinarySub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddBinarySub_fake.arg1_val, bufferPool,
                "datastoreUtilAddBinarySub should be called with bufferPool");
}

/**
 * @test  The datastoreUnsubscribeBinary function must return an error when
 *        datastoreUtilRemoveBinarySub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_binary_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x12345678;
  int ret;

  /* Configure datastoreUtilRemoveBinarySub to fail */
  datastoreUtilRemoveBinarySub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeBinary */
  ret = datastoreUnsubscribeBinary(callback);

  /* Verify function returned the error from datastoreUtilRemoveBinarySub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeBinary should return error from datastoreUtilRemoveBinarySub");

  /* Verify datastoreUtilRemoveBinarySub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveBinarySub_fake.call_count, 1,
                "datastoreUtilRemoveBinarySub should be called once");
  zassert_equal(datastoreUtilRemoveBinarySub_fake.arg0_val, callback,
                "datastoreUtilRemoveBinarySub should be called with the callback");
}

/**
 * @test  The datastoreUnsubscribeBinary function must successfully unsubscribe
 *        when datastoreUtilRemoveBinarySub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_binary_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x87654321;
  int ret;

  /* Configure datastoreUtilRemoveBinarySub to succeed */
  datastoreUtilRemoveBinarySub_fake.return_val = 0;

  /* Call datastoreUnsubscribeBinary */
  ret = datastoreUnsubscribeBinary(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeBinary should return 0 on success");

  /* Verify datastoreUtilRemoveBinarySub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveBinarySub_fake.call_count, 1,
                "datastoreUtilRemoveBinarySub should be called once");
  zassert_equal(datastoreUtilRemoveBinarySub_fake.arg0_val, callback,
                "datastoreUtilRemoveBinarySub should be called with the callback");
}

/**
 * @test  The datastorePauseSubBinary function must return an error when
 *        datastoreUtilSetBinarySubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_binary_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xABCDEF00;
  int ret;

  /* Configure datastoreUtilSetBinarySubPauseState to fail */
  datastoreUtilSetBinarySubPauseState_fake.return_val = -ENOENT;

  /* Call datastorePauseSubBinary */
  ret = datastorePauseSubBinary(callback);

  /* Verify function returned the error from datastoreUtilSetBinarySubPauseState */
  zassert_equal(ret, -ENOENT, "datastorePauseSubBinary should return error from datastoreUtilSetBinarySubPauseState");

  /* Verify datastoreUtilSetBinarySubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.call_count, 1,
                "datastoreUtilSetBinarySubPauseState should be called once");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetBinarySubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg1_val, true,
                "datastoreUtilSetBinarySubPauseState should be called with true (pause)");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetBinarySubPauseState should be called with bufferPool");
}

/**
 * @test  The datastorePauseSubBinary function must successfully pause subscription
 *        when datastoreUtilSetBinarySubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_binary_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x11223344;
  int ret;

  /* Configure datastoreUtilSetBinarySubPauseState to succeed */
  datastoreUtilSetBinarySubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubBinary */
  ret = datastorePauseSubBinary(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubBinary should return 0 on success");

  /* Verify datastoreUtilSetBinarySubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.call_count, 1,
                "datastoreUtilSetBinarySubPauseState should be called once");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetBinarySubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg1_val, true,
                "datastoreUtilSetBinarySubPauseState should be called with true (pause)");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetBinarySubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubBinary function must return an error when
 *        datastoreUtilSetBinarySubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_binary_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x55667788;
  int ret;

  /* Configure datastoreUtilSetBinarySubPauseState to fail */
  datastoreUtilSetBinarySubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubBinary */
  ret = datastoreUnpauseSubBinary(callback);

  /* Verify function returned the error from datastoreUtilSetBinarySubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubBinary should return error from datastoreUtilSetBinarySubPauseState");

  /* Verify datastoreUtilSetBinarySubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.call_count, 1,
                "datastoreUtilSetBinarySubPauseState should be called once");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetBinarySubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg1_val, false,
                "datastoreUtilSetBinarySubPauseState should be called with false (unpause)");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetBinarySubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubBinary function must successfully unpause subscription
 *        when datastoreUtilSetBinarySubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_binary_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x99AABBCC;
  int ret;

  /* Configure datastoreUtilSetBinarySubPauseState to succeed */
  datastoreUtilSetBinarySubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubBinary */
  ret = datastoreUnpauseSubBinary(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubBinary should return 0 on success");

  /* Verify datastoreUtilSetBinarySubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.call_count, 1,
                "datastoreUtilSetBinarySubPauseState should be called once");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetBinarySubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg1_val, false,
                "datastoreUtilSetBinarySubPauseState should be called with false (unpause)");
  zassert_equal(datastoreUtilSetBinarySubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetBinarySubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreReadBinary function must return -EINVAL when values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_binary_invalid_values)
{
  uint32_t datapointId = 10;
  size_t valCount = 5;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadBinary with NULL values */
  ret = datastoreReadBinary(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadBinary should return -EINVAL when values is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadBinary function must return -EINVAL when valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_read_binary_invalid_valcount)
{
  uint32_t datapointId = 15;
  size_t valCount = 0;
  bool values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadBinary with valCount = 0 */
  ret = datastoreReadBinary(datapointId, valCount, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadBinary should return -EINVAL when valCount is 0");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadBinary function must return -EINVAL when response
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_binary_invalid_response)
{
  uint32_t datapointId = 16;
  size_t valCount = 3;
  bool values[3];
  int ret;

  /* Call datastoreReadBinary with NULL response */
  ret = datastoreReadBinary(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadBinary should return -EINVAL when response is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadBinary function must return an error when the
 *        underlying datastoreRead operation fails.
 */
ZTEST(datastore_tests, test_read_binary_operation_failure)
{
  uint32_t datapointId = 18;
  size_t valCount = 2;
  bool values[2];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadBinary - should fail due to buffer allocation failure */
  ret = datastoreReadBinary(datapointId, valCount, &responseQueue, values);

  /* Verify function returned -ENOSPC (buffer allocation error) */
  zassert_equal(ret, -ENOSPC, "datastoreReadBinary should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");
}

/**
 * @test  The datastoreReadBinary function must successfully read binary data
 *        when all parameters are valid and the operation succeeds.
 */
ZTEST(datastore_tests, test_read_binary_success)
{
  uint32_t datapointId = 20;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to bool */
  Data_t valueStorage[3];
  bool *values = (bool *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (bool values stored as uintVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].uintVal = 1;  /* true */
  payloadData[1].uintVal = 0;  /* false */
  payloadData[2].uintVal = 1;  /* true */

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadBinary - should succeed */
  ret = datastoreReadBinary(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadBinary should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_READ, "Message type should be DATASTORE_READ");
  zassert_equal(msg.datapointType, DATAPOINT_BINARY, "Message should have DATAPOINT_BINARY type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");

  /* Verify data was copied - access as Data_t to read the full values */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].uintVal, 1, "First value should be 1 (true)");
  zassert_equal(dataValues[1].uintVal, 0, "Second value should be 0 (false)");
  zassert_equal(dataValues[2].uintVal, 1, "Third value should be 1 (true)");

  /* Verify osMemoryPoolFree was called to free the buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer");
}

/**
 * @test  The datastoreWriteBinary function must return -EINVAL when values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_write_binary_invalid_values)
{
  uint32_t datapointId = 25;
  size_t valCount = 4;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteBinary with NULL values */
  ret = datastoreWriteBinary(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteBinary should return -EINVAL when values is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreWriteBinary function must return -EINVAL when valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_write_binary_invalid_valcount)
{
  uint32_t datapointId = 30;
  size_t valCount = 0;
  bool values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteBinary with valCount = 0 */
  ret = datastoreWriteBinary(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteBinary should return -EINVAL when valCount is 0");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreWriteBinary function must return an error when the
 *        underlying datastoreWrite operation fails.
 */
ZTEST(datastore_tests, test_write_binary_operation_failure)
{
  uint32_t datapointId = 32;
  size_t valCount = 2;
  bool values[2];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteBinary - should fail due to buffer allocation failure */
  ret = datastoreWriteBinary(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -ENOSPC (buffer allocation error) */
  zassert_equal(ret, -ENOSPC, "datastoreWriteBinary should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");
}

/**
 * @test  The datastoreWriteBinary function must successfully write binary data
 *        when all parameters are valid and the operation succeeds.
 */
ZTEST(datastore_tests, test_write_binary_success)
{
  uint32_t datapointId = 35;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast from bool */
  Data_t valueStorage[3] = {{.uintVal = 1}, {.uintVal = 0}, {.uintVal = 1}};
  bool *values = (bool *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteBinary - should succeed */
  ret = datastoreWriteBinary(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteBinary should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_BINARY, "Message should have DATAPOINT_BINARY type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].uintVal, 1, "First value should be 1 (true)");
  zassert_equal(payloadData[1].uintVal, 0, "Second value should be 0 (false)");
  zassert_equal(payloadData[2].uintVal, 1, "Third value should be 1 (true)");

  /* Verify osMemoryPoolFree was NOT called (buffer ownership transferred to queue) */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when response is expected");
}

/**
 * @test  The datastoreSubscribeButton function must return an error when
 *        datastoreUtilAddButtonSub fails.
 */
ZTEST(datastore_tests, test_subscribe_button_failure)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddButtonSub to fail */
  datastoreUtilAddButtonSub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeButton */
  ret = datastoreSubscribeButton(&sub);

  /* Verify function returned the error from datastoreUtilAddButtonSub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeButton should return error from datastoreUtilAddButtonSub");

  /* Verify datastoreUtilAddButtonSub was called with correct parameters */
  zassert_equal(datastoreUtilAddButtonSub_fake.call_count, 1,
                "datastoreUtilAddButtonSub should be called once");
  zassert_equal(datastoreUtilAddButtonSub_fake.arg0_val, &sub,
                "datastoreUtilAddButtonSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddButtonSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddButtonSub should be called with bufferPool");
}

/**
 * @test  The datastoreSubscribeButton function must successfully subscribe
 *        when datastoreUtilAddButtonSub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_button_success)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddButtonSub to succeed */
  datastoreUtilAddButtonSub_fake.return_val = 0;

  /* Call datastoreSubscribeButton */
  ret = datastoreSubscribeButton(&sub);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeButton should return 0 on success");

  /* Verify datastoreUtilAddButtonSub was called with correct parameters */
  zassert_equal(datastoreUtilAddButtonSub_fake.call_count, 1,
                "datastoreUtilAddButtonSub should be called once");
  zassert_equal(datastoreUtilAddButtonSub_fake.arg0_val, &sub,
                "datastoreUtilAddButtonSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddButtonSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddButtonSub should be called with bufferPool");
}

/**
 * @test  The datastoreUnsubscribeButton function must return an error when
 *        datastoreUtilRemoveButtonSub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_button_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xDEADBEEF;
  int ret;

  /* Configure datastoreUtilRemoveButtonSub to fail */
  datastoreUtilRemoveButtonSub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeButton */
  ret = datastoreUnsubscribeButton(callback);

  /* Verify function returned the error from datastoreUtilRemoveButtonSub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeButton should return error from datastoreUtilRemoveButtonSub");

  /* Verify datastoreUtilRemoveButtonSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveButtonSub_fake.call_count, 1,
                "datastoreUtilRemoveButtonSub should be called once");
  zassert_equal(datastoreUtilRemoveButtonSub_fake.arg0_val, callback,
                "datastoreUtilRemoveButtonSub should be called with the callback");
}

/**
 * @test  The datastoreUnsubscribeButton function must successfully unsubscribe
 *        when datastoreUtilRemoveButtonSub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_button_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xCAFEBABE;
  int ret;

  /* Configure datastoreUtilRemoveButtonSub to succeed */
  datastoreUtilRemoveButtonSub_fake.return_val = 0;

  /* Call datastoreUnsubscribeButton */
  ret = datastoreUnsubscribeButton(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeButton should return 0 on success");

  /* Verify datastoreUtilRemoveButtonSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveButtonSub_fake.call_count, 1,
                "datastoreUtilRemoveButtonSub should be called once");
  zassert_equal(datastoreUtilRemoveButtonSub_fake.arg0_val, callback,
                "datastoreUtilRemoveButtonSub should be called with the callback");
}

/**
 * @test  The datastorePauseSubButton function must return an error when
 *        datastoreUtilSetButtonSubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_button_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xFEEDFACE;
  int ret;

  /* Configure datastoreUtilSetButtonSubPauseState to fail */
  datastoreUtilSetButtonSubPauseState_fake.return_val = -ENOENT;

  /* Call datastorePauseSubButton */
  ret = datastorePauseSubButton(callback);

  /* Verify function returned the error from datastoreUtilSetButtonSubPauseState */
  zassert_equal(ret, -ENOENT, "datastorePauseSubButton should return error from datastoreUtilSetButtonSubPauseState");

  /* Verify datastoreUtilSetButtonSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.call_count, 1,
                "datastoreUtilSetButtonSubPauseState should be called once");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetButtonSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetButtonSubPauseState should be called with true (pause)");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetButtonSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastorePauseSubButton function must successfully pause subscription
 *        when datastoreUtilSetButtonSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_button_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xBAADF00D;
  int ret;

  /* Configure datastoreUtilSetButtonSubPauseState to succeed */
  datastoreUtilSetButtonSubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubButton */
  ret = datastorePauseSubButton(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubButton should return 0 on success");

  /* Verify datastoreUtilSetButtonSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.call_count, 1,
                "datastoreUtilSetButtonSubPauseState should be called once");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetButtonSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetButtonSubPauseState should be called with true (pause)");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetButtonSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubButton function must return an error when
 *        datastoreUtilSetButtonSubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_button_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x8BADF00D;
  int ret;

  /* Configure datastoreUtilSetButtonSubPauseState to fail */
  datastoreUtilSetButtonSubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubButton */
  ret = datastoreUnpauseSubButton(callback);

  /* Verify function returned the error from datastoreUtilSetButtonSubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubButton should return error from datastoreUtilSetButtonSubPauseState");

  /* Verify datastoreUtilSetButtonSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.call_count, 1,
                "datastoreUtilSetButtonSubPauseState should be called once");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetButtonSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetButtonSubPauseState should be called with false (unpause)");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetButtonSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubButton function must successfully unpause subscription
 *        when datastoreUtilSetButtonSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_button_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0xC0FFEEEE;
  int ret;

  /* Configure datastoreUtilSetButtonSubPauseState to succeed */
  datastoreUtilSetButtonSubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubButton */
  ret = datastoreUnpauseSubButton(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubButton should return 0 on success");

  /* Verify datastoreUtilSetButtonSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.call_count, 1,
                "datastoreUtilSetButtonSubPauseState should be called once");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetButtonSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetButtonSubPauseState should be called with false (unpause)");
  zassert_equal(datastoreUtilSetButtonSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetButtonSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreReadButton function must return -EINVAL when values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_button_invalid_values)
{
  uint32_t datapointId = 40;
  size_t valCount = 5;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadButton with NULL values */
  ret = datastoreReadButton(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadButton should return -EINVAL when values is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadButton function must return -EINVAL when valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_read_button_invalid_valcount)
{
  uint32_t datapointId = 45;
  size_t valCount = 0;
  ButtonState_t values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadButton with valCount = 0 */
  ret = datastoreReadButton(datapointId, valCount, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadButton should return -EINVAL when valCount is 0");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadButton function must return -EINVAL when response
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_button_invalid_response)
{
  uint32_t datapointId = 50;
  size_t valCount = 3;
  ButtonState_t values[3];
  int ret;

  /* Call datastoreReadButton with NULL response */
  ret = datastoreReadButton(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadButton should return -EINVAL when response is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreReadButton function must return an error when the
 *        underlying datastoreRead operation fails.
 */
ZTEST(datastore_tests, test_read_button_operation_failure)
{
  uint32_t datapointId = 55;
  size_t valCount = 2;
  ButtonState_t values[2];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadButton - should fail due to buffer allocation failure */
  ret = datastoreReadButton(datapointId, valCount, &responseQueue, values);

  /* Verify function returned -ENOSPC (buffer allocation error) */
  zassert_equal(ret, -ENOSPC, "datastoreReadButton should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");
}

/**
 * @test  The datastoreReadButton function must successfully read button data
 *        when all parameters are valid and the operation succeeds.
 */
ZTEST(datastore_tests, test_read_button_success)
{
  uint32_t datapointId = 60;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to ButtonState_t */
  Data_t valueStorage[3];
  ButtonState_t *values = (ButtonState_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (ButtonState_t values stored as uintVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].uintVal = BUTTON_SHORT_PRESSED;
  payloadData[1].uintVal = BUTTON_UNPRESSED;
  payloadData[2].uintVal = BUTTON_LONG_PRESSED;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadButton - should succeed */
  ret = datastoreReadButton(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadButton should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_READ, "Message type should be DATASTORE_READ");
  zassert_equal(msg.datapointType, DATAPOINT_BUTTON, "Message should have DATAPOINT_BUTTON type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");

  /* Verify data was copied - access as Data_t to read the full values */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].uintVal, BUTTON_SHORT_PRESSED, "First value should be BUTTON_SHORT_PRESSED");
  zassert_equal(dataValues[1].uintVal, BUTTON_UNPRESSED, "Second value should be BUTTON_UNPRESSED");
  zassert_equal(dataValues[2].uintVal, BUTTON_LONG_PRESSED, "Third value should be BUTTON_LONG_PRESSED");

  /* Verify osMemoryPoolFree was called to free the buffer */
  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to free the buffer");
}

/**
 * @test  The datastoreWriteButton function must return -EINVAL when values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_write_button_invalid_values)
{
  uint32_t datapointId = 65;
  size_t valCount = 4;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteButton with NULL values */
  ret = datastoreWriteButton(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteButton should return -EINVAL when values is NULL");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreWriteButton function must return -EINVAL when valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_write_button_invalid_valcount)
{
  uint32_t datapointId = 70;
  size_t valCount = 0;
  ButtonState_t values[1];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteButton with valCount = 0 */
  ret = datastoreWriteButton(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteButton should return -EINVAL when valCount is 0");

  /* Verify no buffer allocation was attempted */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 0,
                "osMemoryPoolAlloc should not be called when parameter validation fails");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when parameter validation fails");
}

/**
 * @test  The datastoreWriteButton function must return an error when the
 *        underlying datastoreWrite operation fails.
 */
ZTEST(datastore_tests, test_write_button_operation_failure)
{
  uint32_t datapointId = 75;
  size_t valCount = 2;
  ButtonState_t values[2];
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteButton - should fail due to buffer allocation failure */
  ret = datastoreWriteButton(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -ENOSPC (buffer allocation error) */
  zassert_equal(ret, -ENOSPC, "datastoreWriteButton should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify no message was put in the queue */
  DatastoreMsg_t dummy;
  ret = k_msgq_get(&datastoreQueue, &dummy, K_NO_WAIT);
  zassert_equal(ret, -ENOMSG, "No message should be in the datastore queue when buffer allocation fails");
}

/**
 * @test  The datastoreWriteButton function must successfully write button data
 *        when all parameters are valid and the operation succeeds.
 */
ZTEST(datastore_tests, test_write_button_success)
{
  uint32_t datapointId = 80;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast from ButtonState_t */
  Data_t valueStorage[3] = {{.uintVal = BUTTON_SHORT_PRESSED}, {.uintVal = BUTTON_UNPRESSED}, {.uintVal = BUTTON_LONG_PRESSED}};
  ButtonState_t *values = (ButtonState_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteButton - should succeed */
  ret = datastoreWriteButton(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteButton should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_BUTTON, "Message should have DATAPOINT_BUTTON type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].uintVal, BUTTON_SHORT_PRESSED, "First value should be BUTTON_SHORT_PRESSED");
  zassert_equal(payloadData[1].uintVal, BUTTON_UNPRESSED, "Second value should be BUTTON_UNPRESSED");
  zassert_equal(payloadData[2].uintVal, BUTTON_LONG_PRESSED, "Third value should be BUTTON_LONG_PRESSED");

  /* Verify osMemoryPoolFree was NOT called (buffer ownership transferred to queue) */
  zassert_equal(osMemoryPoolFree_fake.call_count, 0,
                "osMemoryPoolFree should not be called when response is expected");
}

/**
 * @test  The datastoreSubscribeFloat function must return an error when
 *        datastoreUtilAddFloatSub fails.
 */
ZTEST(datastore_tests, test_subscribe_float_failure)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddFloatSub to fail */
  datastoreUtilAddFloatSub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeFloat */
  ret = datastoreSubscribeFloat(&sub);

  /* Verify function returned the error from datastoreUtilAddFloatSub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeFloat should return error from datastoreUtilAddFloatSub");

  /* Verify datastoreUtilAddFloatSub was called with correct parameters */
  zassert_equal(datastoreUtilAddFloatSub_fake.call_count, 1,
                "datastoreUtilAddFloatSub should be called once");
  zassert_equal(datastoreUtilAddFloatSub_fake.arg0_val, &sub,
                "datastoreUtilAddFloatSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddFloatSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddFloatSub should be called with bufferPool");
}

/**
 * @test  The datastoreSubscribeFloat function must successfully subscribe
 *        when datastoreUtilAddFloatSub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_float_success)
{
  DatastoreSubEntry_t sub;
  int ret;

  /* Configure datastoreUtilAddFloatSub to succeed */
  datastoreUtilAddFloatSub_fake.return_val = 0;

  /* Call datastoreSubscribeFloat */
  ret = datastoreSubscribeFloat(&sub);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeFloat should return 0 on success");

  /* Verify datastoreUtilAddFloatSub was called with correct parameters */
  zassert_equal(datastoreUtilAddFloatSub_fake.call_count, 1,
                "datastoreUtilAddFloatSub should be called once");
  zassert_equal(datastoreUtilAddFloatSub_fake.arg0_val, &sub,
                "datastoreUtilAddFloatSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddFloatSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddFloatSub should be called with bufferPool");
}

/**
 * @test  The datastoreUnsubscribeFloat function must return an error when
 *        datastoreUtilRemoveFloatSub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_float_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveFloatSub to fail */
  datastoreUtilRemoveFloatSub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeFloat */
  ret = datastoreUnsubscribeFloat(callback);

  /* Verify function returned the error from datastoreUtilRemoveFloatSub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeFloat should return error from datastoreUtilRemoveFloatSub");

  /* Verify datastoreUtilRemoveFloatSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveFloatSub_fake.call_count, 1,
                "datastoreUtilRemoveFloatSub should be called once");
  zassert_equal(datastoreUtilRemoveFloatSub_fake.arg0_val, callback,
                "datastoreUtilRemoveFloatSub should be called with the callback");
}

/**
 * @test  The datastoreUnsubscribeFloat function must return success when
 *        datastoreUtilRemoveFloatSub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_float_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveFloatSub to succeed */
  datastoreUtilRemoveFloatSub_fake.return_val = 0;

  /* Call datastoreUnsubscribeFloat */
  ret = datastoreUnsubscribeFloat(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeFloat should return 0 on success");

  /* Verify datastoreUtilRemoveFloatSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveFloatSub_fake.call_count, 1,
                "datastoreUtilRemoveFloatSub should be called once");
  zassert_equal(datastoreUtilRemoveFloatSub_fake.arg0_val, callback,
                "datastoreUtilRemoveFloatSub should be called with the callback");
}

/**
 * @test  The datastorePauseSubFloat function must return an error when
 *        datastoreUtilSetFloatSubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_float_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetFloatSubPauseState to fail */
  datastoreUtilSetFloatSubPauseState_fake.return_val = -ENOMEM;

  /* Call datastorePauseSubFloat */
  ret = datastorePauseSubFloat(callback);

  /* Verify function returned the error from datastoreUtilSetFloatSubPauseState */
  zassert_equal(ret, -ENOMEM, "datastorePauseSubFloat should return error from datastoreUtilSetFloatSubPauseState");

  /* Verify datastoreUtilSetFloatSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.call_count, 1,
                "datastoreUtilSetFloatSubPauseState should be called once");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetFloatSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetFloatSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetFloatSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastorePauseSubFloat function must return success when
 *        datastoreUtilSetFloatSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_float_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetFloatSubPauseState to succeed */
  datastoreUtilSetFloatSubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubFloat */
  ret = datastorePauseSubFloat(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubFloat should return 0 on success");

  /* Verify datastoreUtilSetFloatSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.call_count, 1,
                "datastoreUtilSetFloatSubPauseState should be called once");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetFloatSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetFloatSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetFloatSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubFloat function must return an error when
 *        datastoreUtilSetFloatSubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_float_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetFloatSubPauseState to fail */
  datastoreUtilSetFloatSubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubFloat */
  ret = datastoreUnpauseSubFloat(callback);

  /* Verify function returned the error from datastoreUtilSetFloatSubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubFloat should return error from datastoreUtilSetFloatSubPauseState");

  /* Verify datastoreUtilSetFloatSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.call_count, 1,
                "datastoreUtilSetFloatSubPauseState should be called once");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetFloatSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetFloatSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetFloatSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubFloat function must return success when
 *        datastoreUtilSetFloatSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_float_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetFloatSubPauseState to succeed */
  datastoreUtilSetFloatSubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubFloat */
  ret = datastoreUnpauseSubFloat(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubFloat should return 0 on success");

  /* Verify datastoreUtilSetFloatSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.call_count, 1,
                "datastoreUtilSetFloatSubPauseState should be called once");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetFloatSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetFloatSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetFloatSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetFloatSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreReadFloat function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_float_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadFloat with NULL values */
  ret = datastoreReadFloat(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadFloat should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreReadFloat function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_read_float_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadFloat with valCount = 0 */
  ret = datastoreReadFloat(datapointId, 0, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadFloat should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreReadFloat function must return an error when the response
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_float_invalid_response)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  int ret;

  /* Call datastoreReadFloat with NULL response */
  ret = datastoreReadFloat(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadFloat should return -EINVAL when response is NULL");
}

/**
 * @test  The datastoreReadFloat function must return an error when the
 *        underlying datastoreRead operation fails (e.g., buffer allocation fails).
 */
ZTEST(datastore_tests, test_read_float_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadFloat */
  ret = datastoreReadFloat(datapointId, valCount, &responseQueue, values);

  /* Verify function returned error from datastoreRead */
  zassert_equal(ret, -ENOSPC, "datastoreReadFloat should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test  The datastoreReadFloat function must successfully read float values when
 *        the underlying datastoreRead operation succeeds.
 */
ZTEST(datastore_tests, test_read_float_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to float */
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (float values stored as floatVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].floatVal = 1.5f;
  payloadData[1].floatVal = 2.75f;
  payloadData[2].floatVal = 3.125f;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadFloat - should succeed */
  ret = datastoreReadFloat(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadFloat should return 0 on success");

  /* Verify the float values were correctly read */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].floatVal, 1.5f, "values[0] should be 1.5");
  zassert_equal(dataValues[1].floatVal, 2.75f, "values[1] should be 2.75");
  zassert_equal(dataValues[2].floatVal, 3.125f, "values[2] should be 3.125");
}

/**
 * @test  The datastoreWriteFloat function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_write_float_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteFloat with NULL values */
  ret = datastoreWriteFloat(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteFloat should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreWriteFloat function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_write_float_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteFloat with valCount = 0 */
  ret = datastoreWriteFloat(datapointId, values, 0, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteFloat should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreWriteFloat function must return an error when the
 *        underlying datastoreWrite operation fails (e.g., buffer allocation fails).
 */
ZTEST(datastore_tests, test_write_float_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteFloat */
  ret = datastoreWriteFloat(datapointId, values, valCount, &responseQueue);

  /* Verify function returned error from datastoreWrite */
  zassert_equal(ret, -ENOSPC, "datastoreWriteFloat should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test  The datastoreWriteFloat function must successfully write float values when
 *        the underlying datastoreWrite operation succeeds.
 */
ZTEST(datastore_tests, test_write_float_success)
{
  uint32_t datapointId = 90;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast to float */
  Data_t valueStorage[3] = {{.floatVal = 1.5f}, {.floatVal = 2.75f}, {.floatVal = 3.125f}};
  float *values = (float *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteFloat - should succeed */
  ret = datastoreWriteFloat(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteFloat should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_FLOAT, "Message should have DATAPOINT_FLOAT type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].floatVal, 1.5f, "First value should be 1.5");
  zassert_equal(payloadData[1].floatVal, 2.75f, "Second value should be 2.75");
  zassert_equal(payloadData[2].floatVal, 3.125f, "Third value should be 3.125");
}

/**
 * @test  The datastoreSubscribeInt function must return an error when
 *        datastoreUtilAddIntSub fails.
 */
ZTEST(datastore_tests, test_subscribe_int_failure)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddIntSub to fail */
  datastoreUtilAddIntSub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeInt */
  ret = datastoreSubscribeInt(&subEntry);

  /* Verify function returned the error from datastoreUtilAddIntSub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeInt should return error from datastoreUtilAddIntSub");

  /* Verify datastoreUtilAddIntSub was called with correct parameters */
  zassert_equal(datastoreUtilAddIntSub_fake.call_count, 1,
                "datastoreUtilAddIntSub should be called once");
  zassert_equal(datastoreUtilAddIntSub_fake.arg0_val, &subEntry,
                "datastoreUtilAddIntSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddIntSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddIntSub should be called with bufferPool");
}

/**
 * @test  The datastoreSubscribeInt function must return success when
 *        datastoreUtilAddIntSub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_int_success)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddIntSub to succeed */
  datastoreUtilAddIntSub_fake.return_val = 0;

  /* Call datastoreSubscribeInt */
  ret = datastoreSubscribeInt(&subEntry);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeInt should return 0 on success");

  /* Verify datastoreUtilAddIntSub was called with correct parameters */
  zassert_equal(datastoreUtilAddIntSub_fake.call_count, 1,
                "datastoreUtilAddIntSub should be called once");
  zassert_equal(datastoreUtilAddIntSub_fake.arg0_val, &subEntry,
                "datastoreUtilAddIntSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddIntSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddIntSub should be called with bufferPool");
}

/**
 * @test  The datastoreUnsubscribeInt function must return an error when
 *        datastoreUtilRemoveIntSub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_int_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveIntSub to fail */
  datastoreUtilRemoveIntSub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeInt */
  ret = datastoreUnsubscribeInt(callback);

  /* Verify function returned the error from datastoreUtilRemoveIntSub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeInt should return error from datastoreUtilRemoveIntSub");

  /* Verify datastoreUtilRemoveIntSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveIntSub_fake.call_count, 1,
                "datastoreUtilRemoveIntSub should be called once");
  zassert_equal(datastoreUtilRemoveIntSub_fake.arg0_val, callback,
                "datastoreUtilRemoveIntSub should be called with the callback");
}

/**
 * @test  The datastoreUnsubscribeInt function must return success when
 *        datastoreUtilRemoveIntSub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_int_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveIntSub to succeed */
  datastoreUtilRemoveIntSub_fake.return_val = 0;

  /* Call datastoreUnsubscribeInt */
  ret = datastoreUnsubscribeInt(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeInt should return 0 on success");

  /* Verify datastoreUtilRemoveIntSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveIntSub_fake.call_count, 1,
                "datastoreUtilRemoveIntSub should be called once");
  zassert_equal(datastoreUtilRemoveIntSub_fake.arg0_val, callback,
                "datastoreUtilRemoveIntSub should be called with the callback");
}

/**
 * @test  The datastorePauseSubInt function must return an error when
 *        datastoreUtilSetIntSubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_int_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetIntSubPauseState to fail */
  datastoreUtilSetIntSubPauseState_fake.return_val = -ENOMEM;

  /* Call datastorePauseSubInt */
  ret = datastorePauseSubInt(callback);

  /* Verify function returned the error from datastoreUtilSetIntSubPauseState */
  zassert_equal(ret, -ENOMEM, "datastorePauseSubInt should return error from datastoreUtilSetIntSubPauseState");

  /* Verify datastoreUtilSetIntSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.call_count, 1,
                "datastoreUtilSetIntSubPauseState should be called once");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetIntSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetIntSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetIntSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastorePauseSubInt function must return success when
 *        datastoreUtilSetIntSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_int_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetIntSubPauseState to succeed */
  datastoreUtilSetIntSubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubInt */
  ret = datastorePauseSubInt(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubInt should return 0 on success");

  /* Verify datastoreUtilSetIntSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.call_count, 1,
                "datastoreUtilSetIntSubPauseState should be called once");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetIntSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetIntSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetIntSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubInt function must return an error when
 *        datastoreUtilSetIntSubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_int_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetIntSubPauseState to fail */
  datastoreUtilSetIntSubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubInt */
  ret = datastoreUnpauseSubInt(callback);

  /* Verify function returned the error from datastoreUtilSetIntSubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubInt should return error from datastoreUtilSetIntSubPauseState");

  /* Verify datastoreUtilSetIntSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.call_count, 1,
                "datastoreUtilSetIntSubPauseState should be called once");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetIntSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetIntSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetIntSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubInt function must return success when
 *        datastoreUtilSetIntSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_int_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetIntSubPauseState to succeed */
  datastoreUtilSetIntSubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubInt */
  ret = datastoreUnpauseSubInt(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubInt should return 0 on success");

  /* Verify datastoreUtilSetIntSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.call_count, 1,
                "datastoreUtilSetIntSubPauseState should be called once");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetIntSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetIntSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetIntSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetIntSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreReadInt function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_int_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadInt with NULL values */
  ret = datastoreReadInt(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadInt should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreReadInt function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_read_int_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadInt with valCount = 0 */
  ret = datastoreReadInt(datapointId, 0, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadInt should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreReadInt function must return an error when the response
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_int_invalid_response)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  int ret;

  /* Call datastoreReadInt with NULL response */
  ret = datastoreReadInt(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadInt should return -EINVAL when response is NULL");
}

/**
 * @test  The datastoreReadInt function must return an error when the
 *        underlying datastoreRead operation fails (e.g., buffer allocation fails).
 */
ZTEST(datastore_tests, test_read_int_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadInt */
  ret = datastoreReadInt(datapointId, valCount, &responseQueue, values);

  /* Verify function returned error from datastoreRead */
  zassert_equal(ret, -ENOSPC, "datastoreReadInt should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test  The datastoreReadInt function must successfully read int32_t values when
 *        the underlying datastoreRead operation succeeds.
 */
ZTEST(datastore_tests, test_read_int_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to int32_t */
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (int values stored as intVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].intVal = -100;
  payloadData[1].intVal = 0;
  payloadData[2].intVal = 200;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadInt - should succeed */
  ret = datastoreReadInt(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadInt should return 0 on success");

  /* Verify the int values were correctly read */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].intVal, -100, "values[0] should be -100");
  zassert_equal(dataValues[1].intVal, 0, "values[1] should be 0");
  zassert_equal(dataValues[2].intVal, 200, "values[2] should be 200");
}

/**
 * @test  The datastoreWriteInt function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_write_int_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteInt with NULL values */
  ret = datastoreWriteInt(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteInt should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreWriteInt function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_write_int_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteInt with valCount = 0 */
  ret = datastoreWriteInt(datapointId, values, 0, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteInt should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreWriteInt function must return an error when the
 *        underlying datastoreWrite operation fails (e.g., buffer allocation fails).
 */
ZTEST(datastore_tests, test_write_int_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteInt */
  ret = datastoreWriteInt(datapointId, values, valCount, &responseQueue);

  /* Verify function returned error from datastoreWrite */
  zassert_equal(ret, -ENOSPC, "datastoreWriteInt should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test  The datastoreWriteInt function must successfully write int32_t values when
 *        the underlying datastoreWrite operation succeeds.
 */
ZTEST(datastore_tests, test_write_int_success)
{
  uint32_t datapointId = 100;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast to int32_t */
  Data_t valueStorage[3] = {{.intVal = -100}, {.intVal = 0}, {.intVal = 200}};
  int32_t *values = (int32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteInt - should succeed */
  ret = datastoreWriteInt(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteInt should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_INT, "Message should have DATAPOINT_INT type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].intVal, -100, "First value should be -100");
  zassert_equal(payloadData[1].intVal, 0, "Second value should be 0");
  zassert_equal(payloadData[2].intVal, 200, "Third value should be 200");
}

/**
 * @test  The datastoreSubscribeMultiState function must return an error when
 *        datastoreUtilAddMultiStateSub fails.
 */
ZTEST(datastore_tests, test_subscribe_multi_state_failure)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddMultiStateSub to fail */
  datastoreUtilAddMultiStateSub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeMultiState */
  ret = datastoreSubscribeMultiState(&subEntry);

  /* Verify function returned the error from datastoreUtilAddMultiStateSub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeMultiState should return error from datastoreUtilAddMultiStateSub");

  /* Verify datastoreUtilAddMultiStateSub was called with correct parameters */
  zassert_equal(datastoreUtilAddMultiStateSub_fake.call_count, 1,
                "datastoreUtilAddMultiStateSub should be called once");
  zassert_equal(datastoreUtilAddMultiStateSub_fake.arg0_val, &subEntry,
                "datastoreUtilAddMultiStateSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddMultiStateSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddMultiStateSub should be called with bufferPool");
}

/**
 * @test  The datastoreSubscribeMultiState function must return success when
 *        datastoreUtilAddMultiStateSub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_multi_state_success)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddMultiStateSub to succeed */
  datastoreUtilAddMultiStateSub_fake.return_val = 0;

  /* Call datastoreSubscribeMultiState */
  ret = datastoreSubscribeMultiState(&subEntry);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeMultiState should return 0 on success");

  /* Verify datastoreUtilAddMultiStateSub was called with correct parameters */
  zassert_equal(datastoreUtilAddMultiStateSub_fake.call_count, 1,
                "datastoreUtilAddMultiStateSub should be called once");
  zassert_equal(datastoreUtilAddMultiStateSub_fake.arg0_val, &subEntry,
                "datastoreUtilAddMultiStateSub should be called with the subscription entry");
  zassert_equal(datastoreUtilAddMultiStateSub_fake.arg1_val, bufferPool,
                "datastoreUtilAddMultiStateSub should be called with bufferPool");
}

/**
 * @test  The datastoreUnsubscribeMultiState function must return an error when
 *        datastoreUtilRemoveMultiStateSub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_multi_state_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveMultiStateSub to fail */
  datastoreUtilRemoveMultiStateSub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeMultiState */
  ret = datastoreUnsubscribeMultiState(callback);

  /* Verify function returned the error from datastoreUtilRemoveMultiStateSub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeMultiState should return error from datastoreUtilRemoveMultiStateSub");

  /* Verify datastoreUtilRemoveMultiStateSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveMultiStateSub_fake.call_count, 1,
                "datastoreUtilRemoveMultiStateSub should be called once");
  zassert_equal(datastoreUtilRemoveMultiStateSub_fake.arg0_val, callback,
                "datastoreUtilRemoveMultiStateSub should be called with the callback");
}

/**
 * @test  The datastoreUnsubscribeMultiState function must return success when
 *        datastoreUtilRemoveMultiStateSub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_multi_state_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveMultiStateSub to succeed */
  datastoreUtilRemoveMultiStateSub_fake.return_val = 0;

  /* Call datastoreUnsubscribeMultiState */
  ret = datastoreUnsubscribeMultiState(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeMultiState should return 0 on success");

  /* Verify datastoreUtilRemoveMultiStateSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveMultiStateSub_fake.call_count, 1,
                "datastoreUtilRemoveMultiStateSub should be called once");
  zassert_equal(datastoreUtilRemoveMultiStateSub_fake.arg0_val, callback,
                "datastoreUtilRemoveMultiStateSub should be called with the callback");
}

/**
 * @test  The datastorePauseSubMultiState function must return an error when
 *        datastoreUtilSetMultiStateSubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_multi_state_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetMultiStateSubPauseState to fail */
  datastoreUtilSetMultiStateSubPauseState_fake.return_val = -ENOMEM;

  /* Call datastorePauseSubMultiState */
  ret = datastorePauseSubMultiState(callback);

  /* Verify function returned the error from datastoreUtilSetMultiStateSubPauseState */
  zassert_equal(ret, -ENOMEM, "datastorePauseSubMultiState should return error from datastoreUtilSetMultiStateSubPauseState");

  /* Verify datastoreUtilSetMultiStateSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.call_count, 1,
                "datastoreUtilSetMultiStateSubPauseState should be called once");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetMultiStateSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetMultiStateSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetMultiStateSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastorePauseSubMultiState function must return success when
 *        datastoreUtilSetMultiStateSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_multi_state_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetMultiStateSubPauseState to succeed */
  datastoreUtilSetMultiStateSubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubMultiState */
  ret = datastorePauseSubMultiState(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubMultiState should return 0 on success");

  /* Verify datastoreUtilSetMultiStateSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.call_count, 1,
                "datastoreUtilSetMultiStateSubPauseState should be called once");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetMultiStateSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetMultiStateSubPauseState should be called with true for pause state");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetMultiStateSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubMultiState function must return an error when
 *        datastoreUtilSetMultiStateSubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_multi_state_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetMultiStateSubPauseState to fail */
  datastoreUtilSetMultiStateSubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubMultiState */
  ret = datastoreUnpauseSubMultiState(callback);

  /* Verify function returned the error from datastoreUtilSetMultiStateSubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubMultiState should return error from datastoreUtilSetMultiStateSubPauseState");

  /* Verify datastoreUtilSetMultiStateSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.call_count, 1,
                "datastoreUtilSetMultiStateSubPauseState should be called once");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetMultiStateSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetMultiStateSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetMultiStateSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreUnpauseSubMultiState function must return success when
 *        datastoreUtilSetMultiStateSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_multi_state_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetMultiStateSubPauseState to succeed */
  datastoreUtilSetMultiStateSubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubMultiState */
  ret = datastoreUnpauseSubMultiState(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubMultiState should return 0 on success");

  /* Verify datastoreUtilSetMultiStateSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.call_count, 1,
                "datastoreUtilSetMultiStateSubPauseState should be called once");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetMultiStateSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetMultiStateSubPauseState should be called with false for unpause state");
  zassert_equal(datastoreUtilSetMultiStateSubPauseState_fake.arg2_val, bufferPool,
                "datastoreUtilSetMultiStateSubPauseState should be called with bufferPool");
}

/**
 * @test  The datastoreReadMultiState function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_multi_state_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadMultiState with NULL values */
  ret = datastoreReadMultiState(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadMultiState should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreReadMultiState function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_read_multi_state_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadMultiState with valCount = 0 */
  ret = datastoreReadMultiState(datapointId, 0, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadMultiState should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreReadMultiState function must return an error when the response
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_read_multi_state_invalid_response)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  int ret;

  /* Call datastoreReadMultiState with NULL response */
  ret = datastoreReadMultiState(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadMultiState should return -EINVAL when response is NULL");
}

/**
 * @test  The datastoreReadMultiState function must return an error when the
 *        underlying datastoreRead operation fails (e.g., buffer allocation fails).
 */
ZTEST(datastore_tests, test_read_multi_state_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadMultiState */
  ret = datastoreReadMultiState(datapointId, valCount, &responseQueue, values);

  /* Verify function returned error from datastoreRead */
  zassert_equal(ret, -ENOSPC, "datastoreReadMultiState should return -ENOSPC when buffer allocation fails");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test  The datastoreReadMultiState function must successfully read uint32_t values when
 *        the underlying datastoreRead operation succeeds.
 */
ZTEST(datastore_tests, test_read_multi_state_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to uint32_t */
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (uint values stored as uintVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].uintVal = 10;
  payloadData[1].uintVal = 20;
  payloadData[2].uintVal = 30;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadMultiState - should succeed */
  ret = datastoreReadMultiState(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadMultiState should return 0 on success");

  /* Verify the uint values were correctly read */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].uintVal, 10, "values[0] should be 10");
  zassert_equal(dataValues[1].uintVal, 20, "values[1] should be 20");
  zassert_equal(dataValues[2].uintVal, 30, "values[2] should be 30");
}

/**
 * @test  The datastoreWriteMultiState function must return an error when the values
 *        parameter is NULL.
 */
ZTEST(datastore_tests, test_write_multi_state_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteMultiState with NULL values */
  ret = datastoreWriteMultiState(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteMultiState should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreWriteMultiState function must return an error when the valCount
 *        parameter is 0.
 */
ZTEST(datastore_tests, test_write_multi_state_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteMultiState with valCount = 0 */
  ret = datastoreWriteMultiState(datapointId, values, 0, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteMultiState should return -EINVAL when valCount is 0");
}

/**
 * @brief Test datastoreWriteMultiState with operation failure.
 *
 * This test verifies that the datastoreWriteMultiState function properly handles
 * failures from the underlying datastoreWrite operation when buffer allocation fails.
 *
 * @param None
 *
 * @return None
 */
ZTEST(datastore_tests, test_write_multi_state_operation_failure)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  values[0] = 10;
  values[1] = 20;
  values[2] = 30;
  uint8_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteMultiState with valid parameters but allocation will fail */
  ret = datastoreWriteMultiState(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -ENOSPC */
  zassert_equal(ret, -ENOSPC, "datastoreWriteMultiState should return -ENOSPC when buffer allocation fails");
}

/**
 * @brief Test datastoreWriteMultiState successful operation.
 *
 * This test verifies that the datastoreWriteMultiState function successfully writes
 * multi-state datapoint values when all parameters are valid and operations succeed.
 *
 * @param None
 *
 * @return None
 */
ZTEST(datastore_tests, test_write_multi_state_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast to uint32_t */
  Data_t valueStorage[3] = {{.uintVal = 10}, {.uintVal = 20}, {.uintVal = 30}};
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteMultiState - should succeed */
  ret = datastoreWriteMultiState(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteMultiState should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_MULTI_STATE, "Message should have DATAPOINT_MULTI_STATE type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].uintVal, 10, "First value should be 10");
  zassert_equal(payloadData[1].uintVal, 20, "Second value should be 20");
  zassert_equal(payloadData[2].uintVal, 30, "Third value should be 30");
}

/**
 * @test  The datastoreSubscribeUint function must return an error when
 *        datastoreUtilAddUintSub fails.
 */
ZTEST(datastore_tests, test_subscribe_uint_failure)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddUintSub to fail */
  datastoreUtilAddUintSub_fake.return_val = -ENOMEM;

  /* Call datastoreSubscribeUint */
  ret = datastoreSubscribeUint(&subEntry);

  /* Verify function returned the error from datastoreUtilAddUintSub */
  zassert_equal(ret, -ENOMEM, "datastoreSubscribeUint should return error from datastoreUtilAddUintSub");
}

/**
 * @test  The datastoreSubscribeUint function must return success when
 *        datastoreUtilAddUintSub succeeds.
 */
ZTEST(datastore_tests, test_subscribe_uint_success)
{
  DatastoreSubEntry_t subEntry;
  int ret;

  subEntry.callback = (DatastoreSubCb_t)0x1234ABCD;

  /* Configure datastoreUtilAddUintSub to succeed */
  datastoreUtilAddUintSub_fake.return_val = 0;

  /* Call datastoreSubscribeUint */
  ret = datastoreSubscribeUint(&subEntry);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreSubscribeUint should return 0 on success");
}

/**
 * @test  The datastoreUnsubscribeUint function must return an error when
 *        datastoreUtilRemoveUintSub fails.
 */
ZTEST(datastore_tests, test_unsubscribe_uint_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveUintSub to fail */
  datastoreUtilRemoveUintSub_fake.return_val = -ENOENT;

  /* Call datastoreUnsubscribeUint */
  ret = datastoreUnsubscribeUint(callback);

  /* Verify function returned the error from datastoreUtilRemoveUintSub */
  zassert_equal(ret, -ENOENT, "datastoreUnsubscribeUint should return error from datastoreUtilRemoveUintSub");

  /* Verify datastoreUtilRemoveUintSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveUintSub_fake.call_count, 1,
                "datastoreUtilRemoveUintSub should be called once");
  zassert_equal(datastoreUtilRemoveUintSub_fake.arg0_val, callback,
                "datastoreUtilRemoveUintSub should be called with correct callback");
}

/**
 * @test  The datastoreUnsubscribeUint function must return success when
 *        datastoreUtilRemoveUintSub succeeds.
 */
ZTEST(datastore_tests, test_unsubscribe_uint_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilRemoveUintSub to succeed */
  datastoreUtilRemoveUintSub_fake.return_val = 0;

  /* Call datastoreUnsubscribeUint */
  ret = datastoreUnsubscribeUint(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnsubscribeUint should return 0 on success");

  /* Verify datastoreUtilRemoveUintSub was called with correct parameters */
  zassert_equal(datastoreUtilRemoveUintSub_fake.call_count, 1,
                "datastoreUtilRemoveUintSub should be called once");
  zassert_equal(datastoreUtilRemoveUintSub_fake.arg0_val, callback,
                "datastoreUtilRemoveUintSub should be called with correct callback");
}

/**
 * @test  The datastorePauseSubUint function must return an error when
 *        datastoreUtilSetUintSubPauseState fails.
 */
ZTEST(datastore_tests, test_pause_sub_uint_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetUintSubPauseState to fail */
  datastoreUtilSetUintSubPauseState_fake.return_val = -ENOMEM;

  /* Call datastorePauseSubUint */
  ret = datastorePauseSubUint(callback);

  /* Verify function returned the error from datastoreUtilSetUintSubPauseState */
  zassert_equal(ret, -ENOMEM, "datastorePauseSubUint should return error from datastoreUtilSetUintSubPauseState");

  /* Verify datastoreUtilSetUintSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.call_count, 1,
                "datastoreUtilSetUintSubPauseState should be called once");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetUintSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetUintSubPauseState should be called with true for pause state");
}

/**
 * @test  The datastorePauseSubUint function must return success when
 *        datastoreUtilSetUintSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_pause_sub_uint_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetUintSubPauseState to succeed */
  datastoreUtilSetUintSubPauseState_fake.return_val = 0;

  /* Call datastorePauseSubUint */
  ret = datastorePauseSubUint(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastorePauseSubUint should return 0 on success");

  /* Verify datastoreUtilSetUintSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.call_count, 1,
                "datastoreUtilSetUintSubPauseState should be called once");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetUintSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg1_val, true,
                "datastoreUtilSetUintSubPauseState should be called with true for pause state");
}

/**
 * @test  The datastoreUnpauseSubUint function must return an error when
 *        datastoreUtilSetUintSubPauseState fails.
 */
ZTEST(datastore_tests, test_unpause_sub_uint_failure)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetUintSubPauseState to fail */
  datastoreUtilSetUintSubPauseState_fake.return_val = -ENOENT;

  /* Call datastoreUnpauseSubUint */
  ret = datastoreUnpauseSubUint(callback);

  /* Verify function returned the error from datastoreUtilSetUintSubPauseState */
  zassert_equal(ret, -ENOENT, "datastoreUnpauseSubUint should return error from datastoreUtilSetUintSubPauseState");

  /* Verify datastoreUtilSetUintSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.call_count, 1,
                "datastoreUtilSetUintSubPauseState should be called once");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetUintSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetUintSubPauseState should be called with false for unpause state");
}

/**
 * @test  The datastoreUnpauseSubUint function must return success when
 *        datastoreUtilSetUintSubPauseState succeeds.
 */
ZTEST(datastore_tests, test_unpause_sub_uint_success)
{
  DatastoreSubCb_t callback = (DatastoreSubCb_t)0x1234ABCD;
  int ret;

  /* Configure datastoreUtilSetUintSubPauseState to succeed */
  datastoreUtilSetUintSubPauseState_fake.return_val = 0;

  /* Call datastoreUnpauseSubUint */
  ret = datastoreUnpauseSubUint(callback);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreUnpauseSubUint should return 0 on success");

  /* Verify datastoreUtilSetUintSubPauseState was called with correct parameters */
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.call_count, 1,
                "datastoreUtilSetUintSubPauseState should be called once");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg0_val, callback,
                "datastoreUtilSetUintSubPauseState should be called with the callback");
  zassert_equal(datastoreUtilSetUintSubPauseState_fake.arg1_val, false,
                "datastoreUtilSetUintSubPauseState should be called with false for unpause state");
}

/**
 * @test  The datastoreReadUint function must return an error when
 *        the values parameter is NULL.
 */
ZTEST(datastore_tests, test_read_uint_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadUint with NULL values */
  ret = datastoreReadUint(datapointId, valCount, &responseQueue, NULL);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadUint should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreReadUint function must return an error when
 *        the valCount parameter is 0.
 */
ZTEST(datastore_tests, test_read_uint_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreReadUint with valCount = 0 */
  ret = datastoreReadUint(datapointId, 0, &responseQueue, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadUint should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreReadUint function must return an error when
 *        the response parameter is NULL.
 */
ZTEST(datastore_tests, test_read_uint_invalid_response)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  int ret;

  /* Call datastoreReadUint with NULL response */
  ret = datastoreReadUint(datapointId, valCount, NULL, values);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreReadUint should return -EINVAL when response is NULL");
}

/**
 * @test  The datastoreReadUint function must return an error when
 *        the underlying datastoreRead operation fails.
 */
ZTEST(datastore_tests, test_read_uint_operation_failure)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail (simulate buffer allocation failure) */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreReadUint */
  ret = datastoreReadUint(datapointId, valCount, &responseQueue, values);

  /* Verify function returned error from datastoreRead */
  zassert_equal(ret, -ENOSPC, "datastoreReadUint should return -ENOSPC when buffer allocation fails");
}

/**
 * @test  The datastoreReadUint function must successfully read uint values
 *        when all parameters are valid and operations succeed.
 */
ZTEST(datastore_tests, test_read_uint_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be memcpy'd, then cast to uint32_t */
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Setup mock payload with test data (uint values stored as uintVal) */
  mockPayload->dataLen = valCount * sizeof(Data_t);
  payloadData[0].uintVal = 10;
  payloadData[1].uintVal = 20;
  payloadData[2].uintVal = 30;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreReadUint - should succeed */
  ret = datastoreReadUint(datapointId, valCount, &responseQueue, values);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreReadUint should return 0 on success");

  /* Verify the uint values were correctly read */
  Data_t *dataValues = (Data_t *)values;
  zassert_equal(dataValues[0].uintVal, 10, "values[0] should be 10");
  zassert_equal(dataValues[1].uintVal, 20, "values[1] should be 20");
  zassert_equal(dataValues[2].uintVal, 30, "values[2] should be 30");
}

/**
 * @test  The datastoreWriteUint function must return an error when
 *        the values parameter is NULL.
 */
ZTEST(datastore_tests, test_write_uint_invalid_values)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteUint with NULL values */
  ret = datastoreWriteUint(datapointId, NULL, valCount, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteUint should return -EINVAL when values is NULL");
}

/**
 * @test  The datastoreWriteUint function must return an error when
 *        the valCount parameter is 0.
 */
ZTEST(datastore_tests, test_write_uint_invalid_valcount)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Call datastoreWriteUint with valCount = 0 */
  ret = datastoreWriteUint(datapointId, values, 0, &responseQueue);

  /* Verify function returned -EINVAL */
  zassert_equal(ret, -EINVAL, "datastoreWriteUint should return -EINVAL when valCount is 0");
}

/**
 * @test  The datastoreWriteUint function must return an error when
 *        the underlying datastoreWrite operation fails.
 */
ZTEST(datastore_tests, test_write_uint_operation_failure)
{
  uint32_t datapointId = 1;
  Data_t valueStorage[3];
  uint32_t *values = (uint32_t *)valueStorage;
  values[0] = 10;
  values[1] = 20;
  values[2] = 30;
  uint8_t valCount = 3;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  int ret;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure osMemoryPoolAlloc to fail */
  osMemoryPoolAlloc_fake.return_val = NULL;

  /* Call datastoreWriteUint with valid parameters but allocation will fail */
  ret = datastoreWriteUint(datapointId, values, valCount, &responseQueue);

  /* Verify function returned -ENOSPC */
  zassert_equal(ret, -ENOSPC, "datastoreWriteUint should return -ENOSPC when buffer allocation fails");
}

/**
 * @test  The datastoreWriteUint function must successfully write uint values
 *        when all parameters are valid and operations succeed.
 */
ZTEST(datastore_tests, test_write_uint_success)
{
  uint32_t datapointId = 1;
  size_t valCount = 3;
  /* Allocate enough storage for Data_t array that will be cast to uint32_t */
  Data_t valueStorage[3] = {{.uintVal = 10}, {.uintVal = 20}, {.uintVal = 30}};
  uint32_t *values = (uint32_t *)valueStorage;
  struct k_msgq responseQueue;
  char __aligned(4) responseBuffer[sizeof(int)];
  uint8_t payloadBuffer[sizeof(SrvMsgPayload_t) + (valCount * sizeof(Data_t))];
  SrvMsgPayload_t *mockPayload = (SrvMsgPayload_t *)payloadBuffer;
  Data_t *payloadData = (Data_t *)mockPayload->data;
  int ret;
  int successStatus = 0;

  /* Initialize response queue */
  k_msgq_init(&responseQueue, responseBuffer, sizeof(int), 1);

  /* Configure buffer allocation to succeed */
  osMemoryPoolAlloc_fake.return_val = mockPayload;

  /* Configure osMemoryPoolFree to succeed */
  osMemoryPoolFree_fake.return_val = osOK;

  /* Put success status in response queue */
  ret = k_msgq_put(&responseQueue, &successStatus, K_NO_WAIT);
  zassert_equal(ret, 0, "Failed to put success status in response queue");

  /* Call datastoreWriteUint - should succeed */
  ret = datastoreWriteUint(datapointId, values, valCount, &responseQueue);

  /* Verify function returned success */
  zassert_equal(ret, 0, "datastoreWriteUint should return 0 on success");

  /* Verify osMemoryPoolAlloc was called */
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");

  /* Verify message was put in the datastore queue */
  DatastoreMsg_t msg;
  ret = k_msgq_get(&datastoreQueue, &msg, K_NO_WAIT);
  zassert_equal(ret, 0, "Message should be in the datastore queue");
  zassert_equal(msg.msgType, DATASTORE_WRITE, "Message type should be DATASTORE_WRITE");
  zassert_equal(msg.datapointType, DATAPOINT_UINT, "Message should have DATAPOINT_UINT type");
  zassert_equal(msg.datapointId, datapointId, "Message should have correct datapoint ID");
  zassert_equal(msg.valCount, valCount, "Message should have correct value count");
  zassert_equal(msg.response, &responseQueue, "Response queue should be set correctly");

  /* Verify data was copied to payload */
  zassert_equal(payloadData[0].uintVal, 10, "First value should be 10");
  zassert_equal(payloadData[1].uintVal, 20, "Second value should be 20");
  zassert_equal(payloadData[2].uintVal, 30, "Third value should be 30");
}

ZTEST_SUITE(datastore_tests, NULL, datastore_tests_setup, datastore_tests_before, NULL, NULL);
