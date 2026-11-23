// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#ifndef AWSLC_PROVIDER_TEST_FIXTURE_H
#define AWSLC_PROVIDER_TEST_FIXTURE_H

// The fixtures every provider test builds on. The provider is loaded from the
// build tree, so the suite runs without an install step.
//
// There are two, because a provider sees a different library context in each and
// only one of them is what a deployed consumer has:
//
//   ProviderTest       a private OSSL_LIB_CTX per test, so cells are isolated and
//                      a test can assert precisely which providers are present.
//   DefaultLibCtxTest  OpenSSL's default library context, which it names with a
//                      NULL pointer. Almost every real consumer is here.

#include <gtest/gtest.h>

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/provider.h>

#include <memory>
#include <string>
#include <vector>

namespace awslc_provider_test {

// The directory holding the freshly built provider, supplied by CMake.
#ifndef AWSLC_PROVIDER_MODULE_DIR
#error "AWSLC_PROVIDER_MODULE_DIR must be defined by the build"
#endif

// The name the provider is loaded and attributed under. Fetch queries and
// OSSL_PROVIDER_get0_name results are compared against this.
#define AWSLC_TEST_PROVIDER_NAME "awslc"

// A preference, not a requirement: anything the provider does not implement
// falls through to whichever other provider does. The whole safety story rests
// on the leading '?', so tests that assert fallthrough use exactly this string.
#define AWSLC_TEST_PREFER "?provider=awslc"

// A hard requirement, used only to prove attribution: a fetch that succeeds
// under this string cannot have been served by anyone else.
#define AWSLC_TEST_REQUIRE "provider=awslc"

static const char *const kProviderName = AWSLC_TEST_PROVIDER_NAME;
static const char *const kPreferAwslc = AWSLC_TEST_PREFER;
static const char *const kRequireAwslc = AWSLC_TEST_REQUIRE;

struct LibCtxDeleter {
  void operator()(OSSL_LIB_CTX *ctx) const { OSSL_LIB_CTX_free(ctx); }
};
struct ProviderDeleter {
  void operator()(OSSL_PROVIDER *p) const { OSSL_PROVIDER_unload(p); }
};
struct MdDeleter {
  void operator()(EVP_MD *md) const { EVP_MD_free(md); }
};
struct MdCtxDeleter {
  void operator()(EVP_MD_CTX *ctx) const { EVP_MD_CTX_free(ctx); }
};

using LibCtxPtr = std::unique_ptr<OSSL_LIB_CTX, LibCtxDeleter>;
using ProviderPtr = std::unique_ptr<OSSL_PROVIDER, ProviderDeleter>;
using MdPtr = std::unique_ptr<EVP_MD, MdDeleter>;
using MdCtxPtr = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>;

// One drained OpenSSL error record. In the shared fixture because any operation
// with a failure path owes the queue an explanation.
struct ErrorRecord {
  unsigned long code = 0;
  std::string file;
  int line = 0;
  std::string data;

  unsigned long library() const { return ERR_GET_LIB(code); }
  unsigned long reason() const { return ERR_GET_REASON(code); }
  const char *reason_text() const { return ERR_reason_error_string(code); }

  // OpenSSL returns NULL for an unnamed library. Rendered rather than returned raw
  // so that surfaces as a failed comparison instead of a crash in the test.
  std::string library_name() const {
    const char *name = ERR_lib_error_string(code);
    return name == nullptr ? "<unregistered>" : name;
  }
};

// The whole queue, oldest first.
inline std::vector<ErrorRecord> DrainErrors() {
  std::vector<ErrorRecord> records;

  for (;;) {
    const char *file = nullptr;
    const char *data = nullptr;
    int line = 0;
    int flags = 0;
    const unsigned long code =
        ERR_get_error_all(&file, &line, nullptr, &data, &flags);

    if (code == 0) {
      return records;
    }
    ErrorRecord record;
    record.code = code;
    record.file = file == nullptr ? "" : file;
    record.line = line;
    record.data = data == nullptr ? "" : data;
    records.push_back(record);
  }
}

// Only the records the provider filed. OpenSSL raises its own around a dispatch
// call, so position does not identify ours; the private error library does.
inline std::vector<ErrorRecord> ProviderErrors(
    const std::vector<ErrorRecord> &records) {
  std::vector<ErrorRecord> ours;

  for (const ErrorRecord &record : records) {
    if (record.library_name() == AWSLC_TEST_PROVIDER_NAME) {
      ours.push_back(record);
    }
  }
  return ours;
}

// Loads the provider into a private libctx alongside the default provider, which
// is the fallback leg that makes '?provider=awslc' safe.
class ProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    libctx_.reset(OSSL_LIB_CTX_new());
    ASSERT_TRUE(libctx_) << "could not create a library context";

    ASSERT_TRUE(OSSL_PROVIDER_set_default_search_path(
        libctx_.get(), AWSLC_PROVIDER_MODULE_DIR))
        << "could not point OpenSSL at " << AWSLC_PROVIDER_MODULE_DIR;

    awslc_.reset(OSSL_PROVIDER_load(libctx_.get(), kProviderName));
    ASSERT_TRUE(awslc_) << "could not load the provider from "
                        << AWSLC_PROVIDER_MODULE_DIR
                        << "; OSSL_provider_init may have failed";

    default_.reset(OSSL_PROVIDER_load(libctx_.get(), "default"));
    ASSERT_TRUE(default_) << "could not load the default provider";

    // The queue is per-thread and process-wide, so each test starts from empty.
    ERR_clear_error();
  }

  void TearDown() override {
    ERR_clear_error();
    default_.reset();
    awslc_.reset();
    libctx_.reset();
  }

  OSSL_LIB_CTX *libctx() { return libctx_.get(); }
  OSSL_PROVIDER *awslc() { return awslc_.get(); }

 private:
  LibCtxPtr libctx_;
  ProviderPtr awslc_;
  ProviderPtr default_;
};

// Loads the provider into OpenSSL's default library context. This is process
// global and logic within OpenSSL handles it slightly differently from
// explicitly created library contexts.
class DefaultLibCtxTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_TRUE(OSSL_PROVIDER_set_default_search_path(
        nullptr, AWSLC_PROVIDER_MODULE_DIR))
        << "could not point OpenSSL at " << AWSLC_PROVIDER_MODULE_DIR;
    awslc_.reset(OSSL_PROVIDER_try_load(nullptr, kProviderName, 1));
    ASSERT_TRUE(awslc_) << "could not load the provider from "
                        << AWSLC_PROVIDER_MODULE_DIR
                        << " into the default library context;"
                           " OSSL_provider_init may have failed";
  }

  void TearDown() override {
    awslc_.reset();
    EXPECT_TRUE(OSSL_PROVIDER_set_default_search_path(nullptr, nullptr));
  }

  OSSL_PROVIDER *awslc() { return awslc_.get(); }

 private:
  ProviderPtr awslc_;
};

}  // namespace awslc_provider_test

#endif  // AWSLC_PROVIDER_TEST_FIXTURE_H
