/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-04-24
 * @brief     LED Strip Util Tests
 *
 *            Unit tests for LED strip utility functions.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>

DEFINE_FFF_GLOBALS;

/* Prevent LED strip driver header - we'll define types manually */
#define ZEPHYR_INCLUDE_DRIVERS_LED_STRIP_H_

/* Prevent CMSIS OS2 header - we'll define types manually */
#define CMSIS_OS2_H_

/* Wrap functions to use mocks */
#define device_is_ready device_is_ready_mock
#define led_strip_update_channels led_strip_update_channels_mock

/* Mock CMSIS OS2 types */
typedef void *osMemoryPoolId_t;

/* Mock Kconfig options */
#define CONFIG_ENYA_LED_STRIP 1
#define CONFIG_ENYA_LED_STRIP_LOG_LEVEL 3
#define CONFIG_ENYA_LED_STRIP_NUM_CHANNELS 3
#define CONFIG_ENYA_LED_STRIP_PIXEL_COUNT 5

/* FFF fakes list */
#define FFF_FAKES_LIST(FAKE) \
  FAKE(device_is_ready_mock) \
  FAKE(led_strip_update_channels_mock) \
  FAKE(osMemoryPoolNew) \
  FAKE(osMemoryPoolAlloc) \
  FAKE(osMemoryPoolFree)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ledStrip, LOG_LEVEL_DBG);

#undef LOG_MODULE_DECLARE
#define LOG_MODULE_DECLARE(...)

/* Mock device and LED strip functions */
FAKE_VALUE_FUNC(bool, device_is_ready_mock, const struct device *);
FAKE_VALUE_FUNC(int, led_strip_update_channels_mock, const struct device *, uint8_t *, size_t);

/* Mock CMSIS OS2 pool functions */
FAKE_VALUE_FUNC(osMemoryPoolId_t, osMemoryPoolNew, uint32_t, uint32_t, void *);
FAKE_VALUE_FUNC(void *, osMemoryPoolAlloc, osMemoryPoolId_t, uint32_t);
FAKE_VALUE_FUNC(int, osMemoryPoolFree, osMemoryPoolId_t, void *);

/* Mock LED strip device */
static const struct device mock_led_strip_dev __attribute__((unused)) = {
  .name = "mock_led_strip"
};

/* Override device tree macros */
#undef DT_ALIAS
#define DT_ALIAS(name) DT_N_NODELABEL_ws2812

#undef DEVICE_DT_GET
#define DEVICE_DT_GET(node_id) (&mock_led_strip_dev)

/* Mock DT node properties for ws2812 */
#define DT_N_NODELABEL_ws2812_P_chain_length       5
#define DT_N_NODELABEL_ws2812_P_color_mapping_LEN  3

#include "ledStripUtil.c"

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *util_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void util_tests_before(void *fixture)
{
  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * @test applyGlobalBrightness must leave all channels unchanged at full brightness.
 */
ZTEST(ledStripUtil, test_applyGlobalBrightness_fullBrightness)
{
  LedPixel_t frame[5];

  for(size_t i = 0; i < 5; ++i)
  {
    frame[i].ch0 = 255;
    frame[i].ch1 = 128;
    frame[i].ch2 = 64;
  }

  ledStripUtilSetBrightness(255);
  applyGlobalBrightness(frame);

  for(size_t i = 0; i < 5; ++i)
  {
    zassert_equal(frame[i].ch0, 255, "ch0 should be unchanged at full brightness");
    zassert_equal(frame[i].ch1, 128, "ch1 should be unchanged at full brightness");
    zassert_equal(frame[i].ch2, 64,  "ch2 should be unchanged at full brightness");
  }
}

/**
 * @test applyGlobalBrightness must zero all channels at zero brightness.
 */
ZTEST(ledStripUtil, test_applyGlobalBrightness_zeroBrightness)
{
  LedPixel_t frame[5];

  for(size_t i = 0; i < 5; ++i)
  {
    frame[i].ch0 = 255;
    frame[i].ch1 = 128;
    frame[i].ch2 = 64;
  }

  ledStripUtilSetBrightness(0);
  applyGlobalBrightness(frame);

  for(size_t i = 0; i < 5; ++i)
  {
    zassert_equal(frame[i].ch0, 0, "ch0 should be 0 at zero brightness");
    zassert_equal(frame[i].ch1, 0, "ch1 should be 0 at zero brightness");
    zassert_equal(frame[i].ch2, 0, "ch2 should be 0 at zero brightness");
  }
}

/**
 * @test applyGlobalBrightness must scale all channels correctly at partial brightness.
 */
ZTEST(ledStripUtil, test_applyGlobalBrightness_partialBrightness)
{
  LedPixel_t frame[5];

  for(size_t i = 0; i < 5; ++i)
  {
    frame[i].ch0 = 255;
    frame[i].ch1 = 255;
    frame[i].ch2 = 0;
  }

  ledStripUtilSetBrightness(128);
  applyGlobalBrightness(frame);

  for(size_t i = 0; i < 5; ++i)
  {
    zassert_equal(frame[i].ch0, 128, "ch0 should be scaled to 128 at half brightness");
    zassert_equal(frame[i].ch1, 128, "ch1 should be scaled to 128 at half brightness");
    zassert_equal(frame[i].ch2, 0,   "ch2 should remain 0 at half brightness");
  }
}

/**
 * @test ledStripUtilInitStrip must return -EBUSY when the device is not ready.
 */
ZTEST(ledStripUtil, test_initStrip_deviceNotReady)
{
  int result;

  device_is_ready_mock_fake.return_val = false;

  result = ledStripUtilInitStrip();

  zassert_equal(result, -EBUSY, "Expected -EBUSY when device not ready");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once");
}

/**
 * @test ledStripUtilInitStrip must return 0 on success.
 */
ZTEST(ledStripUtil, test_initStrip_success)
{
  int result;

  device_is_ready_mock_fake.return_val = true;

  result = ledStripUtilInitStrip();

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(device_is_ready_mock_fake.call_count, 1,
                "device_is_ready should be called once");
  zassert_equal(device_is_ready_mock_fake.arg0_val, &mock_led_strip_dev,
                "device_is_ready should be called with the LED strip device");
}

/**
 * @test ledStripUtilInitFramebuffers must return -ENOSPC when the framebuffers
 *       allocation fails.
 */
ZTEST(ledStripUtil, test_initFramebuffers_allocFails)
{
  int result;

  osMemoryPoolNew_fake.return_val = NULL;

  result = ledStripUtilInitFramebuffers();

  zassert_equal(result, -ENOSPC, "Expected -ENOSPC when pool allocation fails");
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
}

/**
 * @test ledStripUtilInitFramebuffers must return 0 on success.
 */
ZTEST(ledStripUtil, test_initFramebuffers_success)
{
  int result;

  osMemoryPoolNew_fake.return_val = (osMemoryPoolId_t)0x1000;

  result = ledStripUtilInitFramebuffers();

  zassert_equal(result, 0, "Expected success (0)");
  zassert_equal(osMemoryPoolNew_fake.call_count, 1,
                "osMemoryPoolNew should be called once");
  zassert_equal(osMemoryPoolNew_fake.arg0_val, 2,
                "Pool should have 2 blocks");
  zassert_equal(osMemoryPoolNew_fake.arg1_val,
                DT_N_NODELABEL_ws2812_P_chain_length *
                DT_N_NODELABEL_ws2812_P_color_mapping_LEN,
                "Block size should be pixel_count * num_channels");
}

/**
 * @test ledStripUtilGetNextFramebuffer must return NULL when the pool allocation fails.
 */
ZTEST(ledStripUtil, test_getNextFramebuffer_allocFails)
{
  LedPixel_t *result;

  osMemoryPoolAlloc_fake.return_val = NULL;

  result = ledStripUtilGetNextFramebuffer();

  zassert_is_null(result, "Expected NULL when pool allocation fails");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
}

/**
 * @test ledStripUtilGetNextFramebuffer must return the allocated block on success.
 */
ZTEST(ledStripUtil, test_getNextFramebuffer_success)
{
  LedPixel_t mockBlock[5];
  LedPixel_t *result;

  osMemoryPoolAlloc_fake.return_val = mockBlock;

  result = ledStripUtilGetNextFramebuffer();

  zassert_equal(result, mockBlock,
                "Expected the allocated block returned by osMemoryPoolAlloc");
  zassert_equal(osMemoryPoolAlloc_fake.call_count, 1,
                "osMemoryPoolAlloc should be called once");
  zassert_equal(osMemoryPoolAlloc_fake.arg1_val, FRAMEBUFFER_ALLOC_TIMEOUT,
                "osMemoryPoolAlloc should be called with the allocation timeout");
}

/**
 * @test ledStripUtilActivateFrame must free the current frame and store the new one.
 */
ZTEST(ledStripUtil, test_activateFrame_success)
{
  osMemoryPoolId_t mockPool = (osMemoryPoolId_t)0x1000;
  LedPixel_t oldFrame[5];
  LedPixel_t newFrame[5];

  osMemoryPoolNew_fake.return_val = mockPool;
  ledStripUtilInitFramebuffers();

  ledStripUtilActivateFrame(oldFrame);
  FFF_FAKES_LIST(RESET_FAKE);

  ledStripUtilActivateFrame(newFrame);

  zassert_equal(osMemoryPoolFree_fake.call_count, 1,
                "osMemoryPoolFree should be called once to release the previous frame");
  zassert_equal(osMemoryPoolFree_fake.arg0_val, mockPool,
                "osMemoryPoolFree should be called with the framebuffer pool");
  zassert_equal(osMemoryPoolFree_fake.arg1_val, oldFrame,
                "osMemoryPoolFree should be called with the previous frame");
}

/**
 * @test ledStripUtilPushFrame must return error when updating the LED strip channels fails.
 */
ZTEST(ledStripUtil, test_pushFrame_updateChannelsFails)
{
  int result;

  led_strip_update_channels_mock_fake.return_val = -EIO;

  result = ledStripUtilPushFrame();

  zassert_equal(result, -EIO,
                "ledStripUtilPushFrame should return error from led_strip_update_channels");
  zassert_equal(led_strip_update_channels_mock_fake.call_count, 1,
                "led_strip_update_channels should be called once");
}

/**
 * @test ledStripUtilPushFrame must push the active frame to the LED strip on success.
 */
ZTEST(ledStripUtil, test_pushFrame_success)
{
  LedPixel_t mockFrame[5];
  int result;

  ledStripUtilActivateFrame(mockFrame);

  result = ledStripUtilPushFrame();

  zassert_equal(result, 0, "ledStripUtilPushFrame should return 0 on success");
  zassert_equal(led_strip_update_channels_mock_fake.call_count, 1,
                "led_strip_update_channels should be called once");
  zassert_equal(led_strip_update_channels_mock_fake.arg0_val, &mock_led_strip_dev,
                "led_strip_update_channels should be called with the LED strip device");
  zassert_equal(led_strip_update_channels_mock_fake.arg1_val, (uint8_t *)mockFrame,
                "led_strip_update_channels should be called with the active frame");
  zassert_equal(led_strip_update_channels_mock_fake.arg2_val, CONFIG_ENYA_LED_STRIP_NUM_CHANNELS,
                "led_strip_update_channels should be called with the number of channels");
}

/**
 * @test ledStripUtilSetBrightness must store the brightness value used by applyGlobalBrightness.
 */
ZTEST(ledStripUtil, test_setBrightness_success)
{
  LedPixel_t frame[5];

  for(size_t i = 0; i < 5; ++i)
  {
    frame[i].ch0 = 255;
    frame[i].ch1 = 255;
    frame[i].ch2 = 255;
  }

  ledStripUtilSetBrightness(128);
  applyGlobalBrightness(frame);

  for(size_t i = 0; i < 5; ++i)
  {
    zassert_equal(frame[i].ch0, 128, "ch0 should be scaled to 128 after setting brightness to 128");
    zassert_equal(frame[i].ch1, 128, "ch1 should be scaled to 128 after setting brightness to 128");
    zassert_equal(frame[i].ch2, 128, "ch2 should be scaled to 128 after setting brightness to 128");
  }
}

ZTEST_SUITE(ledStripUtil, NULL, util_tests_setup, util_tests_before, NULL, NULL);
