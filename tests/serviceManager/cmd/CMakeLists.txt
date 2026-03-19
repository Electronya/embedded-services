# Electronya Service Manager Command Tests
# Copyright (C) 2025 by Electronya
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(serviceManagerCmd_test)

# Add test source
target_sources(app
  PRIVATE
  src/main.c
)

# Include directories
target_include_directories(app
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/serviceManager
)

# Enable coverage if requested
if(COVERAGE)
  target_compile_options(app PRIVATE --coverage)
  target_link_options(app PRIVATE --coverage)
endif()
