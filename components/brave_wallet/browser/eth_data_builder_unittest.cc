/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_builder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace erc20 {

TEST(EthCallDataBuilderTest, Transfer) {
  std::string data;
  Transfer("0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f", 0xde0b6b3a7640000,
           &data);
  ASSERT_EQ(
      data,
      "0xa9059cbb000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000000de0b6b3a7640000");
}

TEST(EthCallDataBuilderTest, BalanceOf) {
  std::string data;
  BalanceOf("0x4e02f254184E904300e0775E4b8eeCB1", &data);
  ASSERT_EQ(data,
            "0x70a08231000000000000000000000000000000004e02f254184E904300e0775E"
            "4b8eeCB1");
}

TEST(EthCallDataBuilderTest, Approve) {
  std::string data;
  Approve("0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f", 0xde0b6b3a7640000,
          &data);
  ASSERT_EQ(
      data,
      "0x095ea7b3000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f0000000000000000000000000000000000000000000000000de0b6b3a7640000");
}

}  // namespace erc20

namespace unstoppable_domains {

TEST(EthCallDataBuilderTest, GetMany) {
  std::string data;
  std::vector<std::string> keys = {"crypto.ETH.address"};
  EXPECT_TRUE(GetMany(keys, "brave.crypto", &data));
  EXPECT_EQ(data,
            "0x1bd8cc1a"
            // Offset to the start of keys array.
            "0000000000000000000000000000000000000000000000000000000000000040"
            // Name hash of brave.crypto.
            "77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013"
            // Count of keys array.
            "0000000000000000000000000000000000000000000000000000000000000001"
            // Offset to elements of keys array.
            "0000000000000000000000000000000000000000000000000000000000000020"
            // Count of "crypto.ETH.address"
            "0000000000000000000000000000000000000000000000000000000000000012"
            // Encoding of "crypto.ETH.address"
            "63727970746f2e4554482e616464726573730000000000000000000000000000");

  keys = {"dweb.ipfs.hash", "ipfs.html.value", "browser.redirect_url",
          "ipfs.redirect_domain.value"};
  EXPECT_TRUE(GetMany(keys, "brave.crypto", &data));
  EXPECT_EQ(data,
            "0x1bd8cc1a"
            // Offset to the start of keys array.
            "0000000000000000000000000000000000000000000000000000000000000040"
            // Name hash of brave.crypto.
            "77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013"
            // Count of keys array.
            "0000000000000000000000000000000000000000000000000000000000000004"
            // Offsets to elements of keys array.
            "0000000000000000000000000000000000000000000000000000000000000080"
            "00000000000000000000000000000000000000000000000000000000000000c0"
            "0000000000000000000000000000000000000000000000000000000000000100"
            "0000000000000000000000000000000000000000000000000000000000000140"
            // Count of "dweb.ipfs.hash".
            "000000000000000000000000000000000000000000000000000000000000000e"
            // Encoding of "dweb.ipfs.hash".
            "647765622e697066732e68617368000000000000000000000000000000000000"
            // Count of "ipfs.html.value".
            "000000000000000000000000000000000000000000000000000000000000000f"
            // Encoding of "ipfs.html.value".
            "697066732e68746d6c2e76616c75650000000000000000000000000000000000"
            // Count of "browser.redirect_url".
            "0000000000000000000000000000000000000000000000000000000000000014"
            // Encoding of "browser.redirect_url".
            "62726f777365722e72656469726563745f75726c000000000000000000000000"
            // Count of "ipfs.redirect_domain.value".
            "000000000000000000000000000000000000000000000000000000000000001a"
            // Encoding of "ipfs.redirect_domain.value".
            "697066732e72656469726563745f646f6d61696e2e76616c7565000000000000");
}

}  // namespace unstoppable_domains

}  // namespace brave_wallet
