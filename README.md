# Embedded Services

![Build](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-build.json)
![Tests](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-tests.json)
![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-coverage.json)

Electronya embedded services for Zephyr RTOS based firmware

## Services

### ADC Acquisition Service

High-performance, timer-triggered ADC sampling with DMA transfer, digital filtering, and publish-subscribe pattern for data distribution.

**Key Features:**
- Timer-triggered acquisition with configurable sampling rate
- DMA-based zero-copy data transfer
- 3rd-order cascaded RC digital filtering
- Memory pool-based publish-subscribe mechanism
- VREFINT calibration for accurate voltage measurement

For complete documentation, see [ADC Acquisition Service README](src/adcAcquisition/README.md).

### Datastore Service

Centralized, thread-safe data management service supporting multiple datapoint types with publish-subscribe pattern and optional NVM persistence.

**Key Features:**
- Six datapoint types: Binary, Button, Float, Integer, Multi-State, and Unsigned Integer
- Publish-subscribe pattern for asynchronous data distribution
- Optional non-volatile memory (NVM) persistence
- Message queue-based thread-safe operations
- X-macro compile-time datapoint configuration
- Runtime inspection and modification via shell commands

For complete documentation, see [Datastore Service README](src/datastore/README.md).
