/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include <string_view>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

// All derived from the polkadot-sdk using:
// let pair =
//   polkadot_sdk::sp_core::sr25519::Pair::from_string(
//     DEV_PHRASE, None).unwrap();

constexpr const char kDevPhrase[] =
    "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

constexpr char const kDevSeed[] =
    "FAC7959DBFE72F052E5A0C3C8D6530F202B02FD8F9F5CA3580EC8DEB7797479E0B9F67282F"
    "42D1214E457243B9EB38A0A5ED13DE66C01CFB213A6FE73CEDEF5A";

constexpr char const* kDevPubKey =
    "46EBDDEF8CD9BB167DC30878D7113B7E168E6F0646BEFFD77D69D39BAD76B47A";

// From:
// let s = pair.public().to_string();
// assert_eq!(s, "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
//
// See also:
// https://github.com/paritytech/polkadot-sdk/blob/0d765ce37b258640a6eeb575f6bff76d6a7b7c46/substrate/primitives/core/src/crypto.rs#L49
constexpr const char kDevAddress[] =
    "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV";

std::array<uint8_t, kSr25519SignatureSize> ToSignature(std::string_view hex) {
  std::array<uint8_t, kSr25519SignatureSize> signature_bytes;
  base::HexStringToSpan(hex, signature_bytes);
  return signature_bytes;
}

}  // namespace

TEST(PolkadotKeyring, GenerateRoot) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  EXPECT_EQ(base::HexEncode(seed), kDevSeed);

  // Verify that our seed can be used to derive a keypair with a public key that
  // matches the test vector.
  auto keypair = HDKeySr25519::GenerateFromSeed(
      base::span(seed).first<kSr25519SeedSize>());
  auto pubkey = keypair.GetPublicKey();

  // https://wiki.polkadot.com/learn/learn-account-advanced/#for-the-curious-how-prefixes-work
  const uint16_t substrate_prefix = 42;

  Ss58Address addr;
  addr.prefix = substrate_prefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(base::span<uint8_t const>(pubkey));

  EXPECT_EQ(base::HexEncode(pubkey), kDevPubKey);

  auto dev_addr = addr.Encode().value();
  EXPECT_EQ(dev_addr, kDevAddress);
}

TEST(PolkadotKeyring, Constructor) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();

  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);
  EXPECT_FALSE(keyring.IsTestnet());

  PolkadotKeyring keyring2(base::span(seed).first<kPolkadotSeedSize>(),
                           mojom::KeyringId::kPolkadotTestnet);
  EXPECT_TRUE(keyring2.IsTestnet());
}

TEST(PolkadotKeyring, GetAddress) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // polkadot_sdk::sp_core::crypto::set_default_ss58_version(
  //     Ss58AddressFormat::from(
  //         sp_core::crypto::Ss58AddressFormatRegistry::PolkadotAccount
  // ));
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}//polkadot//0"),
  //     None,
  // )
  // .unwrap();
  //
  // let s = pair.public().to_string();
  // assert_eq!(s, "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb");
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet);

    EXPECT_EQ(keyring.GetAddress(0, 0u),
              "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb");
    EXPECT_EQ(keyring.GetAddress(1, 0u),
              "14QspRnocdHvoDfFnGvvYwskrsSWG8nneJ2BAQ2oSMcsQUxB");
    // This test exercises the caching codepath in the GetAddress
    // implementation.
    EXPECT_EQ(keyring.GetAddress(0, 0u),
              "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb");
  }

  // Testnet.
  // Generated using subkey:
  // subkey inspect --scheme sr25519 --network substrate "bottom drive obey lake
  // curtain smoke basket hold race lonely fit walk//westend//*".
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet);

    EXPECT_EQ(keyring.GetAddress(0, 42u),
              "5HGiBcFgEBMgT6GEuo9SA98sBnGgwHtPKDXiUukT6aqCrKEx");
    EXPECT_EQ(keyring.GetAddress(1, 42u),
              "5CofVLAGjwvdGXvBiP6ddtZYMVbhT5Xke8ZrshUpj2ZXAnND");
    // This test exercises the caching codepath in the GetAddress
    // implementation.
    EXPECT_EQ(keyring.GetAddress(0, 42u),
              "5HGiBcFgEBMgT6GEuo9SA98sBnGgwHtPKDXiUukT6aqCrKEx");
  }
}

TEST(PolkadotKeyring, AddHDAccount) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // polkadot_sdk::sp_core::crypto::set_default_ss58_version(
  //     Ss58AddressFormat::from(
  //         sp_core::crypto::Ss58AddressFormatRegistry::PolkadotAccount
  // ));
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}//polkadot//0"),
  //     None,
  // )
  // .unwrap();
  //
  // let s = pair.public().to_string();
  // assert_eq!(s, "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb");
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet);

    EXPECT_EQ(keyring.AddNewHDAccount(0u).value(),
              "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb");
    EXPECT_EQ(keyring.AddNewHDAccount(1u).value(),
              "14QspRnocdHvoDfFnGvvYwskrsSWG8nneJ2BAQ2oSMcsQUxB");
  }

  // Testnet.
  // Generated using subkey:
  // subkey inspect --scheme sr25519 --network substrate "bottom drive obey lake
  // curtain smoke basket hold race lonely fit walk//westend//*".
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet);

    EXPECT_EQ(keyring.AddNewHDAccount(0u).value(),
              "5HGiBcFgEBMgT6GEuo9SA98sBnGgwHtPKDXiUukT6aqCrKEx");
    EXPECT_EQ(keyring.AddNewHDAccount(1u).value(),
              "5CofVLAGjwvdGXvBiP6ddtZYMVbhT5Xke8ZrshUpj2ZXAnND");
  }
}

TEST(PolkadotKeyring, GetPublicKey) {
  // Derived from the polkadot-sdk using:
  // clang-format off
  //
  // let pair = polkadot_sdk::sp_core::sr25519::Pair::from_string(
  //     &format!("{DEV_PHRASE}//polkadot//0"),
  //     None,
  // )
  // .unwrap();
  // let s = bytes_to_hex(pair.public().as_bytes_ref());
  // assert_eq!(
  //     s,
  //     "9C9C968EBB31417A36BEC0908ECD9EB6E847B44821E521DDA9ADD8C418EF7C30"
  // );
  //
  // clang-format on

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet);

    auto pubkey = keyring.GetPublicKey(0);

    constexpr char const* kPublicKey =
        "9C9C968EBB31417A36BEC0908ECD9EB6E847B44821E521DDA9ADD8C418EF7C30";
    EXPECT_EQ(base::HexEncode(pubkey), kPublicKey);
  }

  // Testnet.
  // Generated using subkey:
  // subkey inspect --scheme sr25519 --network substrate "bottom drive obey lake
  // curtain smoke basket hold race lonely fit walk//westend//*".
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet);

    auto pubkey = keyring.GetPublicKey(0);

    constexpr char const* kPublicKey =
        "E655361D12F3CCCA5F128187CF3F5EEA052BE722746E392C8B498D0D18723470";
    EXPECT_EQ(base::HexEncode(pubkey), kPublicKey);
  }
}

TEST(PolkadotKeyring, SignAndVerifyMessage) {
  auto message = base::byte_span_from_cstring("hello, world!");

  // Mainnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet);

    auto signature = keyring.SignMessage(message, 0);
    auto verified = keyring.VerifyMessage(signature, message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(signature, message, 1);
    EXPECT_FALSE(verified);
  }

  // Testnet.
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet);

    auto signature = keyring.SignMessage(message, 0);
    auto verified = keyring.VerifyMessage(signature, message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(signature, message, 1);
    EXPECT_FALSE(verified);
  }
}

TEST(PolkadotKeyring, VerifyMessage) {
  auto message = base::byte_span_from_cstring("hello, world!");

  // Testnet.
  // Generated using subkey:
  // subkey:latest sign --suri "bottom drive obey lake curtain smoke basket hold
  // race lonely fit walk//westend//0" --message "hello, world!".
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotTestnet);

    std::string signature_hex =
        "4C62835B705663D221F45A70E493C2B48FEEE5B541D3071727139A44A71F1E46E5F536"
        "165977C187FFBCA045170EAC8B8AF33E94EF2D6410C264B536DF9C5C86";

    bool verified =
        keyring.VerifyMessage(ToSignature(signature_hex), message, 0);
    EXPECT_TRUE(verified);

    verified = keyring.VerifyMessage(ToSignature(signature_hex), message, 1);
    EXPECT_FALSE(verified);

    std::string signature_hex2 =
        "20EBF57512301FB068AB1DE33C00DC2A9B020F0F446B5C1EA89D3F4A04A5B05C8206C5"
        "5DCCD8419019FC86F4B8D177CDEF035FC36A0BE8755423BA7377927D8D";

    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 1);
    EXPECT_FALSE(verified);

    // Test with wrong signature data - should fail
    std::string wrong_signature_hex =
        "FFFFFFF512301FB068AB1DE33C00DC2A9B020F0F446B5C1EA89D3F4A04A5B05C8206C5"
        "5DCCD8419019FC86F4B8D177CDEF035FC36A0BE8755423BA7377927D8D";

    verified =
        keyring.VerifyMessage(ToSignature(wrong_signature_hex), message, 0);
    EXPECT_FALSE(verified);
  }

  // Mainnet.
  // Generated using subkey:
  // subkey:latest sign --suri "bottom drive obey lake curtain smoke basket hold
  // race lonely fit walk//polkadot//0" --message "hello, world!".
  {
    auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
    PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                            mojom::KeyringId::kPolkadotMainnet);

    // Test with first mainnet signature vector
    std::string signature_hex =
        "462D47EAABE15026127324CE0917A696BAE38472C57B6670154BC33F5E053B0ACF5BA2"
        "EDC706366B407829DA3083F21C04FD1617B6AED5AED13177A4B2E9358F";

    bool verified =
        keyring.VerifyMessage(ToSignature(signature_hex), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex), message, 1);
    EXPECT_FALSE(verified);

    // Test with second mainnet signature vector
    std::string signature_hex2 =
        "EAB9949387D0D89F0E22DF4D90F5F8CCAA9E6B974DF6CAD39C79764C655EA924378536"
        "559E2D925EE534F31583511BF5050D9CB881AD51CE4607A5AD3C75E78A";

    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 0);
    EXPECT_TRUE(verified);

    // Test with wrong account index - should fail
    verified = keyring.VerifyMessage(ToSignature(signature_hex2), message, 1);
    EXPECT_FALSE(verified);

    // Test with wrong signature data - should fail
    std::string wrong_signature_hex =
        "FFFFFF9387D0D89F0E22DF4D90F5F8CCAA9E6B974DF6CAD39C79764C655EA924378536"
        "559E2D925EE534F31583511BF5050D9CB881AD51CE4607A5AD3C75E78A";

    verified =
        keyring.VerifyMessage(ToSignature(wrong_signature_hex), message, 0);
    EXPECT_FALSE(verified);
  }
}

}  // namespace brave_wallet
