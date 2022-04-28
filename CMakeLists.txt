cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)
project(
  hmmer_reader
  VERSION 0.1.2
  LANGUAGES C)

include(cmake/compiler-options.cmake)
include(cmake/sanitizers.cmake)

add_library(
  hmr
  src/aux.c
  src/error.c
  src/fsm.c
  src/hmr.c
  src/node.c
  src/prof.c
  src/to.c
  src/tok.c)
add_library(HMR::hmr ALIAS hmr)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

target_include_directories(
  hmr
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

set_target_properties(hmr PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(hmr PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_definitions(
  hmr PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(hmr PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_options(hmr PRIVATE ${WARNING_FLAGS})
target_compile_options(hmr PRIVATE "-fverbose-asm")
target_link_libraries(hmr INTERFACE $<$<BOOL:${UNIX}>:m>)

set(EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/hmr/export.h)
include(GenerateExportHeader)
generate_export_header(
  hmr
  BASE_NAME
  HMR
  INCLUDE_GUARD_NAME
  HMR_EXPORT_H
  EXPORT_MACRO_NAME
  HMR_API
  EXPORT_FILE_NAME
  ${EXPORT_FILE})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported()
  set_target_properties(hmr PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS hmr EXPORT hmr-targets)

install(DIRECTORY include/hmr/ DESTINATION include/hmr)

install(
  EXPORT hmr-targets
  FILE hmr-targets.cmake
  NAMESPACE HMR::
  DESTINATION lib/cmake/hmr)

include(CMakePackageConfigHelpers)

set(CMAKE_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/hmr-config.cmake)
configure_package_config_file(hmr-config.cmake.in ${CMAKE_CONFIG_FILE}
                              INSTALL_DESTINATION lib/cmake/hmr)

set(CMAKE_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/hmr-config-version.cmake)
write_basic_package_version_file(${CMAKE_VERSION_FILE}
                                 COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_VERSION_FILE} ${CMAKE_CONFIG_FILE}
        DESTINATION lib/cmake/hmr)
install(FILES ${EXPORT_FILE} DESTINATION include/hmr)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md
              ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md
        DESTINATION share/docs/${PROJECT_NAME})

enable_testing()
add_subdirectory(test)

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Danilo Horta")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HMMER3 ASCII file reader")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_CONTACT "Danilo Horta")
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/package)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_RELOCATABLE YES)
set(CPACK_MONOLITHIC_INSTALL YES)
include(CPack)
