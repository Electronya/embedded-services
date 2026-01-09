# Electronya Embedded Services
# Copyright (C) 2025 by Electronya

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_ENYA_SERVICE_COMMON)
  add_subdirectory(./src/serviceCommon)
endif()

if(CONFIG_ENYA_ADC_ACQUISITION)
  add_subdirectory(./src/adcAcquisition)
endif()
