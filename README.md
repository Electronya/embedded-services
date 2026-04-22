# Embedded Services

![Build](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-build.json)
![Tests](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-tests.json)
![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-coverage.json)

Electronya reusable Zephyr RTOS service library. Consumed as a west module by firmware applications.

## Services

### Service Common

Header-only shared types used by all services for inter-service communication:
- `Data_t` — union covering float, unsigned integer, and signed integer values
- `SrvMsgPayload_t` — memory-pool-backed payload passed between services via message queues
- `ServiceCtrlMsg_t` — lifecycle control messages (stop, suspend)

### Service Manager

Lifecycle supervisor for all registered services. Handles start, stop, suspend, and resume
operations and monitors service health via watchdog-backed heartbeats.

**Key Features:**
- Centralized service lifecycle management (start / stop / suspend / resume)
- Heartbeat monitoring with Zephyr watchdog integration
- Services self-register at init time via the service manager API

For complete documentation, see [Service Manager README](src/serviceManager/README.md).

### ADC Acquisition Service

High-performance, timer-triggered ADC sampling with DMA transfer, digital filtering, and a
publish-subscribe pattern for data distribution.

**Key Features:**
- Timer-triggered acquisition with configurable sampling rate
- DMA-based zero-copy data transfer
- 3rd-order cascaded RC digital filtering with configurable cutoff frequency
- Memory pool-based publish-subscribe mechanism
- VREFINT calibration for accurate voltage measurement
- Shell commands for runtime inspection

For complete documentation, see [ADC Acquisition Service README](src/adcAcquisition/README.md).

### Datastore Service

Centralized, thread-safe data management service supporting multiple datapoint types with a
publish-subscribe pattern and optional NVM persistence.

**Key Features:**
- Six datapoint types: Binary, Button, Float, Integer, Multi-State, and Unsigned Integer
- Publish-subscribe pattern for asynchronous data distribution
- Optional non-volatile memory (NVM) persistence
- Message queue-based thread-safe operations
- X-macro compile-time datapoint configuration
- Shell commands for runtime inspection and modification

For complete documentation, see [Datastore Service README](src/datastore/README.md).

## West Module Usage

Add the module to your application's `west.yml`:

```yaml
manifest:
  projects:
    - name: embedded-services
      url: <repo-url>
      path: apps/embedded-services
      revision: main
```

Enable the desired services in your application's `prj.conf`:

```kconfig
CONFIG_ENYA_EMBEDDED_SERVICES=y
CONFIG_ENYA_SERVICE_MANAGER=y
CONFIG_ENYA_ADC_ACQUISITION=y
CONFIG_ENYA_DATASTORE=y
```

## Testing

Tests run on the Zephyr native simulator — no hardware required.

```bash
# Run all tests with coverage report
./run-tests-coverage.sh

# Run tests for a single service
./run-tests-coverage.sh tests/adcAcquisition
./run-tests-coverage.sh tests/datastore
```

CI runs on Gitea via `.gitea/workflows/test-and-coverage.yml`.
