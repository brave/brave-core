/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_address_utils.h"

#include <stdint.h>

#include <optional>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

struct EnsAddrVector {
  mojom::CoinType coin;
  const char* record_hex;
  // nullptr if the record must be rejected.
  const char* expected;
};

// From @ensdomains/address-encoder 3.0.0 (src/coin/<coin>.test.ts), the
// encoder the ENS app uses to write records, plus hand-written rejects.
constexpr EnsAddrVector kEnsAddrVectors[] = {
    // Rejected: Byron
    {mojom::CoinType::ADA,
     "83581cba970ad36654d8dd8f74274b733452ddeab9a62a397746be3c42ccdda000",
     nullptr},
    // Rejected: Byron
    {mojom::CoinType::ADA,
     "83581c22fb739e75d1f34748c2f03365ac909aff4d5a47bad7b4231c62a949a101581e581"
     "c2374b70ae27c8cc324be7b97285bdde6eeb78354cf6d1110baa37da000",
     nullptr},
    // base key/key
    {mojom::CoinType::ADA,
     "019493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a"
     "06a3acbc34f8c46003c69fe79a3628cefa9c47251",
     "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8c"
     "c3sq835lu7drv2xwl2wywfgse35a3x"},
    // base script/key
    {mojom::CoinType::ADA,
     "11c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f337b62cfff6403a"
     "06a3acbc34f8c46003c69fe79a3628cefa9c47251",
     "addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8c"
     "c3sq835lu7drv2xwl2wywfgs9yc0hh"},
    // base key/script
    {mojom::CoinType::ADA,
     "219493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8ec37b1b5dc0669f1"
     "d3c61a6fddb2e8fde96be87b881c60bce8e8d542f",
     "addr1yx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdj"
     "ar77j6lg0wypcc9uar5d2shs2z78ve"},
    // base script/script
    {mojom::CoinType::ADA,
     "31c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542fc37b1b5dc0669f1"
     "d3c61a6fddb2e8fde96be87b881c60bce8e8d542f",
     "addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdj"
     "ar77j6lg0wypcc9uar5d2shskhj42g"},
    // pointer key
    {mojom::CoinType::ADA,
     "419493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e8198bd431b03",
     "addr1gx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrzqf96k"},
    // pointer script
    {mojom::CoinType::ADA,
     "51c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f8198bd431b03",
     "addr128phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcrtw79hu"},
    // enterprise key
    {mojom::CoinType::ADA,
     "619493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e",
     "addr1vx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzers66hrl8"},
    // enterprise script
    {mojom::CoinType::ADA,
     "71c37b1b5dc0669f1d3c61a6fddb2e8fde96be87b881c60bce8e8d542f",
     "addr1w8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcyjy7wx"},
    // P2PKH
    {mojom::CoinType::BTC, "76a91462e907b15cbf27d5425399ebf6f0fb50ebb88f1888ac",
     "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"},
    // P2SH
    {mojom::CoinType::BTC, "a91462e907b15cbf27d5425399ebf6f0fb50ebb88f1887",
     "3Ai1JZ8pdJb2ksieUV8FsxSNVJCpoPi8W6"},
    // P2WPKH
    {mojom::CoinType::BTC, "0014751e76e8199196d454941c45d1b3a323f1433bd6",
     "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"},
    // Rejected: script/witness program Brave cannot send to
    {mojom::CoinType::BTC,
     "5128751e76e8199196d454941c45d1b3a323f1433bd6751e76e8199196d454941c45d1b3a"
     "323f1433bd6",
     nullptr},
    // Rejected: script/witness program Brave cannot send to
    {mojom::CoinType::BTC, "6002751e", nullptr},
    // Rejected: script/witness program Brave cannot send to
    {mojom::CoinType::BTC, "5210751e76e8199196d454941c45d1b3a323", nullptr},
    // P2WSH
    {mojom::CoinType::BTC,
     "00201863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262",
     "bc1qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qccfmv3"},
    // P2TR
    {mojom::CoinType::BTC,
     "512079be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798",
     "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0"},
    // DOT
    {mojom::CoinType::DOT,
     "0aff6865635ae11013a83835c019d44ec3f865145943f487ae82a8e7bed3a66b",
     "1FRMM8PEiWXYax7rpS6X4XZX1aAAxSWx1CrKTyrVYhV24fg"},
    // Rejected: protocol 0
    {mojom::CoinType::FIL, "009601", nullptr},
    // secp256k1
    {mojom::CoinType::FIL, "01ea0f0ea039b291a0f08fd179e0556a8c3277c0d3",
     "f15ihq5ibzwki2b4ep2f46avlkrqzhpqgtga7pdrq"},
    // Rejected: protocol 2
    {mojom::CoinType::FIL, "02e54dea4f9bc5b47d261819826d5e1fbf8bc5503b",
     nullptr},
    // BLS
    {mojom::CoinType::FIL,
     "03ad58df696e2d4e91ea86c881e938ba4ea81b395e12797b84b9cf314b9546705e839c7a9"
     "9d606b247ddb4f9ac7a3414dd",
     "f3vvmn62lofvhjd2ugzca6sof2j2ubwok6cj4xxbfzz4yuxfkgobpihhd2thlanmsh3w2ptld"
     "2gqkn2jvlss4a"},
    // SOL
    {mojom::CoinType::SOL,
     "8a11e71b96cabbe3216e3153b09694f39fc85022cbc076f79846a3ab4d8c1991",
     "AHy6YZA8BsHgQfVkk7MbwpAN94iyN7Nf1zN4nPqUN32Q"},
    // SOL
    {mojom::CoinType::SOL,
     "18f9d8d877393bbbe8d697a8a2e52879cc7e84f467656d1cce6bab5a8d2637ec",
     "2gVkYWexTHR5Hb2aLeQN3tnngvWzisFKXDUPrgMHpdST"},
    // SOL
    {mojom::CoinType::SOL,
     "a8ed08e3e8fe204de45e7295cc1ad53db096621b878f8c546e5c09f5e48f70b4",
     "CNR8RPMxjY28VsPA6KFq3B8PUdZnrTSC5HSFwKPBR29Z"},
    // P2PKH
    {mojom::CoinType::ZEC, "76a914bc18e286d40706de62928155d6167bf30719857888ac",
     "t1b2ArRwLq6KbdJFzJVYPxgUVT1d9QuBzTf"},
    // Rejected: P2SH
    {mojom::CoinType::ZEC, "a9147d46a730d31f97b1930d3368a967c309bd4d136a87",
     nullptr},
    // Rejected: Sapling shielded
    {mojom::CoinType::ZEC,
     "75b321aa2e074a15858cf47178b865d7ee744c868e8e585b645aff2256644cb45c931fe59"
     "d630ee388c764",
     nullptr},
    // ETH
    {mojom::CoinType::ETH, "314159265dd8dbb310642f98f50c066173c1259b",
     "0x314159265dd8dbb310642f98f50c066173c1259b"},
    // Rejected: zero address
    {mojom::CoinType::ETH, "0000000000000000000000000000000000000000", nullptr},
    // Rejected: 19 bytes
    {mojom::CoinType::ETH, "314159265dd8dbb310642f98f50c066173c125", nullptr},
    // Rejected: 31 bytes
    {mojom::CoinType::SOL,
     "8a11e71b96cabbe3216e3153b09694f39fc85022cbc076f79846a3ab4d8c19", nullptr},
    // Rejected: 33 bytes
    {mojom::CoinType::DOT,
     "0aff6865635ae11013a83835c019d44ec3f865145943f487ae82a8e7bed3a66b00",
     nullptr},
    // Rejected: truncated P2PKH script
    {mojom::CoinType::BTC, "76a91462e907b15cbf27d5425399ebf6f0fb50ebb88f18",
     nullptr},
    // Rejected: protocol byte with no payload
    {mojom::CoinType::FIL, "01", nullptr},
    // Rejected: base address with short payload
    {mojom::CoinType::ADA, "019493315cd92eb5d8c4304e67b7e16ae36d61d345",
     nullptr},
    // Rejected: testnet network tag on the mainnet coin type
    {mojom::CoinType::ADA,
     "009493315cd92eb5d8c4304e67b7e16ae36d61d34502694657811a2c8e337b62cfff6403a"
     "06a3acbc34f8c46003c69fe79a3628cefa9c47251",
     nullptr},
    // Rejected: stake-only (reward) address
    {mojom::CoinType::ADA,
     "e1337b62cfff6403a06a3acbc34f8c46003c69fe79a3628cefa9c47251", nullptr},
};

std::vector<uint8_t> FromHex(const char* hex) {
  std::vector<uint8_t> bytes;
  EXPECT_TRUE(base::HexStringToBytes(hex, &bytes)) << hex;
  return bytes;
}

}  // namespace

TEST(EnsAddressUtilsTest, GetEnsCoinType_EthereumMainnet) {
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, mojom::kMainnetChainId), 60u);
  // Matched by value, not by string.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x01"), 60u);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_OtherEvmChainsUseEnsip11) {
  // 0x80000000 | chainId.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x2105"),
            2147492101u);  // Base
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0xa4b1"),
            2147525809u);  // Arbitrum One
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0xa"),
            2147483658u);  // OP Mainnet
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x89"),
            2147483785u);  // Polygon
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0xa86a"),
            2147526762u);  // Avalanche C-Chain, not SLIP-44 9000
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH,
                           mojom::kFilecoinEthereumMainnetChainId),
            2147483962u);  // Filecoin EVM, not FIL 461
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x7fffffff"), 0xffffffffu);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_EthereumL1TestnetsUseMainnet) {
  // ENSIP-19: L1 testnets may use coin type 60.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, mojom::kSepoliaChainId), 60u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x88bb0"), 60u);  // Hoodi
  // L2 testnets use ENSIP-11.
  EXPECT_EQ(
      GetEnsCoinType(mojom::CoinType::ETH, mojom::kBaseSepoliaTestnetChainId),
      0x80000000u | 0x14a34u);
  EXPECT_EQ(
      GetEnsCoinType(mojom::CoinType::ETH, mojom::kArbitrumSepoliaChainId),
      0x80000000u | 0x66eeeu);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_UnsupportedEvmChainIds) {
  // Larger than 31 bits, e.g. Palm.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x80000000"), std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x2a15c308d"), std::nullopt);
  // 0x80000000 alone is ENSIP-19's default EVM record, not a chain.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x0"), std::nullopt);
  // Malformed.
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, ""), std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x"), std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "2105"), std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0xzz"), std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ETH, "0x0x2105"), std::nullopt);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_NonEvmMainnetsUseSlip44) {
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::SOL, mojom::kSolanaMainnet), 501u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::BTC, mojom::kBitcoinMainnet), 0u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ZEC, mojom::kZCashMainnet), 133u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ADA, mojom::kCardanoMainnet),
            1815u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::DOT, mojom::kPolkadotMainnet),
            354u);
  EXPECT_EQ(
      GetEnsCoinType(mojom::CoinType::DOT, mojom::kPolkadotMainnetAssetHub),
      354u);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::FIL, mojom::kFilecoinMainnet),
            461u);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_NonEvmTestnetsAreUnsupported) {
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::SOL, mojom::kSolanaTestnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::SOL, mojom::kSolanaDevnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::BTC, mojom::kBitcoinTestnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ZEC, mojom::kZCashTestnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::ADA, mojom::kCardanoTestnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::DOT, mojom::kPolkadotTestnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::FIL, mojom::kFilecoinTestnet),
            std::nullopt);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_OtherPolkadotNetworksAreUnsupported) {
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::DOT, mojom::kPolkadotAcala),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::DOT, mojom::kPolkadotKusamaHub),
            std::nullopt);
}

TEST(EnsAddressUtilsTest, GetEnsCoinType_MainnetIdOfAnotherCoin) {
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::BTC, mojom::kZCashMainnet),
            std::nullopt);
  EXPECT_EQ(GetEnsCoinType(mojom::CoinType::SOL, mojom::kMainnetChainId),
            std::nullopt);
}

TEST(EnsAddressUtilsTest, EnsAddrRecordToAddress_AddressEncoderVectors) {
  for (const auto& vector : kEnsAddrVectors) {
    SCOPED_TRACE(testing::Message()
                 << "coin " << vector.coin << " record " << vector.record_hex);
    auto result =
        EnsAddrRecordToAddress(vector.coin, FromHex(vector.record_hex));
    if (vector.expected) {
      EXPECT_EQ(result, vector.expected);
    } else {
      EXPECT_EQ(result, std::nullopt);
    }
  }
}

TEST(EnsAddressUtilsTest, EnsAddrRecordToAddress_EmptyRecord) {
  for (auto coin :
       {mojom::CoinType::ETH, mojom::CoinType::SOL, mojom::CoinType::BTC,
        mojom::CoinType::ZEC, mojom::CoinType::ADA, mojom::CoinType::DOT,
        mojom::CoinType::FIL}) {
    SCOPED_TRACE(testing::Message() << "coin " << coin);
    EXPECT_EQ(EnsAddrRecordToAddress(coin, {}), std::nullopt);
  }
}

TEST(EnsAddressUtilsTest, EnsAddrRecordToAddress_EvmRecordIsChainAgnostic) {
  // The same 20 bytes are returned for every EVM chain. The ENS coin type
  // picks the record; decoding doesn't depend on the chain.
  EXPECT_EQ(EnsAddrRecordToAddress(
                mojom::CoinType::ETH,
                FromHex("314159265dd8dbb310642f98f50c066173c1259b")),
            "0x314159265dd8dbb310642f98f50c066173c1259b");
}

TEST(EnsAddressUtilsTest, EnsAddrRecordToAddress_FilDelegatedIsRejected) {
  // A well-formed protocol 4 (f410) record: protocol byte + 20-byte payload.
  // FilAddress::FromBytes accepts it. address-encoder can't parse f410
  // addresses and writes these bytes only from a nonstandard "f4..." string,
  // which Brave would show as a different f410 address.
  std::vector<uint8_t> record = {4};
  record.resize(21, 0xab);
  EXPECT_EQ(EnsAddrRecordToAddress(mojom::CoinType::FIL, record), std::nullopt);
}

}  // namespace brave_wallet
