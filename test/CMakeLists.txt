add_custom_target(all_tests)
find_package(hope REQUIRED)

# include(FetchContent) FetchContent_Declare( pfam24 URL
# https://deciphon.s3.eu-west-2.amazonaws.com/pfam24.dcp.7z URL_HASH
# SHA1=1d52d936292c3b572efe73ef6830476aa416ced9)

# FetchContent_MakeAvailable(pfam24) FetchContent_GetProperties(pfam24
# SOURCE_DIR srcdir) set(PFAM24_FILEPATH ${srcdir}/pfam24.dcp)

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
endfunction()

hmr_add_test(read_hmm read_hmm.c)
