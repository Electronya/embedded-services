# Electronya SimHub Device Service Tests
# Copyright (C) 2026 by Electronya
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(simhubDevice_test)

target_sources(app
  PRIVATE
  src/main.c
)

target_include_directories(app
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/simhubDevice
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/serviceCommon
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/serviceManager
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/ledStrip
)

if(COVERAGE)
  target_compile_options(app PRIVATE --coverage)
  target_link_options(app PRIVATE --coverage)
endif()
