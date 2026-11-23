# The AWS-LC OpenSSL provider. Off by default: it needs OpenSSL's provider
# headers, which AWS-LC's own tree does not carry.

if(NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR
    "BUILD_AWSLC_PROVIDER requires BUILD_SHARED_LIBS.")
endif()

if(OPENSSL_ROOT_DIR)
  find_path(OPENSSL_INCLUDE_DIR NAMES openssl/ssl.h
    PATHS "${OPENSSL_ROOT_DIR}" PATH_SUFFIXES include NO_DEFAULT_PATH)
  find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto
    PATHS "${OPENSSL_ROOT_DIR}" PATH_SUFFIXES lib lib64 lib/${CMAKE_LIBRARY_ARCHITECTURE}
    NO_DEFAULT_PATH)
  if(NOT OPENSSL_INCLUDE_DIR OR NOT OPENSSL_CRYPTO_LIBRARY)
    message(FATAL_ERROR
      "OPENSSL_ROOT_DIR (${OPENSSL_ROOT_DIR}) must be the install prefix of an "
      "OpenSSL 3.5+ build. Found include: ${OPENSSL_INCLUDE_DIR}, "
      "libcrypto: ${OPENSSL_CRYPTO_LIBRARY}")
  endif()
endif()
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

if(NOT OPENSSL_VERSION)
  message(WARNING
    "Could not determine the OpenSSL version at ${OPENSSL_INCLUDE_DIR}; "
    "the provider needs 3.5 or later.")
elseif(OPENSSL_VERSION VERSION_LESS "3.5")
  message(FATAL_ERROR
    "The provider requires OpenSSL 3.5 or later; found ${OPENSSL_VERSION} at "
    "${OPENSSL_INCLUDE_DIR}.")
endif()

message(STATUS "aws-lc-provider: building against OpenSSL ${OPENSSL_VERSION} "
  "(headers: ${OPENSSL_INCLUDE_DIR})")

# The two object libraries below exist because OpenSSL's and AWS-LC's headers
# cannot share a translation unit. Their include paths are disjoint.

# Front side: OpenSSL's headers only.
add_library(awslc_provider_front OBJECT
  frontend/provider.c
  frontend/registry.c
  frontend/errors.c
  frontend/operations/digests/digests.c
  frontend/operations/digests/sha2.c)
set_target_properties(awslc_provider_front PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Replace the directory-level AWS-LC include paths inherited from the parent.
set_property(TARGET awslc_provider_front PROPERTY INCLUDE_DIRECTORIES
  "${OPENSSL_INCLUDE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}")

# Back side: AWS-LC's headers only.
add_library(awslc_provider_back OBJECT
  backend/mem.c
  backend/info.c
  backend/errors.c
  backend/operations/digests/sha2.c)
set_target_properties(awslc_provider_back PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_add_awslc_include_paths(TARGET awslc_provider_back SCOPE PRIVATE)
target_include_directories(awslc_provider_back PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# The loadable module. OpenSSL resolves a provider by bare name against
# MODULESDIR, so the artifact must be named awslc with the platform's module
# suffix and no "lib" prefix.
add_library(awslc_provider MODULE
  $<TARGET_OBJECTS:awslc_provider_front>
  $<TARGET_OBJECTS:awslc_provider_back>)
set_target_properties(awslc_provider PROPERTIES
  OUTPUT_NAME "awslc"
  PREFIX "")

# This explicitly links the AWS-LC crypto target from within this CMake project
# rather than a system OpenSSL resolution.
# The OSSL_PARAM_* and other core OpenSSL symbols the front side calls are left
# undefined here and resolve at load time against whichever libcrypto dlopen'ed
# the provider
target_link_libraries(awslc_provider PRIVATE crypto)

if(APPLE)
  # OpenSSL's module loader appends .dylib on macOS, while CMake normally gives
  # a MODULE library the .so suffix.
  set_target_properties(awslc_provider PROPERTIES SUFFIX ".dylib")
  # ELF permits those undefined symbols by default; Mach-O rejects them unless
  # told otherwise.
  target_link_options(awslc_provider PRIVATE "-Wl,-undefined,dynamic_lookup")
endif()

if(UNIX AND NOT APPLE)
  # The front side must not import names AWS-LC exports. Backend objects define
  # the complete set of AWS-LC imports expected in the final ELF.
  add_custom_target(awslc_provider_linkage_test
    COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/test/verify_linkage.sh"
      "$<TARGET_FILE:awslc_provider>"
      "${CMAKE_SOURCE_DIR}/crypto/libcrypto.txt"
      "$<TARGET_OBJECTS:awslc_provider_back>"
      "$<TARGET_OBJECTS:awslc_provider_front>"
    DEPENDS awslc_provider
    VERBATIM)
endif()

# A provider needs to expose only OSSL_provider_init. Anything else it exposes is
# surface a consumer could bind to by accident and that we would then owe
# compatibility on. The entry point carries an explicit visibility attribute so
# hiding everything else does not hide it too.
set_target_properties(awslc_provider PROPERTIES C_VISIBILITY_PRESET hidden)

set(AWSLC_PROVIDER_MODULESDIR "${CMAKE_INSTALL_LIBDIR}/ossl-modules" CACHE STRING
  "Install directory of the provider module.")
install(TARGETS awslc_provider LIBRARY DESTINATION "${AWSLC_PROVIDER_MODULESDIR}")

if(BUILD_TESTING AND NOT ANDROID)
  # Two test binaries, for the same reason there are two object libraries: one
  # links OpenSSL's libcrypto and one links AWS-LC's, and a single binary linking
  # both would be the collision this design exists to avoid.
  #
  # awslc_provider_test drives the public EVP API against OpenSSL's libcrypto and
  # reaches AWS-LC only through the provider the loader brings in. That is the
  # arrangement a real consumer has.
  add_executable(awslc_provider_test
    test/test_main.cc
    test/frontend/provider_test.cc
    test/frontend/reachability_test.cc
    test/frontend/operations/digests/digests_test.cc
    test/frontend/operations/digests/sha2_test.cc)

  # Replace the directory-level AWS-LC include paths inherited from the parent.
  set_property(TARGET awslc_provider_test PROPERTY INCLUDE_DIRECTORIES
    "${OPENSSL_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}")

  # The suite drives the public OpenSSL 3.x EVP API with the provider loaded, so
  # it is the one component that links OpenSSL's libcrypto rather than AWS-LC's.
  # It reaches the provider only through dlopen, never by linking it.
  target_link_libraries(awslc_provider_test
    boringssl_gtest
    OpenSSL::Crypto)

  # The test binary needs the provider built, but must not link it.
  add_dependencies(awslc_provider_test awslc_provider)

  # Where the freshly built provider is, so the fixture can point OpenSSL at it
  # without depending on an install step.
  target_compile_definitions(awslc_provider_test PRIVATE
    "AWSLC_PROVIDER_MODULE_DIR=\"$<TARGET_FILE_DIR:awslc_provider>\""
    "AWSLC_PROVIDER_CONFIG_FILE=\"${CMAKE_CURRENT_SOURCE_DIR}/test/provider.cnf\"")
  if(FIPS)
    target_compile_definitions(awslc_provider_test PRIVATE AWSLC_FIPS)
  endif()

  # Both test binaries run straight out of the build tree, so they carry the
  # paths to the OpenSSL they link and to the build's own shared libraries.
  # Without this the suite needs LD_LIBRARY_PATH set by hand, and a missing
  # rpath shows up as a provider that will not load, which reads like a code
  # fault rather than a build one.
  set_target_properties(awslc_provider_test PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}")

  # awslc_provider_backend_test links AWS-LC and no OpenSSL. It covers the
  # guarantees the back side adds on top of AWS-LC that the EVP API above cannot
  # reach, but that a consumer calling a dispatch slot directly can.
  #
  # It links the backend objects and crypto rather than the module, so it proves
  # the source and leaves proving the shipped artifact to
  # awslc_provider_linkage_test.
  add_executable(awslc_provider_backend_test
    test/test_main.cc
    test/backend/errors_test.cc
    test/backend/operations/digests/sha2_test.cc
    $<TARGET_OBJECTS:awslc_provider_back>)

  target_include_directories(awslc_provider_backend_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(awslc_provider_backend_test boringssl_gtest crypto)

  set_target_properties(awslc_provider_backend_test PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}")

  # Attached to all_tests so `ninja all_tests` builds them. They are deliberately
  # not listed in util/all_tests.json: that file is static and its runner does not
  # skip absent binaries, so an entry there would break `ninja run_tests` for every
  # build that does not set BUILD_AWSLC_PROVIDER, which is the default. Their
  # runner is tests/ci/run_aws_lc_provider_tests.sh, which builds this
  # configuration and invokes both binaries directly.
  add_dependencies(all_tests awslc_provider_test awslc_provider_backend_test)
endif()
