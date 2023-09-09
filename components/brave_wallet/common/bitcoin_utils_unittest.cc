
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bitcoin_utils.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BitcoinUtilsUnitTest, Bip0173TestVectors1) {
  // https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#examples

  std::vector<uint8_t> pubkey;
  ASSERT_TRUE(base::HexStringToBytes(
      "0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
      &pubkey));

  EXPECT_EQ(PubkeyToSegwitAddress(pubkey, true),
            "tb1qw508d6qejxtdg4y5r3zarvary0c5xw7kxpjzsx");
  EXPECT_EQ(PubkeyToSegwitAddress(pubkey, false),
            "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");

  // TODO(apaymyshev): support P2WSH.
}

TEST(BitcoinUtilsUnitTest, Bip0173TestVectors2) {
  // https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki#test-vectors

  struct ValidTestCase {
    const char* address;
    const char* opcodes;  // not relevant parts of scriptpubkey.
    const char* pubkey_hash_hex;
    BitcoinAddressType expected_type;
  } test_cases[] = {
      {
          "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4",
          "0014",
          "751e76e8199196d454941c45d1b3a323f1433bd6",
          BitcoinAddressType::kWitnessV0PubkeyHash,
      },
      {
          "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sl5k7",
          "0020",
          "1863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
          BitcoinAddressType::kWitnessV0ScriptHash,
      },
      {
          "bc1pw508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k"
          "7grplx",
          "5128",
          "751e76e8199196d454941c45d1b3a323f1433bd6751e76e8199196d454941c45d1b3"
          "a323f1433bd6",
          BitcoinAddressType::kWitnessUnknown,
      },
      {
          "BC1SW50QA3JX3S",
          "6002",
          "751e",
          BitcoinAddressType::kWitnessUnknown,
      },
      {
          "bc1zw508d6qejxtdg4y5r3zarvaryvg6kdaj",
          "5210",
          "751e76e8199196d454941c45d1b3a323",
          BitcoinAddressType::kWitnessUnknown,
      },
      {
          "tb1qqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesrxh6hy",
          "0020",
          "000000c4a5cad46221b2a187905e5266362b99d5e91c6ce24d165dab93e86433",
          BitcoinAddressType::kWitnessV0ScriptHash,
      }};

  for (auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.address);
    bool testnet = base::StartsWith(test_case.address, "tb",
                                    base::CompareCase::INSENSITIVE_ASCII);
    std::vector<uint8_t> pubkey_hash;
    ASSERT_TRUE(
        base::HexStringToBytes(test_case.pubkey_hash_hex, &pubkey_hash));
    auto decoded_address = DecodeBitcoinAddress(test_case.address);
    ASSERT_TRUE(decoded_address);
    EXPECT_EQ(decoded_address->testnet, testnet);
    EXPECT_EQ(decoded_address->pubkey_hash, pubkey_hash);
    EXPECT_EQ(decoded_address->address_type, test_case.expected_type);
  }

  const char* invalid_test_cases[] = {
      "tc1qw508d6qejxtdg4y5r3zarvary0c5xw7kg3g4ty",
      "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5",
      "BC13W508D6QEJXTDG4Y5R3ZARVARY0C5XW7KN40WF2",
      "bc1rw5uspcuh",
      "bc10w508d6qejxtdg4y5r3zarvary0c5xw7kw508d6qejxtdg4y5r3zarvary0c5xw7k"
      "w5rl"
      "js"
      "90",
      "BC1QR508D6QEJXTDG4Y5R3ZARVARYV98GJ9P",
      "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q0sL5k7",
      "bc1zw508d6qejxtdg4y5r3zarvaryvqyzf3du",
      "tb1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3pjxtptv",
      "bc1gmk9yu",
  };

  for (auto* test_case : invalid_test_cases) {
    SCOPED_TRACE(test_case);

    EXPECT_FALSE(DecodeBitcoinAddress(test_case));
  }
}

TEST(BitcoinUtilsUnitTest, DecodeBitcoinAddress) {
  auto decoded_address =
      DecodeBitcoinAddress("tb1q36yalctnxxznp7znt0cdlvsx4y7vs2nquwvjw8");
  ASSERT_TRUE(decoded_address);
  EXPECT_EQ(base::HexEncode(decoded_address->pubkey_hash),
            "8E89DFE173318530F8535BF0DFB206A93CC82A60");
  EXPECT_EQ(decoded_address->address_type,
            BitcoinAddressType::kWitnessV0PubkeyHash);
  EXPECT_EQ(decoded_address->witness_version, 0u);
  EXPECT_TRUE(decoded_address->testnet);

  decoded_address = DecodeBitcoinAddress(
      "bc1qc7slrfxkknqcq2jevvvkdgvrt8080852dfjewde450xdlk4ugp7szw5tk9");
  ASSERT_TRUE(decoded_address);
  EXPECT_EQ(base::HexEncode(decoded_address->pubkey_hash),
            "C7A1F1A4D6B4C1802A59631966A18359DE779E8A6A65973735A3CCDFDABC407D");
  EXPECT_EQ(decoded_address->address_type,
            BitcoinAddressType::kWitnessV0ScriptHash);
  EXPECT_EQ(decoded_address->witness_version, 0u);
  EXPECT_FALSE(decoded_address->testnet);

  // Unknown witness version.
  decoded_address =
      DecodeBitcoinAddress("tb18gg6x4mkdqy9l7pn8mq523l7ur3uzrh4ydcgzuf");
  ASSERT_TRUE(decoded_address);
  EXPECT_EQ(base::HexEncode(decoded_address->pubkey_hash),
            "42346AEECD010BFF0667D828A8FFDC1C7821DEA4");
  EXPECT_EQ(decoded_address->address_type, BitcoinAddressType::kWitnessUnknown);
  EXPECT_EQ(decoded_address->witness_version, 7u);
  EXPECT_TRUE(decoded_address->testnet);

  // Invalid size for witness v0.
  decoded_address =
      DecodeBitcoinAddress("bc1qgg6x4mkdqy9l7pn8mq523l7ur3uzrhsvmsnvu");
  ASSERT_FALSE(decoded_address);
}

TEST(BitcoinUtilsUnitTest, PubkeyToSegwitAddress) {
  std::vector<uint8_t> pubkey;
  ASSERT_TRUE(base::HexStringToBytes("8E89DFE173318530F8535BF0DFB206A93CC82A60",
                                     &pubkey));

  EXPECT_EQ("bc1qgg6x4mkdqy9l7pn8mq523l7ur3uzrh4ylkdfdz",
            PubkeyToSegwitAddress(pubkey, false));
  EXPECT_EQ("tb1qgg6x4mkdqy9l7pn8mq523l7ur3uzrh4y4sk6k3",
            PubkeyToSegwitAddress(pubkey, true));
}

}  // namespace brave_wallet
