# Copyright (C) 2025 by Electronya
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ENYA_SERVICE_MANAGER)
  zephyr_include_directories(.)

  zephyr_library()
  zephyr_library_sources(
    serviceManager.c
    serviceManagerUtil.c
  )

  if(CONFIG_ENYA_SERVICE_MANAGER_SHELL)
    zephyr_library_sources(serviceManagerCmd.c)
  endif()
endif()
