/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot_keyring.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

// All derived from the polkadot-sdk using:
// let pair =
//   polkadot_sdk::sp_core::sr25519::Pair::from_string(
//     DEV_PHRASE, None).unwrap();

inline constexpr const char* kDevPhrase =
    "bottom drive obey lake curtain smoke basket hold race lonely fit walk";

inline constexpr char const* kDevSeed =
    "FAC7959DBFE72F052E5A0C3C8D6530F202B02FD8F9F5CA3580EC8DEB7797479E";

inline constexpr char const* kDevPubKey =
    "46EBDDEF8CD9BB167DC30878D7113B7E168E6F0646BEFFD77D69D39BAD76B47A";

// From:
// let s = pair.public().to_string();
// assert_eq!(s, "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV");
//
// See also:
// https://github.com/paritytech/polkadot-sdk/blob/0d765ce37b258640a6eeb575f6bff76d6a7b7c46/substrate/primitives/core/src/crypto.rs#L49
inline constexpr const char* kDevAddress =
    "5DfhGyQdFobKM8NsWvEeAKk5EQQgYe9AydgJ7rMB6E1EqRzV";

}  // namespace

TEST(PolkadotKeyring, GenerateRoot) {
  auto seed = PolkadotKeyring::MnemonicToSeed(kDevPhrase).value();
  EXPECT_EQ(base::HexEncode(seed), kDevSeed);

  // Verify that our seed can be used to derive a keypair with a public key that
  // matches the test vector.
  auto keypair = HDKeySr25519::GenerateFromSeed(seed);
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

}  // namespace brave_wallet
