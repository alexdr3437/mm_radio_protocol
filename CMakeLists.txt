# SPDX-License-Identifier: Apache-2.0
if (CONFIG_MM_RADIO)
	file(GLOB app_sources src/*.c)	
	zephyr_sources(${app_sources})
	zephyr_include_directories(include/)
endif()
