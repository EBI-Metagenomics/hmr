cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(
  hmmer_reader
  VERSION 0.0.1
  LANGUAGES C)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# Set RPATH only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
     "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

add_library(hmr src/fsm.c src/token.c src/hmr.c src/prof.c src/node.c)
add_library(HMR::hmr ALIAS hmr)

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
  ${CMAKE_CURRENT_BINARY_DIR}/hmr/export.h)

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  # /wd5105:
  # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/c5105?view=msvc-160
  set(WARNING_FLAGS /W3 /wd5105)
else()
  set(WARNING_FLAGS
      -Wall
      -Wextra
      -Wstrict-prototypes
      -Wshadow
      -Wconversion
      -Wmissing-prototypes
      -Wno-unused-parameter
      -Wsign-conversion
      -Wno-unused-function)
endif()

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
  list(APPEND WARNING_FLAGS -Wno-gnu-designator -Wno-empty-translation-unit
       -Wno-gnu-statement-expression -Wno-nullability-extension)
endif()

option(CI_BUILD "CI, extra flags will be set" OFF)
if(CI_BUILD)
  message(STATUS "CI build enabled")
  add_compile_options(-Werror)
endif()

target_include_directories(
  hmr
  PUBLIC $<INSTALL_INTERFACE:src>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(hmr PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(hmr PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
target_compile_features(hmr PRIVATE c_std_11)
target_compile_definitions(
  hmr PRIVATE $<$<BOOL:${WIN32}>:_CRT_NONSTDC_NO_DEPRECATE>)
target_compile_definitions(hmr PRIVATE $<$<BOOL:${WIN32}>:__extension__>)
target_compile_options(hmr PRIVATE ${WARNING_FLAGS})
target_compile_options(hmr PRIVATE "-fverbose-asm")
target_link_libraries(hmr INTERFACE $<$<BOOL:${UNIX}>:m>)

include(CheckCSourceCompiles)
check_c_source_compiles(
  "
        static void f(char const* err, ...) __attribute__((format(printf, 1, 2)));
        int main(void) {return 0;}
    "
  HAVE_ATTR_FMT)
target_compile_definitions(hmr
                           PRIVATE $<$<BOOL:${HAVE_ATTR_FMT}>:HAVE_ATTR_FMT>)

install(
  TARGETS hmr
  EXPORT hmr-targets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include)
# install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY src/hmr/ DESTINATION hmr)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hmr/export.h
              ${CMAKE_CURRENT_BINARY_DIR}/hmr/config.h DESTINATION hmr)
install(
  EXPORT hmr-targets
  FILE hmr-targets.cmake
  NAMESPACE HMR::
  DESTINATION lib/cmake/hmr)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  hmr-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/hmr-config.cmake
  INSTALL_DESTINATION lib/cmake/hmr
  NO_SET_AND_CHECK_MACRO)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/hmr-config.cmake
        DESTINATION lib/cmake/hmr)

enable_testing()
add_subdirectory(test)
