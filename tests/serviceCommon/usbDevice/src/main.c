/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-05-02
 * @brief     USB Device Tests
 *
 *            Unit tests for the USB device utility.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <stdint.h>

DEFINE_FFF_GLOBALS;

/* Mock Kconfig options */
#define CONFIG_ENYA_USB_DEVICE_LOG_LEVEL    3
#define CONFIG_ENYA_USB_DEVICE_VID          0x1209
#define CONFIG_ENYA_USB_DEVICE_PID          0xE582
#define CONFIG_ENYA_USB_DEVICE_MANUFACTURER "Electronya"
#define CONFIG_ENYA_USB_DEVICE_PRODUCT      "Electronya Device"
#define CONFIG_ENYA_USB_DEVICE_MAX_POWER    250

/* Prevent usbd.h - define types manually */
#define ZEPHYR_INCLUDE_USBD_H_

/* Mock USBD types */
struct usbd_context {};
struct usbd_desc_node {};
struct usbd_config_node {};
struct usbd_msg {};

typedef void (*usbd_msg_cb_t)(struct usbd_context *const ctx,
                              const struct usbd_msg *const msg);

enum usbd_speed
{
  USBD_SPEED_FS = 0,
  USBD_SPEED_HS,
};

#define USB_BCC_MISCELLANEOUS 0xEF
#define USB_SCD_SELF_POWERED  0x40

/* Mock USB class Kconfig — define one IAD class to test the IAD code triple path */
#define CONFIG_USBD_CDC_ACM_CLASS 1

/* Mock USBD definition macros */
#define USBD_DEVICE_DEFINE(name, dev, vid, pid) \
  static struct usbd_context name

#define USBD_DESC_LANG_DEFINE(name) \
  static struct usbd_desc_node name

#define USBD_DESC_MANUFACTURER_DEFINE(name, str) \
  static struct usbd_desc_node name

#define USBD_DESC_PRODUCT_DEFINE(name, str) \
  static struct usbd_desc_node name

#define USBD_DESC_SERIAL_NUMBER_DEFINE(name) \
  static struct usbd_desc_node name

#define USBD_DESC_CONFIG_DEFINE(name, str) \
  static struct usbd_desc_node name __attribute__((unused))

#define USBD_CONFIGURATION_DEFINE(name, attr, power, desc) \
  static struct usbd_config_node name

/* Wrap functions to use mocks */
#define usbd_add_descriptor         usbd_add_descriptor_mock
#define usbd_add_configuration      usbd_add_configuration_mock
#define usbd_register_all_classes   usbd_register_all_classes_mock
#define usbd_device_set_code_triple usbd_device_set_code_triple_mock
#define usbd_msg_register_cb        usbd_msg_register_cb_mock
#define usbd_init                   usbd_init_mock
#define usbd_enable                 usbd_enable_mock
#define usbd_disable                usbd_disable_mock

/* FFF fakes list */
#define FFF_FAKES_LIST(FAKE)               \
  FAKE(usbd_add_descriptor_mock)           \
  FAKE(usbd_add_configuration_mock)        \
  FAKE(usbd_register_all_classes_mock)     \
  FAKE(usbd_device_set_code_triple_mock)   \
  FAKE(usbd_msg_register_cb_mock)          \
  FAKE(usbd_init_mock)                     \
  FAKE(usbd_enable_mock)                   \
  FAKE(usbd_disable_mock)

/* Setup logging */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbDevice, LOG_LEVEL_DBG);

#undef LOG_MODULE_REGISTER
#define LOG_MODULE_REGISTER(...)

/* FFF mock declarations */
FAKE_VALUE_FUNC(int, usbd_add_descriptor_mock,
                struct usbd_context *, struct usbd_desc_node *);
FAKE_VALUE_FUNC(int, usbd_add_configuration_mock,
                struct usbd_context *, enum usbd_speed, struct usbd_config_node *);
FAKE_VALUE_FUNC(int, usbd_register_all_classes_mock,
                struct usbd_context *, enum usbd_speed, uint8_t, const char *const *);
FAKE_VALUE_FUNC(int, usbd_device_set_code_triple_mock,
                struct usbd_context *, enum usbd_speed, uint8_t, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, usbd_msg_register_cb_mock,
                struct usbd_context *, usbd_msg_cb_t);
FAKE_VALUE_FUNC(int, usbd_init_mock,    struct usbd_context *);
FAKE_VALUE_FUNC(int, usbd_enable_mock,  struct usbd_context *);
FAKE_VALUE_FUNC(int, usbd_disable_mock, struct usbd_context *);

/* Helper for per-call descriptor failure */
static int descriptorCallCount;
static int descriptorFailOn;

static int addDescriptorWithNthFail(struct usbd_context *ctx,
                                    struct usbd_desc_node *node)
{
  ARG_UNUSED(ctx);
  ARG_UNUSED(node);
  return (++descriptorCallCount == descriptorFailOn) ? -EINVAL : 0;
}

/* Mock message callback */
static void mockMsgCb(struct usbd_context *const ctx,
                      const struct usbd_msg *const msg)
{
  ARG_UNUSED(ctx);
  ARG_UNUSED(msg);
}

#include "usbDevice.c"

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *usbDevice_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void usbDevice_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  descriptorCallCount = 0;
  descriptorFailOn    = 0;
}

/**
 * @test usbDeviceInit must return error when the language descriptor fails.
 */
ZTEST(usbDevice, test_init_langDescFails)
{
  int result;

  descriptorFailOn = 1;
  usbd_add_descriptor_mock_fake.custom_fake = addDescriptorWithNthFail;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when language descriptor fails");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 1,
                "usbd_add_descriptor should be called once");
  zassert_equal(usbd_add_descriptor_mock_fake.arg0_history[0], &usbCtx,
                "usbd_add_descriptor should be called with the USB context");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[0], &usbLangDesc,
                "usbd_add_descriptor should be called with the language descriptor");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 0,
                "usbd_add_configuration should not be called");
}

/**
 * @test usbDeviceInit must return error when the manufacturer descriptor fails.
 */
ZTEST(usbDevice, test_init_mfrDescFails)
{
  int result;

  descriptorFailOn = 2;
  usbd_add_descriptor_mock_fake.custom_fake = addDescriptorWithNthFail;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when manufacturer descriptor fails");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 2,
                "usbd_add_descriptor should be called twice");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[1], &usbMfrDesc,
                "second call should be with the manufacturer descriptor");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 0,
                "usbd_add_configuration should not be called");
}

/**
 * @test usbDeviceInit must return error when the product descriptor fails.
 */
ZTEST(usbDevice, test_init_productDescFails)
{
  int result;

  descriptorFailOn = 3;
  usbd_add_descriptor_mock_fake.custom_fake = addDescriptorWithNthFail;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when product descriptor fails");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 3,
                "usbd_add_descriptor should be called 3 times");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[2], &usbProductDesc,
                "third call should be with the product descriptor");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 0,
                "usbd_add_configuration should not be called");
}

/**
 * @test usbDeviceInit must return error when the serial number descriptor fails.
 */
ZTEST(usbDevice, test_init_snDescFails)
{
  int result;

  descriptorFailOn = 4;
  usbd_add_descriptor_mock_fake.custom_fake = addDescriptorWithNthFail;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when serial number descriptor fails");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 4,
                "usbd_add_descriptor should be called 4 times");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[3], &usbSnDesc,
                "fourth call should be with the serial number descriptor");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 0,
                "usbd_add_configuration should not be called");
}

/**
 * @test usbDeviceInit must return error when adding the FS configuration fails.
 */
ZTEST(usbDevice, test_init_addConfigFails)
{
  int result;

  usbd_add_configuration_mock_fake.return_val = -EINVAL;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when add configuration fails");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 4,
                "All 4 descriptors should be added before configuration");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 1,
                "usbd_add_configuration should be called once");
  zassert_equal(usbd_add_configuration_mock_fake.arg0_val, &usbCtx,
                "usbd_add_configuration should be called with the USB context");
  zassert_equal(usbd_add_configuration_mock_fake.arg1_val, USBD_SPEED_FS,
                "usbd_add_configuration should be called with FS speed");
  zassert_equal(usbd_add_configuration_mock_fake.arg2_val, &usbFsConfig,
                "usbd_add_configuration should be called with the FS config");
  zassert_equal(usbd_register_all_classes_mock_fake.call_count, 0,
                "usbd_register_all_classes should not be called");
}

/**
 * @test usbDeviceInit must return error when registering classes fails.
 */
ZTEST(usbDevice, test_init_registerClassesFails)
{
  int result;

  usbd_register_all_classes_mock_fake.return_val = -EINVAL;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when register classes fails");
  zassert_equal(usbd_register_all_classes_mock_fake.call_count, 1,
                "usbd_register_all_classes should be called once");
  zassert_equal(usbd_register_all_classes_mock_fake.arg0_val, &usbCtx,
                "usbd_register_all_classes should be called with the USB context");
  zassert_equal(usbd_register_all_classes_mock_fake.arg1_val, USBD_SPEED_FS,
                "usbd_register_all_classes should be called with FS speed");
  zassert_equal(usbd_register_all_classes_mock_fake.arg2_val, 1,
                "usbd_register_all_classes should use config slot 1");
  zassert_is_null(usbd_register_all_classes_mock_fake.arg3_val,
                  "usbd_register_all_classes should be called with NULL blocklist");
  zassert_equal(usbd_init_mock_fake.call_count, 0,
                "usbd_init should not be called");
}

/**
 * @test usbDeviceInit must return error when registering the message callback fails.
 */
ZTEST(usbDevice, test_init_registerCbFails)
{
  int result;

  usbd_msg_register_cb_mock_fake.return_val = -EINVAL;

  result = usbDeviceInit(mockMsgCb);

  zassert_equal(result, -EINVAL,
                "Expected error when message callback registration fails");
  zassert_equal(usbd_msg_register_cb_mock_fake.call_count, 1,
                "usbd_msg_register_cb should be called once");
  zassert_equal(usbd_msg_register_cb_mock_fake.arg0_val, &usbCtx,
                "usbd_msg_register_cb should be called with the USB context");
  zassert_equal(usbd_msg_register_cb_mock_fake.arg1_val, mockMsgCb,
                "usbd_msg_register_cb should be called with the provided callback");
  zassert_equal(usbd_init_mock_fake.call_count, 0,
                "usbd_init should not be called");
}

/**
 * @test usbDeviceInit must return error when usbd_init fails.
 */
ZTEST(usbDevice, test_init_usbdInitFails)
{
  int result;

  usbd_init_mock_fake.return_val = -EINVAL;

  result = usbDeviceInit(NULL);

  zassert_equal(result, -EINVAL,
                "Expected error when usbd_init fails");
  zassert_equal(usbd_init_mock_fake.call_count, 1,
                "usbd_init should be called once");
  zassert_equal(usbd_init_mock_fake.arg0_val, &usbCtx,
                "usbd_init should be called with the USB context");
}

/**
 * @test usbDeviceInit must succeed without calling usbd_msg_register_cb when
 *       msgCb is NULL, and must set the IAD code triple when an IAD class is enabled.
 */
ZTEST(usbDevice, test_init_nullCb_success)
{
  int result;

  result = usbDeviceInit(NULL);

  zassert_equal(result, 0, "Expected success");
  zassert_equal(usbd_add_descriptor_mock_fake.call_count, 4,
                "All 4 descriptors should be added");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[0], &usbLangDesc,
                "first descriptor should be the language descriptor");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[1], &usbMfrDesc,
                "second descriptor should be the manufacturer descriptor");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[2], &usbProductDesc,
                "third descriptor should be the product descriptor");
  zassert_equal(usbd_add_descriptor_mock_fake.arg1_history[3], &usbSnDesc,
                "fourth descriptor should be the serial number descriptor");
  zassert_equal(usbd_add_configuration_mock_fake.call_count, 1,
                "usbd_add_configuration should be called once");
  zassert_equal(usbd_register_all_classes_mock_fake.call_count, 1,
                "usbd_register_all_classes should be called once");
  zassert_equal(usbd_device_set_code_triple_mock_fake.call_count, 1,
                "usbd_device_set_code_triple should be called once");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg0_val, &usbCtx,
                "usbd_device_set_code_triple should be called with the USB context");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg1_val, USBD_SPEED_FS,
                "usbd_device_set_code_triple should be called with FS speed");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg2_val, USB_BCC_MISCELLANEOUS,
                "IAD class enabled: code triple class should be USB_BCC_MISCELLANEOUS");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg3_val, 0x02,
                "IAD class enabled: code triple subclass should be 0x02");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg4_val, 0x01,
                "IAD class enabled: code triple protocol should be 0x01");
  zassert_equal(usbd_msg_register_cb_mock_fake.call_count, 0,
                "usbd_msg_register_cb should not be called with NULL callback");
  zassert_equal(usbd_init_mock_fake.call_count, 1,
                "usbd_init should be called once");
  zassert_equal(usbd_init_mock_fake.arg0_val, &usbCtx,
                "usbd_init should be called with the USB context");
}

/**
 * @test usbDeviceInit must succeed and register the callback when msgCb is not NULL.
 */
ZTEST(usbDevice, test_init_withCb_success)
{
  int result;

  result = usbDeviceInit(mockMsgCb);

  zassert_equal(result, 0, "Expected success");
  zassert_equal(usbd_msg_register_cb_mock_fake.call_count, 1,
                "usbd_msg_register_cb should be called once");
  zassert_equal(usbd_msg_register_cb_mock_fake.arg0_val, &usbCtx,
                "usbd_msg_register_cb should be called with the USB context");
  zassert_equal(usbd_msg_register_cb_mock_fake.arg1_val, mockMsgCb,
                "usbd_msg_register_cb should be called with the provided callback");
  zassert_equal(usbd_init_mock_fake.call_count, 1,
                "usbd_init should be called once");
  zassert_equal(usbd_init_mock_fake.arg0_val, &usbCtx,
                "usbd_init should be called with the USB context");
}

/**
 * @test usbDeviceEnable must return error when usbd_enable fails.
 */
ZTEST(usbDevice, test_enable_fails)
{
  int result;

  usbd_enable_mock_fake.return_val = -EINVAL;

  result = usbDeviceEnable();

  zassert_equal(result, -EINVAL, "Expected error when usbd_enable fails");
  zassert_equal(usbd_enable_mock_fake.call_count, 1,
                "usbd_enable should be called once");
  zassert_equal(usbd_enable_mock_fake.arg0_val, &usbCtx,
                "usbd_enable should be called with the USB context");
}

/**
 * @test usbDeviceEnable must return 0 on success.
 */
ZTEST(usbDevice, test_enable_success)
{
  int result;

  result = usbDeviceEnable();

  zassert_equal(result, 0, "Expected success");
  zassert_equal(usbd_enable_mock_fake.call_count, 1,
                "usbd_enable should be called once");
  zassert_equal(usbd_enable_mock_fake.arg0_val, &usbCtx,
                "usbd_enable should be called with the USB context");
}

/**
 * @test usbDeviceDisable must return error when usbd_disable fails.
 */
ZTEST(usbDevice, test_disable_fails)
{
  int result;

  usbd_disable_mock_fake.return_val = -EINVAL;

  result = usbDeviceDisable();

  zassert_equal(result, -EINVAL, "Expected error when usbd_disable fails");
  zassert_equal(usbd_disable_mock_fake.call_count, 1,
                "usbd_disable should be called once");
  zassert_equal(usbd_disable_mock_fake.arg0_val, &usbCtx,
                "usbd_disable should be called with the USB context");
}

/**
 * @test usbDeviceDisable must return 0 on success.
 */
ZTEST(usbDevice, test_disable_success)
{
  int result;

  result = usbDeviceDisable();

  zassert_equal(result, 0, "Expected success");
  zassert_equal(usbd_disable_mock_fake.call_count, 1,
                "usbd_disable should be called once");
  zassert_equal(usbd_disable_mock_fake.arg0_val, &usbCtx,
                "usbd_disable should be called with the USB context");
}

ZTEST_SUITE(usbDevice, NULL, usbDevice_tests_setup, usbDevice_tests_before, NULL, NULL);
