/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      usbDevice.h
 * @author    jbacon
 * @date      2026-05-02
 * @brief     USB Device Utility
 *
 *            Shared USB device stack utility. Owns the USBD context,
 *            string descriptors, and FS configuration. All USB-capable
 *            services depend on this for USB initialization.
 *
 * @defgroup  service-common service-common
 *
 * @{
 */

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>

/**
 * @brief   Initialize the USB device.
 *
 * @param[in]   msgCb: The USB lifecycle message callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int usbDeviceInit(usbd_msg_cb_t msgCb);

/**
 * @brief   Enable the USB device.
 *
 * @return  0 if successful, the error code otherwise.
 */
int usbDeviceEnable(void);

/**
 * @brief   Disable the USB device.
 *
 * @return  0 if successful, the error code otherwise.
 */
int usbDeviceDisable(void);

#endif /* USB_DEVICE_H */

/** @} */
