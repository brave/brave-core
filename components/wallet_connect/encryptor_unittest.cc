/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/wallet_connect/encryptor.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "base/strings/string_number_conversions.h"
#include "crypto/random.h"

namespace wallet_connect {

TEST(EncryptorUnitTest, EncryptAndDecrypt) {
  std::array<uint8_t, 32> key;
  crypto::RandBytes(key);

  Encryptor encryptor(key);

  for (const std::string& plaintext : {
           "{test: brave12}",     // 15 bytes, needs padding
           "{test: brave123}",    // 16 bytes, no padding needed
           "{test: brave12345}",  // 18 bytes, needs padding
       }) {
    auto encrypt_result = encryptor.Encrypt(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()));
    ASSERT_TRUE(encrypt_result.has_value()) << encrypt_result.error();

    auto payload = std::move(encrypt_result).value();

    auto decrypt_result = encryptor.Decrypt(payload);
    ASSERT_TRUE(decrypt_result.has_value()) << decrypt_result.error();
    const auto decrypted_value = decrypt_result.value();
    EXPECT_EQ(std::string(decrypted_value.begin(), decrypted_value.end()),
              plaintext);
  }
}

TEST(EncryptorUnitTest, DecryptSpecificData) {
  types::EncryptionPayload payload;
  payload.data =
      "170ac2b0c8ba61ac268455c42eb72c452e23888c6b357bcfc1b8c4c12770690c714e2171"
      "ceee0fa4aa639bcbfb9c6b111cbad0f73759c782253a3b4c0da1c43e",
  payload.hmac =
      "f779131fb8976435eb6984c23f597ffdf2f2a7122543d27907774c0f92142d33";
  payload.iv = "81413061def750d1a8f857d98d66584d";
  std::vector<uint8_t> key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(
      "2254c5145902fe280fb035e98bea896e024b78ccab33a62a38f538c860d60339",
      &key_bytes));
  std::array<uint8_t, 32> key;
  std::copy_n(key_bytes.begin(), 32, key.begin());
  Encryptor encryptor(key);
  auto decrypt_result = encryptor.Decrypt(payload);
  ASSERT_TRUE(decrypt_result.has_value()) << decrypt_result.error();
  const auto decrypted_value = decrypt_result.value();
  EXPECT_EQ(
      std::string(decrypted_value.begin(), decrypted_value.end()),
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"wc_test\",\"params\":[]}");
}

TEST(EncryptorUnitTest, DecryptBadData) {
  std::vector<uint8_t> key_bytes;
  ASSERT_TRUE(base::HexStringToBytes(
      "2254c5145902fe280fb035e98bea896e024b78ccab33a62a38f538c860d60339",
      &key_bytes));
  std::array<uint8_t, 32> key;
  std::copy_n(key_bytes.begin(), 32, key.begin());
  Encryptor encryptor(key);

  types::EncryptionPayload payload;
  auto decrypt_result = encryptor.Decrypt(payload);
  EXPECT_FALSE(decrypt_result.has_value());
  EXPECT_EQ(decrypt_result.error(),
            "Payload contains invalid hex string: {\n   \"data\": \"\",\n   "
            "\"hmac\": \"\",\n   \"iv\": \"\"\n}\n");

  payload.data =
      "170ac2b0c8ba61ac268455c42eb72c452e23888c6b357bcfc1b8c4c12770690c714e2171"
      "ceee0fa4aa639bcbfb9c6b111cbad0f73759c782253a3b4c0da1c43e",
  // truncated HMAC
      payload.hmac =
          "f779131fb8976435eb6984c23f597ffdf2f2a7122543d27907774c0f92";
  payload.iv = "81413061def750d1a8f857d98d66584d";
  decrypt_result = encryptor.Decrypt(payload);
  EXPECT_FALSE(decrypt_result.has_value());
  EXPECT_EQ(decrypt_result.error(), "HMAC mismatched");

  // wrong HMAC
  payload.hmac =
      "c077af99b4e3c5d79e8a4ddd6ca98ad3c77252249dd2f0adba84e4d7aae96966";
  decrypt_result = encryptor.Decrypt(payload);
  EXPECT_FALSE(decrypt_result.has_value());
  EXPECT_EQ(decrypt_result.error(), "HMAC mismatched");
}

}  // namespace wallet_connect
