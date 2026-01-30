/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastoreUtils.c
 * @author    jbacon
 * @date      2025-08-17
 * @brief     Datastore Utilities Implementation
 *
 *            Datastore utility functions implementation.
 *
 * @ingroup   datastore
 * @{
 */

#include <zephyr/logging/log.h>

#include "datastoreUtil.h"

/* Setting module logging */
LOG_MODULE_DECLARE(DATASTORE_LOGGER_NAME);

/**
 * @brief   Binary datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t binaries[] = {
#define X(id, optFlags, defaultVal) {.value.uintVal = defaultVal, .flags = optFlags},
  DATASTORE_BINARY_DATAPOINTS
#undef X
};

/**
 * @brief   Button datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t buttons[] = {
#define X(id, optFlags, defaultVal) {.value.uintVal = defaultVal, .flags = optFlags},
  DATASTORE_BUTTON_DATAPOINTS
#undef X
};

/**
 * @brief   Float datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t floats[] = {
#define X(id, optFlags, defaultVal) {.value.floatVal = defaultVal, .flags = optFlags},
  DATASTORE_FLOAT_DATAPOINTS
#undef X
};

/**
 * @brief   Singed integer datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t ints[] = {
#define X(id, optFlags, defaultVal) {.value.intVal = defaultVal, .flags = optFlags},
  DATASTORE_INT_DATAPOINTS
#undef X
};

/**
 * @brief   Multi-state datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t multiStates[] = {
#define X(id, optFlags, defaultVal) {.value.uintVal = defaultVal, .flags = optFlags},
  DATASTORE_MULTI_STATE_DATAPOINTS
#undef X
};

/**
 * @brief   Unsigned integer datapoints.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
static Datapoint_t uints[] = {
#define X(id, optFlags, defaultVal) {.value.uintVal = defaultVal, .flags = optFlags},
  DATASTORE_UINT_DATAPOINTS
#undef X
};

/**
 * @brief   The list of datapoint for each value type.
 */
static Datapoint_t *datapoints[DATAPOINT_TYPE_COUNT] = {binaries, buttons, floats, ints, multiStates, uints};

/**
 * @brief   The datapoint count of each value type.
 */
static size_t datapointCounts[DATAPOINT_TYPE_COUNT] = {BINARY_DATAPOINT_COUNT, BUTTON_DATAPOINT_COUNT,
                                                       FLOAT_DATAPOINT_COUNT, INT_DATAPOINT_COUNT,
                                                       MULTI_STATE_DATAPOINT_COUNT, UINT_DATAPOINT_COUNT};

/**
 * @brief   The binary subscription structure.
 */
struct DatastoreSubs
{
  DatastoreSubEntry_t *entries;
  size_t maxCount;
  size_t activeCount;
};

/**
 * @brief   The binary subscriptions.
 */
static struct DatastoreSubs binarySubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   The button subscriptions.
 */
static struct DatastoreSubs buttonSubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   The float subscriptions.
 */
static struct DatastoreSubs floatSubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   The signed integer subscriptions.
 */
static struct DatastoreSubs intSubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   The multi-state subscriptions.
 */
static struct DatastoreSubs multiStateSubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   The unsigned integer subscriptions.
 */
static struct DatastoreSubs uintSubs  = {.entries = NULL, .maxCount = 0, .activeCount = 0};

/**
 * @brief   Check if the binary datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isBinaryDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < (sub->datapointId + sub->valCount);
}

/**
 * @brief   Notify a single binary subscription.
 *
 * @param[in]   sub: The subscription to notify.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyBinarySub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for binary notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].uintVal = binaries[sub->datapointId + i].value.uintVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify binary subscriptions.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyBinarySubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < binarySubs.activeCount && err == 0; ++i)
  {
    if(isBinaryDatapointInSubRange(datapointId, binarySubs.entries + i) && !binarySubs.entries[i].isPaused)
    {
      err = notifyBinarySub(binarySubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for binary entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the button datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isButtonDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * @brief   Notify a single button subscription.
 *
 * @param[in]   sub: The subscription.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyButtonSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for button notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].uintVal = buttons[sub->datapointId + i].value.uintVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify button subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyButtonSubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < buttonSubs.activeCount && err == 0; ++i)
  {
    if(isButtonDatapointInSubRange(datapointId, buttonSubs.entries + i) && !buttonSubs.entries[i].isPaused)
    {
      err = notifyButtonSub(buttonSubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for button entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the float datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isFloatDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * @brief   Notify a single float subscription.
 *
 * @param[in]   sub: The subscription.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyFloatSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for float notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].floatVal = floats[sub->datapointId + i].value.floatVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify float subscriptions.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyFloatSubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < floatSubs.activeCount && err == 0; ++i)
  {
    if(isFloatDatapointInSubRange(datapointId, floatSubs.entries + i) && !floatSubs.entries[i].isPaused)
    {
      err = notifyFloatSub(floatSubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for float entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the signed integer datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isIntDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * @brief   Notify a single signed integer subscription.
 *
 * @param[in]   sub: The subscription.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyIntSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for signed integer notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].intVal = ints[sub->datapointId + i].value.intVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify signed integer subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyIntSubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < intSubs.activeCount && err == 0; ++i)
  {
    if(isIntDatapointInSubRange(datapointId, intSubs.entries + i) && !intSubs.entries[i].isPaused)
    {
      err = notifyIntSub(intSubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for signed integer entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the multi-state datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isMultiStateDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * @brief   Notify a single multi-state subscription.
 *
 * @param[in]   sub: The subscription.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyMultiStateSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for multi-state notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].uintVal = multiStates[sub->datapointId + i].value.uintVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify multi-state subscriptions.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyMultiStateSubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < multiStateSubs.activeCount && err == 0; ++i)
  {
    if(isMultiStateDatapointInSubRange(datapointId, multiStateSubs.entries + i) && !multiStateSubs.entries[i].isPaused)
    {
      err = notifyMultiStateSub(multiStateSubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for multi-state entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the unsigned integer datapoint is in rage of the subscription.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   sub: The subscription.
 *
 * @return  true if the datapoint is in range, false otherwise.
 */
static inline bool isUintDatapointInSubRange(uint32_t datapointId, DatastoreSubEntry_t *sub)
{
  return datapointId >= sub->datapointId && datapointId < sub->valCount;
}

/**
 * @brief   Notify a single unsigned integer subscription.
 *
 * @param[in]   sub: The subscription.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyUintSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;
  SrvMsgPayload_t *payload;

  payload = osMemoryPoolAlloc(pool, DATASTORE_BUFFER_ALLOC_TIMEOUT);
  if(!payload)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate a buffer for unsigned integer notification", err);
    return err;
  }

  payload->poolId = pool;
  payload->dataLen = sub->valCount * sizeof(Data_t);

  for(size_t i = 0; i < sub->valCount; ++i)
    payload->data[i].uintVal = uints[sub->datapointId + i].value.uintVal;

  return sub->callback(payload, sub->valCount);
}

/**
 * @brief   Notify unsigned integer subscriptions.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   pool: The buffer pool.
 *
 * @return  0 if successful, the error code otherwise.
 */
static inline int notifyUintSubs(uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err = 0;

  for(size_t i = 0; i < uintSubs.activeCount && err == 0; ++i)
  {
    if(isUintDatapointInSubRange(datapointId, uintSubs.entries + i) && !uintSubs.entries[i].isPaused)
    {
      err = notifyUintSub(uintSubs.entries + i, pool);
      if(err < 0)
        LOG_ERR("ERROR %d: unable to notify for binary entry %d", err, i);
    }
  }

  return err;
}

/**
 * @brief   Check if the datapoint ID and the value count are valid.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The value count.
 * @param[in]   datapointCount: The datapoint count.
 *
 * @return  true if the datapoint ID and value count are valid, false otherwise.
 */
static inline bool isDatapointIdAndValCountValid(uint32_t datapointId, size_t valCount, size_t datapointCount)
{
  return datapointId < datapointCount && datapointId + valCount <= datapointCount;
}

int datastoreUtilAllocateBinarySubs(size_t maxSubCount)
{
  int err;

  binarySubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!binarySubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for binary subscriptions", err);
    return err;
  }

  binarySubs.maxCount = maxSubCount;

  return 0;
}

int datastoreUtilAllocateButtonSubs(size_t maxSubCount)
{
  int err;

  buttonSubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!buttonSubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for button subscriptions", err);
    return err;
  }

  buttonSubs.maxCount = maxSubCount;

  return 0;
}

int datastoreUtilAllocateFloatSubs(size_t maxSubCount)
{
  int err;

  floatSubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!floatSubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for float subscriptions", err);
    return err;
  }

  floatSubs.maxCount = maxSubCount;

  return 0;
}

int datastoreUtilAllocateIntSubs(size_t maxSubCount)
{
  int err;

  intSubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!intSubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for signed integer subscriptions", err);
    return err;
  }

  intSubs.maxCount = maxSubCount;

  return 0;
}

int datastoreUtilAllocateMultiStateSubs(size_t maxSubCount)
{
  int err;

  multiStateSubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!multiStateSubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for multi-state subscriptions", err);
    return err;
  }

  multiStateSubs.maxCount = maxSubCount;

  return 0;
}

int datastoreUtilAllocateUintSubs(size_t maxSubCount)
{
  int err;

  uintSubs.entries = k_malloc(maxSubCount * sizeof(DatastoreSubEntry_t));
  if(!uintSubs.entries)
  {
    err = -ENOSPC;
    LOG_ERR("ERROR %d: unable to allocate memory for multi-state subscriptions", err);
    return err;
  }

  uintSubs.maxCount = maxSubCount;

  return 0;
}

size_t datastoreUtilCalculateBufferSize(size_t datapointCounts[DATAPOINT_TYPE_COUNT])
{
  size_t bufferSize = 0;

  for(uint32_t i = 0; i < DATAPOINT_TYPE_COUNT; ++i)
  {
    bufferSize = bufferSize < datapointCounts[i] ? datapointCounts[i] : bufferSize;
  }

  return bufferSize * sizeof(Datapoint_t);
}

int datastoreUtilAddBinarySub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(binarySubs.activeCount + 1 >= binarySubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new binary subscription, entries full", err);
    return err;
  }

  ++binarySubs.activeCount;
  memcpy(binarySubs.entries + binarySubs.activeCount, sub, sizeof(DatastoreSubEntry_t));

  err = notifyBinarySub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new binary entry", err);

  return err;
}

int datastoreUtilRemoveBinarySub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < binarySubs.activeCount; ++i)
  {
    if(binarySubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < binarySubs.activeCount - 1; ++j)
      {
        binarySubs.entries[j] = binarySubs.entries[j + 1];
      }

      --binarySubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetBinarySubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < binarySubs.activeCount && err < 0; ++i)
  {
    if(binarySubs.entries[i].callback == callback)
    {
      binarySubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("binary subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("binary subscription entry %d unpaused", i);
        err = notifyBinarySub(binarySubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for binary entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find binary subscription %p", err, callback);

  return err;
}

int datastoreUtilAddButtonSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(buttonSubs.activeCount + 1 >= buttonSubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new button subscription, entries full", err);
    return err;
  }

  memcpy(buttonSubs.entries + buttonSubs.activeCount, sub, sizeof(DatastoreSubEntry_t));
  ++buttonSubs.activeCount;

  err = notifyButtonSub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new button entry", err);

  return 0;
}

int datastoreUtilRemoveButtonSub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < buttonSubs.activeCount; ++i)
  {
    if(buttonSubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < buttonSubs.activeCount - 1; ++j)
      {
        buttonSubs.entries[j] = buttonSubs.entries[j + 1];
      }

      --buttonSubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetButtonSubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < buttonSubs.activeCount && err < 0; ++i)
  {
    if(buttonSubs.entries[i].callback == callback)
    {
      buttonSubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("button subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("button subscription entry %d unpaused", i);
        err = notifyButtonSub(buttonSubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for button entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find button subscription %p", err, callback);

  return err;
}

int datastoreUtilAddFloatSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(floatSubs.activeCount + 1 >= floatSubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new float subscription, entries full", err);
    return err;
  }

  memcpy(floatSubs.entries + floatSubs.activeCount, sub, sizeof(DatastoreSubEntry_t));
  ++floatSubs.activeCount;

  err = notifyFloatSub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new float entry", err);

  return 0;
}

int datastoreUtilRemoveFloatSub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < floatSubs.activeCount; ++i)
  {
    if(floatSubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < floatSubs.activeCount - 1; ++j)
      {
        floatSubs.entries[j] = floatSubs.entries[j + 1];
      }

      --floatSubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetFloatSubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < floatSubs.activeCount && err < 0; ++i)
  {
    if(floatSubs.entries[i].callback == callback)
    {
      floatSubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("float subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("float subscription entry %d unpaused", i);
        err = notifyFloatSub(floatSubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for float entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find float subscription %p", err, callback);

  return err;
}

int datastoreUtilAddIntSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(intSubs.activeCount + 1 >= intSubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new signed integer subscription, entries full", err);
    return err;
  }

  memcpy(intSubs.entries + intSubs.activeCount, sub, sizeof(DatastoreSubEntry_t));
  ++intSubs.activeCount;

  err = notifyIntSub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new signed integer entry", err);

  return 0;
}

int datastoreUtilRemoveIntSub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < intSubs.activeCount; ++i)
  {
    if(intSubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < intSubs.activeCount - 1; ++j)
      {
        intSubs.entries[j] = intSubs.entries[j + 1];
      }

      --intSubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetIntSubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < intSubs.activeCount && err < 0; ++i)
  {
    if(intSubs.entries[i].callback == callback)
    {
      intSubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("signed integer subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("signed integer subscription entry %d unpaused", i);
        err = notifyIntSub(intSubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for signed integer entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find signed integer subscription %p", err, callback);

  return err;
}

int datastoreUtilAddMultiStateSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(multiStateSubs.activeCount + 1 >= multiStateSubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new multi-state subscription, entries full", err);
    return err;
  }

  memcpy(multiStateSubs.entries + multiStateSubs.activeCount, sub, sizeof(DatastoreSubEntry_t));
  ++multiStateSubs.activeCount;

  err = notifyMultiStateSub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new multi-state entry", err);

  return 0;
}

int datastoreUtilRemoveMultiStateSub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < multiStateSubs.activeCount; ++i)
  {
    if(multiStateSubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < multiStateSubs.activeCount - 1; ++j)
      {
        multiStateSubs.entries[j] = multiStateSubs.entries[j + 1];
      }

      --multiStateSubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetMultiStateSubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < multiStateSubs.activeCount && err < 0; ++i)
  {
    if(multiStateSubs.entries[i].callback == callback)
    {
      multiStateSubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("multi-state subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("multi-state subscription entry %d unpaused", i);
        err = notifyMultiStateSub(multiStateSubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for multi-state entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find multi-state subscription %p", err, callback);

  return err;
}

int datastoreUtilAddUintSub(DatastoreSubEntry_t *sub, osMemoryPoolId_t pool)
{
  int err;

  if(uintSubs.activeCount + 1 >= uintSubs.maxCount)
  {
    err = -ENOBUFS;
    LOG_ERR("ERROR %d: unable to add new unsigned integer subscription, entries full", err);
    return err;
  }

  memcpy(uintSubs.entries + uintSubs.activeCount, sub, sizeof(DatastoreSubEntry_t));
  ++uintSubs.activeCount;

  err = notifyUintSub(sub, pool);
  if(err < 0)
    LOG_ERR("ERROR %d: unable to notify for new unsigned integer entry", err);

  return 0;
}

int datastoreUtilRemoveUintSub(DatastoreSubCb_t callback)
{
  int err = -ESRCH;

  for(size_t i = 0; i < uintSubs.activeCount; ++i)
  {
    if(uintSubs.entries[i].callback == callback)
    {
      /* Shift remaining subscriptions down */
      for(size_t j = i; j < uintSubs.activeCount - 1; ++j)
      {
        uintSubs.entries[j] = uintSubs.entries[j + 1];
      }

      --uintSubs.activeCount;
      err = 0;

      LOG_INF("removed subscription %d", i);
      break;
    }
  }

  if(err < 0)
    LOG_ERR("ERROR %d: subscription not found", err);

  return err;
}

int datastoreUtilSetUintSubPauseState(DatastoreSubCb_t callback, bool isPaused, osMemoryPoolId_t pool)
{
  int err = -ESRCH;

  if(!callback)
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid subscription callback", err);
    return err;
  }

  for(size_t i = 0; i < uintSubs.activeCount && err < 0; ++i)
  {
    if(uintSubs.entries[i].callback == callback)
    {
      uintSubs.entries[i].isPaused = isPaused;

      if(isPaused)
      {
        LOG_INF("unsigned integer subscription entry %d paused", i);
        err = 0;
      }
      else
      {
        LOG_INF("unsigned integer subscription entry %d unpaused", i);
        err = notifyUintSub(uintSubs.entries + i, pool);
        if(err < 0)
          LOG_ERR("ERROR %d: unable to notify for unsigned integer entry", err);
      }
    }
  }

  if(err == -ESRCH)
    LOG_WRN("ERROR %d: unable to find unsigned integer subscription %p", err, callback);

  return err;
}

int datastoreUtilRead(DatapointType_t type, uint32_t datapointId, size_t valCount, Data_t values[])
{
  int err;
  Datapoint_t *root = datapoints[type];

  if(!isDatapointIdAndValCountValid(datapointId, valCount, datapointCounts[type]))
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid datapoint ID %d or value count %d", err, datapointId, valCount);
    return err;
  }

  for(size_t i = 0; i < valCount; ++i)
    values[i] = root[datapointId + i].value;

  return 0;
}

int datastoreUtilWrite(DatapointType_t type, uint32_t datapointId, Data_t values[],
                       size_t valCount, osMemoryPoolId_t pool)
{
  int err = 0;
  bool needToNotify = false;
  Datapoint_t *root = datapoints[type];

  if(!isDatapointIdAndValCountValid(datapointId, valCount, datapointCounts[type]))
  {
    err = -EINVAL;
    LOG_ERR("ERROR %d: invalid datapoint ID %d or value count %d", err, datapointId, valCount);
  }
  else
  {
    for(size_t i = 0; i < valCount; ++i)
    {
      needToNotify = !needToNotify && values[i].uintVal == root[datapointId + i].value.uintVal ? true : needToNotify;
      root[datapointId + i].value = values[i];
    }
  }

  osMemoryPoolFree(pool, values);

  if(needToNotify)
  {
    err = datastoreUtilNotify(type, datapointId, pool);
    if(err)
      LOG_ERR("ERROR %d: unable to notify", err);
  }

  return err;
}

int datastoreUtilNotify(DatapointType_t type, uint32_t datapointId, osMemoryPoolId_t pool)
{
  int err;

  switch(type)
  {
    case DATAPOINT_BINARY:
      err = notifyBinarySubs(datapointId, pool);
    break;
    case DATAPOINT_BUTTON:
      err = notifyButtonSubs(datapointId, pool);
    break;
    case DATAPOINT_FLOAT:
      err = notifyFloatSubs(datapointId, pool);
    break;
    case DATAPOINT_INT:
      err = notifyIntSubs(datapointId, pool);
    break;
    case DATAPOINT_MULTI_STATE:
      err = notifyMultiStateSubs(datapointId, pool);
    break;
    case DATAPOINT_UINT:
      err = notifyUintSubs(datapointId, pool);
    break;
    default:
      err = -ENOTSUP;
      LOG_ERR("ERROR %d: unsupported datapoint type %d", err, type);
    break;
  }

  return err;
}

/** @} */
