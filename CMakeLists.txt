cmake_minimum_required(VERSION 3.10)

# Default to size-optimized build (-Os), matching Arduino IDE behaviour.
# Override with: cmake -B build -DCMAKE_BUILD_TYPE=Debug
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

project(osrx VERSION 1.0.0 LANGUAGES C)

# C89 -- required for 8-bit MCU compatibility.
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Optional tuning
# ---------------------------------------------------------------------------
set(OSRX_ID_MAX   "" CACHE STRING "Override OSRX_ID_MAX   (max sensor ID length + NUL)")
set(OSRX_UNIT_MAX "" CACHE STRING "Override OSRX_UNIT_MAX (max unit string length + NUL)")
set(OSRX_BODY_MAX "" CACHE STRING "Override OSRX_BODY_MAX (max body byte count)")

# Flash-reduction switches:
#   -DOSRX_VALIDATE_CRC8=0   skip CRC-8/SMBUS check  (~80 B saved on AVR)
#   -DOSRX_VALIDATE_CRC16=0  skip CRC-16 check        (~90 B saved on AVR)
#   -DOSRX_NO_PARSER=1       exclude streaming parser (~316 B saved on AVR)
#   -DOSRX_NO_TIMESTAMP=1    drop ts_sec field        (~20 B Flash, 4 B RAM)
set(OSRX_VALIDATE_CRC8  "" CACHE STRING "1=check CRC-8 (default), 0=skip (saves ~80 B Flash)")
set(OSRX_VALIDATE_CRC16 "" CACHE STRING "1=check CRC-16 (default), 0=skip (saves ~90 B Flash)")
option(OSRX_NO_PARSER    "Exclude osrx_parser.c from the library (~316 B Flash saved)" OFF)
option(OSRX_NO_TIMESTAMP "Drop ts_sec from osrx_packet_meta (~20 B Flash, 4 B RAM)"    OFF)

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
set(OSRX_SOURCES
    src/osrx_crc.c
    src/osrx_b62.c
    src/osrx_packet.c
    src/osrx_sensor.c
)
if(NOT OSRX_NO_PARSER)
    list(APPEND OSRX_SOURCES src/osrx_parser.c)
endif()

add_library(osrx STATIC ${OSRX_SOURCES})
target_include_directories(osrx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/osrx>
)

if(OSRX_ID_MAX)
    target_compile_definitions(osrx PUBLIC OSRX_ID_MAX=${OSRX_ID_MAX})
endif()
if(OSRX_UNIT_MAX)
    target_compile_definitions(osrx PUBLIC OSRX_UNIT_MAX=${OSRX_UNIT_MAX})
endif()
if(OSRX_BODY_MAX)
    target_compile_definitions(osrx PUBLIC OSRX_BODY_MAX=${OSRX_BODY_MAX})
endif()
if(OSRX_VALIDATE_CRC8 STREQUAL "0")
    target_compile_definitions(osrx PUBLIC OSRX_VALIDATE_CRC8=0)
endif()
if(OSRX_VALIDATE_CRC16 STREQUAL "0")
    target_compile_definitions(osrx PUBLIC OSRX_VALIDATE_CRC16=0)
endif()
if(OSRX_NO_PARSER)
    target_compile_definitions(osrx PUBLIC OSRX_NO_PARSER=1)
endif()
if(OSRX_NO_TIMESTAMP)
    target_compile_definitions(osrx PUBLIC OSRX_NO_TIMESTAMP=1)
endif()

# ---------------------------------------------------------------------------
# Examples
# ---------------------------------------------------------------------------
option(OSRX_BUILD_EXAMPLES "Build example programs" ON)
if(OSRX_BUILD_EXAMPLES)
    add_executable(basic_rx examples/basic_rx.c)
    target_link_libraries(basic_rx PRIVATE osrx)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(OSRX_BUILD_TESTS "Build unit tests" ON)
if(OSRX_BUILD_TESTS)
    enable_testing()
    add_executable(test_parse tests/test_parse.c)
    target_link_libraries(test_parse PRIVATE osrx)
    add_test(NAME test_parse COMMAND test_parse)
endif()

# ---------------------------------------------------------------------------
# Install + CMake package distribution
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 1. Install the library binary and export the CMake target.
install(TARGETS osrx
    EXPORT  osrxTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 2. Install the public headers.
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/osrx)

# 3. Generate and install the export file (osrxTargets.cmake) so
#    downstream projects can import the osrx::osrx target.
install(EXPORT osrxTargets
    FILE      osrxTargets.cmake
    NAMESPACE osrx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osrx
)

# 4. Generate the versioned package config file and install both
#    osrxConfig.cmake and osrxConfigVersion.cmake.
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/osrxConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/osrxConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osrx
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/osrxConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/osrxConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/osrxConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/osrx
)

