# SimHub Device Service

## Overview

The SimHub Device service implements the SimHub Standard Arduino/Dash serial protocol over
USB CDC ACM, allowing SimHub (PC sim-racing dashboard software) to drive the device's LED
strip at runtime. It is a **producer** for the LED Strip service — it receives RGB frame data
from SimHub and submits it via `ledStripGetNextFramebuffer()` / `ledStripUpdateFrame()`.

The service owns no rendering logic. Frame data arrives verbatim from SimHub and is forwarded
directly to the LED strip service. Global brightness, refresh rate, and hardware push are all
handled by the LED strip service.

USB device stack initialization is **not** owned by this service. It is handled by the
`usbDevice` utility in `serviceCommon`, which is shared with other USB services (e.g. future
HID/controller service). This service registers its CDC ACM class with `usbDeviceAddClass()`
during its own init; the application enables USB after all services are registered.

## Protocol

The SimHub Standard Arduino/Dash protocol uses a fixed 6-byte sync preamble followed by an
ASCII command string. All responses are terminated with `\r\n`.

### Sync Preamble

Every command is prefixed with exactly **6 × `0xFF`** bytes:

```
(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)(0xFF)<command>
```

### Commands

#### Protocol Version — `proto`

SimHub queries the protocol version to verify compatibility.

```
Host → Device:  FF FF FF FF FF FF proto
Device → Host:  SIMHUB_1.0\r\n
```

#### LED Count — `ledsc`

SimHub queries the number of LEDs to allocate its frame buffer.

```
Host → Device:  FF FF FF FF FF FF ledsc
Device → Host:  <N>\r\n
```

`N` is read from DTS `DT_PROP(DT_ALIAS(led_strip), chain_length)` at compile time.

#### Set LEDs — `sleds`

SimHub pushes a full RGB frame. Pixel data follows the command string immediately, with a
3-byte integrity terminator at the end.

```
Host → Device:  FF FF FF FF FF FF sleds (R0)(G0)(B0)(R1)(G1)(B1)...(RN)(GN)(BN)(FF)(FE)(FD)
Device → Host:  (no response)
```

- `N × 3` raw bytes of pixel data in RGB order
- Terminator `(0xFF)(0xFE)(0xFD)` for integrity verification
- Because `N` is known at compile time, the parser reads exactly `N × 3` bytes then verifies
  the terminator — no scanning required

#### Unlock — `unlock`

Not supported. This command is intended for Arduino firmware upload via SimHub, which is not
applicable to this device. The command is **silently discarded** — the parser logs a debug
message and returns to the WAIT_SYNC state. No response is sent.

## Architecture

### Protocol Parser State Machine

The parser is implemented in `simhubDeviceProto.c` and is fully decoupled from the CDC UART
and threading. It consumes one byte at a time via `simhubDeviceProtoFeed()`.

```
WAIT_SYNC ──(6 × 0xFF)──► CMD_READ ──(5 chars)──► CMD_DISPATCH
                                                       │
                          ┌────────────────────────────┤
                          │                            │
                     proto/ledsc/unlock             sleds
                          │                            │
                    send response            ► LED_DATA
                    → WAIT_SYNC              read N×3 bytes into framebuffer
                                             verify (FF)(FE)(FD) terminator
                                             ledStripUpdateFrame()
                                             → WAIT_SYNC
```

- **WAIT_SYNC**: counts consecutive `0xFF` bytes; any non-`0xFF` resets the counter
- **CMD_READ**: buffers bytes until 5 characters are collected; dispatches on recognized
  command; resets to WAIT_SYNC on unknown command
- **LED_DATA**: reads exactly `pixelCount × 3` bytes directly into a framebuffer obtained
  from `ledStripGetNextFramebuffer()`; verifies the 3-byte terminator; submits via
  `ledStripUpdateFrame()` on success or discards the frame on terminator mismatch

### Thread Model

A single dedicated thread reads from the CDC ACM UART using the **async UART API**. An RX
callback posts to a semaphore; the thread wakes, drains the RX ring buffer byte by byte
through the protocol parser, then updates its heartbeat.

```
simhubDeviceThread:
  loop:
    k_sem_take(&rxSem)          ← woken by UART async RX callback
    drain RX ring buffer        ← feed bytes into protocol state machine
    serviceManagerUpdateHeartbeat()
```

This keeps the thread sleeping between UART bursts with no CPU burn.

### USB Device Utility (`serviceCommon`)

USB stack initialization is shared across all USB-capable services via the `usbDevice`
utility in `serviceCommon`. The intended call order in the application's `systemInit` is:

```c
usbDeviceInit();          /* init USBD context + descriptors */
simhubDeviceInit();       /* registers CDC ACM class via usbDeviceAddClass() */
/* hidControllerInit();  */  /* future: registers HID class */
usbDeviceEnable();        /* usbd_enable() — USB enumeration starts */
serviceManagerStartAll();
```

`simhubDeviceInit()` calls `usbDeviceAddClass()` internally. The application never touches
USB class registration directly.

### DTS

The CDC ACM UART is bound via a DTS alias `simhub-uart` pointing to a
`zephyr,cdc-acm-uart` child node under the board's `zephyr_udc0`:

```dts
&zephyr_udc0 {
    cdc_acm_uart0: cdc_acm_uart0 {
        compatible = "zephyr,cdc-acm-uart";
    };
};

/ {
    aliases {
        simhub-uart = &cdc_acm_uart0;
    };
};
```

## Configuration

### Kconfig Options

Enable the service in `prj.conf`:

```kconfig
CONFIG_ENYA_USB_DEVICE=y
CONFIG_ENYA_SIMHUB_DEVICE=y
```

Required USB descriptor configuration (in `serviceCommon`):

```kconfig
CONFIG_ENYA_USB_DEVICE_VID=0x2FE3
CONFIG_ENYA_USB_DEVICE_PID=0x0001
CONFIG_ENYA_USB_DEVICE_MANUFACTURER="Electronya"
CONFIG_ENYA_USB_DEVICE_PRODUCT="Electronya SimHub Device"
```

Tuning options:

```kconfig
CONFIG_ENYA_SIMHUB_DEVICE_LOG_LEVEL=3
CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE=1024
CONFIG_ENYA_SIMHUB_DEVICE_THREAD_PRIORITY=6
CONFIG_ENYA_SIMHUB_DEVICE_SERVICE_PRIORITY=2
CONFIG_ENYA_SIMHUB_DEVICE_HEARTBEAT_INTERVAL_MS=1000
CONFIG_ENYA_SIMHUB_DEVICE_SHELL=y
```

| Symbol | Default | Description |
|--------|---------|-------------|
| `CONFIG_ENYA_SIMHUB_DEVICE_LOG_LEVEL` | 3 | 0=OFF 1=ERR 2=WRN 3=INF 4=DBG |
| `CONFIG_ENYA_SIMHUB_DEVICE_STACK_SIZE` | 1024 | Thread stack size (bytes) |
| `CONFIG_ENYA_SIMHUB_DEVICE_THREAD_PRIORITY` | 6 | Preemptible thread priority |
| `CONFIG_ENYA_SIMHUB_DEVICE_SERVICE_PRIORITY` | 2 | Service Manager priority (0=CRITICAL, 1=CORE, 2=APPLICATION) |
| `CONFIG_ENYA_SIMHUB_DEVICE_HEARTBEAT_INTERVAL_MS` | 1000 | Heartbeat interval (ms) |
| `CONFIG_ENYA_SIMHUB_DEVICE_SHELL` | y | Enable shell commands |

### Dependencies

```
CONFIG_ENYA_USB_DEVICE   (serviceCommon — USB stack owner)
    └── CONFIG_ENYA_SIMHUB_DEVICE
            └── CONFIG_ENYA_LED_STRIP
```

## API Usage

### Initialization

```c
#include "simhubDevice.h"

/* Called after usbDeviceInit(), before usbDeviceEnable() */
int err = simhubDeviceInit();
if (err < 0) {
  LOG_ERR("SimHub device init failed: %d", err);
  return err;
}
```

## Shell Commands

Shell commands are registered under `simhub`. Enable with
`CONFIG_ENYA_SIMHUB_DEVICE_SHELL=y`.

| Command | Description |
|---------|-------------|
| `simhub status` | Connection state, total frames received, current frame rate |
| `simhub reset` | Force parser back to WAIT_SYNC (recover from desync) |
| `simhub info` | Protocol version string and pixel count from DTS |

```console
uart:~$ simhub info
Protocol: SIMHUB_1.0
Pixels:   5

uart:~$ simhub status
Connected:  yes
Frames:     1234
Rate:       60 fps

uart:~$ simhub reset
Parser reset to WAIT_SYNC
```

## Testing

Tests live in `tests/simhubDevice/` and follow the same include-the-source pattern used
elsewhere in this project.

### `proto/` — Protocol Parser Unit Tests

Tests `simhubDeviceProto.c` in isolation. No threading, no UART, no LED strip.

- Sync detection: partial sync, noise before sync, exact 6 × `0xFF`
- `proto` → response bytes equal `"SIMHUB_1.0\r\n"`
- `ledsc` → response bytes equal `"<N>\r\n"`
- `sleds` valid frame → `struct led_rgb` values match input bytes, `ledStripUpdateFrame()` called
- `sleds` bad terminator → frame discarded, `ledStripUpdateFrame()` not called, parser resets
- `unlock` → silently discarded, parser returns to WAIT_SYNC, no response sent

### `service/` — Service Integration Tests

Tests `simhubDevice.c` with mocked dependencies (CDC UART, LED strip API, service manager).

- `simhubDeviceInit()`: CDC ACM device not ready → error returned
- `simhubDeviceInit()`: USB class registration fails → error returned
- `simhubDeviceInit()`: success → service registered with service manager
- Thread startup: state confirmed as running with service manager
- Stop message: thread aborts, stopped state confirmed to service manager
- Suspend/resume: timer stops on suspend, resumes correctly
- Frame path: valid `sleds` frame → `ledStripGetNextFramebuffer()` and
  `ledStripUpdateFrame()` called with correct pixel data

## Development Plan

### Step 1 — `serviceCommon` USB device utility

`usbDevice.h/c` + Kconfig + CMakeLists update. Foundation for everything USB — the USBD
context must exist before any service can register a class.

### Step 2 — DTS overlay + `prj.conf`

Add the `cdc_acm_uart0` node under `zephyr_udc0` and the `simhub-uart` alias to the board
overlay. Enable `CONFIG_ENYA_USB_DEVICE`, `CONFIG_USB_DEVICE_STACK_NEXT`, and
`CONFIG_USBD_CDC_ACM_CLASS` in `prj.conf`. Verify the build compiles clean with a stub
`simhubDeviceInit()` before writing any protocol code.

### Step 3 — Protocol parser (`simhubDeviceProto.h/c`)

Implement the state machine in complete isolation — no UART, no threading, no LED strip.
A single `simhubDeviceProtoFeed(uint8_t byte)` entry point and parser callbacks. Fully
testable without hardware.

### Step 4 — Protocol parser tests (`tests/simhubDevice/proto/`)

Write and pass all `proto/` test cases before touching the service. Parser bugs are far
cheaper to catch here than during integration.

### Step 5 — Service skeleton (`simhubDevice.h/c`)

Thread creation, CDC UART async RX setup, service manager registration, stop/suspend/resume
lifecycle. Parser fed from the thread's RX drain loop; LED strip API called from parser
callbacks. No shell yet.

### Step 6 — Service build wiring

Kconfig, CMakeLists, `systemInit` call order (`usbDeviceInit` → `simhubDeviceInit` →
`usbDeviceEnable` → `serviceManagerStartAll`), `main.c` update.

### Step 7 — Service tests (`tests/simhubDevice/service/`)

Init paths, thread lifecycle, stop/suspend/resume, and frame submission path — all with
mocked dependencies.

### Step 8 — Shell commands (`simhubDeviceCmd.c`)

`simhub status`, `simhub reset`, `simhub info`. Implemented once the service state is
accessible.

### Step 9 — Manual integration test

Flash, connect USB, open SimHub, configure the device, verify the strip updates. Adjust
buffer sizes or timing if needed.

## Troubleshooting

### SimHub Not Detecting Device

**Symptom**: SimHub does not enumerate or recognize the device.

**Solutions**:
- Verify `CONFIG_ENYA_USB_DEVICE=y` and `CONFIG_ENYA_SIMHUB_DEVICE=y` are set
- Confirm `usbDeviceEnable()` is called after `simhubDeviceInit()` in system init
- Check the DTS alias `simhub-uart` is bound to the CDC ACM UART node
- Verify VID/PID in `CONFIG_ENYA_USB_DEVICE_VID` / `CONFIG_ENYA_USB_DEVICE_PID`

### Strip Not Updating After Connect

**Symptom**: SimHub connects and sends data but the LED strip does not update.

**Solutions**:
- Run `simhub status` — confirm frames are being received
- Run `simhub reset` to force parser resync
- Verify `CONFIG_ENYA_LED_STRIP=y` and the LED strip service is started
- Check `CONFIG_ENYA_SIMHUB_DEVICE_LOG_LEVEL=4` for parser debug output

### Parser Desynced

**Symptom**: `simhub status` shows frames received but strip shows wrong colors or stops.

**Cause**: Byte stream corruption (e.g. USB glitch) left the parser in an unexpected state.

**Solution**: Run `simhub reset` or power-cycle — the parser returns to WAIT_SYNC and
resynchronizes on the next 6 × `0xFF` preamble from SimHub.
