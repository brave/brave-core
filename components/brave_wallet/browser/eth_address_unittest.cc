/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthAddressUnitTest, FromPublicKey) {
  std::vector<uint8_t> public_key;
  EXPECT_TRUE(base::HexStringToBytes(
      "3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf0"
      "66d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1d",
      &public_key));
  EthAddress address = EthAddress::FromPublicKey(public_key);
  EXPECT_EQ(address.ToHex(), "0x2f015c60e0be116b1f0cd534704db9c92118fb6a");

  public_key.clear();
  EXPECT_TRUE(base::HexStringToBytes(
      "3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf0"
      "66d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae744",
      &public_key));
  address = EthAddress::FromPublicKey(public_key);
  EXPECT_TRUE(address.IsEmpty());

  public_key.clear();
  EXPECT_TRUE(base::HexStringToBytes(
      "3a443d8381a6798a70c6ff9304bdc8cb0163c23211d11628fae52ef9e0dca11a001cf0"
      "66d56a8156fc201cd5df8a36ef694eecd258903fca7086c1fae7441e1dffff",
      &public_key));
  address = EthAddress::FromPublicKey(public_key);
  EXPECT_TRUE(address.IsEmpty());
}

TEST(EthAddressUnitTest, FromHex) {
  EthAddress address =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthAddress address2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EXPECT_EQ(address.ToHex(), "0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EXPECT_EQ(address2.ToHex(), "0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EXPECT_EQ(address, address2);
  EthAddress address3 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  EXPECT_EQ(address3.ToHex(), "0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  EXPECT_NE(address, address3);

  address = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb");
  EXPECT_TRUE(address.IsEmpty());

  address = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a11");
  EXPECT_TRUE(address.IsEmpty());

  address = EthAddress::FromHex("0x123");
  EXPECT_TRUE(address.IsEmpty());
}

TEST(EthAddressUnitTest, ToChecksumAddress) {
  const char* eip55_cases[] = {
      // All caps
      "0x52908400098527886E0F7030069857D2E4169EE7",
      "0x8617E340B3D01FA5F11F306F4090FD50E238070D",
      // All Lower
      "0xde709f2102306220921060314715629080e2fb77",
      "0x27b1fdb04752bbc536007a920d24acb045561c26",
      // Normal
      "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed",
      "0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359",
      "0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB",
      "0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb",
  };

  for (size_t i = 0; i < sizeof(eip55_cases) / sizeof(eip55_cases[0]); ++i) {
    EthAddress address =
        EthAddress::FromHex(base::ToLowerASCII(eip55_cases[i]));
    EXPECT_EQ(address.ToChecksumAddress(), eip55_cases[i]);
  }

  const struct {
    const char* address;
    uint256_t chain_id;
  } eip1191_cases[] = {
      // eth_mainnet
      {"0x27b1fdb04752bbc536007a920d24acb045561c26", 1},
      {"0x3599689E6292b81B2d85451025146515070129Bb", 1},
      {"0x42712D45473476b98452f434e72461577D686318", 1},
      {"0x52908400098527886E0F7030069857D2E4169EE7", 1},
      {"0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed", 1},
      {"0x6549f4939460DE12611948b3f82b88C3C8975323", 1},
      {"0x66f9664f97F2b50F62D13eA064982f936dE76657", 1},
      {"0x8617E340B3D01FA5F11F306F4090FD50E238070D", 1},
      {"0x88021160C5C792225E4E5452585947470010289D", 1},
      {"0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb", 1},
      {"0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB", 1},
      {"0xde709f2102306220921060314715629080e2fb77", 1},
      {"0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359", 1},
      // rinkeby
      {"0x27b1fdb04752bbc536007a920d24acb045561c26", 4},
      {"0x3599689E6292b81B2d85451025146515070129Bb", 4},
      {"0x42712D45473476b98452f434e72461577D686318", 4},
      {"0x52908400098527886E0F7030069857D2E4169EE7", 4},
      {"0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed", 4},
      {"0x6549f4939460DE12611948b3f82b88C3C8975323", 4},
      {"0x66f9664f97F2b50F62D13eA064982f936dE76657", 4},
      {"0x8617E340B3D01FA5F11F306F4090FD50E238070D", 4},
      {"0x88021160C5C792225E4E5452585947470010289D", 4},
      {"0xD1220A0cf47c7B9Be7A2E6BA89F429762e7b9aDb", 4},
      {"0xdbF03B407c01E7cD3CBea99509d93f8DDDC8C6FB", 4},
      {"0xde709f2102306220921060314715629080e2fb77", 4},
      {"0xfB6916095ca1df60bB79Ce92cE3Ea74c37c5d359", 4},
      // rsk_mainnet
      {"0x27b1FdB04752BBc536007A920D24ACB045561c26", 30},
      {"0x3599689E6292B81B2D85451025146515070129Bb", 30},
      {"0x42712D45473476B98452f434E72461577d686318", 30},
      {"0x52908400098527886E0F7030069857D2E4169ee7", 30},
      {"0x5aaEB6053f3e94c9b9a09f33669435E7ef1bEAeD", 30},
      {"0x6549F4939460DE12611948B3F82B88C3C8975323", 30},
      {"0x66F9664f97f2B50F62d13EA064982F936de76657", 30},
      {"0x8617E340b3D01Fa5f11f306f4090fd50E238070D", 30},
      {"0x88021160c5C792225E4E5452585947470010289d", 30},
      {"0xD1220A0Cf47c7B9BE7a2e6ba89F429762E7B9adB", 30},
      {"0xDBF03B407c01E7CD3cBea99509D93F8Dddc8C6FB", 30},
      {"0xDe709F2102306220921060314715629080e2FB77", 30},
      {"0xFb6916095cA1Df60bb79ce92cE3EA74c37c5d359", 30},
      // rsk_testnet
      {"0x27B1FdB04752BbC536007a920D24acB045561C26", 31},
      {"0x3599689e6292b81b2D85451025146515070129Bb", 31},
      {"0x42712D45473476B98452F434E72461577D686318", 31},
      {"0x52908400098527886E0F7030069857D2e4169EE7", 31},
      {"0x5aAeb6053F3e94c9b9A09F33669435E7EF1BEaEd", 31},
      {"0x6549f4939460dE12611948b3f82b88C3c8975323", 31},
      {"0x66f9664F97F2b50f62d13eA064982F936DE76657", 31},
      {"0x8617e340b3D01fa5F11f306F4090Fd50e238070d", 31},
      {"0x88021160c5C792225E4E5452585947470010289d", 31},
      {"0xd1220a0CF47c7B9Be7A2E6Ba89f429762E7b9adB", 31},
      {"0xdbF03B407C01E7cd3cbEa99509D93f8dDDc8C6fB", 31},
      {"0xDE709F2102306220921060314715629080e2Fb77", 31},
      {"0xFb6916095CA1dF60bb79CE92ce3Ea74C37c5D359", 31},
  };

  for (size_t i = 0; i < sizeof(eip1191_cases) / sizeof(eip1191_cases[0]);
       ++i) {
    EthAddress address =
        EthAddress::FromHex(base::ToLowerASCII(eip1191_cases[i].address));
    EXPECT_EQ(address.ToChecksumAddress(eip1191_cases[i].chain_id),
              eip1191_cases[i].address);
  }
}

}  // namespace brave_wallet
