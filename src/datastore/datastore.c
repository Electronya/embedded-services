/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastore.c
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Service Implementation
 *
 *            Datastore service implementation.
 *
 * @ingroup   datastore
 * @{
 */

#include <zephyr/logging/log.h>
#include <string.h>

#include "datastore.h"
#include "datastoreUtil.h"
#include "serviceManager.h"

/* Setting module logging */
LOG_MODULE_REGISTER(DATASTORE_LOGGER_NAME, CONFIG_ENYA_DATASTORE_LOG_LEVEL);

#ifdef CONFIG_ZTEST
#ifndef DATASTORE_RUN_ITERATIONS
#define DATASTORE_RUN_ITERATIONS 1
#endif
#endif

/**
 * @brief   The datastore response timeout [ms].
 */
#define DATASTORE_RESPONSE_TIMEOUT                              (5)

/**
 * @brief   The datastore buffer count.
 */
#define DATASTORE_BUFFER_COUNT                                  (10)

/**
 * @brief The thread stack.
*/
K_THREAD_STACK_DEFINE(datastoreStack, CONFIG_ENYA_DATASTORE_STACK_SIZE);

/**
 * @brief   The datastore message type.
 */
typedef enum
{
  DATASTORE_READ = 0,
  DATASTORE_WRITE,
  DATASTORE_STOP,
  DATASTORE_SUSPEND,
  DATASTORE_MSG_TYPE_COUNT,
} DatastoreMsgtype_t;

/**
 * @brief   The datastore message.
 */
typedef struct
{
  DatastoreMsgtype_t msgType;
  DatapointType_t datapointType;
  uint32_t datapointId;
  size_t valCount;
  SrvMsgPayload_t *payload;
  struct k_msgq *response;
} DatastoreMsg_t;

/**
 * @brief   The service thread.
 */
static struct k_thread thread;

/**
 * @brief   The datastore buffer pool.
 */
static osMemoryPoolId_t bufferPool = NULL;

/**
 * @brief   The datastore message queue.
 */
K_MSGQ_DEFINE(datastoreQueue, sizeof(DatastoreMsg_t), DATASTORE_MSG_COUNT, 4);

/**
 * @brief   The datastore service thread function.
 *
 * @param[in]   p1: Thread first parameter.
 * @param[in]   p2: Thread second parameter.
 * @param[in]   p3: Thread third parameter.
 */
static void run(void *p1, void *p2, void *p3)
{
  int err;
  int errOp = 0;
  DatastoreMsg_t msg;

  LOG_INF("starting thread");

  // TODO: Initialize the datapoints from the NVM.

#ifdef CONFIG_ZTEST
  for(size_t i = 0; i < DATASTORE_RUN_ITERATIONS; ++i)
#else
  for(;;)
#endif
  {
    if(k_msgq_get(&datastoreQueue, &msg, K_MSEC(CONFIG_ENYA_DATASTORE_MSGQ_TIMEOUT)) == 0)
    {
      switch(msg.msgType)
      {
        case DATASTORE_READ:
          errOp = datastoreUtilRead(msg.datapointType, msg.datapointId, msg.valCount, msg.payload->data);
        break;
        case DATASTORE_WRITE:
          errOp = datastoreUtilWrite(msg.datapointType, msg.datapointId, msg.payload->data, msg.valCount, bufferPool);
          osMemoryPoolFree(msg.payload->poolId, msg.payload);
        break;
        case DATASTORE_STOP:
          serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
          return;
        case DATASTORE_SUSPEND:
          serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
          k_thread_suspend(k_current_get());
        break;
        default:
          LOG_WRN("unsupported message type %d", msg.msgType);
        break;
      }

      if(msg.response)
      {
        err = k_msgq_put(msg.response, &errOp, K_NO_WAIT);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to respond to operation %d for datapoint type %d with ID %d",
                  err, msg.msgType, msg.datapointType, msg.datapointId);
      }
    }

    serviceManagerUpdateHeartbeat(k_current_get());
  }
}

/**
 * @brief   Start callback: starts the datastore thread.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStart(void)
{
  k_thread_start(&thread);
  return 0;
}

/**
 * @brief   Stop callback: enqueues a stop message to the datastore queue.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onStop(void)
{
  int err;
  DatastoreMsg_t msg = { .msgType = DATASTORE_STOP };

  err = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue datastore stop message", err);

  return err;
}

/**
 * @brief   Suspend callback: enqueues a suspend message to the datastore queue.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onSuspend(void)
{
  int err;
  DatastoreMsg_t msg = { .msgType = DATASTORE_SUSPEND };

  err = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to enqueue datastore suspend message", err);

  return err;
}

/**
 * @brief   Resume callback: resumes the datastore thread.
 *
 * @return  0 if successful, the error code otherwise.
 */
static int onResume(void)
{
  k_thread_resume(&thread);
  return 0;
}

int datastoreInit(void)
{
  int err;
  k_tid_t threadId;
  size_t datapointCounts[DATAPOINT_TYPE_COUNT] = {BINARY_DATAPOINT_COUNT, BUTTON_DATAPOINT_COUNT, FLOAT_DATAPOINT_COUNT,
                                                  INT_DATAPOINT_COUNT, MULTI_STATE_DATAPOINT_COUNT, UINT_DATAPOINT_COUNT};
  ServiceDescriptor_t descriptor = {
    .priority            = CONFIG_ENYA_DATASTORE_SERVICE_PRIORITY,
    .heartbeatIntervalMs = CONFIG_ENYA_DATASTORE_HEARTBEAT_INTERVAL_MS,
    .start               = onStart,
    .stop                = onStop,
    .suspend             = onSuspend,
    .resume              = onResume,
  };

  err = datastoreUtilAllocateBinarySubs(CONFIG_ENYA_DATASTORE_MAX_BINARY_SUBS);
  if(err < 0)
    return err;

  err = datastoreUtilAllocateButtonSubs(CONFIG_ENYA_DATASTORE_MAX_BUTTON_SUBS);
  if(err < 0)
    return err;

  err = datastoreUtilAllocateFloatSubs(CONFIG_ENYA_DATASTORE_MAX_FLOAT_SUBS);
  if(err < 0)
    return err;

  err = datastoreUtilAllocateIntSubs(CONFIG_ENYA_DATASTORE_MAX_INT_SUBS);
  if(err < 0)
    return err;

  err = datastoreUtilAllocateMultiStateSubs(CONFIG_ENYA_DATASTORE_MAX_MULTI_STATE_SUBS);
  if(err < 0)
    return err;

  err = datastoreUtilAllocateUintSubs(CONFIG_ENYA_DATASTORE_MAX_UINT_SUBS);
  if(err < 0)
    return err;

  bufferPool = osMemoryPoolNew(DATASTORE_BUFFER_COUNT, datastoreUtilCalculateBufferSize(datapointCounts), NULL);
  if(!bufferPool)
    return -ENOSPC;

  threadId = k_thread_create(&thread, datastoreStack, CONFIG_ENYA_DATASTORE_STACK_SIZE, run,
                             NULL, NULL, NULL, K_PRIO_PREEMPT(CONFIG_ENYA_DATASTORE_THREAD_PRIORITY), 0, K_FOREVER);

  err = k_thread_name_set(threadId, STRINGIFY(DATASTORE_LOGGER_NAME));
  if(err < 0)
  {
    LOG_ERR("ERROR %d: unable to set datastore thread name", err);
    return err;
  }

  descriptor.threadId = threadId;

  err = serviceManagerRegisterSrv(&descriptor);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to register datastore service", err);

  return err;
}

int datastoreRead(DatapointType_t datapointType, uint32_t datapointId, size_t valCount,
                  struct k_msgq *response, Data_t values[])
{
  int err;
  int resStatus = 0;
  DatastoreMsg_t msg = {.msgType = DATASTORE_READ, .datapointType = datapointType, .datapointId = datapointId,
                        .valCount = valCount, .payload = NULL, .response = response };

  msg.payload = osMemoryPoolAlloc(bufferPool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!msg.payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for operation", err);
    return err;
  }

  msg.payload->poolId = bufferPool;
  msg.payload->dataLen = msg.valCount * sizeof(Data_t);

  err = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  if(err < 0)
  {
    osMemoryPoolFree(bufferPool, msg.payload);
    return err;
  }

  err = k_msgq_get(response, &resStatus, K_MSEC(DATASTORE_RESPONSE_TIMEOUT));
  if(err < 0)
  {
    osMemoryPoolFree(bufferPool, msg.payload);
    return err;
  }

  if(resStatus == 0)
    memcpy(values, msg.payload->data, msg.payload->dataLen);

  osMemoryPoolFree(bufferPool, msg.payload);

  return resStatus;
}

int datastoreWrite(DatapointType_t datapointType, uint32_t datapointId,
                   Data_t values[], size_t valCount, struct k_msgq *response)
{
  int err;
  int resStatus = 0;
  DatastoreMsg_t msg = {.msgType = DATASTORE_WRITE, .datapointType = datapointType, .datapointId = datapointId,
                        .valCount = valCount, .payload = NULL, .response = response };

  msg.payload = osMemoryPoolAlloc(bufferPool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!msg.payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for operation", err);
    return err;
  }

  msg.payload->poolId = bufferPool;
  msg.payload->dataLen = msg.valCount * sizeof(Data_t);

  memcpy(msg.payload->data, values, msg.payload->dataLen);

  err = k_msgq_put(&datastoreQueue, &msg, K_NO_WAIT);
  if(err < 0)
  {
    osMemoryPoolFree(bufferPool, msg.payload);
    return err;
  }

  if(response)
  {
    err = k_msgq_get(response, &resStatus, K_MSEC(DATASTORE_RESPONSE_TIMEOUT));
    if(err < 0)
    {
      osMemoryPoolFree(bufferPool, msg.payload);
      return err;
    }
  }

  return resStatus;
}

int datastoreSubscribeBinary(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddBinarySub(sub, bufferPool);
}

int datastoreUnsubscribeBinary(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveBinarySub(callback);
}

int datastorePauseSubBinary(DatastoreSubCb_t callback)
{
  return datastoreUtilSetBinarySubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubBinary(DatastoreSubCb_t callback)
{
  return datastoreUtilSetBinarySubPauseState(callback, false, bufferPool);
}

int datastoreReadBinary(uint32_t datapointId, size_t valCount, struct k_msgq *response, bool values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_BINARY, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read binary datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteBinary(uint32_t datapointId, bool values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_BINARY, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write binary datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreSubscribeButton(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddButtonSub(sub, bufferPool);
}

int datastoreUnsubscribeButton(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveButtonSub(callback);
}

int datastorePauseSubButton(DatastoreSubCb_t callback)
{
  return datastoreUtilSetButtonSubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubButton(DatastoreSubCb_t callback)
{
  return datastoreUtilSetButtonSubPauseState(callback, false, bufferPool);
}

int datastoreReadButton(uint32_t datapointId, size_t valCount, struct k_msgq *response, ButtonState_t values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_BUTTON, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read button datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteButton(uint32_t datapointId, ButtonState_t values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_BUTTON, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write button datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreSubscribeFloat(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddFloatSub(sub, bufferPool);
}

int datastoreUnsubscribeFloat(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveFloatSub(callback);
}

int datastorePauseSubFloat(DatastoreSubCb_t callback)
{
  return datastoreUtilSetFloatSubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubFloat(DatastoreSubCb_t callback)
{
  return datastoreUtilSetFloatSubPauseState(callback, false, bufferPool);
}

int datastoreReadFloat(uint32_t datapointId, size_t valCount, struct k_msgq *response, float values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_FLOAT, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read float datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteFloat(uint32_t datapointId, float values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_FLOAT, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write float datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreSubscribeInt(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddIntSub(sub, bufferPool);
}

int datastoreUnsubscribeInt(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveIntSub(callback);
}

int datastorePauseSubInt(DatastoreSubCb_t callback)
{
  return datastoreUtilSetIntSubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubInt(DatastoreSubCb_t callback)
{
  return datastoreUtilSetIntSubPauseState(callback, false, bufferPool);
}

int datastoreReadInt(uint32_t datapointId, size_t valCount, struct k_msgq *response, int32_t values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_INT, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read signed integer datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteInt(uint32_t datapointId, int32_t values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_INT, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write signed integer datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreSubscribeMultiState(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddMultiStateSub(sub, bufferPool);
}

int datastoreUnsubscribeMultiState(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveMultiStateSub(callback);
}

int datastorePauseSubMultiState(DatastoreSubCb_t callback)
{
  return datastoreUtilSetMultiStateSubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubMultiState(DatastoreSubCb_t callback)
{
  return datastoreUtilSetMultiStateSubPauseState(callback, false, bufferPool);
}

int datastoreReadMultiState(uint32_t datapointId, size_t valCount, struct k_msgq *response, uint32_t values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_MULTI_STATE, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read multi-state datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteMultiState(uint32_t datapointId, uint32_t values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_MULTI_STATE, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write multi-state datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreSubscribeUint(DatastoreSubEntry_t *sub)
{
  return datastoreUtilAddUintSub(sub, bufferPool);
}

int datastoreUnsubscribeUint(DatastoreSubCb_t callback)
{
  return datastoreUtilRemoveUintSub(callback);
}

int datastorePauseSubUint(DatastoreSubCb_t callback)
{
  return datastoreUtilSetUintSubPauseState(callback, true, bufferPool);
}

int datastoreUnpauseSubUint(DatastoreSubCb_t callback)
{
  return datastoreUtilSetUintSubPauseState(callback, false, bufferPool);
}

int datastoreReadUint(uint32_t datapointId, size_t valCount, struct k_msgq *response, uint32_t values[])
{
  int err;

  if(!values || valCount == 0 || !response)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreRead(DATAPOINT_UINT, datapointId, valCount, response, (Data_t *)values);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to read unsigned integer datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

int datastoreWriteUint(uint32_t datapointId, uint32_t values[], size_t valCount, struct k_msgq *response)
{
  int err;

  if(!values || valCount == 0)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid operation parameters", err);
    return err;
  }

  err = datastoreWrite(DATAPOINT_UINT, datapointId, (Data_t *)values, valCount, response);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to write unsigned integer datapoint %d up to datapoint %d", err, datapointId, datapointId + valCount);

  return err;
}

/** @} */
