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
  zassert_equal(k_thread_create_mock_fake.arg2_val, DATASTORE_STACK_SIZE,
                "k_thread_create should be called with DATASTORE_STACK_SIZE");
  zassert_equal(k_thread_create_mock_fake.arg3_val, run,
                "k_thread_create should be called with run function");

  /* Verify thread name was set */
  zassert_equal(k_thread_name_set_mock_fake.call_count, 1,
                "k_thread_name_set should be called once");
  zassert_equal(k_thread_name_set_mock_fake.arg0_val, mockThreadId,
                "k_thread_name_set should be called with thread ID from k_thread_create");
}

ZTEST_SUITE(datastore_tests, NULL, datastore_tests_setup, datastore_tests_before, NULL, NULL);
