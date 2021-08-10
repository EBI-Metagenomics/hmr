add_custom_target(all_tests)
find_package(hope REQUIRED)

include(FetchContent)
FetchContent_Declare(
  three_profs
  URL https://hmmer-reader.s3.eu-west-2.amazonaws.com/three-profs.hmm.7z
  URL_HASH SHA1=099404b43b5c0416061d8e911af785d6fd2ee6f3)

FetchContent_MakeAvailable(three_profs)
FetchContent_GetProperties(three_profs SOURCE_DIR srcdir)
set(THREE_PROFS_FILEPATH ${srcdir}/three-profs.hmm)

function(hmr_add_test name srcs)
  add_executable(${name} ${srcs})
  add_dependencies(all_tests ${name})
  target_link_libraries(${name} PRIVATE HMR::hmr)
  target_link_libraries(${name} PUBLIC HOPE::hope)
  target_compile_options(${name} PRIVATE ${WARNING_FLAGS})
  target_compile_features(${name} PRIVATE c_std_11)
  add_test(NAME ${name} COMMAND ${name})

  set(TMPDIR "${CMAKE_CURRENT_BINARY_DIR}/${name}.tmp")
  add_custom_command(
    TARGET ${name}
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR})
  target_compile_definitions(${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
  target_compile_definitions(
    ${name} PUBLIC "THREE_PROFS_FILEPATH=\"${THREE_PROFS_FILEPATH}\"")
endfunction()

hmr_add_test(read_hmm read_hmm.c)
