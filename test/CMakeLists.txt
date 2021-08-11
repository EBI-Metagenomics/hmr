add_custom_target(all_tests)
find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(MAKE_DIRECTORY ${ASSETS})
set(URL "https://hmmer-reader.s3.eu-west-2.amazonaws.com")

file(DOWNLOAD ${URL}/three-profs.hmm.7z ${ASSETS}/three-profs.hmm.7z
     EXPECTED_HASH SHA1=099404b43b5c0416061d8e911af785d6fd2ee6f3)
file(ARCHIVE_EXTRACT INPUT ${ASSETS}/three-profs.hmm.7z DESTINATION ${ASSETS})

file(DOWNLOAD ${URL}/empty.hmm ${ASSETS}/empty.hmm
     EXPECTED_HASH SHA1=da39a3ee5e6b4b0d3255bfef95601890afd80709)

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
  target_compile_definitions(${name} PUBLIC "ASSETS=\"${ASSETS}\"")
  target_compile_definitions(${name} PUBLIC "TMPDIR=\"${TMPDIR}\"")
endfunction()

hmr_add_test(read_hmm read_hmm.c)
