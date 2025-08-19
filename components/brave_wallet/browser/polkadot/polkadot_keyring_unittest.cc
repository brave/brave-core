/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include <string_view>

#include "base/strings/string_number_conversions.h"
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

TEST(PolkadotKeyring, GetUnifiedAddress) {
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

  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  constexpr char const* kAddress0 =
      "14YLzDFZTwnkcJkFij4Km7g5LdkLqKHy47xYGPN6HsLJpfnb";

  constexpr char const* kAddress1 =
      "14QspRnocdHvoDfFnGvvYwskrsSWG8nneJ2BAQ2oSMcsQUxB";

  // https://github.com/paritytech/ss58-registry/blob/57344a5258f1abff8fdb4689e91ff594ca54f02d/ss58-registry.json#L14-L16
  constexpr uint16_t const kPolkadotPrefix = 0;

  EXPECT_EQ(keyring.GetAddress(0, kPolkadotPrefix), kAddress0);
  EXPECT_EQ(keyring.GetAddress(1, kPolkadotPrefix), kAddress1);
  // This test exercises the caching codepath in the GetAddress
  // implementation.
  EXPECT_EQ(keyring.GetAddress(0, kPolkadotPrefix), kAddress0);
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

  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  auto pubkey = keyring.GetPublicKey(0);

  constexpr char const* kPublicKey =
      "9C9C968EBB31417A36BEC0908ECD9EB6E847B44821E521DDA9ADD8C418EF7C30";
  EXPECT_EQ(base::HexEncode(pubkey), kPublicKey);
}

TEST(PolkadotKeyring, SignAndVerifyMessage) {
  auto seed = bip39::MnemonicToEntropyToSeed(kDevPhrase).value();
  PolkadotKeyring keyring(base::span(seed).first<kPolkadotSeedSize>(),
                          mojom::KeyringId::kPolkadotMainnet);

  std::string_view message = "hello, world!";
  auto signature = keyring.SignMessage(base::as_byte_span(message), 0);
  auto verified =
      keyring.VerifyMessage(signature, base::as_byte_span(message), 0);
  EXPECT_TRUE(verified);

  verified = keyring.VerifyMessage(signature, base::as_byte_span(message), 1);
  EXPECT_FALSE(verified);
}

}  // namespace brave_wallet
