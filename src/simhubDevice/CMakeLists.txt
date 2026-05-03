# Electronya SimHub Device Service
# Copyright (C) 2026 by Electronya
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

if(NOT DEFINED CONFIG_ZTEST)
  zephyr_library_sources(
    simhubDevice.c
    simhubDevUtil.c
    simhubDevProto.c
  )

  if(CONFIG_ENYA_SIMHUB_DEVICE_SHELL)
    zephyr_library_sources(simhubDevCmd.c)
  endif()
endif()
