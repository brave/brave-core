/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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
  EXPECT_EQ(HexEncodeLower(*mainnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "73fea80d424276ad0978d4fe5310e8bc2d485f5f6bb3bf87612989f112ad5a7d"
            "88c951e5bfbb9f886159fce334ffc6e53d56ace4c8c93c52f8da1fd92a8dd8d3"
            "3de2e51df9d84fd270ff568a8e4ad3a19f4c7801c1eb2e1a44395b0b96f21b07");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qz"
      "2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3"
      "jcu5d8ps7zex2k2xt3uqxgjqnnj83ws8lhrn648jjxtwq"
      "2ytjqp");
  EXPECT_EQ(HexEncodeLower(*testnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "73fea80d424276ad0978d4fe5310e8bc2d485f5f6bb3bf87612989f112ad5a7d"
            "88c951e5bfbb9f886159fce334ffc6e53d56ace4c8c93c52f8da1fd92a8dd8d3"
            "3de2e51df9d84fd270ff568a8e4ad3a19f4c7801c1eb2e1a44395b0b96f21b07");
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
  EXPECT_EQ(HexEncodeLower(*mainnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "f9162b91126212b71500e89dc7da31111dfc1466a9f24f48a34e7ea529d2d338"
            "7799df826eca2117d8cb4c84136806ca57107925754326c3ddb7ffec3a4ccd2d"
            "86814627ce627f201cfd7bafbf1ad66dc64e00534f9c0e20ed40231c836af109");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qq"
      "y6nhfyks7wdu3dudslys37v252w2nwhv0fw2nfawemmn8"
      "k8ttq8f3gag0h89aepvx3xf69g0l9pf80tqv7cve0l33s"
      "w96paj");
  EXPECT_EQ(HexEncodeLower(*testnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "f9162b91126212b71500e89dc7da31111dfc1466a9f24f48a34e7ea529d2d338"
            "7799df826eca2117d8cb4c84136806ca57107925754326c3ddb7ffec3a4ccd2d"
            "86814627ce627f201cfd7bafbf1ad66dc64e00534f9c0e20ed40231c836af109");
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
      "qt0pru382hy9vjlsxv3ye02z50sfvt8xunscg5pgden77"
      "z73dpdfng2ctw2ekqplqgrljelz7h4dneac27nn3qx3rq"
      "rhqvwd");
  EXPECT_EQ(HexEncodeLower(*mainnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "63c5d69570349e4233a0575811464f0e8a3fd329abe76e9bdc3d3f1b95982179"
            "a45ece90549a7719fdb7b6b102bae034b13676aa4b39ad1296ec95bbac68447a"
            "85eca08f33a2aba606eb1fd3d1fc9fb2b49338a657cf75661e7022b718a9d303");

  CardanoHDKeyring testnet_keyring(base::span(entropy), kCardanoTestnet);
  EXPECT_EQ(
      testnet_keyring.GetAddress(0, CardanoKeyId(CardanoKeyRole::kExternal, 0))
          ->address_string,
      "addr_test"
      "1qq"
      "qt0pru382hy9vjlsxv3ye02z50sfvt8xunscg5pgden77"
      "z73dpdfng2ctw2ekqplqgrljelz7h4dneac27nn3qx3rq"
      "qpavzj");
  EXPECT_EQ(HexEncodeLower(*testnet_keyring.SignMessage(
                0, CardanoKeyId(CardanoKeyRole::kExternal, 0),
                base::byte_span_from_cstring("brave"))),
            "63c5d69570349e4233a0575811464f0e8a3fd329abe76e9bdc3d3f1b95982179"
            "a45ece90549a7719fdb7b6b102bae034b13676aa4b39ad1296ec95bbac68447a"
            "85eca08f33a2aba606eb1fd3d1fc9fb2b49338a657cf75661e7022b718a9d303");
}

}  // namespace brave_wallet
