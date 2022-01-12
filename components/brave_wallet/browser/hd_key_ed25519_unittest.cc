/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_key_ed25519.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

namespace brave_wallet {
TEST(HDKeyEd25519UnitTest, Test) {
  const char mnemonic[] =
      "scare piece awesome elite long drift control cabbage glass dash coral "
      "angry";
  std::unique_ptr<std::vector<uint8_t>> seed =
      MnemonicToSeed(std::string(mnemonic), "");
  auto master_key = HDKeyEd25519::GenerateFromSeed(*seed);
  auto child0 = master_key->DeriveChildFromPath("m/44'/501'/0'/0'");
  EXPECT_EQ(child0->GetBase58EncodedPublicKey(),
            "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu");
  EXPECT_EQ(child0->GetBase58EncodedKeypair(),
            "3WoEqkmeTX4BRTS3KNJCsqy7LktvEwbFSoqwMhC7xNgCG3zhwUptkT6KkJcbTpVJGX"
            "Rw9pd8CYVxZ8wLt8cUoVZb");
  auto child1 = master_key->DeriveChildFromPath("m/44'/501'/1'/0'");
  EXPECT_EQ(child1->GetBase58EncodedPublicKey(),
            "D37CnANGLynWiWmkdAETRNe3nLS7f59SbmK9kK8xSjcu");
  EXPECT_EQ(child1->GetBase58EncodedKeypair(),
            "4pNHX6ATNXad3KZTb2PXTosW5ceaxqx45M9NH9pjcZCH9qoQKx6RMzUjuzm6J9Y2uw"
            "jCxJc5JsjL1TrGr1X3nPFP");
  auto child2 = master_key->DeriveChildFromPath("m/44'/501'/2'/0'");
  EXPECT_EQ(child2->GetBase58EncodedPublicKey(),
            "HEuGsnLvkzHxmmCrFAPJpfSsGvW1zK6bSQykmPRhLxmY");
  EXPECT_EQ(child2->GetBase58EncodedKeypair(),
            "47rewUeufUCmtmes3uAGAo7AyM3bBYTvJdD1jQs9MGwB4eYn8SAyQUMNc9b5wFRhQy"
            "CP9WwmP7JMPAA9U9Q5E8xr");
  auto child3 = master_key->DeriveChildFromPath("m/44'/501'/0'/0");
  EXPECT_FALSE(child3);
  auto child4 = master_key->DeriveChildFromPath("12345");
  EXPECT_FALSE(child4);
  auto child5 = master_key->DeriveChild(0x80000000);
  EXPECT_FALSE(child5);
}

TEST(HDKeyEd25519UnitTest, TestVector1) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));
  // m
  auto master_key = HDKeyEd25519::GenerateFromSeed(bytes);
  EXPECT_EQ(master_key->GetBase58EncodedPublicKey(),
            "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");
  // m/0'/1'/2'/2'/1000000000'
  auto child = master_key->DeriveChildFromPath("m/0'/1'/2'/2'/1000000000'");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "53n47S4RT9ozx5KrpH6uYfdnAjrTBJri8qZJBvRfw1Bf");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3sVsV9myuRDg4rio4n3ftoP3NsUDzjVk6i8WiTg9veDsiALQjt9QEfUckJkutYUgzm"
            "wwz55D49JUDFic5Fu2gDjX");
  // m/0'
  child = master_key->DeriveChild(0);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "ATcCGRoY87cSJESCXbHXEX6CDWQxepAViUvVnNsELhRu");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "36crUN2YvuPXEpRXNmdtv5W1veeXHZvMqSe4Egqu4Ski9FHtbdizagf9Kfj8e7sD4S"
            "e5YCqQQ2vpUuKGycM8WhF9");
  // m/0'/1'
  child = child->DeriveChild(1);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "2hMz2f8WbLw5m2icKR2WVrcizvnguw8xaAnXjaeohuHQ");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "4ZCMMnibQjY732c95g1bK5aWzZpR3H1HAqGMeh1B4xpcUWkpxJyUVfwqVBjft1bpRA"
            "WjiJTaUUPWFJEqKWn6cVZp");
  // m/0'/1'/2'
  child = child->DeriveChild(2);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "CkYmXLvWehLXBzUAJ3g3wsfc5QjoCtWtSydquF7HDxXS");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3w45HeUP7x8DhVFxmUwsww19XUdxNZeTuMQQBFJCXAaGtYLvjUVvWovNX7aKpjp5pa"
            "YERPr1jgWEvGeemRm2bCBJ");
  // m/0'/1'/2'/2'
  child = child->DeriveChild(2);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "ALYYdMp2jVV4HGsZZPfLy1BQLMHL2CQG5XHpzr2XiHCw");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "ycUieXQauHN9msp7beGkDcUPwF4g3YhzqUXwVihv8PJbF96Eyeh1PFTxhzP4AaXt5U"
            "QCR3mVsrs8AiPCKMCLs2s");
  // m/0'/1'/2'/2'/1000000000'
  child = child->DeriveChild(1000000000);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "53n47S4RT9ozx5KrpH6uYfdnAjrTBJri8qZJBvRfw1Bf");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3sVsV9myuRDg4rio4n3ftoP3NsUDzjVk6i8WiTg9veDsiALQjt9QEfUckJkutYUgzm"
            "wwz55D49JUDFic5Fu2gDjX");
}

TEST(HDKeyEd25519UnitTest, TestVector2) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(base::HexStringToBytes(
      "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996"
      "93908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
      &bytes));
  // m
  auto master_key = HDKeyEd25519::GenerateFromSeed(bytes);
  EXPECT_EQ(master_key->GetBase58EncodedPublicKey(),
            "AgmjPHe5Qs4VakvXHGnd6NsYjaxt4suMUtf39TayrSfb");
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "ToTfZTGTYncQcR7P7PduNLKDd8sNHMKsB7td24qCZzwzzZ65fA8y7Ht3o7nwojMzoV"
            "rD9M6Y7vPKznLJPjpwgLZ");
  // m/0'/2147483647'/1'/2147483646'/2'
  auto child =
      master_key->DeriveChildFromPath("m/0'/2147483647'/1'/2147483646'/2'");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "5nUZbtNefYa7tWHdpQApxsjPLtTZpKuZYnKDsd2dXADu");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "2hhXd52y2dVVJGUkr6kikm3LcMQcPSwhWaB1GDU7nAMRWXbjAuG1G9mjdSETpAEAJ1"
            "vV9nQrvhARxQDc6iEEbpU7");
  // m/0'
  child = master_key->DeriveChild(0);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "A5uN5c31sqKK4x82gXeHzsBFpBTTusPDHBZT111V3u4i");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "Rm2NBwPiLaJoWaetGVz9Jy1T477CS2FfM4Q5JmWgCLRhX54T8zHX57RH6LgR2kRXTc"
            "DwPVMAQi4nxFVH2DJiXkA");
  // m/0'/2147483647'
  child = child->DeriveChild(2147483647);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "7AiuCW2Mg2vRAHsrVmsM3uFky4XRaXHqqcemSp6Bract");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "5gi27AKyRrB5rvX9yPT39WpRak9B5QAXSZLvFDoqb7nQGhKLTqhTLeUgax4FVGGurZ"
            "PQNjRX6N9sn4o7f5rSAeWG");
  // m/0'/2147483647'/1'
  child = child->DeriveChild(1);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "488Z1Z7moahUL7Np2JMrApWbWwdUEBzSfEioz9vj7vCc");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "27BCpwH2qcy7ANSVAisHjBN3CQyfzKyV4qcSet2YP1X5aCsoKS9kwcxqvJdVNcBWN3"
            "xuKFviozGBrUsbhXumYa9z");
  // m/0'/2147483647'/1'/2147483646'
  child = child->DeriveChild(2147483646);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "GJ2famWaTaWgT5oYvi1dqA7cvtoKMzyje1Pcx1bL9Nsc");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "2mJCNeA9JefF3B2gikqrR22BWa2ETCZNwijZvDn7XktHRVYj7sXhTt93sr7SqkBUp8"
            "h2pLb6V3nzpYN4mB9paeDQ");
  // m/0'/2147483647'/1'/2147483646'/2'
  child = child->DeriveChild(2);
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "5nUZbtNefYa7tWHdpQApxsjPLtTZpKuZYnKDsd2dXADu");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "2hhXd52y2dVVJGUkr6kikm3LcMQcPSwhWaB1GDU7nAMRWXbjAuG1G9mjdSETpAEAJ1"
            "vV9nQrvhARxQDc6iEEbpU7");
}

TEST(HDKeyEd25519UnitTest, Errors) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));
  auto master_key = HDKeyEd25519::GenerateFromSeed(bytes);

  // path contains normal index
  auto child1 = master_key->DeriveChildFromPath("m/0'/1'/2'/3'/4");
  EXPECT_FALSE(child1);

  // invalid path
  auto child2 = master_key->DeriveChildFromPath("BRAVE0'1'2'3'4'");
  EXPECT_FALSE(child2);

  // index is too big for hardened index
  auto child3 = master_key->DeriveChild(0x80000000);
  EXPECT_FALSE(child3);
}

}  // namespace brave_wallet
