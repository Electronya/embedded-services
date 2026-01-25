# Embedded-Services

![Build](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-build.json)
![Tests](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-tests.json)
![Coverage](https://img.shields.io/endpoint?url=https://gist.githubusercontent.com/JudeBake/6aa3080953d92cad7d9ec93b8606c348/raw/embedded-services-coverage.json)

Electronya embedded services for Zephyr RTOS based firmware

## ADC Acquisition Service

### Overview

The ADC Acquisition Service provides high-performance, timer-triggered ADC sampling with DMA transfer, digital filtering, and a publish-subscribe mechanism for distributing ADC data to multiple consumers. The service is fully configured via devicetree and operates in a dedicated thread.

### Features

- **Timer-Triggered Acquisition**: Configurable sampling rate using hardware timer triggers
- **DMA-Based Transfer**: Efficient zero-copy data acquisition using Direct Memory Access
- **3rd-Order Cascaded RC Filtering**: Digital low-pass filtering with configurable cutoff frequency
- **Publish-Subscribe Pattern**: Memory pool-based asynchronous data distribution to multiple subscribers
- **VREFINT Calibration**: Automatic VDD voltage measurement using internal voltage reference
- **Thread-Safe**: Dedicated service thread with synchronization primitives
- **Devicetree-Driven**: ADC channels and trigger timer configured via devicetree
- **Shell Commands**: Runtime debugging and inspection via Zephyr shell

### Architecture

#### Thread Model
The service runs in a dedicated thread that:
1. Waits for timer-triggered ADC conversions (via semaphore)
2. Processes raw ADC data and applies digital filtering
3. Calculates voltage values using VREFINT calibration
4. Notifies subscribers at configured intervals

#### Data Flow
```
Timer Trigger → ADC → DMA → Raw Buffer → Digital Filter → Voltage Conversion → Subscribers
```

#### Memory Pool Pattern
The service uses CMSIS-RTOS v2 memory pools to pass ADC data to subscribers:
- Pool is created with `2 × maxSubCount` blocks
- Each block contains: `sizeof(AdcSubData_t) + (channelCount × sizeof(float))`
- Data includes embedded pool ID for proper cleanup
- Subscribers **must** free memory after processing using `osMemoryPoolFree(data->pool, data)`

### Configuration

#### Kconfig Options

Enable the service in `prj.conf`:
```kconfig
CONFIG_ENYA_ADC_ACQUISITION=y
CONFIG_ENYA_ADC_ACQUISITION_LOG_LEVEL=3
CONFIG_ENYA_ADC_ACQUISITION_STACK_SIZE=1024
```

Required dependencies:
```kconfig
# CMSIS-RTOS v2 for memory pools
CONFIG_CMSIS_RTOS_V2=y
CONFIG_CMSIS_V2_MEM_SLAB_MAX_DYNAMIC_SIZE=1024

# Heap for dynamic allocation
CONFIG_HEAP_MEM_POOL_SIZE=8192
```

#### Devicetree Configuration

Define ADC channels in `zephyr,user` node and create an alias for the trigger timer:

```dts
/ {
  aliases {
    adc-trigger = &adc_trigger_counter;
  };

  zephyr,user {
    io-channels = <&adc1 0>, <&adc1 1>, <&adc1 4>, <&adc1 13>;
    vref-channel-index = <3>;  /* Index of VREFINT channel */
  };
};

&timers6 {
  status = "okay";
  st,prescaler = <31999>;  /* 64MHz / 32000 = 2kHz timer clock */

  adc_trigger_counter: counter {
    status = "okay";
  };
};

&adc1 {
  status = "okay";
  pinctrl-0 = <&adc1_in0_pa0>, <&adc1_in1_pa1>, <&adc1_in4_pa4>;
  pinctrl-names = "default";

  dmas = <&dmamux1 1 5 (STM32_DMA_PERIPH_TO_MEMORY | STM32_DMA_MEM_INC |
                         STM32_DMA_PERIPH_16BITS | STM32_DMA_MEM_16BITS)>;
  dma-names = "adc";

  channel0: channel@0 {
    reg = <0>;
    zephyr,gain = "ADC_GAIN_1";
    zephyr,reference = "ADC_REF_INTERNAL";
    zephyr,acquisition-time = <ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 40)>;
    zephyr,oversampling = <4>;
    zephyr,resolution = <12>;
  };
  /* Additional channels... */
};
```

### Digital Filtering

The service implements a 3rd-order cascaded RC low-pass filter using integer mathematics for efficiency.

#### Filter Equation
Each stage uses: `y[n] = y[n-1] + α × (x[n] - y[n-1])`
where `α = tau / 512` (FILTER_PRESCALE = 9)

#### Calculating Filter Tau

To calculate the tau value for a desired 3rd-order cutoff frequency (`fc_3rd`):

1. Calculate the required 1st-order cutoff:
   ```
   fc_1st = fc_3rd / 0.5098
   ```

2. Calculate alpha:
   ```
   α = 1 - exp(-2π × fc_1st / fs)
   ```
   where `fs` is the sampling frequency (1/samplingRate)

3. Calculate tau:
   ```
   tau = α × 512
   ```

4. Round to nearest integer (valid range: 1 to 511)

#### Example Calculation
For `fs = 2000 Hz` (samplingRate = 500 μs) and desired `fc_3rd = 10 Hz`:
```
fc_1st = 10 / 0.5098 ≈ 19.6 Hz
α = 1 - exp(-2π × 19.6 / 2000) ≈ 0.0614
tau = 0.0614 × 512 ≈ 31
```

**Note**: Each RC stage has cutoff `fc_1st`, but cascading three stages results in:
```
fc_3rd = fc_1st × 0.5098
```

### API Usage

#### Initialization

```c
#include "adcAcquisition.h"

AdcConfig_t adcConfig = {
  .samplingRate = 500,    /* 2 kHz sampling (500 μs period) */
  .filterTau = 31         /* 10 Hz 3rd-order cutoff */
};

AdcSubConfig_t subConfig = {
  .maxSubCount = 4,
  .notificationRate = 10  /* Notify subscribers every 10 ms */
};

k_tid_t threadId;
int err = adcAcqInit(&adcConfig, &subConfig, 5, &threadId);
if (err < 0) {
  LOG_ERR("ADC init failed: %d", err);
  return err;
}

err = adcAcqStart();
if (err < 0) {
  LOG_ERR("ADC start failed: %d", err);
  return err;
}
```

#### Subscribing to ADC Data

```c
int adcCallback(AdcSubData_t *data)
{
  int err = 0;

  /* Validate data */
  if (data->valCount != 4) {
    osMemoryPoolFree(data->pool, data);
    return -EINVAL;
  }

  /* Process ADC values */
  float adc0 = data->values[0];
  float adc1 = data->values[1];
  float adc4 = data->values[2];
  float vrefint = data->values[3];

  /* Do work with values... */
  err = processData(adc0, adc1, adc4);

  /* CRITICAL: Always free memory back to pool */
  osMemoryPoolFree(data->pool, data);

  return err;
}

/* Subscribe to ADC updates */
err = adcAcqSubscribe(adcCallback);
if (err < 0) {
  LOG_ERR("Subscribe failed: %d", err);
}
```

#### Managing Subscriptions

```c
/* Pause a subscription (stop receiving callbacks) */
err = adcAcqPauseSubscription(adcCallback);

/* Resume a paused subscription */
err = adcAqcUnpauseSubscription(adcCallback);

/* Unsubscribe completely */
err = adcAcqUnsubscribe(adcCallback);
```

### Shell Commands

The service provides shell commands for runtime debugging:

```bash
# Get the number of configured ADC channels
uart:~$ adc_acq get_chan_count
SUCCESS: channel count: 4

# Get raw ADC value for channel 0
uart:~$ adc_acq get_raw 0
SUCCESS: channel 0 raw value: 8192

# Get voltage value for channel 0
uart:~$ adc_acq get_volt 0
SUCCESS: channel 0 volt value: 1.650000
```

### Memory Pool Sizing

The service automatically calculates memory pool size based on configuration:

- **Block Size**: `sizeof(AdcSubData_t) + (channelCount × sizeof(float))`
  - For 4 channels: `8 + (4 × 4) = 24 bytes` per block

- **Block Count**: `2 × maxSubCount`
  - For 4 max subscribers: `2 × 4 = 8 blocks`

- **Total Pool Memory**: `blockSize × blockCount`
  - For example: `24 × 8 = 192 bytes`

Ensure `CONFIG_CMSIS_V2_MEM_SLAB_MAX_DYNAMIC_SIZE` is larger than the total pool memory.

### Best Practices

1. **Always Free Memory**: Subscribers must call `osMemoryPoolFree(data->pool, data)` after processing
2. **Keep Callbacks Fast**: Callbacks run in the service thread context; avoid blocking operations
3. **Use Message Queues**: For heavy processing, copy data to a message queue and free immediately:
   ```c
   int callback(AdcSubData_t *data) {
     int err = k_msgq_put(&myQueue, data->values, K_NO_WAIT);
     osMemoryPoolFree(data->pool, data);
     return err;
   }
   ```
4. **Configure Adequate Heap**: Ensure heap size accommodates memory pool requirements
5. **Match Sampling and Notification Rates**: Set notification rate as a multiple of sampling period

### Troubleshooting

#### Pool Allocation Failures
**Symptom**: `ERROR: pool allocation failed for subscription X`

**Causes**:
- All pool blocks in use (subscribers not freeing memory)
- Callbacks taking too long (blocking pool returns)

**Solutions**:
- Verify all callbacks free memory with `osMemoryPoolFree()`
- Increase `maxSubCount` (increases pool size)
- Reduce `notificationRate` (slower callback frequency)

#### Memory Pool Creation Failure
**Symptom**: `ERROR -12: unable to create subscription data pool`

**Causes**:
- `CONFIG_HEAP_MEM_POOL_SIZE` too small
- `CONFIG_CMSIS_V2_MEM_SLAB_MAX_DYNAMIC_SIZE` too small

**Solutions**:
- Increase heap: `CONFIG_HEAP_MEM_POOL_SIZE=8192`
- Increase CMSIS limit: `CONFIG_CMSIS_V2_MEM_SLAB_MAX_DYNAMIC_SIZE=1024`

### Implementation Notes

- Service uses VREFINT (internal voltage reference) for VDD measurement and voltage calibration
- Timer trigger prevents ADC overruns with a busy flag
- Digital filter state is maintained across conversions for each channel
- Subscription array uses dynamic allocation with compile-time limits
- Thread priority should be set based on system requirements (typically 5-10)
