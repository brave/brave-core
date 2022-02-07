/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

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

}  // namespace brave_wallet
