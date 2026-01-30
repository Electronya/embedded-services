# Electronya Embedded Datastore
# Copyright (C) 2025 by Electronya

# Find Zephyr. This also loads Zephyr's build system.
cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_ENYA_DATASTORE)
  zephyr_include_directories(./)

  if(NOT DEFINED CONFIG_ZTEST)
    zephyr_library_sources(
      ./datastore.c
      ./datastoreUtil.c
      ./datastoreCmd.c
    )
  endif()
endif()
