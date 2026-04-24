# Electronya Embedded Datastore
# Copyright (C) 2025 by Electronya

if(CONFIG_ENYA_DATASTORE)
  zephyr_include_directories(.)

  if(NOT DEFINED CONFIG_ZTEST)
    zephyr_library_sources(
      datastore.c
      datastoreUtil.c
    )

    if(CONFIG_ENYA_DATASTORE_SHELL)
      zephyr_library_sources(
        datastoreCmd.c
        datastoreCmdUtil.c
      )
    endif()
  endif()
endif()
