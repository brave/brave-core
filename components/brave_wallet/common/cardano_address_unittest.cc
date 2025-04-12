/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cardano_address.h"

#include <array>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

inline constexpr char kPaymentPartHash[] =
    "9493315CD92EB5D8C4304E67B7E16AE36D61D34502694657811A2C8E";
inline constexpr char kStakePartHash[] =
    "337B62CFFF6403A06A3ACBC34F8C46003C69FE79A3628CEFA9C47251";

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

  auto unsupported = std::to_array({
      /*kBasePaymentKeyStakeKey,*/
      kBasePaymentScriptStakeKey,
      kBasePaymentKeyStakeScript,
      kBasePaymentScriptStakeScript,
      kPointerKey,
      kPointerScript,
      kEnterpriseKey,
      kEnterpriseScript,
      kRewardKey,
      kRewardScript,
      /*kTestnetBasePaymentKeyStakeKey,*/
      kTestnetBasePaymentScriptStakeKey,
      kTestnetBasePaymentKeyStakeScript,
      kTestnetBasePaymentScriptStakeScript,
      kTestnetPointerKey,
      kTestnetPointerScript,
      kTestnetEnterpriseKey,
      kTestnetEnterpriseScript,
      kTestnetRewardKey,
      kTestnetRewardScript,
      kByronMainnetYoroi,
      kByronTestnetDaedalus,
  });

  for (auto* address : unsupported) {
    EXPECT_FALSE(CardanoAddress::FromString(address))
        << testing::Message(address);
  }

  auto base_payment_key_stake_key =
      CardanoAddress::FromString(kBasePaymentKeyStakeKey);
  ASSERT_TRUE(base_payment_key_stake_key);
  EXPECT_FALSE(base_payment_key_stake_key->IsTestnet());
  EXPECT_EQ(base_payment_key_stake_key->ToString(), kBasePaymentKeyStakeKey);
  EXPECT_EQ(base::HexEncode(base_payment_key_stake_key->ToCborBytes()),
            std::string("01") + kPaymentPartHash + kStakePartHash);
  EXPECT_EQ(
      base_payment_key_stake_key,
      CardanoAddress::FromParts(false, test::HexToArray<28>(kPaymentPartHash),
                                test::HexToArray<28>(kStakePartHash)));

  auto testnet_base_payment_key_stake_key =
      CardanoAddress::FromString(kTestnetBasePaymentKeyStakeKey);
  ASSERT_TRUE(testnet_base_payment_key_stake_key);
  EXPECT_TRUE(testnet_base_payment_key_stake_key->IsTestnet());
  EXPECT_EQ(testnet_base_payment_key_stake_key->ToString(),
            kTestnetBasePaymentKeyStakeKey);
  EXPECT_EQ(base::HexEncode(testnet_base_payment_key_stake_key->ToCborBytes()),
            std::string("00") + kPaymentPartHash + kStakePartHash);
  EXPECT_EQ(
      testnet_base_payment_key_stake_key,
      CardanoAddress::FromParts(true, test::HexToArray<28>(kPaymentPartHash),
                                test::HexToArray<28>(kStakePartHash)));
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
