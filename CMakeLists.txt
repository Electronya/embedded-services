# Electronya Embedded Services
# Copyright (C) 2025 by Electronya

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_ENYA_ADC_ACQUISITION)
  zephyr_include_directories(./src/adcAcquisition/)

  if(NOT DEFINED CONFIG_ZTEST)
    zephyr_library_sources(
      ./src/adcAcquisition/adcAcquisition.c
      ./src/adcAcquisition/adcAcquisitionUtil.c
      ./src/adcAcquisition/adcAcquisitionFilter.c
      ./src/adcAcquisition/adcAcquisitionCmd.c
    )
  endif()
endif()
