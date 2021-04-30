/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_keyring.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HDKeyringUnitTest, ConstructRootHDKey) {
  HDKeyring keyring;
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  keyring.ConstructRootHDKey(seed, "m/44'/60'/0'/0");
  EXPECT_EQ(keyring.master_key_->GetPrivateExtendedKey(),
            "xprv9s21ZrQH143K3gWQTKSxNE9PXf6jyGYt2oTP7RNF47NemqwAwWF5nUkCjsYyB5"
            "adUPLNuu2XQoPCy9P596CdE1Bf3oW7eApGB2DcX3nZUAj");
  EXPECT_EQ(keyring.root_->GetPrivateExtendedKey(),
            "xprvA1YGbmYkUq9KMyPwADQehauc1vG7TSbNLc1dwYbvU7VzyAr7TPhj9VoJJoP2CV"
            "5kDmXXSZvbJ79ieLnD7Pt4rhbuaQjVr2JE3vcDBAvDoUg");
  EXPECT_EQ(keyring.root_->GetPublicExtendedKey(),
            "xpub6EXd1H5eKChcaTUQGEwf4irLZx6bruKDhpwEjw1Y2T2yqyBFzw1yhJ7nA5EeBK"
            "ozqYKB8jHxmhe7bEqyBEdPNWyPgCm2aZfs9tbLVYujvL3");
}

TEST(HDKeyringUnitTest, Accounts) {
  HDKeyring keyring;
  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  keyring.ConstructRootHDKey(seed, "m/44'/60'/0'/0");
  keyring.AddAccounts();
  EXPECT_EQ(keyring.GetAddress(0),
            "0x2166fB4e11D44100112B1124ac593081519cA1ec");
  keyring.AddAccounts(2);
  std::vector<std::string> accounts = keyring.GetAccounts();
  EXPECT_EQ(accounts.size(), 3u);
  EXPECT_EQ(keyring.GetAddress(1),
            "0x2A22ad45446E8b34Da4da1f4ADd7B1571Ab4e4E7");
  EXPECT_EQ(keyring.GetAddress(2),
            "0x02e77f0e2fa06F95BDEa79Fad158477723145838");
  for (size_t i = 0; i < accounts.size(); ++i)
    EXPECT_EQ(accounts[i], keyring.GetAddress(i));
  // remove the index 1 account
  keyring.RemoveAccount("0x2A22ad45446E8b34Da4da1f4ADd7B1571Ab4e4E7");
  accounts = keyring.GetAccounts();
  EXPECT_EQ(accounts.size(), 2u);
  EXPECT_EQ(keyring.GetAddress(0),
            "0x2166fB4e11D44100112B1124ac593081519cA1ec");
  EXPECT_EQ(keyring.GetAddress(1),
            "0x02e77f0e2fa06F95BDEa79Fad158477723145838");
  for (size_t i = 0; i < accounts.size(); ++i)
    EXPECT_EQ(accounts[i], keyring.GetAddress(i));

  keyring.RemoveAccount("0xDEADBEEFdeadbeefdeadbeefdeadbeefDEADBEEF");
  EXPECT_EQ(accounts.size(), 2u);

  EXPECT_TRUE(keyring.GetAddress(4).empty());
  HDKeyring keyring2;
  keyring2.ConstructRootHDKey(seed, "m|123|44444");
  EXPECT_TRUE(keyring2.GetAddress(0).empty());
}

TEST(HDKeyringUnitTest, SignTransaction) {
  // Specific signature check is in eth_transaction_unittest.cc
  HDKeyring keyring;
  EthTransaction tx(
      0x09, 0x4a817c800, 0x5208,
      EthAddress::FromHex("0x3535353535353535353535353535353535353535"),
      0x0de0b6b3a7640000, std::vector<uint8_t>());
  keyring.SignTransaction("0xDEADBEEFdeadbeefdeadbeefdeadbeefDEADBEEF", &tx);
  EXPECT_FALSE(tx.IsSigned());

  std::vector<uint8_t> seed;
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  keyring.ConstructRootHDKey(seed, "m/44'/60'/0'/0");
  keyring.AddAccounts();
  keyring.SignTransaction(keyring.GetAddress(0), &tx);
  EXPECT_TRUE(tx.IsSigned());
}

TEST(HDKeyringUnitTest, SignMessage) {
  std::vector<uint8_t> private_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "6969696969696969696969696969696969696969696969696969696969696969",
      &private_key));

  std::unique_ptr<HDKey> key = std::make_unique<HDKey>();
  key->SetPrivateKey(private_key);

  HDKeyring keyring;
  keyring.accounts_.push_back(std::move(key));
  EXPECT_EQ(keyring.GetAddress(0),
            "0xbE93f9BacBcFFC8ee6663f2647917ed7A20a57BB");

  std::vector<uint8_t> message;
  EXPECT_TRUE(base::HexStringToBytes("68656c6c6f20776f726c64", &message));
  const std::vector<uint8_t> sig =
      keyring.SignMessage(keyring.GetAddress(0), message);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(sig)),
            "ce909e8ea6851bc36c007a0072d0524b07a3ff8d4e623aca4c71ca8e57250c4d0a"
            "3fc38fa8fbaaa81ead4b9f6bd03356b6f8bf18bccad167d78891636e1d6956");

  EXPECT_TRUE(
      keyring.SignMessage("0xDEADBEEFdeadbeefdeadbeefdeadbeefDEADBEEF", message)
          .empty());
}

TEST(HDKeyringUnitTest, ClearData) {
  HDKeyring keyring;
  std::vector<uint8_t> seed;
  EXPECT_TRUE(keyring.empty());
  EXPECT_TRUE(base::HexStringToBytes(
      "13ca6c28d26812f82db27908de0b0b7b18940cc4e9d96ebd7de190f706741489907ef65b"
      "8f9e36c31dc46e81472b6a5e40a4487e725ace445b8203f243fb8958",
      &seed));
  keyring.ConstructRootHDKey(seed, "m/44'/60'/0'/0");
  EXPECT_TRUE(keyring.empty());
  keyring.AddAccounts();
  EXPECT_FALSE(keyring.empty());
  keyring.ClearData();
  EXPECT_TRUE(keyring.empty());
}

}  // namespace brave_wallet
