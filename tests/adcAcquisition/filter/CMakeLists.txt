# Copyright (C) 2025 by Electronya
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Determine if we're in unit testing mode
if(BOARD STREQUAL "unit_testing" OR BOARD STREQUAL "unit_testing/unit_testing")
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target testbinary)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target app)
endif()

project(adcAcquisitionFilter_test)

# Add test source (includes filter implementation internally)
target_sources(${target}
  PRIVATE
  src/main.c
)

# Include directories
target_include_directories(${target}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/adcAcquisition
)

# Enable coverage if requested
if(COVERAGE)
  target_compile_options(${target} PRIVATE --coverage)
  target_link_options(${target} PRIVATE --coverage)
endif()
