# SPDX-License-Identifier: Apache-2.0
include(FindPythonInterp)

message(STATUS "Python executable: ${PYTHON_EXECUTABLE}")


set(gen_dir ${CMAKE_BINARY_DIR}/zephyr/include/generated/app)

# Generate configuration files for enc key
file(STRINGS ${CMAKE_SOURCE_DIR}/keys/enc_key enc_key)
configure_file(${CMAKE_SOURCE_DIR}/config/app.conf.in ${gen_dir}/app.conf)
list(APPEND OVERLAY_CONFIG ${gen_dir}/app.conf)

set(MCUBOOT_KEY_FILE ${CMAKE_SOURCE_DIR}/keys/key.pem)
configure_file(${CMAKE_SOURCE_DIR}/config/mcuboot.conf.in ${gen_dir}/mcuboot.conf)

set(mcuboot_DTC_OVERLAY_FILE
    ${CMAKE_SOURCE_DIR}/config/app.overlay
)

set(mcuboot_CONF_FILE
	${gen_dir}/mcuboot.conf
)

set(DTC_OVERLAY_FILE
	${CMAKE_SOURCE_DIR}/config/app.overlay
)

set(PM_STATIC_YML_FILE
	${CMAKE_SOURCE_DIR}/config/pm_static.nomcuboot.yml
)
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)
include(FindPythonInterp)
set (py_cmd "generate_file_table.py")
set(CMAKE_VERBOSE_MAKEFILE ON)

message(STATUS "mcuboot conf: ${mcuboot_CONF_FILE}")

target_sources(app PRIVATE
  src/main.cpp
  src/battery.cpp
  src/measurement.cpp
  src/log/log.cpp
  src/security.cpp
  src/firmware_update.cpp
  src/system.cpp
  src/mesh.cpp
  src/radio.c
)

add_custom_target(
  gen_file 
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src/log
  COMMAND ${PYTHON_EXECUTABLE} ${py_cmd}
)

add_dependencies(app gen_file)

if(CONFIG_BOOTLOADER_MCUBOOT)
  set_target_properties(zephyr_property_target PROPERTIES SIGNING_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/custom_signing.cmake)
endif()
