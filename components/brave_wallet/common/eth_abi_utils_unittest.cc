/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::ElementsAreArray;

namespace {
// Captured response of `error OffchainLookup(address sender, string[] urls,
// bytes callData, bytes4 callbackFunction, bytes extraData)` for
// offchainexample.eth. https://eips.ethereum.org/EIPS/eip-3668

// clang-format off
  constexpr char kOffchainLookupResponse[] =
      "0x556f1830"
      "000000000000000000000000c1735677a60884abbcf72295e88d47764beda282"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "0000000000000000000000000000000000000000000000000000000000000160"
      "f4d4d2f800000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000280"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000047"
      "68747470733a2f2f6f6666636861696e2d7265736f6c7665722d6578616d706c"
      "652e75632e722e61707073706f742e636f6d2f7b73656e6465727d2f7b646174"
      "617d2e6a736f6e00000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000e4"
      "9061b92300000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "0000008000000000000000000000000000000000000000000000000000000000"
      "000000150f6f6666636861696e6578616d706c65036574680000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000000243b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafb"
      "eadf58e6beaaa16c000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000000000000000e4"
      "9061b92300000000000000000000000000000000000000000000000000000000"
      "0000004000000000000000000000000000000000000000000000000000000000"
      "0000008000000000000000000000000000000000000000000000000000000000"
      "000000150f6f6666636861696e6578616d706c65036574680000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "000000243b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafb"
      "eadf58e6beaaa16c000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000";
// clang-format on
}  // namespace

namespace brave_wallet::eth_abi {
TEST(EthAbiUtilsTest, OffchainLookup) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(kOffchainLookupResponse, &bytes));

  auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);

  EXPECT_EQ("0x556f1830", ToHex(selector));

  EXPECT_EQ(ExtractAddressFromTuple(args, 0).ToHex(),
            "0xc1735677a60884abbcf72295e88d47764beda282");

  EXPECT_THAT(
      *ExtractStringArrayFromTuple(args, 1),
      ElementsAreArray({"https://offchain-resolver-example.uc.r.appspot.com/"
                        "{sender}/{data}.json"}));

  EXPECT_EQ(ToHex(*ExtractBytesFromTuple(args, 2)),
            "0x9061b92300000000000000000000000000000000000000000000000000000000"
            "000000400000000000000000000000000000000000000000000000000000000000"
            "000080000000000000000000000000000000000000000000000000000000000000"
            "00150f6f6666636861696e6578616d706c65036574680000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000024"
            "3b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafbeadf58e6be"
            "aaa16c00000000000000000000000000000000000000000000000000000000");

  EXPECT_EQ(ToHex(*ExtractFixedBytesFromTuple(args, 4, 3)), "0xf4d4d2f8");

  EXPECT_EQ(ToHex(*ExtractBytesFromTuple(args, 4)),
            "0x9061b92300000000000000000000000000000000000000000000000000000000"
            "000000400000000000000000000000000000000000000000000000000000000000"
            "000080000000000000000000000000000000000000000000000000000000000000"
            "00150f6f6666636861696e6578616d706c65036574680000000000000000000000"
            "000000000000000000000000000000000000000000000000000000000000000024"
            "3b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafbeadf58e6be"
            "aaa16c00000000000000000000000000000000000000000000000000000000");
}

TEST(EthAbiUtilsTest, OffchainLookupBy1Test) {
  std::vector<uint8_t> bytes_base;
  ASSERT_TRUE(PrefixedHexStringToBytes(kOffchainLookupResponse, &bytes_base));

  // Try to alter each byte and expect no crashes.
  for (auto i = 0u; i < bytes_base.size(); ++i) {
    for (auto d : {-1, 1}) {
      auto bytes = bytes_base;
      bytes[i] += d;

      auto [_, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);

      ExtractAddressFromTuple(args, 0);
      ExtractStringArrayFromTuple(args, 1);
      ExtractBytesFromTuple(args, 2);
      ExtractFixedBytesFromTuple(args, 4, 3);
      ExtractBytesFromTuple(args, 4);
    }
  }
}

TEST(EthAbiUtilsTest, DISABLED_BytesToUint256) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractFunctionSelectorAndArgsFromCall) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractAddress) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractAddressFromTuple) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractBytes) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractString) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractStringArrayFromTuple) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractBytesFromTuple) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_ExtractFixedBytesFromTuple) {
  GTEST_FAIL();
}

TEST(EthAbiUtilsTest, DISABLED_EncodeCall) {
  GTEST_FAIL();
}

}  // namespace brave_wallet::eth_abi
