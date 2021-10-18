cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
project(
  hmmer_reader
  VERSION 0.0.9
  LANGUAGES C)

# Generate compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(external/cmake-boilerplate/cbp.cmake)

cbp_set_rpath()

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

cbp_hide_functions(hmr)
cbp_set_warning_flags()
cbp_ci_build_option()
cbp_generate_export_header(hmr EXPORT_FILE)

target_include_directories(
  hmr
  PUBLIC $<INSTALL_INTERFACE:include>
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

cbp_install(hmr MERGED)
install(FILES ${EXPORT_FILE} DESTINATION include/hmr)

enable_testing()
add_subdirectory(test)
