
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/bitcoin_utils.h"

#include <optional>

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
    std::optional<BitcoinAddressType> expected_type;
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
          std::nullopt,  // We don't support unkown witness versions
      },
      {
          "BC1SW50QA3JX3S", "6002", "751e",
          std::nullopt,  // We don't support unkown witness versions
      },
      {
          "bc1zw508d6qejxtdg4y5r3zarvaryvg6kdaj", "5210",
          "751e76e8199196d454941c45d1b3a323",
          std::nullopt,  // We don't support unkown witness versions
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
    if (test_case.expected_type) {
      ASSERT_TRUE(decoded_address);
      EXPECT_EQ(decoded_address->testnet, testnet);
      EXPECT_EQ(decoded_address->pubkey_hash, pubkey_hash);
      EXPECT_EQ(decoded_address->address_type, test_case.expected_type);
    } else {
      ASSERT_FALSE(decoded_address);
    }
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

// https://github.com/sparrowwallet/drongo/blob/9ae1f68dc42529085edcc8c10d9bcfdbf9639448/src/test/java/com/sparrowwallet/drongo/address/AddressTest.java#L12
TEST(BitcoinUtilsUnitTest, DecodeBitcoinAddressSparrowWalletTestVectors) {
  std::optional<DecodedBitcoinAddress> decoded;

  auto decoded1 =
      DecodeBitcoinAddress("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");
  ASSERT_TRUE(decoded1);
  EXPECT_EQ(BitcoinAddressType::kWitnessV0PubkeyHash, decoded1->address_type);
  EXPECT_EQ("751E76E8199196D454941C45D1B3A323F1433BD6",
            base::HexEncode(decoded1->pubkey_hash));
  EXPECT_FALSE(decoded1->testnet);

  auto decoded2 = DecodeBitcoinAddress(
      "bc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qccfmv3");
  ASSERT_TRUE(decoded2);
  EXPECT_EQ(BitcoinAddressType::kWitnessV0ScriptHash, decoded2->address_type);
  EXPECT_EQ("1863143C14C5166804BD19203356DA136C985678CD4D27A1B8C6329604903262",
            base::HexEncode(decoded2->pubkey_hash));
  EXPECT_FALSE(decoded2->testnet);

  auto decoded3 = DecodeBitcoinAddress("19Sp9dLinHy3dKo2Xxj53ouuZWAoVGGhg8");
  ASSERT_TRUE(decoded3);
  EXPECT_EQ(BitcoinAddressType::kPubkeyHash, decoded3->address_type);
  EXPECT_EQ("5CA2C35A3E6C0816E0B7628E2786ED404CA6DE86",
            base::HexEncode(decoded3->pubkey_hash));
  EXPECT_FALSE(decoded3->testnet);

  auto decoded4 = DecodeBitcoinAddress("34jnjFM4SbaB7Q8aMtNDG849RQ1gUYgpgo");
  ASSERT_TRUE(decoded4);
  EXPECT_EQ(BitcoinAddressType::kScriptHash, decoded4->address_type);
  EXPECT_EQ("216E9C5E38E5B0A9E7E3B0107F7BEB3F42426417",
            base::HexEncode(decoded4->pubkey_hash));
  EXPECT_FALSE(decoded4->testnet);

  auto decoded5 =
      DecodeBitcoinAddress("tb1qawkzyj2l5yck5jq4wyhkc4837x088580y9uyk8");
  ASSERT_TRUE(decoded5);
  EXPECT_EQ(BitcoinAddressType::kWitnessV0PubkeyHash, decoded5->address_type);
  EXPECT_EQ("EBAC22495FA1316A4815712F6C54F1F19E73D0EF",
            base::HexEncode(decoded5->pubkey_hash));
  EXPECT_TRUE(decoded5->testnet);

  auto decoded6 = DecodeBitcoinAddress(
      "tb1q8kdkthp5a6vfrdas84efkpv25ul3s9wpzc755cra8av48xq4a7wsjcsdma");
  ASSERT_TRUE(decoded6);
  EXPECT_EQ(BitcoinAddressType::kWitnessV0ScriptHash, decoded6->address_type);
  EXPECT_EQ("3D9B65DC34EE9891B7B03D729B058AA73F1815C1163D4A607D3F59539815EF9D",
            base::HexEncode(decoded6->pubkey_hash));
  EXPECT_TRUE(decoded6->testnet);

  auto decoded7 = DecodeBitcoinAddress("mng6R5oLWBBo8iFWU9Mx4zFy5pWhrWMeW2");
  ASSERT_TRUE(decoded7);
  EXPECT_EQ(BitcoinAddressType::kPubkeyHash, decoded7->address_type);
  EXPECT_EQ("4E836743C76D796ECBEB4B24F41A6EC950B84BDE",
            base::HexEncode(decoded7->pubkey_hash));
  EXPECT_TRUE(decoded7->testnet);

  auto decoded8 = DecodeBitcoinAddress("n1S1rnnZm3RdW9iuAF6Hjk3gLZWGc59zDi");
  ASSERT_TRUE(decoded8);
  EXPECT_EQ(BitcoinAddressType::kPubkeyHash, decoded8->address_type);
  EXPECT_EQ("DA73694A73FE98752FA9CD52AC6E4EC40D6386FC",
            base::HexEncode(decoded8->pubkey_hash));
  EXPECT_TRUE(decoded8->testnet);

  auto decoded9 = DecodeBitcoinAddress("2NCZUtUt6gzXyBiPEQi5yQyrgR6f6F6Ki6A");
  ASSERT_TRUE(decoded9);
  EXPECT_EQ(BitcoinAddressType::kScriptHash, decoded9->address_type);
  EXPECT_EQ("D3DE50A7F247252146D43FDFED2F4A1C4376FD57",
            base::HexEncode(decoded9->pubkey_hash));
  EXPECT_TRUE(decoded9->testnet);

  // No segnet support
  auto decoded10 = DecodeBitcoinAddress("2NCZUtUt6gzXyBiPEQi5yQyrgR6f6F6Ki6A");
  ASSERT_TRUE(decoded10);
  EXPECT_EQ(BitcoinAddressType::kScriptHash, decoded10->address_type);
  EXPECT_EQ("D3DE50A7F247252146D43FDFED2F4A1C4376FD57",
            base::HexEncode(decoded10->pubkey_hash));
  EXPECT_TRUE(decoded10->testnet);

  auto decoded11 = DecodeBitcoinAddress(
      "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0");
  ASSERT_TRUE(decoded11);
  EXPECT_EQ(BitcoinAddressType::kWitnessV1Taproot, decoded11->address_type);
  EXPECT_EQ("79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
            base::HexEncode(decoded11->pubkey_hash));
  EXPECT_FALSE(decoded11->testnet);

  auto decoded12 = DecodeBitcoinAddress(
      "tb1pqqqqp399et2xygdj5xreqhjjvcmzhxw4aywxecjdzew6hylgvsesf3hn0c");
  ASSERT_TRUE(decoded12);
  EXPECT_EQ(BitcoinAddressType::kWitnessV1Taproot, decoded12->address_type);
  EXPECT_EQ("000000C4A5CAD46221B2A187905E5266362B99D5E91C6CE24D165DAB93E86433",
            base::HexEncode(decoded12->pubkey_hash));
  EXPECT_TRUE(decoded12->testnet);
}

TEST(BitcoinUtilsUnitTest, DecodeBitcoinAddressBitcoinCoreTestVectors) {
  // Bitcoin-core test vectors are effectively covered by
  // BitcoinSerializer.AddressToScriptPubkey_BitcoinCoreTestVectors
  SUCCEED();
}

TEST(BitcoinUtilsUnitTest, DecodeBitcoinAddress) {
  auto decoded_address =
      DecodeBitcoinAddress("tb1q36yalctnxxznp7znt0cdlvsx4y7vs2nquwvjw8");
  ASSERT_TRUE(decoded_address);
  EXPECT_EQ(base::HexEncode(decoded_address->pubkey_hash),
            "8E89DFE173318530F8535BF0DFB206A93CC82A60");
  EXPECT_TRUE(decoded_address->testnet);
  EXPECT_EQ(decoded_address->address_type,
            BitcoinAddressType::kWitnessV0PubkeyHash);
  EXPECT_TRUE(decoded_address->testnet);

  decoded_address = DecodeBitcoinAddress(
      "bc1qc7slrfxkknqcq2jevvvkdgvrt8080852dfjewde450xdlk4ugp7szw5tk9");
  ASSERT_TRUE(decoded_address);
  EXPECT_EQ(base::HexEncode(decoded_address->pubkey_hash),
            "C7A1F1A4D6B4C1802A59631966A18359DE779E8A6A65973735A3CCDFDABC407D");
  EXPECT_FALSE(decoded_address->testnet);
  EXPECT_EQ(decoded_address->address_type,
            BitcoinAddressType::kWitnessV0ScriptHash);
  EXPECT_FALSE(decoded_address->testnet);

  // Unknown witness version.
  decoded_address =
      DecodeBitcoinAddress("tb18gg6x4mkdqy9l7pn8mq523l7ur3uzrh4ydcgzuf");
  ASSERT_FALSE(decoded_address);

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

TEST(BitcoinUtilsUnitTest, ApplyFeeRate) {
  EXPECT_EQ(0u, ApplyFeeRate(0, 0));
  EXPECT_EQ(3261u, ApplyFeeRate(23.123, 141));
}

}  // namespace brave_wallet
