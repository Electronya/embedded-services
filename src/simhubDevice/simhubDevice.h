/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      simhubDevice.h
 * @author    jbacon
 * @date      2026-05-03
 * @brief     SimHub Device Service
 *
 *            SimHub device service API. Implements the SimHub Standard
 *            Arduino/Dash serial protocol over CDC ACM USB, receiving RGB
 *            frame data from SimHub and forwarding it to the LED strip
 *            service.
 *
 * @defgroup  simhubDevice SimHub Device Service
 *
 * @{
 */

#ifndef SIMHUB_DEVICE_H
#define SIMHUB_DEVICE_H

/**
 * @brief   Initialize the SimHub device service.
 *
 *          Binds the CDC ACM UART from the simhub-uart DTS alias, creates the
 *          service thread, and registers with the service manager. The thread
 *          does not start until the service manager calls the start callback.
 *
 * @return  0 if successful, the error code otherwise.
 */
int simhubDeviceInit(void);

#endif /* SIMHUB_DEVICE_H */

/** @} */
