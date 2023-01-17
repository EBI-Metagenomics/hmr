cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(
  hmr
  VERSION 0.6.0
  LANGUAGES C)
set(PROJECT_DESCRIPTION "HMMER ASCII file reader.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)

include(cmake/sanitizers.cmake)
include(cmake/warnings.cmake)

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

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(hmr PUBLIC -DHMR_STATIC_DEFINE)
endif()

target_include_directories(
  hmr
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
         $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  PRIVATE ${PROJECT_SOURCE_DIR}/src)

set_target_properties(hmr PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(hmr PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_options(hmr PRIVATE ${WARNING_FLAGS})

install(
  TARGETS hmr
  EXPORT hmr-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
install(DIRECTORY include/hmr/ DESTINATION include/hmr)

export(
  TARGETS hmr
  NAMESPACE HMR::
  FILE hmr-targets.cmake)
install(
  EXPORT hmr-targets
  NAMESPACE HMR::
  DESTINATION lib/cmake/hmr)

install(FILES ${PROJECT_SOURCE_DIR}/README.md ${PROJECT_SOURCE_DIR}/LICENSE.md
        DESTINATION share/docs/hmr)

include(CMakePackageConfigHelpers)

set(project_config ${PROJECT_BINARY_DIR}/hmr-config.cmake)
set(version_config ${PROJECT_BINARY_DIR}/hmr-config-version.cmake)

configure_package_config_file(hmr-config.cmake.in ${project_config}
                              INSTALL_DESTINATION lib/cmake/hmr)

write_basic_package_version_file(${version_config}
                                 COMPATIBILITY SameMajorVersion)

install(FILES ${project_config} ${version_config}
        DESTINATION lib/cmake/hmr)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  enable_testing()
  add_subdirectory(test)
endif()
