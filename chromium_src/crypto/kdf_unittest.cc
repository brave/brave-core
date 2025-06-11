/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/crypto/kdf_unittest.cc"

#include <array>

#include "base/strings/string_number_conversions.h"

TEST(KDFTest, Pbkdf2HmacSha256KnownAnswers) {
  struct TestCase {
    std::string password;
    std::string salt;
    crypto::kdf::Pbkdf2HmacSha256Params params;
    size_t len;
    const char* result;
  };

  // https://github.com/openssl/openssl/blob/openssl-3.4.0/test/recipes/30-test_evp_data/evpkdf_pbkdf2.txt
  constexpr auto kCases = std::to_array<TestCase>({
      {"password",
       "salt",
       {1},
       32,
       "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b"},
      {"password",
       "salt",
       {2},
       32,
       "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43"},
      {"password",
       "salt",
       {4096},
       32,
       "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a"},
  });

  for (const auto& c : kCases) {
    std::vector<uint8_t> key(c.len);
    ASSERT_TRUE(crypto::kdf::DeriveKeyPbkdf2HmacSha256(
        c.params, base::as_byte_span(c.password), base::as_byte_span(c.salt),
        key));

    std::vector<uint8_t> result_bytes(c.len);
    ASSERT_TRUE(base::HexStringToSpan(c.result, result_bytes));
    EXPECT_EQ(key, result_bytes);
  }
}

TEST(KDFTest, Pbkdf2HmacSha512KnownAnswers) {
  struct TestCase {
    std::string password;
    std::string salt;
    crypto::kdf::Pbkdf2HmacSha512Params params;
    size_t len;
    const char* result;
  };

  // https://github.com/openssl/openssl/blob/openssl-3.4.0/test/recipes/30-test_evp_data/evpkdf_pbkdf2.txt
  constexpr auto kCases = std::to_array<TestCase>({
      {"password",
       "salt",
       {1},
       64,
       "867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5d513554e1c8cf252"
       "c02d470a285a0501bad999bfe943c08f050235d7d68b1da55e63f73b60a57fce"},
      {"password",
       "salt",
       {2},
       64,
       "e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f0040713f18aefdb866d53c"
       "f76cab2868a39b9f7840edce4fef5a82be67335c77a6068e04112754f27ccf4e"},
      {"password",
       "salt",
       {4096},
       64,
       "d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f87f6902e072f457b51"
       "43f30602641b3d55cd335988cb36b84376060ecd532e039b742a239434af2d5"},
  });

  for (const auto& c : kCases) {
    std::vector<uint8_t> key(c.len);
    ASSERT_TRUE(crypto::kdf::DeriveKeyPbkdf2HmacSha512(
        c.params, base::as_byte_span(c.password), base::as_byte_span(c.salt),
        key));

    std::vector<uint8_t> result_bytes(c.len);
    ASSERT_TRUE(base::HexStringToSpan(c.result, result_bytes));
    EXPECT_EQ(key, result_bytes);
  }
}

// Test that our implementation in DeriveKeyScryptNoCheck produces results that
// match the upstream's implementation in DeriveKeyScrypt.
TEST(KDFTest, ScryptNoCheckKnownAnswers) {
  struct TestCase {
    std::string password;
    std::string salt;
    crypto::kdf::ScryptParams params;
    size_t len;
    const char* result;
  };

  // RFC 7914 test vectors - note that RFC 7914 does not specify
  // max_memory_bytes so we just pass 0 here and let BoringSSL figure it out for
  // us.
  constexpr auto kCases = std::to_array<TestCase>({
      {"password",
       "NaCl",
       {.cost = 1024, .block_size = 8, .parallelization = 16},
       64,
       "fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b373162"
       "2eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640"},
  });

  for (const auto& c : kCases) {
    std::vector<uint8_t> key(c.len);
    EXPECT_TRUE(crypto::kdf::DeriveKeyScryptNoCheck(
        c.params, base::as_byte_span(c.password), base::as_byte_span(c.salt),
        key));

    std::vector<uint8_t> key_with_check(c.len);
    crypto::kdf::DeriveKeyScrypt(c.params, base::as_byte_span(c.password),
                                 base::as_byte_span(c.salt), key_with_check,
                                 crypto::SubtlePassKey::ForTesting());
    EXPECT_EQ(key, key_with_check);

    std::vector<uint8_t> result_bytes(c.len);
    ASSERT_TRUE(base::HexStringToSpan(c.result, result_bytes));
    EXPECT_EQ(key, result_bytes);
  }
}

// Test that our implementation in DeriveKeyScryptNoCheck doesn't cause a crash
// when invalid parameter is passed in.
TEST(KDFTest, InvalidScryptNoCheckParameters) {
  constexpr auto cases = std::to_array<crypto::kdf::ScryptParams>({
      // cost parameter is not a power of 2
      {.cost = 1023, .block_size = 8, .parallelization = 16},
      // TODO: others, after we document the exact constraints
  });

  for (const auto& c : cases) {
    std::vector<uint8_t> key(64);
    EXPECT_FALSE(crypto::kdf::DeriveKeyScryptNoCheck(
        c, base::as_byte_span("password"), base::as_byte_span("NaCl"), key));
  }
}
