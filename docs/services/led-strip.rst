LED Strip Service
=================

Overview
--------

The LED Strip Service is a transport-layer service responsible for writing pixel data to a
single RGB LED strip at a constant refresh rate via the Zephyr LED strip driver API. It is
driver-agnostic — any strip supported by ``led_strip_update_rgb()`` (WS2812, APA102, etc.)
is supported through DTS configuration.

The service owns no color logic. All color computation and animation are the responsibility
of the producer (e.g. an animation service). The LED strip service only applies global
brightness scaling and pushes the current frame to hardware on every timer tick.

Features
--------

- **Constant-Rate Refresh**: Drives the strip at a fixed rate set by Kconfig, independent of
  producer frame rate
- **Driver-Agnostic**: Uses ``led_strip_update_rgb()`` — works with any Zephyr LED strip driver
- **RGB Support**: Uses Zephyr's standard ``struct led_rgb`` pixel type (``.r``, ``.g``, ``.b``)
- **Double-Buffered**: Two-block CMSIS pool eliminates tearing between producer and renderer
- **Global Brightness**: Applied in-place when a new frame is activated; no extra copy buffer needed
- **Service Manager Integration**: Heartbeat, priority-ordered startup, stop/suspend lifecycle
- **Shell Commands**: Runtime brightness control and frame submission via Zephyr shell

Architecture
------------

Pixel Type
~~~~~~~~~~

The service uses Zephyr's standard ``struct led_rgb`` type from ``<zephyr/drivers/led_strip.h>``:

.. code-block:: c

   struct led_rgb {
       uint8_t r;
       uint8_t g;
       uint8_t b;
   };

Channel order in the wire protocol is defined by the DTS ``color-mapping`` property of the
LED strip device and is handled transparently by the Zephyr LED strip driver.

Double Buffer
~~~~~~~~~~~~~

The service owns a CMSIS memory pool with exactly **2 blocks**, each of size
``chain_length × sizeof(struct led_rgb)`` bytes. The pixel count is read directly from the
DTS ``chain-length`` property, eliminating any risk of mismatch between software and hardware.

.. code-block::

   Pool (2 blocks of chain_length × sizeof(struct led_rgb) bytes)
     Block A → held by LED strip service  (current frame, pushed every tick)
     Block B → held by producer           (being written)

   On submit: service frees A, holds B → producer can allocate A again

The service holds a pointer to the current block and renders directly from it on every timer
tick — no copy is made. When a new frame arrives via the message queue, global brightness is
applied in-place to the new block, the old block is freed, and the new pointer is stored.

Thread Model
~~~~~~~~~~~~

The service runs a dedicated thread that:

1. Waits for the next timer tick (``k_timer_status_sync``)
2. Drains pending messages from its control queue (brightness updates, new frame submissions,
   stop/suspend commands)
3. Calls ``led_strip_update_rgb()`` with the current pool block
4. Updates its heartbeat

Refresh timing is driven by ``k_timer`` at ``CONFIG_ENYA_LED_STRIP_REFRESH_RATE_HZ``.

.. mermaid::

   flowchart TD
       A[Timer tick] --> B{Message in queue?}
       B -- Yes --> C[Process: BRIGHTNESS / NEW_FRAME / STOP / SUSPEND]
       C --> B
       B -- No --> D[led_strip_update_rgb with active block]
       D --> E[serviceManagerUpdateHeartbeat]
       E --> A

Global Brightness
~~~~~~~~~~~~~~~~~

Brightness is a value from ``0`` (off) to ``255`` (full). When a new frame is activated,
each channel is scaled in-place: ``ch = (ch * brightness) / 255``. The pool block is modified
directly — no scratch buffer is used.

.. important::

   Because brightness is applied in-place when the frame is activated, the producer must
   not read back channel values from a submitted frame.

Service Manager Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The LED strip service registers itself with the service manager at init time. The service
manager handles lifecycle (start / stop / suspend / resume) and monitors health via periodic
heartbeats. ``ledStripInit()`` must be called before ``serviceManagerStart()``.

.. mermaid::

   sequenceDiagram
       participant App
       participant SvcMgr as Service Manager
       participant LedStrip as LED Strip Service

       App->>SvcMgr: serviceManagerInit()
       App->>LedStrip: ledStripInit()
       LedStrip->>SvcMgr: serviceManagerRegisterSrv(&descriptor)
       App->>SvcMgr: serviceManagerStartAll()
       SvcMgr->>LedStrip: start() callback then k_thread_start()
       LedStrip-->>SvcMgr: serviceManagerUpdateHeartbeat() [every tick]

Configuration
-------------

DTS
~~~

Bind the service to a LED strip device via alias in your board overlay. Example for a
WS2812 strip on UART (USART3 / PB10, GRB color-mapping):

.. code-block:: dts

   / {
       aliases {
           led-strip = &ws2812;
       };
   };

   &usart3 {
       pinctrl-0 = <&usart3_tx_pb10>;
       pinctrl-names = "default";
       current-speed = <115200>;
       status = "okay";

       ws2812: ws2812 {
           compatible = "worldsemi,ws2812-uart";
           chain-length = <5>;
           color-mapping = <LED_COLOR_ID_GREEN
                            LED_COLOR_ID_RED
                            LED_COLOR_ID_BLUE>;
           reset-delay = <280>;
           baud-rate = <4000000>;
       };
   };

Kconfig Options
~~~~~~~~~~~~~~~

Enable the service in ``prj.conf``:

.. code-block:: kconfig

   CONFIG_ENYA_LED_STRIP=y
   CONFIG_ENYA_LED_STRIP_LOG_LEVEL=3

Optional configuration:

.. code-block:: kconfig

   # Refresh rate in Hz (range 1–120)
   CONFIG_ENYA_LED_STRIP_REFRESH_RATE_HZ=60

   # Message queue depth
   CONFIG_ENYA_LED_STRIP_MSG_COUNT=4

   # Thread settings
   CONFIG_ENYA_LED_STRIP_STACK_SIZE=1024
   CONFIG_ENYA_LED_STRIP_THREAD_PRIORITY=5

   # Service Manager integration
   CONFIG_ENYA_LED_STRIP_SERVICE_PRIORITY=2
   CONFIG_ENYA_LED_STRIP_HEARTBEAT_INTERVAL_MS=1000

   # Shell commands
   CONFIG_ENYA_LED_STRIP_SHELL=y

.. list-table::
   :header-rows: 1
   :widths: 40 10 50

   * - Symbol
     - Default
     - Description
   * - ``CONFIG_ENYA_LED_STRIP_REFRESH_RATE_HZ``
     - 60
     - Refresh rate in Hz, range 1–120
   * - ``CONFIG_ENYA_LED_STRIP_MSG_COUNT``
     - 4
     - Control message queue depth
   * - ``CONFIG_ENYA_LED_STRIP_STACK_SIZE``
     - 1024
     - Thread stack size (bytes)
   * - ``CONFIG_ENYA_LED_STRIP_THREAD_PRIORITY``
     - 5
     - Preemptible thread priority
   * - ``CONFIG_ENYA_LED_STRIP_SERVICE_PRIORITY``
     - 2
     - Service Manager priority (0=CRITICAL, 1=CORE, 2=APPLICATION)
   * - ``CONFIG_ENYA_LED_STRIP_HEARTBEAT_INTERVAL_MS``
     - 1000
     - Heartbeat interval (ms)
   * - ``CONFIG_ENYA_LED_STRIP_LOG_LEVEL``
     - 3
     - 0=OFF 1=ERR 2=WRN 3=INF 4=DBG
   * - ``CONFIG_ENYA_LED_STRIP_SHELL``
     - y
     - Enable shell commands

API Usage
---------

Initialization
~~~~~~~~~~~~~~

.. code-block:: c

   #include "ledStrip.h"

   int err = ledStripInit();
   if (err < 0) {
     LOG_ERR("LED strip init failed: %d", err);
     return err;
   }

Submitting a Frame
~~~~~~~~~~~~~~~~~~

Allocate a block from the service pool, fill it with RGB data, then submit it.
After ``ledStripUpdateFrame()`` returns, the frame is owned by the service — do not
modify it.

.. code-block:: c

   struct led_rgb *frame = ledStripGetNextFramebuffer();
   if (frame == NULL) {
     LOG_WRN("no frame buffer available");
     return -ENOMEM;
   }

   for (size_t i = 0; i < DT_PROP(DT_ALIAS(led_strip), chain_length); i++) {
     frame[i].r = red[i];
     frame[i].g = green[i];
     frame[i].b = blue[i];
   }

   int err = ledStripUpdateFrame(frame);
   if (err < 0) {
     LOG_ERR("frame submit failed: %d", err);
   }

``ledStripUpdateFrame()`` enqueues a message to the service thread and returns immediately.
Brightness is applied in-place when the service thread activates the frame.

Setting Brightness
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   ledStripSetBrightness(128);   /* ~50% brightness */

Non-blocking — enqueues a control message. Brightness takes effect when the service thread
processes the next frame.

Shell Commands
--------------

Shell commands are registered under ``led``. Enable with ``CONFIG_ENYA_LED_STRIP_SHELL=y``.

.. list-table::
   :header-rows: 1
   :widths: 35 35 30

   * - Command
     - Arguments
     - Description
   * - ``led pc``
     - —
     - Print pixel count (from DTS ``chain-length``)
   * - ``led br <val>``
     - ``val``: 0–255
     - Set global brightness
   * - ``led sf <r0> <g0> <b0> ...``
     - ``chain_length × 3`` values
     - Submit a full frame

.. code-block:: console

   # Get pixel count
   uart:~$ led pc
   SUCCESS: Pixel count: 5

   # Set brightness to 50%
   uart:~$ led br 128
   SUCCESS: brightness set to 128

   # Submit a frame (5-pixel strip example)
   uart:~$ led sf 255 0 0  0 255 0  0 0 255  255 255 0  0 255 255
   SUCCESS: frame submitted

Implementing a Producer
-----------------------

A producer that drives the LED strip must:

1. **Allocate a framebuffer** from the service pool:

   .. code-block:: c

      struct led_rgb *frame = ledStripGetNextFramebuffer();
      if (frame == NULL) {
        /* pool exhausted — both blocks in use, retry next tick */
        return -ENOMEM;
      }

2. **Fill the frame** with RGB values:

   .. code-block:: c

      for (size_t i = 0; i < PIXEL_COUNT; i++) {
        frame[i].r = ...;
        frame[i].g = ...;
        frame[i].b = ...;
      }

3. **Submit the frame**:

   .. code-block:: c

      ledStripUpdateFrame(frame);

4. **Do not free or modify the frame** after submitting — ownership transfers to the service.

5. **Handle allocation failure** by backing off: if the pool has no free block (both held by
   service and a previous unprocessed submission), retry on the next animation tick.

Troubleshooting
---------------

Strip Not Updating
~~~~~~~~~~~~~~~~~~~

**Symptom**: Strip shows no output or stays on the last frame.

**Solutions**:

- Verify the DTS alias ``led-strip`` is correctly bound to the strip device
- Confirm the DTS ``chain-length`` matches the physical strip length
- Use ``led sf`` from the shell to verify the service is running

Wrong Colors
~~~~~~~~~~~~~

**Symptom**: Colors appear correct but in the wrong channel order (e.g. red appears as green).

**Cause**: WS2812 strips typically use GRB wire order. The Zephyr ``ws2812-uart`` driver maps
``struct led_rgb`` fields to wire bytes using the DTS ``color-mapping`` property automatically.

**Solution**: Verify your overlay has the correct ``color-mapping`` for your strip hardware.

Frame Allocation Failure
~~~~~~~~~~~~~~~~~~~~~~~~

**Symptom**: ``ledStripGetNextFramebuffer()`` returns ``NULL``.

**Cause**: Both pool blocks are in use — one held by the service as the current frame, one
pending in the message queue (previous submission not yet consumed).

**Solution**: Reduce producer frame rate or increase ``CONFIG_ENYA_LED_STRIP_MSG_COUNT``.

API Reference
-------------

.. doxygengroup:: ledStrip
   :project: embedded-services
   :members:
