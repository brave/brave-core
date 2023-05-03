/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_keyring.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr char kBip84TestMnemonic[] =
    "abandon abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon about";
}

namespace brave_wallet {
using mojom::BitcoinKeyId;

// https://github.com/bitcoin/bips/blob/master/bip-0084.mediawiki#test-vectors
TEST(BitcoinKeyringUnitTest, TestVectors) {
  BitcoinKeyring keyring(false);
  keyring.ConstructRootHDKey(*MnemonicToSeed(kBip84TestMnemonic, ""),
                             "m/84'/0'");

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 0, 0))),
      "0330D54FD0DD420A6E5F8D3624F5F3482CAE350F79D5F0753BF5BEEF9C2D91AF3C");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 0, 1))),
      "03E775FD51F0DFB8CD865D9FF1CCA2A158CF651FE997FDC9FEE9C1D3B5E995EA77");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 1, 0))),
      "03025324888E429AB8E3DBAF1F7802648B9CD01E9B418485C5FA4C1B9B5700E1A6");

  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 0, 0)),
            "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 0, 1)),
            "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 1, 0)),
            "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el");
}

TEST(BitcoinKeyringUnitTest, GetAddress) {
  BitcoinKeyring keyring(false);
  keyring.ConstructRootHDKey(*MnemonicToSeed(kBip84TestMnemonic, ""),
                             "m/84'/0'");

  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 0, 0)),
            "bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 0, 1)),
            "bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(0, 1, 0)),
            "bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el");

  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(1, 0, 0)),
            "bc1qku0qh0mc00y8tk0n65x2tqw4trlspak0fnjmfz");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(1, 0, 1)),
            "bc1qx0tpa0ctsy5v8xewdkpf69hhtz5cw0rf5uvyj6");
  EXPECT_EQ(keyring.GetAddress(BitcoinKeyId(1, 1, 0)),
            "bc1qt0x83f5vmnapgl2gjj9r3d67rcghvjaqrvgpck");

  BitcoinKeyring testnet_keyring(true);
  testnet_keyring.ConstructRootHDKey(*MnemonicToSeed(kBip84TestMnemonic, ""),
                                     "m/84'/1'");

  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(0, 0, 0)),
            "tb1q6rz28mcfaxtmd6v789l9rrlrusdprr9pqcpvkl");
  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(0, 0, 1)),
            "tb1qd7spv5q28348xl4myc8zmh983w5jx32cjhkn97");
  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(0, 1, 0)),
            "tb1q9u62588spffmq4dzjxsr5l297znf3z6j5p2688");

  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(1, 0, 0)),
            "tb1qp7shgcwx3mpzgxjvff0d77vuhchcldzfy60x6s");
  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(1, 0, 1)),
            "tb1qynt29nsj8j972la4lu3efu42m5us2svmc8ekx8");
  EXPECT_EQ(testnet_keyring.GetAddress(BitcoinKeyId(1, 1, 0)),
            "tb1qkvjfredfz59jwvqru7a2spvugqd7dlx6e4aqvm");
}

TEST(BitcoinKeyringUnitTest, GetPubkey) {
  BitcoinKeyring keyring(false);
  keyring.ConstructRootHDKey(*MnemonicToSeed(kBip84TestMnemonic, ""),
                             "m/84'/0'");

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 0, 0))),
      "0330D54FD0DD420A6E5F8D3624F5F3482CAE350F79D5F0753BF5BEEF9C2D91AF3C");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 0, 1))),
      "03E775FD51F0DFB8CD865D9FF1CCA2A158CF651FE997FDC9FEE9C1D3B5E995EA77");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(0, 1, 0))),
      "03025324888E429AB8E3DBAF1F7802648B9CD01E9B418485C5FA4C1B9B5700E1A6");

  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(1, 0, 0))),
      "035CE17D6438A499E0C7FEF59B43FD7B2CB6E4A31B598F6A4C20CA94854EAC9D36");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(1, 0, 1))),
      "0366DC739A33F2C600B99927735BD2FEEA5C1D78142D2D0D3917623C4AF09E8BBC");
  EXPECT_EQ(
      base::HexEncode(*keyring.GetPubkey(BitcoinKeyId(1, 1, 0))),
      "025695996D13031C54896990E6E38DB5849F5A64FA81142B452D6E23C36FD83880");

  BitcoinKeyring testnet_keyring(true);
  testnet_keyring.ConstructRootHDKey(*MnemonicToSeed(kBip84TestMnemonic, ""),
                                     "m/84'/1'");

  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(0, 0, 0))),
      "02E7AB2537B5D49E970309AAE06E9E49F36CE1C9FEBBD44EC8E0D1CCA0B4F9C319");
  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(0, 0, 1))),
      "03EEED205A69022FED4A62A02457F3699B19C06BF74BF801ACC6D9AE84BC16A9E1");
  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(0, 1, 0))),
      "035D49ECCD54D0099E43676277C7A6D4625D611DA88A5DF49BF9517A7791A777A5");

  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(1, 0, 0))),
      "024AC8DA6430EC1C3D7DB1C01EBCB26F037303A28565587B76A275CD5D286DADE0");
  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(1, 0, 1))),
      "03392B97B3B3900E27431BDF516E0A5A8B6706D1827B85567FC0E45FA3109A0BC7");
  EXPECT_EQ(
      base::HexEncode(*testnet_keyring.GetPubkey(BitcoinKeyId(1, 1, 0))),
      "03780B696D530DEF424B80368C5F401D12FBF7B59A56CA559AB083DFD2AF405568");
}

TEST(BitcoinKeyringUnitTest, SignBitcoinMessage) {
  // TODO(apaymyshev): make test
}

}  // namespace brave_wallet
