/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cardano_address.h"

#include <array>

#include "base/strings/string_number_conversions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

inline constexpr char kPaymentPartHash[] =
    "9493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E";
inline constexpr char kStakePartHash[] =
    "337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251";
inline constexpr char kScriptPartHash[] =
    "C37B1B5DC0669F1D3C61A6FDDB2E8FDE96BE87B881C60BCE8E8D542F";
inline constexpr char kPointer[] = "8198BD431B03";

// https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/util-dev/src/Cip19TestVectors.ts#L31
inline constexpr char kBasePaymentKeyStakeKey[] =
    "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc"
    "3sq835lu7drv2xwl2wywfgse35a3x";
inline constexpr char kBasePaymentScriptStakeKey[] =
    "addr1z8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc"
    "3sq835lu7drv2xwl2wywfgs9yc0hh";
inline constexpr char kBasePaymentKeyStakeScript[] =
    "addr1yx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdja"
    "r77j6lg0wypcc9uar5d2shs2z78ve";
inline constexpr char kBasePaymentScriptStakeScript[] =
    "addr1x8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdja"
    "r77j6lg0wypcc9uar5d2shskhj42g";
inline constexpr char kPointerKey[] =
    "addr1gx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrzqf96k";
inline constexpr char kPointerScript[] =
    "addr128phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcrtw79hu";
inline constexpr char kEnterpriseKey[] =
    "addr1vx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzers66hrl8";
inline constexpr char kEnterpriseScript[] =
    "addr1w8phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcyjy7wx";
inline constexpr char kRewardKey[] =
    "stake1uyehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gh6ffgw";
inline constexpr char kRewardScript[] =
    "stake178phkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcccycj5";
inline constexpr char kTestnetBasePaymentKeyStakeKey[] =
    "addr_"
    "test1qz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktcd8cc"
    "3sq835lu7drv2xwl2wywfgs68faae";
inline constexpr char kTestnetBasePaymentScriptStakeKey[] =
    "addr_"
    "test1zrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gten0d3vllmyqwsx5wktcd8cc"
    "3sq835lu7drv2xwl2wywfgsxj90mg";
inline constexpr char kTestnetBasePaymentKeyStakeScript[] =
    "addr_"
    "test1yz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerkr0vd4msrxnuwnccdxlhdja"
    "r77j6lg0wypcc9uar5d2shsf5r8qx";
inline constexpr char kTestnetBasePaymentScriptStakeScript[] =
    "addr_"
    "test1xrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gt7r0vd4msrxnuwnccdxlhdja"
    "r77j6lg0wypcc9uar5d2shs4p04xh";
inline constexpr char kTestnetPointerKey[] =
    "addr_test1gz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer5pnz75xxcrdw5vky";
inline constexpr char kTestnetPointerScript[] =
    "addr_test12rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtupnz75xxcryqrvmw";
inline constexpr char kTestnetEnterpriseKey[] =
    "addr_test1vz2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzerspjrlsz";
inline constexpr char kTestnetEnterpriseScript[] =
    "addr_test1wrphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcl6szpr";
inline constexpr char kTestnetRewardKey[] =
    "stake_test1uqehkck0lajq8gr28t9uxnuvgcqrc6070x3k9r8048z8y5gssrtvn";
inline constexpr char kTestnetRewardScript[] =
    "stake_test17rphkx6acpnf78fuvxn0mkew3l0fd058hzquvz7w36x4gtcljw6kf";
inline constexpr char kByronMainnetYoroi[] =
    "Ae2tdPwUPEZFRbyhz3cpfC2CumGzNkFBN2L42rcUc2yjQpEkxDbkPodpMAi";
inline constexpr char kByronTestnetDaedalus[] =
    "37btjrVyb4KEB2STADSsj3MYSAdj52X5FrFWpw2r7Wmj2GDzXjFRsHWuZqrw7zSkwopv8Ci3VW"
    "eg6bisU9dgJxW5hb2MZYeduNKbQJrqz3zVBsu9nT";
}  // namespace

// https://github.com/input-output-hk/cardano-js-sdk/blob/5bc90ee9f24d89db6ea4191d705e7383d52fef6a/packages/util-dev/src/Cip19TestVectors.ts#L31
TEST(CardanoAddress, TestVectors) {
  EXPECT_FALSE(CardanoAddress::FromString(""));

  struct TestCase {
    std::string address;
    CardanoAddress::AddressType address_type;
    bool testnet;
    std::string hex_cbor_header;
    std::string hex_part1;
    std::string hex_part2;
  };

  TestCase test_cases[] = {
      {
          kBasePaymentKeyStakeKey,
          CardanoAddress::AddressType::kPaymentKeyHashStakeKeyHash,
          false,
          std::string("01"),
          kPaymentPartHash,
          kStakePartHash,
      },
      {
          kBasePaymentScriptStakeKey,
          CardanoAddress::AddressType::kScriptHashStakeKeyHash,
          false,
          std::string("11"),
          kScriptPartHash,
          kStakePartHash,
      },
      {
          kBasePaymentKeyStakeScript,
          CardanoAddress::AddressType::kPaymentKeyHashScriptHash,
          false,
          std::string("21"),
          kPaymentPartHash,
          kScriptPartHash,
      },
      {
          kBasePaymentScriptStakeScript,
          CardanoAddress::AddressType::kScriptHashScriptHash,
          false,
          std::string("31"),
          kScriptPartHash,
          kScriptPartHash,
      },
      {
          kPointerKey,
          CardanoAddress::AddressType::kPaymentKeyHashPointer,
          false,
          std::string("41"),
          kPaymentPartHash,
          kPointer,
      },
      {
          kPointerScript,
          CardanoAddress::AddressType::kScriptHashPointer,
          false,
          std::string("51"),
          kScriptPartHash,
          kPointer,
      },
      {
          kEnterpriseKey,
          CardanoAddress::AddressType::kPaymentKeyHashNoDelegation,
          false,
          std::string("61"),
          kPaymentPartHash,
      },
      {
          kEnterpriseScript,
          CardanoAddress::AddressType::kScriptHashNoDelegation,
          false,
          std::string("71"),
          kScriptPartHash,
      },
      {
          kRewardKey,
          CardanoAddress::AddressType::kNoPaymentStakeHash,
          false,
          std::string("E1"),
          kStakePartHash,
      },
      {
          kRewardScript,
          CardanoAddress::AddressType::kNoPaymentScriptHash,
          false,
          std::string("F1"),
          kScriptPartHash,
      },
      {
          kTestnetBasePaymentKeyStakeKey,
          CardanoAddress::AddressType::kPaymentKeyHashStakeKeyHash,
          true,
          std::string("00"),
          kPaymentPartHash,
          kStakePartHash,
      },
      {
          kTestnetBasePaymentScriptStakeKey,
          CardanoAddress::AddressType::kScriptHashStakeKeyHash,
          true,
          std::string("10"),
          kScriptPartHash,
          kStakePartHash,
      },
      {
          kTestnetBasePaymentKeyStakeScript,
          CardanoAddress::AddressType::kPaymentKeyHashScriptHash,
          true,
          std::string("20"),
          kPaymentPartHash,
          kScriptPartHash,
      },
      {
          kTestnetBasePaymentScriptStakeScript,
          CardanoAddress::AddressType::kScriptHashScriptHash,
          true,
          std::string("30"),
          kScriptPartHash,
          kScriptPartHash,
      },
      {
          kTestnetPointerKey,
          CardanoAddress::AddressType::kPaymentKeyHashPointer,
          true,
          std::string("40"),
          kPaymentPartHash,
          kPointer,
      },
      {
          kTestnetPointerScript,
          CardanoAddress::AddressType::kScriptHashPointer,
          true,
          std::string("50"),
          kScriptPartHash,
          kPointer,
      },
      {
          kTestnetEnterpriseKey,
          CardanoAddress::AddressType::kPaymentKeyHashNoDelegation,
          true,
          std::string("60"),
          kPaymentPartHash,
      },
      {
          kTestnetEnterpriseScript,
          CardanoAddress::AddressType::kScriptHashNoDelegation,
          true,
          std::string("70"),
          kScriptPartHash,
      },
      {
          kTestnetRewardKey,
          CardanoAddress::AddressType::kNoPaymentStakeHash,
          true,
          std::string("E0"),
          kStakePartHash,
      },
      {
          kTestnetRewardScript,
          CardanoAddress::AddressType::kNoPaymentScriptHash,
          true,
          std::string("F0"),
          kScriptPartHash,
      },

  };

  for (auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.address);

    auto addr = CardanoAddress::FromString(test_case.address);
    ASSERT_TRUE(addr);
    EXPECT_EQ(addr->IsTestnet(), test_case.testnet);
    EXPECT_EQ(addr->ToString(), test_case.address);
    EXPECT_EQ(
        base::HexEncode(addr->ToCborBytes()),
        test_case.hex_cbor_header + test_case.hex_part1 + test_case.hex_part2);

    std::vector<uint8_t> payload;
    base::HexStringToBytes(test_case.hex_part1 + test_case.hex_part2, &payload);
    EXPECT_EQ(addr->ToString(),
              CardanoAddress::FromPayload(
                  test_case.address_type,
                  test_case.testnet ? CardanoAddress::NetworkTag::kTestnets
                                    : CardanoAddress::NetworkTag::kMainnet,
                  payload)
                  ->ToString());
  }

  auto unsupported = std::to_array({

      kByronMainnetYoroi,
      kByronTestnetDaedalus,
  });

  for (auto* address : unsupported) {
    EXPECT_FALSE(CardanoAddress::FromString(address))
        << testing::Message(address);
  }
}

TEST(CardanoAddress, InvalidInput) {
  auto invalid_cases = std::to_array({
      "",
      "1",
      "a",
      "addr2qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktc"
      "d8cc3sq835lu7drv2xwl2wywfgse35a3x",
      "addr1qx2fxv2umyhttkxyxp8x0dlpdt3k6cwng5pxj3jhsydzer3n0d3vllmyqwsx5wktc"
      "d8cc3sq835lu7drv2xwl2wywfgse35a3xse35a3x",
  });

  for (auto* address : invalid_cases) {
    EXPECT_FALSE(CardanoAddress::FromString(address))
        << testing::Message(address);
  }
}

}  // namespace brave_wallet
