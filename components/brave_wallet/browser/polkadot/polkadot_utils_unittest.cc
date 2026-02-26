/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr const char kDevPhrase[] =
    "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

}  // namespace

TEST(PolkadotUtils, DestinationAddressParsing) {
  // Account at:
  // https://assethub-westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
  // https://polkadot.subscan.io/account/14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3

  EXPECT_EQ(ParsePolkadotAccount(
                "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty", 42)
                .value()
                .ToString(),
            "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty");

  EXPECT_EQ(ParsePolkadotAccount(
                "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3", 0)
                .value()
                .ToString(),
            "14E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3");

  EXPECT_EQ(
      ParsePolkadotAccount(
          R"(0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48)",
          0)
          .value()
          .ToString(),
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48");

  // Address isn't 0x-prefixed
  EXPECT_FALSE(ParsePolkadotAccount(
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48", 0));

  // Invalid ss58 prefix.
  EXPECT_FALSE(ParsePolkadotAccount(
      "4FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "24E5nqKAp3oAJcmzgZhUD2RcptBeUBScxKHgJKU4HPNcKVf3", 0));

  // Address is too long.
  EXPECT_FALSE(ParsePolkadotAccount(
      "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty694ty", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234",
      0));
  EXPECT_FALSE(ParsePolkadotAccount(
      R"(0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a481234)",
      42));

  // Address is too short.
  EXPECT_FALSE(ParsePolkadotAccount(
      "5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694t", 42));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4", 0));
  EXPECT_FALSE(ParsePolkadotAccount(
      "0x8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a", 0));
  EXPECT_FALSE(ParsePolkadotAccount("", 0));

  // Random nonsense.
  EXPECT_FALSE(ParsePolkadotAccount("random string full of random words", 0));
}

TEST(PolkadotUtils, Uint128MojomConversions) {
  // Zeroes.
  EXPECT_EQ(Uint128ToMojom(0), mojom::uint128::New(0, 0));
  EXPECT_EQ(MojomToUint128(mojom::uint128::New(0, 0)), uint128_t{0});

  auto uint64_max = std::numeric_limits<uint64_t>::max();
  auto uint128_max = std::numeric_limits<uint128_t>::max();

  // Low bits set.
  EXPECT_EQ(Uint128ToMojom(uint64_max), mojom::uint128::New(0, uint64_max));
  EXPECT_EQ(MojomToUint128(mojom::uint128::New(0, uint64_max)),
            uint128_t{uint64_max});

  // High bits set.
  EXPECT_EQ(Uint128ToMojom(uint128_t{uint64_max} << 64),
            mojom::uint128::New(uint64_max, 0));
  EXPECT_EQ(MojomToUint128(mojom::uint128::New(uint64_max, 0)),
            uint128_t{uint64_max} << 64);

  // All bits set.
  EXPECT_EQ(Uint128ToMojom(uint128_max),
            mojom::uint128::New(uint64_max, uint64_max));
  EXPECT_EQ(MojomToUint128(mojom::uint128::New(uint64_max, uint64_max)),
            uint128_max);

  // Normal/sane value.
  EXPECT_EQ(Uint128ToMojom(1234), mojom::uint128::New(0, 1234));
  EXPECT_EQ(MojomToUint128(mojom::uint128::New(0, 1234)), uint128_t{1234});
}

// EncodePrivateKeyForExport / DecodePrivateKeyFromExport tests (Polkadot.js
// format).

TEST(PolkadotUtils, EncodePrivateKeyForExport) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);
  std::array<uint8_t, kScryptSaltSize> salt_bytes;
  salt_bytes.fill(1);
  std::array<uint8_t, kSecretboxNonceSize> nonce_bytes;
  nonce_bytes.fill(2);
  keyring.SetRandBytesForTesting(salt_bytes, nonce_bytes);

  // Test account 0
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(0);
    std::string address = keyring.GetAddress(0, kSubstratePrefix);
    auto private_key_0 = EncodePrivateKeyForExport(pkcs8, address, kPassword,
                                                   &salt_bytes, &nonce_bytes);
    std::optional<base::Value> json_value =
        base::JSONReader::Read(*private_key_0, 5);
    constexpr const char kExpectedJson[] =
        R"({
            "address":"5Fc3qszVcAXHAmjjm61KcxqvV1kh91jpydE476NjjnJneNdP",
            "encoded":"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAgAAAAQAAAAgAAAACAgICAgICAgICAgICAgICAgICAgICAgJwZYie6q9xdZXp0Tp0awNekjcrjm4Ge+Vh5Lwh9XlJ3sEQ6F4cJUGsR6Kx5IcNP7LBci3ArjzqlJ7/qOSQzS/rJ45+1kPakLVG2YZXQWW3LAzdc6CkDXzrzYnrUF3DyhY6sm59VLHwd6azVzFxqAMd+NJYVWxAxUlESkQlJafdg/4z3wmY",
            "encoding":{"content":["pkcs8","sr25519"],"type":["scrypt","xsalsa20-poly1305"],"version":"3"}})";
    std::optional<base::Value> expected_json_value =
        base::JSONReader::Read(kExpectedJson, 5);
    EXPECT_EQ(*json_value, *expected_json_value);
  }

  // Test account 1
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(1);
    std::string address = keyring.GetAddress(1, kSubstratePrefix);
    auto private_key_1 = EncodePrivateKeyForExport(pkcs8, address, kPassword,
                                                   &salt_bytes, &nonce_bytes);
    std::optional<base::Value> json_value_1 =
        base::JSONReader::Read(*private_key_1, 5);
    constexpr const char kExpectedJsonAccount1[] =
        R"({
            "address":"5FUag6Xjkr2TMgejpdsvQo3c1FSrZqEeZoHh173StGbME4XF",
            "encoded":"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAgAAAAQAAAAgAAAACAgICAgICAgICAgICAgICAgICAgICAgI35vhV4Hb3Cr04X+unU7qckjcrjm4Ge+Vh5Lwh9XlJ3tSn4/Z3n+oAn5X+MByi6HOHjolX4w2S+vHn7qX2ExTqqMqygndQ5qbu68HgGm326rq2dZ8nZ9n6VnS1dX88/MQ6sm59VLuGiKa4uQ4KX1JrUFoTxhgvl83I4WrM1y+1/nbb1yyn",
            "encoding":{"content":["pkcs8","sr25519"],"type":["scrypt","xsalsa20-poly1305"],"version":"3"}})";
    std::optional<base::Value> expected_json_value_1 =
        base::JSONReader::Read(kExpectedJsonAccount1, 5);
    EXPECT_EQ(*json_value_1, *expected_json_value_1);
  }

  // Empty password (should fail)
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(0);
    std::string address = keyring.GetAddress(0, kSubstratePrefix);
    EXPECT_FALSE(
        EncodePrivateKeyForExport(pkcs8, address, "", nullptr, nullptr));
  }
}

TEST(PolkadotUtils, EncodePrivateKeyForExport_Testnet) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotTestnet);
  std::array<uint8_t, kScryptSaltSize> salt_bytes;
  salt_bytes.fill(1);
  std::array<uint8_t, kSecretboxNonceSize> nonce_bytes;
  nonce_bytes.fill(2);
  keyring.SetRandBytesForTesting(salt_bytes, nonce_bytes);

  // Test account 0
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(0);
    std::string address = keyring.GetAddress(0, kSubstratePrefix);
    auto private_key_0 = EncodePrivateKeyForExport(pkcs8, address, kPassword,
                                                   &salt_bytes, &nonce_bytes);
    std::optional<base::Value> json_value =
        base::JSONReader::Read(*private_key_0, 5);
    constexpr const char kExpectedJson[] =
        R"({
            "address":"5HGiBcFgEBMgT6GEuo9SA98sBnGgwHtPKDXiUukT6aqCrKEx",
            "encoded":"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAgAAAAQAAAAgAAAACAgICAgICAgICAgICAgICAgICAgICAgK8fiV6lGVSFewy6uxsU1C4kjcrjm4Ge+Vh5Lwh9XlJ3vAbQs+/0vSBKGe71ik2n/owjXKi9/fBCOIbIDIttGbopozlHDAyPEOrftWc4aiMZfTHRTh/Hb0kJ4C79LBJ84Y6sm59VMs51zUalbzBwa9c75OqlTCtqRouH8891IU51jczQkHY",
            "encoding":{"content":["pkcs8","sr25519"],"type":["scrypt","xsalsa20-poly1305"],"version":"3"}})";
    std::optional<base::Value> expected_json_value =
        base::JSONReader::Read(kExpectedJson, 5);
    EXPECT_EQ(*json_value, *expected_json_value);
  }

  // Test account 1
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(1);
    std::string address = keyring.GetAddress(1, kSubstratePrefix);
    auto private_key_1 = EncodePrivateKeyForExport(pkcs8, address, kPassword,
                                                   &salt_bytes, &nonce_bytes);
    std::optional<base::Value> json_value_1 =
        base::JSONReader::Read(*private_key_1, 5);
    constexpr const char kExpectedJsonAccount1[] =
        R"({
            "address":"5CofVLAGjwvdGXvBiP6ddtZYMVbhT5Xke8ZrshUpj2ZXAnND",
            "encoded":"AQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAgAAAAQAAAAgAAAACAgICAgICAgICAgICAgICAgICAgICAgLCddKZgcxBjl0hYwjTBbfXkjcrjm4Ge+Vh5Lwh9XlJ3lxHMOsL8JTT373MVhPUPjpg0fdTnx8C0Rn6NlqE25XqYVmzHtu08FNDkPHRB7gGS7QEMooZrcX7+67a+1Uv3HE6sm59VA2vdfwY70yn/WROki1+SZ1OLWclpgVjEDift12grx7X",
            "encoding":{"content":["pkcs8","sr25519"],"type":["scrypt","xsalsa20-poly1305"],"version":"3"}})";
    std::optional<base::Value> expected_json_value_1 =
        base::JSONReader::Read(kExpectedJsonAccount1, 5);
    EXPECT_EQ(*json_value_1, *expected_json_value_1);
  }

  // Empty password (should fail)
  {
    auto pkcs8 = keyring.GetPkcs8KeyForTesting(0);
    std::string address = keyring.GetAddress(0, kSubstratePrefix);
    EXPECT_FALSE(
        EncodePrivateKeyForExport(pkcs8, address, "", nullptr, nullptr));
  }
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_Roundtrip) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  // Account 0: encode then decode
  {
    auto encoded_json = keyring.EncodePrivateKeyForExport(0, kPassword);
    ASSERT_TRUE(encoded_json.has_value());

    auto original_pkcs8_key = keyring.GetPkcs8KeyForTesting(0);
    auto decoded_pkcs8_key =
        DecodePrivateKeyFromExport(*encoded_json, kPassword);
    ASSERT_TRUE(decoded_pkcs8_key.has_value());
    EXPECT_EQ(*decoded_pkcs8_key, original_pkcs8_key);

    auto decoded_keypair =
        HDKeySr25519::CreateFromPkcs8(base::span(*decoded_pkcs8_key));
    ASSERT_TRUE(decoded_keypair.has_value());
    EXPECT_EQ(decoded_keypair->GetPublicKey(), keyring.GetPublicKey(0));
  }

  // Account 1: encode then decode
  {
    auto encoded_json = keyring.EncodePrivateKeyForExport(1, kPassword);
    ASSERT_TRUE(encoded_json.has_value());

    auto original_pkcs8_key = keyring.GetPkcs8KeyForTesting(1);
    auto decoded_pkcs8_key =
        DecodePrivateKeyFromExport(*encoded_json, kPassword);
    ASSERT_TRUE(decoded_pkcs8_key.has_value());
    EXPECT_EQ(*decoded_pkcs8_key, original_pkcs8_key);
  }
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_WrongPassword) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";
  const std::string kWrongPassword = "wrong_password";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  auto encoded_json = keyring.EncodePrivateKeyForExport(0, kPassword);
  ASSERT_TRUE(encoded_json.has_value());

  EXPECT_FALSE(
      DecodePrivateKeyFromExport(*encoded_json, kWrongPassword).has_value());
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_EmptyPassword) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  auto encoded_json = keyring.EncodePrivateKeyForExport(0, kPassword);
  ASSERT_TRUE(encoded_json.has_value());

  EXPECT_FALSE(DecodePrivateKeyFromExport(*encoded_json, "").has_value());
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_InvalidJSON) {
  const std::string kPassword = "test_password_123";

  EXPECT_FALSE(
      DecodePrivateKeyFromExport("{ invalid json }", kPassword).has_value());
  EXPECT_FALSE(DecodePrivateKeyFromExport("", kPassword).has_value());
  EXPECT_FALSE(DecodePrivateKeyFromExport(R"({"address":"test"})", kPassword)
                   .has_value());
  EXPECT_FALSE(
      DecodePrivateKeyFromExport(R"({"encoded":123})", kPassword).has_value());
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_Testnet) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotTestnet);

  auto encoded_json = keyring.EncodePrivateKeyForExport(0, kPassword);
  ASSERT_TRUE(encoded_json.has_value());

  auto original_pkcs8_key = keyring.GetPkcs8KeyForTesting(0);
  auto decoded_pkcs8_key = DecodePrivateKeyFromExport(*encoded_json, kPassword);
  ASSERT_TRUE(decoded_pkcs8_key.has_value());
  EXPECT_EQ(*decoded_pkcs8_key, original_pkcs8_key);
}

TEST(PolkadotUtils, DecodePrivateKeyFromExport_MissingParts) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  const std::string kPassword = "test_password_123";

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotTestnet);

  auto valid_json = keyring.EncodePrivateKeyForExport(0, kPassword);
  ASSERT_TRUE(valid_json.has_value());

  auto valid_dict = base::JSONReader::ReadDict(*valid_json, 0);
  ASSERT_TRUE(valid_dict.has_value());

  auto dict_to_json = [](const base::DictValue& dict) -> std::string {
    std::string json_string;
    base::JSONWriter::Write(dict, &json_string);
    return json_string;
  };

  // Missing pkcs8 in content.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    auto* content_list = encoding_dict->FindList("content");
    ASSERT_TRUE(content_list);
    content_list->clear();
    content_list->Append("sr25519");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing sr25519 in content.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    auto* content_list = encoding_dict->FindList("content");
    ASSERT_TRUE(content_list);
    content_list->clear();
    content_list->Append("pkcs8");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing scrypt in type.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    auto* type_list = encoding_dict->FindList("type");
    ASSERT_TRUE(type_list);
    type_list->clear();
    type_list->Append("xsalsa20-poly1305");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing xsalsa20-poly1305 in type.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    auto* type_list = encoding_dict->FindList("type");
    ASSERT_TRUE(type_list);
    type_list->clear();
    type_list->Append("scrypt");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Version mismatch.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    encoding_dict->Set("version", "2");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing content.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    encoding_dict->Remove("content");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing encoding.
  {
    auto test_dict = valid_dict->Clone();
    test_dict.Remove("encoding");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing type.
  {
    auto test_dict = valid_dict->Clone();
    auto* encoding_dict = test_dict.FindDict("encoding");
    ASSERT_TRUE(encoding_dict);
    encoding_dict->Remove("type");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Missing encoded.
  {
    auto test_dict = valid_dict->Clone();
    test_dict.Remove("encoded");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Corrupted encoded - invalid base64.
  {
    auto test_dict = valid_dict->Clone();
    test_dict.Set(
        "encoded",
        "00EBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEBAQEAgAAAAQAAAAgAAAACAgICAgICAg"
        "ICAgICAgICAgICAgICAgLCddKZgcxBjl0hYwjTBbfXkjcrjm4Ge+"
        "Vh5Lwh9XlJ3lxHMOsL8JTT373MVhPUPjpg0fdTnx8C0Rn6NlqE25XqYVmzHtu08FNDkPHR"
        "B7gGS7QEMooZrcX7+67a+1Uv3HE6sm59VA2vdfwY70yn/"
        "WROki1+SZ1OLWclpgVjEDift12grx7X");
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }

  // Corrupted encoded - wrong length.
  {
    auto test_dict = valid_dict->Clone();
    const std::string* original_encoded = test_dict.FindString("encoded");
    ASSERT_TRUE(original_encoded);
    std::string shortened_encoded =
        original_encoded->substr(0, original_encoded->size() - 10);
    test_dict.Set("encoded", shortened_encoded);
    EXPECT_FALSE(
        DecodePrivateKeyFromExport(dict_to_json(test_dict), kPassword));
  }
}

}  // namespace brave_wallet
