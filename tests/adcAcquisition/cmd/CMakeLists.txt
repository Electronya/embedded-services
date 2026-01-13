# Copyright (C) 2026 by Electronya
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(adcAcquisitionCmd_test)

target_sources(app
  PRIVATE
  src/main.c
)

target_include_directories(app
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/adcAcquisition
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/serviceCommon
)

if(COVERAGE)
  target_compile_options(app PRIVATE --coverage)
  target_link_options(app PRIVATE --coverage)
endif()
