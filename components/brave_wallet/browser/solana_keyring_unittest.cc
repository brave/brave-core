/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
const char mnemonic[] =
    "scare piece awesome elite long drift control cabbage glass dash coral "
    "angry";
}  // namespace

TEST(SolanaKeyringUnitTest, ConstructRootHDKey) {
  SolanaKeyring keyring;
  std::unique_ptr<std::vector<uint8_t>> seed =
      MnemonicToSeed(std::string(mnemonic), "");
  keyring.ConstructRootHDKey(*seed, "m/44'/501'");

  EXPECT_EQ(keyring.master_key_->GetEncodedPrivateKey(),
            "2ZVNHVyrLDCBa6qxWmzDDDQEC9ZGuBt9JTynUBq67FKFp7R5nb32X8d4UN9gKnbgnP"
            "U4RBmdugcfPxtBbSvJdbM7");
  EXPECT_EQ(keyring.root_->GetEncodedPrivateKey(),
            "XUPar98T8X5HyvSw4pKk2cFi2zCMxzNcm8CJoQgDa3CjFpFKQic2cAFJhvaMgQCAQj"
            "Rs4sHHjiTqhAZ8F3tVR8D");
}

TEST(SolanaKeyringUnitTest, Accounts) {
  SolanaKeyring keyring;
  std::unique_ptr<std::vector<uint8_t>> seed =
      MnemonicToSeed(std::string(mnemonic), "");
  keyring.ConstructRootHDKey(*seed, "m/44'/501'");

  keyring.AddAccounts();
  EXPECT_EQ(keyring.GetAddress(0),
            "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu");
  EXPECT_EQ(keyring.GetEncodedPrivateKey(
                "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu"),
            "3WoEqkmeTX4BRTS3KNJCsqy7LktvEwbFSoqwMhC7xNgCG3zhwUptkT6KkJcbTpVJGX"
            "Rw9pd8CYVxZ8wLt8cUoVZb");

  keyring.AddAccounts(2);
  std::vector<std::string> accounts = keyring.GetAccounts();
  EXPECT_EQ(accounts.size(), 3u);

  EXPECT_EQ(keyring.GetAddress(1),
            "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu");
  EXPECT_EQ(keyring.GetEncodedPrivateKey(
                "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu"),
            "4pNHX6ATNXad3KZTb2PXTosW5ceaxqx45M9NH9pjcZCH9qoQKx6RMzUjuzm6J9Y2uw"
            "jCxJc5JsjL1TrGr1X3nPFP");
  EXPECT_EQ(keyring.GetAddress(2),
            "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY");

  for (size_t i = 0; i < accounts.size(); ++i) {
    EXPECT_EQ(accounts[i], keyring.GetAddress(i));
    EXPECT_EQ(keyring.GetAccountIndex(accounts[i]), i);
  }
  EXPECT_FALSE(keyring.GetAccountIndex("0x123"));

  // remove the last account
  keyring.RemoveAccount();
  accounts = keyring.GetAccounts();
  EXPECT_EQ(accounts.size(), 2u);
  EXPECT_EQ(keyring.GetAddress(0),
            "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu");
  EXPECT_EQ(keyring.GetAddress(1),
            "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu");
  for (size_t i = 0; i < accounts.size(); ++i) {
    EXPECT_EQ(accounts[i], keyring.GetAddress(i));
    EXPECT_EQ(keyring.GetAccountIndex(accounts[i]), i);
  }

  keyring.AddAccounts(1);
  EXPECT_EQ(keyring.GetAccounts().size(), 3u);
  EXPECT_EQ(keyring.GetAddress(2),
            "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY");
  EXPECT_EQ(keyring.GetEncodedPrivateKey(
                "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY"),
            "47rewUeufUCmtmes3uAGAo7AyM3bBYTvJdD1jQs9MGwB4eYn8SAyQUMNc9b5wFRhQy"
            "CP9WwmP7JMPAA9U9Q5E8xr");

  EXPECT_TRUE(keyring.GetEncodedPrivateKey("brave").empty());
}

TEST(SolanaKeyringUnitTest, SignMessage) {
  SolanaKeyring keyring;
  std::unique_ptr<std::vector<uint8_t>> seed =
      MnemonicToSeed(std::string(mnemonic), "");
  keyring.ConstructRootHDKey(*seed, "m/44'/501'");

  keyring.AddAccounts();
  EXPECT_EQ(keyring.GetAddress(0),
            "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu");

  // Message: Hello Brave
  const std::vector<uint8_t> message = {72, 101, 108, 108, 111, 32,
                                        66, 114, 97,  118, 101};
  const std::vector<uint8_t> expected_signature = {
      2,   179, 226, 40,  228, 8,   248, 176, 39,  21,  205, 26,  136,
      7,   92,  162, 178, 18,  181, 212, 58,  93,  159, 167, 207, 74,
      58,  102, 213, 60,  21,  217, 236, 188, 90,  75,  120, 116, 130,
      104, 20,  185, 45,  50,  115, 244, 223, 167, 114, 6,   225, 189,
      103, 51,  156, 215, 22,  207, 130, 197, 57,  39,  186, 12};
  auto signature = keyring.SignMessage(keyring.GetAddress(0), message);
  EXPECT_EQ(signature, expected_signature);
}

TEST(SolanaKeyringUnitTest, ImportAccount) {
  SolanaKeyring keyring;
  std::vector<uint8_t> private_key;
  ASSERT_TRUE(base::HexStringToBytes(
      "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7",
      &private_key));
  keyring.ImportAccount(private_key);
  EXPECT_EQ(keyring.GetImportedAccountsNumber(), 1u);
  EXPECT_EQ(keyring.GetEncodedPrivateKey(
                "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");

  private_key.clear();
  ASSERT_TRUE(base::HexStringToBytes(
      "bee602cc7dd4c1be27d8459892ab4e23f7a1d31ffde8cdd50542068ada52a201",
      &private_key));
  keyring.ImportAccount(private_key);
  EXPECT_EQ(keyring.GetImportedAccountsNumber(), 2u);

  EXPECT_FALSE(keyring.RemoveImportedAccount("InvalidAddress"));
  EXPECT_TRUE(keyring.RemoveImportedAccount(
      "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ"));
  EXPECT_EQ(keyring.GetImportedAccountsNumber(), 1u);
}

}  // namespace brave_wallet
