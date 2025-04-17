/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

using bip39::MnemonicToSeed;
using mojom::CardanoKeyId;
using mojom::CardanoKeyRole;
using mojom::KeyringId::kCardanoMainnet;
using mojom::KeyringId::kCardanoTestnet;

// https://github.com/Emurgo/cardano-serialization-lib/blob/e86bb2542e3b38c506a9fba269fc073bb18dbe60/rust/src/tests/address.rs#L124-L155
TEST(CardanoHDKeyringUnitTest, TestVectors1) {
  auto entropy =
      std::to_array<uint8_t>({0xdf, 0x9e, 0xd2, 0x5e, 0xd1, 0x46, 0xbf, 0x43,
                              0x33, 0x6a, 0x5d, 0x7c, 0xf7, 0x39, 0x59, 0x94});

  CardanoHDKeyring mainnet_keyring(base::span(entropy), kCardanoMainnet);
  EXPECT_EQ(
      mainnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr"
      "1qx"
      "2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3"
      "jcu5d8ps7zex2k2xt3uqxgjqnnj83ws8lhrn648jjxtwq"
      "fjkjv7");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qz"
      "2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3"
      "jcu5d8ps7zex2k2xt3uqxgjqnnj83ws8lhrn648jjxtwq"
      "2ytjqp");
}

// https://github.com/Emurgo/cardano-serialization-lib/blob/e86bb2542e3b38c506a9fba269fc073bb18dbe60/rust/src/tests/address.rs#L351-L382
TEST(CardanoHDKeyringUnitTest, TestVectors2) {
  auto entropy = std::to_array<uint8_t>({
      0x4e, 0x82, 0x8f, 0x9a, 0x67, 0xdd, 0xcf, 0xf0, 0xe6, 0x39, 0x1a,
      0xd4, 0xf2, 0x6d, 0xdb, 0x75, 0x79, 0xf5, 0x9b, 0xa1, 0x4b, 0x6d,
      0xd4, 0xba, 0xf6, 0x3d, 0xcf, 0xdb, 0x9d, 0x24, 0x20, 0xda,
  });

  CardanoHDKeyring mainnet_keyring(base::span(entropy), kCardanoMainnet);
  EXPECT_EQ(
      mainnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr"
      "1qy"
      "y6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmn8"
      "k8ttq8f3gag0h89aepvx3xf69g0l9pf80tqv7cve0l33s"
      "dn8p3d");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qq"
      "y6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmn8"
      "k8ttq8f3gag0h89aepvx3xf69g0l9pf80tqv7cve0l33s"
      "w96paj");
}

TEST(CardanoHDKeyringUnitTest, TestVectorsAbandon24) {
  // Full zeros entropy of 24-words 'abandon abandon ... abandon art' mnemonic.
  std::array<uint8_t, 32> entropy = {};

  CardanoHDKeyring mainnet_keyring(base::span(entropy), kCardanoMainnet);
  EXPECT_EQ(
      mainnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr"
      "1qy"
      "qt0pru382hy9vjlsxv3ye02z50sfvt8xunscg5pgden77z"
      "73dpdfng2ctw2ekqplqgrljelz7h4dneac27nn3qx3rqrh"
      "qvwd");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qq"
      "qt0pru382hy9vjlsxv3ye02z50sfvt8xunscg5pgden77z"
      "73dpdfng2ctw2ekqplqgrljelz7h4dneac27nn3qx3rqqp"
      "avzj");
}

}  // namespace brave_wallet
