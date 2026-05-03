/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      main.c
 * @author    jbacon
 * @date      2026-05-02
 * @brief     USB Device No-IAD Tests
 *
 *            Unit tests for usbDevice compiled without any IAD USB class
 *            (HID, MSC, DFU, custom vendor). Verifies that usbd_device_set_code_triple
 *            is called with (0, 0, 0) instead of the CDC/MIDI/Audio IAD triple.
 *
 *            Built as a separate binary because the IAD branch is resolved at
 *            compile time via preprocessor conditionals in usbDevice.c.
 */

#include <zephyr/ztest.h>
#include <zephyr/fff.h>
#include <stdint.h>

DEFINE_FFF_GLOBALS;

/* Mock Kconfig options */
#define CONFIG_ENYA_USB_DEVICE_LOG_LEVEL    3
#define CONFIG_ENYA_USB_DEVICE_VID          0x2FE3
#define CONFIG_ENYA_USB_DEVICE_PID          0x0001
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

/* No IAD class defined — CONFIG_USBD_CDC_ACM_CLASS and friends are absent */

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

#include "usbDevice.c"

/**
 * @brief Setup function called before all tests in the suite.
 */
static void *usbDeviceNoIad_tests_setup(void)
{
  return NULL;
}

/**
 * @brief Setup function called before each test in the suite.
 */
static void usbDeviceNoIad_tests_before(void *fixture)
{
  ARG_UNUSED(fixture);

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();
}

/**
 * @test usbDeviceInit must set the zero code triple when no IAD class is enabled.
 */
ZTEST(usbDeviceNoIad, test_init_zeroCodeTriple)
{
  int result;

  result = usbDeviceInit(NULL);

  zassert_equal(result, 0, "Expected success");
  zassert_equal(usbd_device_set_code_triple_mock_fake.call_count, 1,
                "usbd_device_set_code_triple should be called once");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg0_val, &usbCtx,
                "usbd_device_set_code_triple should be called with the USB context");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg1_val, USBD_SPEED_FS,
                "usbd_device_set_code_triple should be called with FS speed");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg2_val, 0,
                "No IAD class: code triple class should be 0");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg3_val, 0,
                "No IAD class: code triple subclass should be 0");
  zassert_equal(usbd_device_set_code_triple_mock_fake.arg4_val, 0,
                "No IAD class: code triple protocol should be 0");
}

ZTEST_SUITE(usbDeviceNoIad, NULL, usbDeviceNoIad_tests_setup,
            usbDeviceNoIad_tests_before, NULL, NULL);
