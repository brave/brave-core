/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/crypto/kdf_unittest.cc"

TEST(KDFTest, Pbkdf2HmacSha256KnownAnswers) {
  struct TestCase {
    std::string password;
    std::string salt;
    crypto::kdf::Pbkdf2HmacSha1Params params;
    size_t len;
    const char* result;
  };

  constexpr auto cases = std::to_array<TestCase>({
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

  for (const auto& c : cases) {
    std::vector<uint8_t> key(c.len);
    crypto::kdf::DeriveKeyPbkdf2HmacSha256(
        c.params, base::as_byte_span(c.password), base::as_byte_span(c.salt),
        key, crypto::SubtlePassKey::ForTesting());

    std::vector<uint8_t> result_bytes(c.len);
    ASSERT_TRUE(base::HexStringToSpan(c.result, result_bytes));
    EXPECT_EQ(key, result_bytes);
  }
}
