/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/curve25519.h"

namespace brave_wallet {

namespace {

bool VerifySignature(const HDKeyEd25519& key,
                     base::span<const uint8_t> msg,
                     base::span<const uint8_t, kEd25519SignatureSize> sig) {
  return !!ED25519_verify(msg.data(), msg.size(), sig.data(),
                          key.GetPublicKeyAsSpan().data());
}

}  // namespace

// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#test-vector-1-for-ed25519
TEST(HDKeyEd25519UnitTest, TestVector1) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

  // m
  auto master_key = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m");
  EXPECT_EQ(HexEncodeLower(master_key->GetPrivateKeyAsSpan()),
            "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7");
  EXPECT_EQ(HexEncodeLower(master_key->GetPublicKeyAsSpan()),
            "a4b2856bfec510abab89753fac1ac0e1112364e7d250545963f135f2a33188ed");
  EXPECT_EQ(master_key->GetBase58EncodedPublicKey(),
            "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");

  // m/0'/1'/2'/2'/1000000000'
  auto child =
      HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m/0'/1'/2'/2'/1000000000'");
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e8793");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "3c24da049451555d51a7014a37337aa4e12d41e485abccfa46b47dfb2af54b7a");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "53n47S4RT9ozx5KrpH6uYfdnAjrTBJri8qZJBvRfw1Bf");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3sVsV9myuRDg4rio4n3ftoP3NsUDzjVk6i8WiTg9veDsiALQjt9QEfUckJkutYUgzm"
            "wwz55D49JUDFic5Fu2gDjX");
  // m/0'
  child = master_key->DeriveHardenedChild(0);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "68e0fe46dfb67e368c75379acec591dad19df3cde26e63b93a8e704f1dade7a3");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "8c8a13df77a28f3445213a0f432fde644acaa215fc72dcdf300d5efaa85d350c");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "ATcCGRoY87cSJESCXbHXEX6CDWQxepAViUvVnNsELhRu");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "36crUN2YvuPXEpRXNmdtv5W1veeXHZvMqSe4Egqu4Ski9FHtbdizagf9Kfj8e7sD4S"
            "e5YCqQQ2vpUuKGycM8WhF9");
  // m/0'/1'
  child = child->DeriveHardenedChild(1);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "b1d0bad404bf35da785a64ca1ac54b2617211d2777696fbffaf208f746ae84f2");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "1932a5270f335bed617d5b935c80aedb1a35bd9fc1e31acafd5372c30f5c1187");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "2hMz2f8WbLw5m2icKR2WVrcizvnguw8xaAnXjaeohuHQ");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "4ZCMMnibQjY732c95g1bK5aWzZpR3H1HAqGMeh1B4xpcUWkpxJyUVfwqVBjft1bpRA"
            "WjiJTaUUPWFJEqKWn6cVZp");
  // m/0'/1'/2'
  child = child->DeriveHardenedChild(2);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "92a5b23c0b8a99e37d07df3fb9966917f5d06e02ddbd909c7e184371463e9fc9");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "ae98736566d30ed0e9d2f4486a64bc95740d89c7db33f52121f8ea8f76ff0fc1");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "CkYmXLvWehLXBzUAJ3g3wsfc5QjoCtWtSydquF7HDxXS");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3w45HeUP7x8DhVFxmUwsww19XUdxNZeTuMQQBFJCXAaGtYLvjUVvWovNX7aKpjp5pa"
            "YERPr1jgWEvGeemRm2bCBJ");
  // m/0'/1'/2'/2'
  child = child->DeriveHardenedChild(2);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "30d1dc7e5fc04c31219ab25a27ae00b50f6fd66622f6e9c913253d6511d1e662");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "8abae2d66361c879b900d204ad2cc4984fa2aa344dd7ddc46007329ac76c429c");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "ALYYdMp2jVV4HGsZZPfLy1BQLMHL2CQG5XHpzr2XiHCw");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "ycUieXQauHN9msp7beGkDcUPwF4g3YhzqUXwVihv8PJbF96Eyeh1PFTxhzP4AaXt5U"
            "QCR3mVsrs8AiPCKMCLs2s");
  // m/0'/1'/2'/2'/1000000000'
  child = child->DeriveHardenedChild(1000000000);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e8793");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "3c24da049451555d51a7014a37337aa4e12d41e485abccfa46b47dfb2af54b7a");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "53n47S4RT9ozx5KrpH6uYfdnAjrTBJri8qZJBvRfw1Bf");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "3sVsV9myuRDg4rio4n3ftoP3NsUDzjVk6i8WiTg9veDsiALQjt9QEfUckJkutYUgzm"
            "wwz55D49JUDFic5Fu2gDjX");
}

// https://github.com/satoshilabs/slips/blob/master/slip-0010.md#test-vector-2-for-ed25519
TEST(HDKeyEd25519UnitTest, TestVector2) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(base::HexStringToBytes(
      "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996"
      "93908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
      &bytes));

  // m
  auto master_key_base = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m");
  HDKeyEd25519* master_key = static_cast<HDKeyEd25519*>(master_key_base.get());
  EXPECT_EQ(HexEncodeLower(master_key->GetPrivateKeyAsSpan()),
            "171cb88b1b3c1db25add599712e36245d75bc65a1a5c9e18d76f9f2b1eab4012");
  EXPECT_EQ(HexEncodeLower(master_key->GetPublicKeyAsSpan()),
            "8fe9693f8fa62a4305a140b9764c5ee01e455963744fe18204b4fb948249308a");
  EXPECT_EQ(master_key->GetBase58EncodedPublicKey(),
            "AgmjPHe5Qs4VakvXHGnd6NsYjaxt4suMUtf39TayrSfb");
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "ToTfZTGTYncQcR7P7PduNLKDd8sNHMKsB7td24qCZzwzzZ65fA8y7Ht3o7nwojMzoV"
            "rD9M6Y7vPKznLJPjpwgLZ");
  // m/0'/2147483647'/1'/2147483646'/2'
  auto child = master_key->GenerateFromSeedAndPath(
      bytes, "m/0'/2147483647'/1'/2147483646'/2'");
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "551d333177df541ad876a60ea71f00447931c0a9da16f227c11ea080d7391b8d");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "47150c75db263559a70d5778bf36abbab30fb061ad69f69ece61a72b0cfa4fc0");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "5nUZbtNefYa7tWHdpQApxsjPLtTZpKuZYnKDsd2dXADu");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "2hhXd52y2dVVJGUkr6kikm3LcMQcPSwhWaB1GDU7nAMRWXbjAuG1G9mjdSETpAEAJ1"
            "vV9nQrvhARxQDc6iEEbpU7");
  // m/0'
  child = master_key->DeriveHardenedChild(0);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "1559eb2bbec5790b0c65d8693e4d0875b1747f4970ae8b650486ed7470845635");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "86fab68dcb57aa196c77c5f264f215a112c22a912c10d123b0d03c3c28ef1037");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "A5uN5c31sqKK4x82gXeHzsBFpBTTusPDHBZT111V3u4i");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "Rm2NBwPiLaJoWaetGVz9Jy1T477CS2FfM4Q5JmWgCLRhX54T8zHX57RH6LgR2kRXTc"
            "DwPVMAQi4nxFVH2DJiXkA");
  // m/0'/2147483647'
  child = child->DeriveHardenedChild(2147483647);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "ea4f5bfe8694d8bb74b7b59404632fd5968b774ed545e810de9c32a4fb4192f4");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "5ba3b9ac6e90e83effcd25ac4e58a1365a9e35a3d3ae5eb07b9e4d90bcf7506d");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "7AiuCW2Mg2vRAHsrVmsM3uFky4XRaXHqqcemSp6Bract");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "5gi27AKyRrB5rvX9yPT39WpRak9B5QAXSZLvFDoqb7nQGhKLTqhTLeUgax4FVGGurZ"
            "PQNjRX6N9sn4o7f5rSAeWG");
  // m/0'/2147483647'/1'
  child = child->DeriveHardenedChild(1);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "3757c7577170179c7868353ada796c839135b3d30554bbb74a4b1e4a5a58505c");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "2e66aa57069c86cc18249aecf5cb5a9cebbfd6fadeab056254763874a9352b45");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "488Z1Z7moahUL7Np2JMrApWbWwdUEBzSfEioz9vj7vCc");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "27BCpwH2qcy7ANSVAisHjBN3CQyfzKyV4qcSet2YP1X5aCsoKS9kwcxqvJdVNcBWN3"
            "xuKFviozGBrUsbhXumYa9z");
  // m/0'/2147483647'/1'/2147483646'
  child = child->DeriveHardenedChild(2147483646);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "5837736c89570de861ebc173b1086da4f505d4adb387c6a1b1342d5e4ac9ec72");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "e33c0f7d81d843c572275f287498e8d408654fdf0d1e065b84e2e6f157aab09b");
  EXPECT_EQ(child->GetBase58EncodedPublicKey(),
            "GJ2famWaTaWgT5oYvi1dqA7cvtoKMzyje1Pcx1bL9Nsc");
  EXPECT_EQ(child->GetBase58EncodedKeypair(),
            "2mJCNeA9JefF3B2gikqrR22BWa2ETCZNwijZvDn7XktHRVYj7sXhTt93sr7SqkBUp8"
            "h2pLb6V3nzpYN4mB9paeDQ");
  // m/0'/2147483647'/1'/2147483646'/2'
  child = child->DeriveHardenedChild(2);
  EXPECT_EQ(HexEncodeLower(child->GetPrivateKeyAsSpan()),
            "551d333177df541ad876a60ea71f00447931c0a9da16f227c11ea080d7391b8d");
  EXPECT_EQ(HexEncodeLower(child->GetPublicKeyAsSpan()),
            "47150c75db263559a70d5778bf36abbab30fb061ad69f69ece61a72b0cfa4fc0");
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
  auto master_key = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m");

  // path contains normal index
  auto child1 = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m/0'/1'/2'/3'/4");
  EXPECT_FALSE(child1);

  // invalid path
  auto child2 = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "BRAVE0'1'2'3'4'");
  EXPECT_FALSE(child2);

  // index is too big for hardened index
  auto child3 = master_key->DeriveHardenedChild(0x80000000);
  EXPECT_FALSE(child3);
}

TEST(HDKeyEd25519UnitTest, EncodePrivateKeyForExport) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));
  auto master_key = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m");
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");
}

TEST(HDKeyEd25519UnitTest, SignAndVerify) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));
  auto key = HDKeyEd25519::GenerateFromSeedAndPath(bytes, "m");
  const std::vector<uint8_t> msg_a(32, 0x00);
  const std::vector<uint8_t> msg_b(32, 0x08);
  const auto sig_a = key->Sign(msg_a);
  const auto sig_b = key->Sign(msg_b);

  EXPECT_TRUE(VerifySignature(*key, msg_a, sig_a));
  EXPECT_TRUE(VerifySignature(*key, msg_b, sig_b));

  // wrong signature
  EXPECT_FALSE(VerifySignature(*key, msg_a, sig_b));
  EXPECT_FALSE(VerifySignature(*key, msg_b, sig_a));
}

TEST(HDKeyEd25519UnitTest, GenerateFromPrivateKey) {
  std::array<uint8_t, 64> key_pair;
  ASSERT_TRUE(base::HexStringToSpan(
      "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e7"
      "a4b2856bfec510abab89753fac1ac0e1112364e7d250545963f135f2a33188ed",
      base::span(key_pair)));
  auto master_key = HDKeyEd25519::GenerateFromKeyPair(key_pair);
  EXPECT_EQ(master_key->GetBase58EncodedKeypair(),
            "sCzwsBKmKtk5Hgb4YUJAduQ5nmJq4GTyzCXhrKonAGaexa83MgSZuTSMS6TSZTndnC"
            "YbQtaJQKLXET9jVjepWXe");
  EXPECT_EQ(master_key->GetBase58EncodedPublicKey(),
            "C5ukMV73nk32h52MjxtnZXTrrr7rupD9CTDDRnYYDRYQ");

  EXPECT_EQ(
      "6260C446B2BA429541722F6BAABBEEAF3D1B5981DA326A2DB66804B5EACE770D"
      "58CFBA0E0D409A3054E80C00505215C7CCD7A040F23364005A47CDE7FCED1400",
      base::HexEncode(master_key->Sign(base::byte_span_from_cstring("hello"))));

  key_pair.back() = 0;
  EXPECT_FALSE(HDKeyEd25519::GenerateFromKeyPair(key_pair));
}

}  // namespace brave_wallet
