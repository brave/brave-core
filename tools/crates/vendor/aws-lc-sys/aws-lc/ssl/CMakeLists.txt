add_library(
  ssl

  bio_ssl.cc
  crypto_policy.cc
  custom_extensions.cc
  d1_both.cc
  d1_lib.cc
  d1_pkt.cc
  d1_srtp.cc
  dtls_method.cc
  dtls_record.cc
  encrypted_client_hello.cc
  extensions.cc
  handoff.cc
  handshake.cc
  handshake_client.cc
  handshake_server.cc
  s3_both.cc
  s3_lib.cc
  s3_pkt.cc
  ssl_aead_ctx.cc
  ssl_asn1.cc
  ssl_buffer.cc
  ssl_cert.cc
  ssl_cipher.cc
  ssl_decrepit.c
  ssl_file.cc
  ssl_key_share.cc
  ssl_lib.cc
  ssl_privkey.cc
  ssl_session.cc
  ssl_stat.cc
  ssl_text.cc
  ssl_transcript.cc
  ssl_transfer_asn1.cc
  ssl_versions.cc
  ssl_x509.cc
  t1_enc.cc
  tls_method.cc
  tls_record.cc
  tls13_both.cc
  tls13_client.cc
  tls13_enc.cc
  tls13_server.cc
)
target_compile_definitions(ssl PRIVATE BORINGSSL_IMPLEMENTATION)

target_link_libraries(ssl crypto)

target_add_awslc_include_paths(TARGET ssl SCOPE PUBLIC)

set_target_properties(ssl PROPERTIES
    OUTPUT_NAME "${SSL_LIB_NAME}")

if(SET_LIB_SONAME)
  set_target_properties(ssl PROPERTIES
      VERSION "${SOFTWARE_VERSION}"
      SOVERSION "${ABI_VERSION}")
endif()

# Apply symbol versioning for shared libraries. The version script is a
# checked-in source file, so if versioning is enabled it must be present;
# apply_version_script() fatal-errors if it is missing rather than silently
# producing an unversioned library.
if(ENABLE_SYMBOL_VERSIONING)
  apply_version_script(
    TARGET ssl
    VERSION_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/libssl.map"
    NAMESPACE "${SYMBOL_VERSION_NAMESPACE}"
  )
endif()

if(BUILD_TESTING)
  # Tests that connect to a live, remote host. Kept as its own executable, and
  # deliberately out of util/all_tests.json, so the default test run stays
  # hermetic; environments without outbound access (e.g. internal sandbox
  # builds) simply do not run it. Built against libssl and libcrypto because
  # these tests use libssl to retrieve a certificate chain from a host.
  # Skip on Generic systems (e.g., WASI) which don't have BSD sockets.
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Generic")
    add_executable(
      ${INTEGRATION_TEST_EXEC}

      ../crypto/ocsp/ocsp_integration_test.cc
      ../tool-openssl/s_client_integration_test.cc

      # Link closure for SClientTool.
      ../tool-openssl/ordered_args.cc
      ../tool-openssl/s_client.cc
      ../tool/args.cc
      ../tool/client.cc
      ../tool/fd.cc
      ../tool/file.cc
      ../tool/transport_common.cc

      $<TARGET_OBJECTS:crypto_test_data>
    )
    target_link_libraries(${INTEGRATION_TEST_EXEC}
            boringssl_gtest_main ssl )
    target_add_awslc_include_paths(TARGET ${INTEGRATION_TEST_EXEC} SCOPE PRIVATE)
    add_dependencies(all_tests ${INTEGRATION_TEST_EXEC})
    set_test_location(${INTEGRATION_TEST_EXEC})
  endif()

  add_executable(
    ${SSL_TEST_EXEC}

    ../crypto/test/file_util.cc
    span_test.cc
    ssl_test.cc
    ssl_client_hello_test.cc
    ssl_c_test.c
    ssl_key_share_test.cc
    ssl_alps_test.cc
    ssl_common_test.cc
    ssl_misc_test.cc
    ssl_version_test.cc
    ssl_ech_test.cc
    ssl_quic_test.cc
    ssl_hybrid_handshake_test.cc
    ssl_encoding_test.cc
    ssl_ciphers_test.cc
    ssl_crypto_policy_test.cc
    ssl_handshake_test.cc
  )

  target_link_libraries(${SSL_TEST_EXEC} boringssl_gtest_main ssl)
  target_add_awslc_include_paths(TARGET ${SSL_TEST_EXEC} SCOPE PRIVATE)
  set_test_location(${SSL_TEST_EXEC})

  if(WIN32)
    target_link_libraries(${SSL_TEST_EXEC} ws2_32)
  endif()
  add_dependencies(all_tests ${SSL_TEST_EXEC})
  add_subdirectory(test)
endif()

if(NOT CMAKE_VERSION VERSION_LESS "3.12")
  install(TARGETS ssl
          EXPORT ssl-targets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            NAMELINK_COMPONENT Development
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  install(TARGETS ssl
          EXPORT ssl-targets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(MSVC AND CMAKE_BUILD_TYPE_LOWER MATCHES "relwithdebinfo" AND FIPS)
  install(FILES $<TARGET_FILE_DIR:ssl>/ssl.pdb DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

configure_file("cmake/ssl-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ssl-config.cmake"
    @ONLY)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ssl-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/ssl/cmake"
    COMPONENT Development)

if (BUILD_SHARED_LIBS)
   set (TARGET_DIR "shared")
else()
   set (TARGET_DIR "static")
endif()

install(EXPORT ssl-targets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/ssl/cmake/${TARGET_DIR}"
    NAMESPACE AWS::
    COMPONENT Development)
