/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/crypto/symmetric_key_unittest.cc"

class SymmetricKeyDeriveKeyFromPasswordUsingPbkdf2Sha256Test
    : public testing::TestWithParam<PBKDF2TestVector> {};

TEST_P(SymmetricKeyDeriveKeyFromPasswordUsingPbkdf2Sha256Test,
       DeriveKeyFromPasswordUsingPbkdf2Sha256) {
  PBKDF2TestVector test_data(GetParam());
  std::unique_ptr<crypto::SymmetricKey> key(
      crypto::SymmetricKey::DeriveKeyFromPasswordUsingPbkdf2Sha256(
          test_data.algorithm, test_data.password, test_data.salt,
          test_data.rounds, test_data.key_size_in_bits));
  ASSERT_TRUE(key);

  const std::string& raw_key = key->key();
  EXPECT_EQ(test_data.key_size_in_bits / 8, raw_key.size());
  EXPECT_EQ(test_data.expected, base::ToLowerASCII(base::HexEncode(
                                    raw_key.data(), raw_key.size())));
}

static const PBKDF2TestVector kTestVectorsPbkdf2Sha256[] = {
    {
        crypto::SymmetricKey::AES,
        "password",
        "salt",
        1,
        256,
        "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b",
    },
    {
        crypto::SymmetricKey::AES,
        "password",
        "salt",
        2,
        256,
        "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43",
    },
    {
        crypto::SymmetricKey::AES,
        "password",
        "salt",
        4096,
        256,
        "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a",
    },
};

INSTANTIATE_TEST_SUITE_P(All,
                         SymmetricKeyDeriveKeyFromPasswordUsingPbkdf2Sha256Test,
                         testing::ValuesIn(kTestVectorsPbkdf2Sha256));
