/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      usbDevice.c
 * @author    jbacon
 * @date      2026-05-02
 * @brief     USB Device Utility
 *
 *            Shared USB device stack utility. Owns the USBD context,
 *            string descriptors, and FS configuration. All USB-capable
 *            services depend on this for USB initialization.
 *
 * @ingroup   service-common
 *
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

#include "usbDevice.h"

LOG_MODULE_REGISTER(usbDevice, CONFIG_ENYA_USB_DEVICE_LOG_LEVEL);

USBD_DEVICE_DEFINE(usbCtx,
                   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   CONFIG_ENYA_USB_DEVICE_VID,
                   CONFIG_ENYA_USB_DEVICE_PID);

USBD_DESC_LANG_DEFINE(usbLangDesc);
USBD_DESC_MANUFACTURER_DEFINE(usbMfrDesc, CONFIG_ENYA_USB_DEVICE_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(usbProductDesc, CONFIG_ENYA_USB_DEVICE_PRODUCT);
USBD_DESC_SERIAL_NUMBER_DEFINE(usbSnDesc);

USBD_DESC_CONFIG_DEFINE(usbFsCfgDesc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(usbFsConfig, USB_SCD_SELF_POWERED,
                          CONFIG_ENYA_USB_DEVICE_MAX_POWER, &usbFsCfgDesc);

int usbDeviceInit(usbd_msg_cb_t msgCb)
{
  int err;

  err = usbd_add_descriptor(&usbCtx, &usbLangDesc);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to add language descriptor", err);
    return err;
  }

  err = usbd_add_descriptor(&usbCtx, &usbMfrDesc);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to add manufacturer descriptor", err);
    return err;
  }

  err = usbd_add_descriptor(&usbCtx, &usbProductDesc);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to add product descriptor", err);
    return err;
  }

  err = usbd_add_descriptor(&usbCtx, &usbSnDesc);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to add serial number descriptor", err);
    return err;
  }

  err = usbd_add_configuration(&usbCtx, USBD_SPEED_FS, &usbFsConfig);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to add FS configuration", err);
    return err;
  }

  err = usbd_register_all_classes(&usbCtx, USBD_SPEED_FS, 1, NULL);
  if(err)
  {
    LOG_ERR("ERROR %d: failed to register USB classes", err);
    return err;
  }

  usbd_device_set_code_triple(&usbCtx, USBD_SPEED_FS,
                              USB_BCC_MISCELLANEOUS, 0x02, 0x01);

  if(msgCb)
  {
    err = usbd_msg_register_cb(&usbCtx, msgCb);
    if(err)
    {
      LOG_ERR("ERROR %d: failed to register message callback", err);
      return err;
    }
  }

  err = usbd_init(&usbCtx);
  if(err)
    LOG_ERR("ERROR %d: failed to initialize USB device", err);

  return err;
}

int usbDeviceEnable(void)
{
  int err;

  err = usbd_enable(&usbCtx);
  if(err)
    LOG_ERR("ERROR %d: failed to enable USB device", err);

  return err;
}

int usbDeviceDisable(void)
{
  int err;

  err = usbd_disable(&usbCtx);
  if(err)
    LOG_ERR("ERROR %d: failed to disable USB device", err);

  return err;
}

/** @} */
