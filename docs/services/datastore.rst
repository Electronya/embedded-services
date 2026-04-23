Datastore Service
=================

Overview
--------

The Datastore Service provides a centralized, thread-safe data management layer for embedded
applications. It stores multiple datapoint types, notifies subscribers on changes, and
optionally persists values to non-volatile memory. All operations are asynchronous —
clients submit requests via a message queue, and the datastore thread processes them
in order.

Features
--------

- **Six Datapoint Types**: Binary, Button, Float, Integer, Multi-State, and Unsigned Integer
- **Publish-Subscribe Pattern**: Asynchronous callbacks on datapoint value changes
- **Thread-Safe**: Dedicated service thread with message queue synchronization
- **Blocking Read/Write**: Typed helpers block until the operation completes or times out
- **Optional NVM Persistence**: Per-datapoint flag to survive power cycles
- **X-Macro Configuration**: Compile-time datapoint definitions in ``datastoreMeta.h``
- **Shell Commands**: Runtime inspection and modification via Zephyr shell
- **Service Manager Integration**: Lifecycle management and watchdog-backed heartbeat monitoring

Architecture
------------

Thread Model
~~~~~~~~~~~~

The datastore runs in a dedicated thread that loops at ``CONFIG_ENYA_DATASTORE_MSGQ_TIMEOUT``
intervals:

1. Dequeues read/write/stop/suspend messages from the internal message queue
2. Executes the operation against the datapoint storage arrays
3. Notifies active subscribers when a datapoint value changes
4. Sends an ``int`` status code back to the caller via the caller-supplied response queue

Read and write operations block on the caller side until the datastore thread responds
or the operation times out (5 ms internal timeout for reads, configurable for writes).

Data Flow
~~~~~~~~~

.. mermaid::

   flowchart LR
       A[Client] -->|datastoreRead / Write| B[Message Queue]
       B --> C[Datastore Thread]
       C --> D[Datapoint Storage]
       C -->|int status| E[Response Queue]
       C -->|SrvMsgPayload_t*| F[Subscribers]
       E --> A

Subscription Pattern
~~~~~~~~~~~~~~~~~~~~

Subscriptions are registered per datapoint type and ID range. When a write commits
successfully, the datastore thread allocates a ``SrvMsgPayload_t`` block from the internal
CMSIS memory pool, copies the new values into it, and calls each active subscriber callback.

.. important::

   The **callback is responsible for freeing the payload** before returning:

   .. code-block:: c

      osMemoryPoolFree(payload->poolId, payload);

   Failing to free the payload will eventually exhaust the pool and cause write operations
   to fail with ``-ENOSPC``.

Service Manager Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The datastore registers itself with the service manager at init time. The service manager
handles lifecycle (start / stop / suspend / resume) and monitors health via periodic
heartbeats. ``datastoreInit()`` must be called before ``serviceManagerStart()``.

Configuration
-------------

Kconfig Options
~~~~~~~~~~~~~~~

Enable the datastore in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_ENYA_DATASTORE=y
   CONFIG_ENYA_DATASTORE_LOG_LEVEL=3

   # Required dependency
   CONFIG_CMSIS_RTOS_V2=y

Optional configuration:

.. code-block:: kconfig

   # Thread configuration
   CONFIG_ENYA_DATASTORE_STACK_SIZE=512
   CONFIG_ENYA_DATASTORE_THREAD_PRIORITY=5
   CONFIG_ENYA_DATASTORE_MSGQ_TIMEOUT=100

   # Service Manager integration
   CONFIG_ENYA_DATASTORE_SERVICE_PRIORITY=1
   CONFIG_ENYA_DATASTORE_HEARTBEAT_INTERVAL_MS=1000

   # Maximum subscribers per datapoint type
   CONFIG_ENYA_DATASTORE_MAX_BINARY_SUBS=2
   CONFIG_ENYA_DATASTORE_MAX_BUTTON_SUBS=2
   CONFIG_ENYA_DATASTORE_MAX_FLOAT_SUBS=2
   CONFIG_ENYA_DATASTORE_MAX_INT_SUBS=2
   CONFIG_ENYA_DATASTORE_MAX_MULTI_STATE_SUBS=2
   CONFIG_ENYA_DATASTORE_MAX_UINT_SUBS=2

   # Shell commands
   CONFIG_ENYA_DATASTORE_SHELL=y
   CONFIG_ENYA_DATASTORE_BUFFER_SIZE=32

Defining Datapoints
~~~~~~~~~~~~~~~~~~~

Applications must provide a ``datastoreMeta.h`` in their ``src/`` directory. This file
defines all datapoints using X-macros in the format ``X(id, flags, default_value)``.

**Flags:**

- ``DATAPOINT_NO_FLAG_MASK`` — value is not persisted to NVM
- ``DATAPOINT_FLAG_NVM_MASK`` — value is persisted to NVM

.. code-block:: c

   #ifndef DATASTORE_META_H
   #define DATASTORE_META_H

   #include "datastoreTypes.h"

   /* Multi-state enumerations */
   typedef enum
   {
     SYSTEM_IDLE = 0,
     SYSTEM_RUNNING,
     SYSTEM_FAULT,
     SYSTEM_STATE_COUNT
   } SystemState_t;

   #define DATASTORE_BINARY_DATAPOINTS \
     X(SENSOR_ENABLED,  DATAPOINT_FLAG_NVM_MASK, true)  \
     X(ALARM_ACTIVE,    DATAPOINT_NO_FLAG_MASK,  false)

   #define DATASTORE_BUTTON_DATAPOINTS \
     X(USER_BUTTON,     DATAPOINT_NO_FLAG_MASK,  0)

   #define DATASTORE_FLOAT_DATAPOINTS \
     X(TEMPERATURE,     DATAPOINT_NO_FLAG_MASK,  25.0f) \
     X(SETPOINT,        DATAPOINT_FLAG_NVM_MASK, 20.0f)

   #define DATASTORE_INT_DATAPOINTS \
     X(ERROR_COUNT,     DATAPOINT_NO_FLAG_MASK,  0)

   #define DATASTORE_MULTI_STATE_DATAPOINTS \
     X(SYSTEM_MODE,     DATAPOINT_FLAG_NVM_MASK, SYSTEM_IDLE)

   #define DATASTORE_UINT_DATAPOINTS \
     X(CYCLE_COUNT,     DATAPOINT_NO_FLAG_MASK,  0)

   #endif /* DATASTORE_META_H */

The datapoint IDs are generated as enumerations in ``datastore.h`` via the same X-macros.
Use the enum names (e.g. ``SENSOR_ENABLED``) when calling the API.

API Usage
---------

Initialization
~~~~~~~~~~~~~~

.. code-block:: c

   #include "datastore.h"

   int err = datastoreInit();
   if (err < 0) {
     LOG_ERR("Datastore init failed: %d", err);
     return err;
   }

Reading Datapoints
~~~~~~~~~~~~~~~~~~

The typed read helpers are blocking: they return once the datastore thread completes the
read or the internal timeout (5 ms) expires. The caller must provide a response queue
with element size ``sizeof(int)`` and capacity of at least 1.

.. code-block:: c

   bool values[2];
   int resStatus;
   struct k_msgq responseQueue;
   k_msgq_init(&responseQueue, (char *)&resStatus, sizeof(int), 1);

   int err = datastoreReadBinary(SENSOR_ENABLED, 2, &responseQueue, values);
   if (err == 0) {
     LOG_INF("SENSOR_ENABLED=%d  ALARM_ACTIVE=%d", values[0], values[1]);
   }

The same pattern applies to all typed helpers: ``datastoreReadButton``,
``datastoreReadFloat``, ``datastoreReadInt``, ``datastoreReadMultiState``,
``datastoreReadUint``.

Writing Datapoints
~~~~~~~~~~~~~~~~~~

Write with confirmation (blocks until committed):

.. code-block:: c

   bool values[] = {true};
   int resStatus;
   struct k_msgq responseQueue;
   k_msgq_init(&responseQueue, (char *)&resStatus, sizeof(int), 1);

   int err = datastoreWriteBinary(SENSOR_ENABLED, values, 1, &responseQueue);

Write without confirmation (fire-and-forget):

.. code-block:: c

   bool values[] = {false};
   datastoreWriteBinary(ALARM_ACTIVE, values, 1, NULL);

Subscribing to Datapoint Changes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Callbacks receive a ``SrvMsgPayload_t`` containing the new values as ``Data_t`` elements.
The callback **must** free the payload before returning.

.. code-block:: c

   int onSensorEnabledChanged(SrvMsgPayload_t *payload, size_t valCount)
   {
     bool enabled = payload->data[0].uintVal;

     /* CRITICAL: always free payload before returning */
     osMemoryPoolFree(payload->poolId, payload);

     if (enabled) {
       startSensor();
     } else {
       stopSensor();
     }
     return 0;
   }

   DatastoreSubEntry_t sub = {
     .datapointId = SENSOR_ENABLED,
     .valCount    = 1,
     .isPaused    = false,
     .callback    = onSensorEnabledChanged,
   };

   int err = datastoreSubscribeBinary(&sub);
   if (err < 0) {
     LOG_ERR("Subscribe failed: %d", err);
   }

Managing Subscriptions
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   /* Pause a subscription (callbacks stop until unpaused) */
   err = datastorePauseSubBinary(onSensorEnabledChanged);

   /* Resume a paused subscription */
   err = datastoreUnpauseSubBinary(onSensorEnabledChanged);

   /* Remove subscription completely */
   err = datastoreUnsubscribeBinary(onSensorEnabledChanged);

The same ``Subscribe`` / ``Unsubscribe`` / ``PauseSub`` / ``UnpauseSub`` pattern is
available for all types: ``Button``, ``Float``, ``Int``, ``MultiState``, ``Uint``.

Shell Commands
--------------

The datastore shell commands are registered under ``ds`` and operate on datapoints by name
(case-insensitive). Enable with ``CONFIG_ENYA_DATASTORE_SHELL=y``.

List All Datapoints
~~~~~~~~~~~~~~~~~~~

Prints a table showing ID, name, type, and current value for every datapoint:

.. code-block:: console

   uart:~$ ds ls
   ID  Name                                     Type            Value
   -----------------------------------------------------------------------
   0   SENSOR_ENABLED                           Binary          true
   1   ALARM_ACTIVE                             Binary          false
   0   USER_BUTTON                              Button          unpressed
   0   TEMPERATURE                              Float           25.000
   1   SETPOINT                                 Float           20.000
   0   ERROR_COUNT                              Integer         0
   0   SYSTEM_MODE                              Multi-State     0
   0   CYCLE_COUNT                              Unsigned Int    0

Read a Datapoint
~~~~~~~~~~~~~~~~

.. code-block:: console

   # Read a single value (default count = 1)
   uart:~$ ds read sensor_enabled
   SUCCESS: read 1 value(s) from SENSOR_ENABLED
   SENSOR_ENABLED = true

   # Read multiple consecutive values
   uart:~$ ds read temperature 2
   SUCCESS: read 2 value(s) from TEMPERATURE
   TEMPERATURE = 25.000
   SETPOINT = 20.000

Write a Datapoint
~~~~~~~~~~~~~~~~~

.. code-block:: console

   # Write a binary value
   uart:~$ ds write alarm_active false
   SUCCESS: wrote 1 value(s) to ALARM_ACTIVE

   # Write a button state (unpressed | short_pressed | long_pressed)
   uart:~$ ds write user_button short_pressed
   SUCCESS: wrote 1 value(s) to USER_BUTTON

   # Write a float value
   uart:~$ ds write setpoint 22.5
   SUCCESS: wrote 1 value(s) to SETPOINT

   # Write an integer value
   uart:~$ ds write error_count -5
   SUCCESS: wrote 1 value(s) to ERROR_COUNT

   # Write a multi-state value (numeric enum value)
   uart:~$ ds write system_mode 1
   SUCCESS: wrote 1 value(s) to SYSTEM_MODE

Best Practices
--------------

1. **Always Free Subscription Payload**: Call ``osMemoryPoolFree(payload->poolId, payload)``
   before the callback returns — every time, without exception.
2. **Keep Callbacks Fast**: Callbacks run in the datastore thread; blocking delays
   will stall all pending operations.
3. **Copy and Defer Heavy Processing**: Copy payload data to a local buffer, free immediately,
   then post to your own queue for processing:

   .. code-block:: c

      int callback(SrvMsgPayload_t *payload, size_t valCount)
      {
        Data_t local[MY_VAL_COUNT];
        memcpy(local, payload->data, payload->dataLen);
        osMemoryPoolFree(payload->poolId, payload);
        return k_msgq_put(&myQueue, local, K_NO_WAIT);
      }

4. **Initialize Before Service Manager**: Call ``datastoreInit()`` before
   ``serviceManagerStart()`` so the service is registered in time.
5. **Use NVM Flags Sparingly**: Only apply ``DATAPOINT_FLAG_NVM_MASK`` to values that
   genuinely require persistence; NVM writes add latency and wear.
6. **Response Queue Lifetime**: The response queue passed to read/write must remain
   valid until the call returns (stack allocation is fine for blocking calls).

Troubleshooting
---------------

Pool Exhaustion (-ENOSPC on Write)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: Write operations return ``-ENOSPC``.

**Causes**:

- Subscription callbacks not freeing ``payload`` before returning
- Pool block count too low for the number of concurrent notifications

**Solutions**:

- Verify every callback calls ``osMemoryPoolFree(payload->poolId, payload)``
- Increase ``CONFIG_ENYA_DATASTORE_MAX_*_SUBS`` (increases pool block count)
- Increase ``CONFIG_HEAP_MEM_POOL_SIZE`` if pool allocation itself is failing

Subscription Not Receiving Updates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: Callback is never invoked after a write.

**Solutions**:

- Verify ``datastoreSubscribe*`` returned 0
- Ensure the subscription is not paused (``isPaused = false`` at registration)
- Confirm the write targets the correct ``datapointId`` and type
- Check that the write is completing successfully (non-NULL response queue)

Response Timeout (-EAGAIN on Read/Write)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: Read or write returns ``-EAGAIN`` or ``-ETIMEDOUT``.

**Causes**:

- Datastore thread stalled (callback blocked or subscription loop taking too long)
- ``DATASTORE_MSG_COUNT`` (10) queue is full under heavy load

**Solutions**:

- Ensure callbacks are non-blocking and free payload promptly
- Reduce request rate or use fire-and-forget writes (``NULL`` response queue)

Implementation Notes
--------------------

- Internal message queue capacity is fixed at 10 (``DATASTORE_MSG_COUNT`` in ``datastoreTypes.h``)
- Each read/write allocates one block from the internal CMSIS pool (10 blocks, sized to
  the largest datapoint array across all types)
- NVM persistence is not yet implemented (``TODO`` in ``datastore.c``)
- Thread priority is set via ``CONFIG_ENYA_DATASTORE_THREAD_PRIORITY``

API Reference
-------------

.. doxygengroup:: datastore
   :project: embedded-services
   :members:
