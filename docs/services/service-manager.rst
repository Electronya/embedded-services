Service Manager
===============

Overview
--------

The Service Manager is the lifecycle supervisor for all Electronya embedded services. It
starts registered services in priority order, monitors their health via a per-service
heartbeat mechanism, and gates hardware watchdog feeding on all services staying alive.
State changes (start, stop, suspend, resume) are requested asynchronously through the
service manager's message queue and executed on its dedicated monitor thread.

Features
--------

- **Priority-ordered startup**: Services start in ``CRITICAL → CORE → APPLICATION`` order
- **Asynchronous lifecycle control**: Start, stop, suspend, and resume via message queue
- **Heartbeat monitoring**: Per-service configurable interval; watchdog starved after 3 consecutive misses
- **Hardware watchdog integration**: Watchdog fed on every monitor loop iteration only when all services are healthy
- **Thread-name-based logging**: Log messages identify services by their Zephyr thread name
- **Shell commands**: Runtime inspection and control via Zephyr shell

Architecture
------------

Thread Model
~~~~~~~~~~~~

The service manager runs a dedicated monitor thread that loops at
``CONFIG_SVC_MGR_LOOP_PERIOD_MS`` intervals. On each iteration it:

1. Drains all pending lifecycle requests from its message queue
2. Checks the heartbeat of every registered running service
3. Feeds the hardware watchdog — **only if** all services passed their heartbeat check

.. mermaid::

   flowchart TD
       A[Loop start] --> B{Message in queue?}
       B -- Yes --> C[Execute START / STOP / SUSPEND / RESUME]
       C --> B
       B -- No / Timeout --> D[Check heartbeats for all services]
       D --> E{All heartbeats OK?}
       E -- Yes --> F[Feed hardware watchdog]
       E -- No --> G[Log error, skip watchdog feed]
       F --> A
       G --> A

Startup Sequence
~~~~~~~~~~~~~~~~

.. mermaid::

   sequenceDiagram
       participant App
       participant SvcMgr as Service Manager
       participant Svc as Service N

       App->>SvcMgr: serviceManagerInit()
       Note over SvcMgr: Init HW watchdog, clear registry, start monitor thread
       App->>Svc: svcNInit()
       Svc->>SvcMgr: serviceManagerRegisterSrv(&descriptor)
       App->>SvcMgr: serviceManagerStartAll()
       SvcMgr->>Svc: start() callback then k_thread_start() [CRITICAL first]
       Svc-->>SvcMgr: serviceManagerUpdateHeartbeat(threadId) [periodic]

Heartbeat Protocol
~~~~~~~~~~~~~~~~~~

Each registered service must periodically call ``serviceManagerUpdateHeartbeat()`` from
its own thread. The monitor checks every loop iteration whether
``heartbeatIntervalMs`` has elapsed since the last update. Each elapsed interval
increments ``missedHeartbeats``. When ``missedHeartbeats`` exceeds 3 consecutive misses,
the monitor stops feeding the hardware watchdog, which eventually triggers a system reset.

Heartbeat checking is skipped for services in ``SVC_STATE_STOPPED`` or
``SVC_STATE_SUSPENDED`` — only running services are monitored.

State Confirmation Protocol
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Services that handle STOP or SUSPEND must call ``serviceManagerConfirmState()`` before
actually stopping or suspending. This updates the registry so the monitor thread has
accurate state:

- **STOP**: call ``serviceManagerConfirmState(threadId, SVC_STATE_STOPPED)`` then exit the
  thread loop
- **SUSPEND**: call ``serviceManagerConfirmState(threadId, SVC_STATE_SUSPENDED)`` then call
  ``k_thread_suspend(k_current_get())``

START and RESUME are handled directly by the service manager via ``k_thread_start()`` /
``k_thread_resume()`` because the service thread cannot read its control queue when it
has not started or is suspended.

Configuration
-------------

Kconfig Options
~~~~~~~~~~~~~~~

Enable the service manager in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_ENYA_SERVICE_MANAGER=y
   CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL=3

``CONFIG_WATCHDOG`` is automatically selected.

Tuning options:

.. code-block:: kconfig

   CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE=2048
   CONFIG_ENYA_SERVICE_MANAGER_THREAD_PRIORITY=1

   # Maximum services that can be registered (also sets the message queue depth)
   CONFIG_SVC_MGR_MAX_SERVICES=16

   # Hardware watchdog timeout — must be >> LOOP_PERIOD_MS × MAX_SERVICES
   CONFIG_SVC_MGR_WDT_TIMEOUT_MS=5000

   # How often the monitor thread runs
   CONFIG_SVC_MGR_LOOP_PERIOD_MS=100

   # Shell commands
   CONFIG_ENYA_SERVICE_MANAGER_SHELL=y

.. list-table::
   :header-rows: 1
   :widths: 40 10 50

   * - Symbol
     - Default
     - Description
   * - ``CONFIG_ENYA_SERVICE_MANAGER_STACK_SIZE``
     - 2048
     - Monitor thread stack size (bytes)
   * - ``CONFIG_ENYA_SERVICE_MANAGER_LOG_LEVEL``
     - 3
     - 0=OFF 1=ERR 2=WRN 3=INF 4=DBG
   * - ``CONFIG_SVC_MGR_MAX_SERVICES``
     - 16
     - Maximum registered services (1–64); also sets queue depth
   * - ``CONFIG_SVC_MGR_WDT_TIMEOUT_MS``
     - 5000
     - Hardware watchdog timeout (ms)
   * - ``CONFIG_SVC_MGR_LOOP_PERIOD_MS``
     - 100
     - Monitor loop period (ms)
   * - ``CONFIG_ENYA_SERVICE_MANAGER_THREAD_PRIORITY``
     - 1
     - Preemptible thread priority
   * - ``CONFIG_ENYA_SERVICE_MANAGER_SHELL``
     - y
     - Enable shell commands

API Usage
---------

Initialization
~~~~~~~~~~~~~~

Call ``serviceManagerInit()`` **before** registering any services. It initializes the
hardware watchdog, clears the service registry, and starts the monitor thread.

.. code-block:: c

   #include "serviceManager.h"

   int err = serviceManagerInit();
   if (err < 0) {
     LOG_ERR("Service manager init failed: %d", err);
     return err;
   }

Registering a Service
~~~~~~~~~~~~~~~~~~~~~

Register each service **after** ``serviceManagerInit()`` and **before**
``serviceManagerStartAll()``. Services typically self-register at the end of their own
``init`` function.

.. code-block:: c

   ServiceDescriptor_t descriptor = {
     .threadId            = myServiceThreadId,
     .priority            = SVC_PRIORITY_CORE,
     .heartbeatIntervalMs = 1000,
     .start               = onStart,
     .stop                = onStop,
     .suspend             = onSuspend,
     .resume              = onResume,
   };

   err = serviceManagerRegisterSrv(&descriptor);
   if (err < 0) {
     LOG_ERR("Register failed: %d", err);
     return err;
   }

Starting All Services
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   err = serviceManagerStartAll();
   if (err < 0) {
     LOG_ERR("Start all failed: %d", err);
     return err;
   }

Iterates ``CRITICAL → CORE → APPLICATION`` and calls each service's ``start`` callback
followed by ``k_thread_start()``.

Runtime Lifecycle Control
~~~~~~~~~~~~~~~~~~~~~~~~~

All request functions are non-blocking — they enqueue a message and return immediately.
The monitor thread processes the request on its next iteration.

.. code-block:: c

   /* Stop a running service */
   serviceManagerRequestStop(myServiceThreadId);

   /* Suspend a running service */
   serviceManagerRequestSuspend(myServiceThreadId);

   /* Resume a suspended service */
   serviceManagerRequestResume(myServiceThreadId);

   /* Start a stopped service */
   serviceManagerRequestStart(myServiceThreadId);

Implementing a Managed Service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A service managed by the service manager must:

1. **Heartbeat** on every loop iteration:

   .. code-block:: c

      serviceManagerUpdateHeartbeat(k_current_get());

2. **Confirm state** before stopping or suspending:

   .. code-block:: c

      /* Handling a STOP request */
      serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
      return;   /* exit thread loop */

      /* Handling a SUSPEND request */
      serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
      k_thread_suspend(k_current_get());
      /* execution resumes here after serviceManagerRequestResume() */

3. **Provide lifecycle callbacks** in its descriptor for any state transition it handles.

The recommended pattern for stop/suspend signalling is a Zephyr message queue (as used
by the ADC Acquisition and Datastore services): the ``stop``/``suspend`` callbacks enqueue
a control message, and the service thread dequeues it on its next loop iteration.

Complete Service Integration Example
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include <zephyr/kernel.h>
   #include "serviceManager.h"
   #include "serviceCommon.h"

   K_THREAD_STACK_DEFINE(myServiceStack, 1024);
   static struct k_thread myServiceThread;

   K_MSGQ_DEFINE(myServiceCtrlQueue, sizeof(ServiceCtrlMsg_t), 4, 4);

   static int onStop(void)
   {
     ServiceCtrlMsg_t msg = SVC_CTRL_STOP;
     return k_msgq_put(&myServiceCtrlQueue, &msg, K_NO_WAIT);
   }

   static int onSuspend(void)
   {
     ServiceCtrlMsg_t msg = SVC_CTRL_SUSPEND;
     return k_msgq_put(&myServiceCtrlQueue, &msg, K_NO_WAIT);
   }

   static int onStart(void)
   {
     k_thread_start(&myServiceThread);
     return 0;
   }

   static int onResume(void)
   {
     k_thread_resume(&myServiceThread);
     return 0;
   }

   static void myServiceRun(void *p1, void *p2, void *p3)
   {
     ServiceCtrlMsg_t ctrlMsg;

     for (;;) {
       if (k_msgq_get(&myServiceCtrlQueue, &ctrlMsg, K_NO_WAIT) == 0) {
         if (ctrlMsg == SVC_CTRL_STOP) {
           serviceManagerConfirmState(k_current_get(), SVC_STATE_STOPPED);
           return;
         }
         if (ctrlMsg == SVC_CTRL_SUSPEND) {
           serviceManagerConfirmState(k_current_get(), SVC_STATE_SUSPENDED);
           k_thread_suspend(k_current_get());
         }
       }

       doWork();
       serviceManagerUpdateHeartbeat(k_current_get());
       k_sleep(K_MSEC(100));
     }
   }

   int myServiceInit(void)
   {
     k_tid_t threadId = k_thread_create(&myServiceThread, myServiceStack, 1024,
                                        myServiceRun, NULL, NULL, NULL,
                                        K_PRIO_PREEMPT(5), 0, K_FOREVER);
     k_thread_name_set(threadId, "myService");

     ServiceDescriptor_t desc = {
       .threadId            = threadId,
       .priority            = SVC_PRIORITY_CORE,
       .heartbeatIntervalMs = 500,
       .start               = onStart,
       .stop                = onStop,
       .suspend             = onSuspend,
       .resume              = onResume,
     };
     return serviceManagerRegisterSrv(&desc);
   }

Shell Commands
--------------

Shell commands are registered under ``srv_mgr``. Enable with
``CONFIG_ENYA_SERVICE_MANAGER_SHELL=y``.

List All Services
~~~~~~~~~~~~~~~~~

.. code-block:: console

   uart:~$ srv_mgr ls
   Index Name                 Priority     State      Heartbeat(ms)   Missed
   0     adcAcquisition       core         running    1000            0
   1     datastore            core         running    1000            0
   2     myService            application  running    500             0

Start / Stop / Suspend / Resume
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All commands take the registry index shown by ``srv_mgr ls``:

.. code-block:: console

   uart:~$ srv_mgr start 2
   SUCCESS: service 2 started

   uart:~$ srv_mgr stop 2
   SUCCESS: service 2 stopped

   uart:~$ srv_mgr suspend 1
   SUCCESS: service 1 suspended

   uart:~$ srv_mgr resume 1
   SUCCESS: service 1 resumed

Troubleshooting
---------------

Hardware Watchdog Resets
~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: System resets unexpectedly.

**Cause**: One or more services stopped heartbeating for more than 3 consecutive
``heartbeatIntervalMs`` periods, starving the watchdog.

**Solutions**:

- Run ``srv_mgr ls`` and check the ``Missed`` column
- Ensure the service calls ``serviceManagerUpdateHeartbeat()`` at least once per
  ``heartbeatIntervalMs``
- Increase ``heartbeatIntervalMs`` in the descriptor if the service loop is legitimately slow
- Increase ``CONFIG_SVC_MGR_WDT_TIMEOUT_MS`` as a temporary debugging measure

Service Stuck in Stopping / Suspending
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: Service state does not reach ``stopped`` or ``suspended`` after a request.

**Cause**: Service thread is not calling ``serviceManagerConfirmState()`` before
stopping/suspending.

**Solutions**:

- Verify the stop/suspend callback correctly signals the service thread
- Verify the thread loop actually reaches ``serviceManagerConfirmState()``
- Check for blocking calls in the service that prevent the thread from running

Registry Full
~~~~~~~~~~~~~

**Symptom**: ``serviceManagerRegisterSrv()`` returns an error.

**Solution**: Increase ``CONFIG_SVC_MGR_MAX_SERVICES``.

Message Queue Full
~~~~~~~~~~~~~~~~~~

**Symptom**: ``serviceManagerRequest*()`` returns an error.

**Cause**: More lifecycle requests were enqueued than the queue depth allows.
Queue depth equals ``CONFIG_SVC_MGR_MAX_SERVICES``.

**Solutions**:

- Increase ``CONFIG_SVC_MGR_MAX_SERVICES``
- Reduce the rate of lifecycle requests

Implementation Notes
--------------------

- The missed-heartbeat threshold is hardcoded at 3 (``SVC_MGR_MAX_MISSED_HEARTBEATS`` in
  ``serviceManagerUtil.c``) — heartbeat miss counting does not block the watchdog until this
  threshold is exceeded
- ``serviceManagerConfirmState()`` resets ``lastHeartbeatMs`` and ``missedHeartbeats`` when
  a service leaves the running state, so a freshly resumed service gets a clean slate
- The monitor thread starts immediately in ``serviceManagerInit()`` (``K_NO_WAIT``); it
  processes lifecycle requests and feeds the watchdog even before any services are registered
- Watchdog is configured with ``WDT_OPT_PAUSE_HALTED_BY_DBG`` so debugging does not
  trigger spurious resets

API Reference
-------------

.. doxygengroup:: serviceManager
   :project: embedded-services
   :members:
