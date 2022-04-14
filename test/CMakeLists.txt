find_package(hope REQUIRED)

set(ASSETS ${CMAKE_CURRENT_BINARY_DIR}/assets)
file(MAKE_DIRECTORY ${ASSETS})
set(URL "https://f002.backblazeb2.com/file/hmmer-reader")

file(DOWNLOAD ${URL}/three-profs.hmm ${ASSETS}/three-profs.hmm
     EXPECTED_HASH MD5=326b9aa04c6f6213e475d5174cc3746c)

file(DOWNLOAD ${URL}/empty.hmm ${ASSETS}/empty.hmm
     EXPECTED_HASH MD5=d41d8cd98f00b204e9800998ecf8427e)

file(DOWNLOAD ${URL}/corrupted1.hmm ${ASSETS}/corrupted1.hmm
     EXPECTED_HASH MD5=df1ad566e561f92385c2d60946035eb9)

file(DOWNLOAD ${URL}/corrupted2.hmm ${ASSETS}/corrupted2.hmm
     EXPECTED_HASH MD5=c4d5cbac2aa65a76ba3dcd9bf57b7ed2)

file(DOWNLOAD ${URL}/corrupted3.hmm ${ASSETS}/corrupted3.hmm
     EXPECTED_HASH MD5=0a2b5db84ef6a5d40493226a914cde15)

file(DOWNLOAD ${URL}/corrupted4.hmm ${ASSETS}/corrupted4.hmm
     EXPECTED_HASH MD5=79730bd582b6f3ce6e1ac10946c05768)

file(DOWNLOAD ${URL}/corrupted5.hmm ${ASSETS}/corrupted5.hmm
     EXPECTED_HASH MD5=214e9f50e207150a85079c0fa2facd56)

file(DOWNLOAD ${URL}/corrupted6.hmm ${ASSETS}/corrupted6.hmm
     EXPECTED_HASH MD5=333a6b78b54c856f0ac32845901fa617)

file(DOWNLOAD ${URL}/corrupted7.hmm ${ASSETS}/corrupted7.hmm
     EXPECTED_HASH MD5=94eb1d8922400b15e2c751b370011f2c)

file(DOWNLOAD ${URL}/corrupted8.hmm ${ASSETS}/corrupted8.hmm
     EXPECTED_HASH MD5=1a17560d9c4808e0d84f95dfa41e671b)

function(hmr_add_test name srcs)
  add_executable(${name} ${srcs})
  target_link_libraries(${name} PRIVATE HMR::hmr)
  target_link_libraries(${name} PRIVATE HOPE::hope)
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
