/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_data_builder.h"

#include "brave/components/brave_wallet/common/hex_utils.h"
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

TEST(EthCallDataBuilderTest, Allowance) {
  std::string data;
  Allowance("0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
            "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", &data);
  ASSERT_EQ(
      data,
      "0xdd62ed3e000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a");
}

}  // namespace erc20

namespace erc721 {

TEST(EthCallDataBuilderTest, TransferFromOrSafeTransferFrom) {
  std::string data;
  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0xf", &token_id));
  TransferFromOrSafeTransferFrom(
      false, "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", token_id, &data);
  ASSERT_EQ(
      data,
      "0x23b872dd000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000f");

  TransferFromOrSafeTransferFrom(
      true, "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460f",
      "0xBFb30a082f650C2A15D0632f0e87bE4F8e64460a", token_id, &data);
  ASSERT_EQ(
      data,
      "0x42842e0e000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e6446"
      "0f000000000000000000000000BFb30a082f650C2A15D0632f0e87bE4F8e64460a000000"
      "000000000000000000000000000000000000000000000000000000000f");
}

TEST(EthCallDataBuilderTest, OwnerOf) {
  std::string data;
  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0xf", &token_id));
  OwnerOf(token_id, &data);
  ASSERT_EQ(data,
            "0x6352211e00000000000000000000000000000000000000000000000000000000"
            "0000000f");
}

TEST(EthCallDataBuilderTest, TokenUri) {
  std::string data;
  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0xf", &token_id));
  TokenUri(token_id, &data);
  ASSERT_EQ(data,
            "0xc87b56dd00000000000000000000000000000000000000000000000000000000"
            "0000000f");
}

}  // namespace erc721

namespace erc1155 {

TEST(EthCallDataBuilderTest, SafeTransferFrom) {
  std::string data;

  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0x0", &token_id));

  uint256_t value;
  ASSERT_TRUE(HexValueToUint256("0x1", &value));
  SafeTransferFrom("0x16e4476c8fddc552e3b1c4b8b56261d85977fe52",
                   "0xe53960ca4712052615a6eb4c635b44514b2b42b6", token_id,
                   value, &data);
  ASSERT_EQ(data,
            "0xf242432a00000000000000000000000016e4476c8fddc552e3b1c4b8b56261d8"
            "5977fe52000000000000000000000000e53960ca4712052615a6eb4c635b44514b"
            "2b42b6000000000000000000000000000000000000000000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000000"
            "0100000000000000000000000000000000000000000000000000000000000000a0"
            "0000000000000000000000000000000000000000000000000000000000000000");
}

TEST(EthCallDataBuilderTest, BalanceOf) {
  std::string data;
  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0x1", &token_id));
  BalanceOf("0xe53960ca4712052615a6eb4c635b44514b2b42b6", token_id, &data);
  ASSERT_EQ(
      data,
      "0x00fdd58e000000000000000000000000e53960ca4712052615a6eb4c635b44514b2b42"
      "b60000000000000000000000000000000000000000000000000000000000000001");
}

TEST(EthCallDataBuilderTest, Uri) {
  std::string data;
  uint256_t token_id;
  ASSERT_TRUE(HexValueToUint256("0xf", &token_id));
  Uri(token_id, &data);
  ASSERT_EQ(data,
            "0x0e89341c00000000000000000000000000000000000000000000000000000000"
            "0000000f");
}

}  // namespace erc1155

namespace erc165 {

TEST(EthCallDataBuilderTest, SupportsInterface) {
  std::string data;
  EXPECT_TRUE(SupportsInterface("0xffffffff", &data));
  EXPECT_EQ(data,
            "0x01ffc9a7ffffffff000000000000000000000000000000000000000000000000"
            "00000000");

  EXPECT_FALSE(SupportsInterface("", &data));
  EXPECT_FALSE(SupportsInterface("123", &data));
  EXPECT_FALSE(SupportsInterface("0xff", &data));
}

}  // namespace erc165

namespace unstoppable_domains {

TEST(EthCallDataBuilderTest, GetMany) {
  std::vector<std::string> keys = {"crypto.ETH.address"};
  auto data = GetMany(keys, "brave.crypto");
  EXPECT_TRUE(data);
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
  data = GetMany(keys, "brave.crypto");
  EXPECT_TRUE(data);
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

TEST(EthCallDataBuilderTest, Get) {
  auto data = Get("crypto.ETH.address", "brave.crypto");
  EXPECT_TRUE(data);
  EXPECT_EQ(data,
            "0x1be5e7ed"
            "0000000000000000000000000000000000000000000000000000000000000040"
            // Name hash of brave.crypto.
            "77252571a99feee8f5e6b2f0c8b705407d395adc00b3c8ebcc7c19b2ea850013"
            // Count of "crypto.ETH.address"
            "0000000000000000000000000000000000000000000000000000000000000012"
            // Encoding of "crypto.ETH.address"
            "63727970746f2e4554482e616464726573730000000000000000000000000000");
}

}  // namespace unstoppable_domains

namespace ens {

TEST(EthCallDataBuilderTest, Resolver) {
  std::string data;
  EXPECT_TRUE(Resolver("brantly.eth", &data));
  EXPECT_EQ(data,
            "0x0178b8bf"
            // Name hash of brantly.eth.
            "43fcd34d8589090581e1d2bdcf5dc17feb05b2006401fb1c3fdded335a465b51");
}

TEST(EthCallDataBuilderTest, ContentHash) {
  std::string data;
  EXPECT_TRUE(ContentHash("brantly.eth", &data));
  EXPECT_EQ(data,
            "0xbc1c58d1"
            "43fcd34d8589090581e1d2bdcf5dc17feb05b2006401fb1c3fdded335a465b51");
}

TEST(EthCallDataBuilderTest, Addr) {
  std::string data;
  EXPECT_TRUE(Addr("brantly.eth", &data));
  EXPECT_EQ(data,
            "0x3b3b57de"
            "43fcd34d8589090581e1d2bdcf5dc17feb05b2006401fb1c3fdded335a465b51");
}

}  // namespace ens

}  // namespace brave_wallet
