Embedded Services
=================

Electronya reusable Zephyr RTOS service library. Consumed as a west module by firmware applications.

Services
--------

Service Common
~~~~~~~~~~~~~~

Header-only shared types used by all services for inter-service communication:

- ``Data_t`` — union covering float, unsigned integer, and signed integer values
- ``SrvMsgPayload_t`` — memory-pool-backed payload passed between services via message queues
- ``ServiceCtrlMsg_t`` — lifecycle control messages (stop, suspend)

Service Manager
~~~~~~~~~~~~~~~

Lifecycle supervisor for all registered services. Handles start, stop, suspend, and resume
operations and monitors service health via watchdog-backed heartbeats.

**Key Features:**

- Centralized service lifecycle management (start / stop / suspend / resume)
- Heartbeat monitoring with Zephyr watchdog integration
- Services self-register at init time via the service manager API

See :doc:`services/service-manager` for the full API reference.

ADC Acquisition Service
~~~~~~~~~~~~~~~~~~~~~~~

High-performance, timer-triggered ADC sampling with DMA transfer, digital filtering, and a
publish-subscribe pattern for data distribution.

**Key Features:**

- Timer-triggered acquisition with configurable sampling rate
- DMA-based zero-copy data transfer
- 3rd-order cascaded RC digital filtering with configurable cutoff frequency
- Memory pool-based publish-subscribe mechanism
- VREFINT calibration for accurate voltage measurement
- Shell commands for runtime inspection

See :doc:`services/adc-acquisition` for the full API reference.

Datastore Service
~~~~~~~~~~~~~~~~~

Centralized, thread-safe data management service supporting multiple datapoint types with a
publish-subscribe pattern and optional NVM persistence.

**Key Features:**

- Six datapoint types: Binary, Button, Float, Integer, Multi-State, and Unsigned Integer
- Publish-subscribe pattern for asynchronous data distribution
- Optional non-volatile memory (NVM) persistence
- Message queue-based thread-safe operations
- X-macro compile-time datapoint configuration
- Shell commands for runtime inspection and modification

See :doc:`services/datastore` for the full API reference.

LED Strip Service
~~~~~~~~~~~~~~~~~

Transport-layer service that drives a single RGB LED strip at a constant refresh rate via
the Zephyr LED strip driver API. Producer-agnostic — handles double-buffering, global
brightness scaling, and hardware push; color logic lives in the producer.

**Key Features:**

- Constant-rate refresh independent of producer frame rate
- Standard Zephyr ``struct led_rgb`` pixel type
- Two-block CMSIS memory pool for tear-free double buffering
- Global brightness scaling applied in-place at frame activation
- Shell commands for brightness control and direct frame submission

See :doc:`services/led-strip` for the full API reference.

West Module Usage
-----------------

Add the module to your application's ``west.yml``:

.. code-block:: yaml

   manifest:
     projects:
       - name: embedded-services
         url: <repo-url>
         path: apps/embedded-services
         revision: main

Enable the desired services in your application's ``prj.conf``:

.. code-block:: kconfig

   CONFIG_ENYA_EMBEDDED_SERVICES=y
   CONFIG_ENYA_SERVICE_MANAGER=y
   CONFIG_ENYA_ADC_ACQUISITION=y
   CONFIG_ENYA_DATASTORE=y

Testing
-------

Tests run on the Zephyr native simulator — no hardware required.

.. code-block:: bash

   # Run all tests with coverage report
   ./run-tests-coverage.sh

   # Run tests for a single service
   ./run-tests-coverage.sh tests/adcAcquisition
   ./run-tests-coverage.sh tests/datastore

.. toctree::
   :maxdepth: 2
   :hidden:

   services/adc-acquisition
   services/datastore
   services/led-strip
   services/service-manager
