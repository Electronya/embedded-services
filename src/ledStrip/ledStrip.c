/**
 * Copyright (C) 2026 by Electronya
 *
 * @file      ledStrip.c
 * @author    jbacon
 * @date      2026-04-24
 * @brief     LED Strip Updater
 *
 *            LED strip updater service.
 *
 * @ingroup ledStrip LED Strip Updater
 *
 * @{
 */

#include <zephyr/logging/log.h>

#include "ledStrip.h"
#include "ledStripUtil.h"

/* Setting module logging */
LOG_MODULE_REGISTER(LED_STRIP_LOGGER_NAME, CONFIG_ENYA_LED_STRIP_LOG_LEVEL);

/**
 * @brief The thread stack.
 */
K_THREAD_STACK_DEFINE(ledStripStack, CONFIG_ENYA_LED_STRIP_STACK_SIZE);

/**
 * @brief   LED strip updater thread.
 *
 * @param[in]   p1: Thread first parameter.
 * @param[in]   p2: Thread second parameter.
 * @param[in]   p3: Thread third parameter.
 */
// static void run(void *p1, void *p2, void *p3) { int err; }

int ledStripInit(void)
{
  // int err;

  return 0;
}

/** @} */
