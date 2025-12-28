# Electronya ADC Acquisition Service
# Copyright (C) 2025 by Electronya

zephyr_include_directories(.)

if(NOT DEFINED CONFIG_ZTEST)
  zephyr_library_sources(
    adcAcquisition.c
    adcAcquisitionUtil.c
    adcAcquisitionFilter.c
    adcAcquisitionCmd.c
  )
endif()
