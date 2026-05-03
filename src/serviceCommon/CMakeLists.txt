# Electronya Embedded Services - Service Common
# Copyright (C) 2026 by Electronya

# Header-only module - just add include directories
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_ENYA_USB_DEVICE usbDevice.c)
