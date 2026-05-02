# Electronya LED Strip Service
# Copyright (C) 2026 by Electronya

if(CONFIG_ENYA_LED_STRIP)
  zephyr_include_directories(.)

  if(NOT DEFINED CONFIG_ZTEST)
    zephyr_library_sources(
      ledStrip.c
      ledStripUtil.c
    )

    if(CONFIG_ENYA_LED_STRIP_SHELL)
      zephyr_library_sources(ledStripCmd.c)
    endif()
  endif()
endif()
