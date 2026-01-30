/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      datastore.h
 * @author    jbacon
 * @date      2025-08-10
 * @brief     Datastore Service
 *
 *            Datastore service API definition
 *
 * @defgroup  datastore datastore
 *
 * @{
 */

#ifndef DATASTORE_SRV
#define DATASTORE_SRV

#include <zephyr/kernel.h>
#include <zephyr/portability/cmsis_os2.h>

#include "datastoreTypes.h"
#include "datastoreMeta.h"
#include "serviceCommon.h"

/**
 * @brief   Binary datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum BinaryDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_BINARY_DATAPOINTS
#undef X
  BINARY_DATAPOINT_COUNT,
};

/**
 * @brief   Button datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum ButtonDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_BUTTON_DATAPOINTS
#undef X
  BUTTON_DATAPOINT_COUNT,
};

/**
 * @brief   Float datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum FloatDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_FLOAT_DATAPOINTS
#undef X
  FLOAT_DATAPOINT_COUNT,
};

/**
 * @brief   signed integer datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum IntDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_INT_DATAPOINTS
#undef X
  INT_DATAPOINT_COUNT,
};

/**
 * @brief   Multi-state datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum MultiStateDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_MULTI_STATE_DATAPOINTS
#undef X
  MULTI_STATE_DATAPOINT_COUNT,
};

/**
 * @brief   Unsigned integer datapoint IDs.
 * @note    Data is coming from X-macros in datastoreMeta.h
 */
enum UintDatapoint
{
#define X(name, flags, defaultVal) name,
  DATASTORE_UINT_DATAPOINTS
#undef X
  UINT_DATAPOINT_COUNT,
};

/**
 * @brief   The subscription callback.
 */
typedef int (*DatastoreSubCb_t)(SrvMsgPayload_t *payload, size_t valCount);

/**
 * @brief   The subscription record.
 */
typedef struct
{
  uint32_t datapointId;                 /**< The datapoint ID */
  size_t valCount;                      /**< The datapoint count */
  bool isPaused;                        /**< The paused subscription flag */
  DatastoreSubCb_t callback;            /**< The subscription callback */
} DatastoreSubEntry_t;

/**
 * @brief   Initialize the datastore.
 *
 * @param[in]   maxSubs: The maximum subscriptions for each datatype.
 * @param[in]   priority: The datastore thread priority
 * @param[out]  threadId: The service thread ID.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreInit(size_t maxSubs[DATAPOINT_TYPE_COUNT], uint32_t priority, k_tid_t *threadId);

/**
 * @brief   Read a datapoint.
 *
 * @param[in]   datapointType: The datapoint type.
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreRead(DatapointType_t datapointType, uint32_t datapointId, size_t valCount,
                  struct k_msgq *response, Data_t values[]);

/**
 * @brief   Write a datapoint
 *
 * @param[in]   datapointType: The datapoint type.
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWrite(DatapointType_t datapointType, uint32_t datapointId,
                   Data_t values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to binary datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeBinary(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to binary datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeBinary(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to binary datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubBinary(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to binary datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubBinary(DatastoreSubCb_t callback);

/**
 * @brief   Read a binary datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadBinary(uint32_t datapointId, size_t valCount, struct k_msgq *response, bool values[]);

/**
 * @brief   Write a binary datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteBinary(uint32_t datapointId, bool values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to button datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeButton(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to button datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeButton(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to button datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubButton(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to button datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubButton(DatastoreSubCb_t callback);

/**
 * @brief   Read a button datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadButton(uint32_t datapointId, size_t valCount, struct k_msgq *response, ButtonState_t values[]);

/**
 * @brief   Write a button datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteButton(uint32_t datapointId, ButtonState_t values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to float datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeFloat(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to float datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeFloat(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to float datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubFloat(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to float datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubFloat(DatastoreSubCb_t callback);

/**
 * @brief   Read a float datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadFloat(uint32_t datapointId, size_t valCount, struct k_msgq *response, float values[]);

/**
 * @brief   Write a float datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteFloat(uint32_t datapointId, float values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to integer datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeInt(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to signed integer datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeInt(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to signed integer datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubInt(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to signed integer datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubInt(DatastoreSubCb_t callback);

/**
 * @brief   Read a integer datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadInt(uint32_t datapointId, size_t valCount, struct k_msgq *response, int32_t values[]);

/**
 * @brief   Write a integer datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteInt(uint32_t datapointId, int32_t values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to multi-state datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeMultiState(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to multi-state datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeMultiState(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to multi-state datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubMultiState(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to multi-state datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubMultiState(DatastoreSubCb_t callback);

/**
 * @brief   Read a multi-state datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadMultiState(uint32_t datapointId, size_t valCount, struct k_msgq *response, uint32_t values[]);

/**
 * @brief   Write a multi-state datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteMultiState(uint32_t datapointId, uint32_t values[], size_t valCount, struct k_msgq *response);

/**
 * @brief   Subscribe to unsigned integer datapoint.
 *
 * @param[in]   sub: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreSubscribeUint(DatastoreSubEntry_t *sub);

/**
 * @brief   Unsubscribe to unsigned integer datapoint.
 *
 * @param[in]   callback: The callback of the subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnsubscribeUint(DatastoreSubCb_t callback);

/**
 * @brief   Pause subscription to unsigned integer datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastorePauseSubUint(DatastoreSubCb_t callback);

/**
 * @brief   Unpause subscription to unsigned integer datapoint.
 *
 * @param[in]   callback: The subscription.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreUnpauseSubUint(DatastoreSubCb_t callback);

/**
 * @brief   Read an unsigned integer datapoint.
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   valCount: The count of value to read.
 * @param[in]   response: The response queue.
 * @param[out]  values: The output buffer.
 *
 * @return  0 if successful, the error code otherwise.
 */
int datastoreReadUint(uint32_t datapointId, size_t valCount, struct k_msgq *response, uint32_t values[]);

/**
 * @brief   Write an unsigned integer datapoint
 *
 * @param[in]   datapointId: The datapoint ID.
 * @param[in]   values: The values to write.
 * @param[in]   valCount: The count of values to write.
 * @param[in]   response: The response queue (NULL, if not needed).
 *
 * @return  0 if successful, the error code.
 */
int datastoreWriteUint(uint32_t datapointId, uint32_t values[], size_t valCount, struct k_msgq *response);

#endif    /* DATASTORE_SRV */

/** @} */
