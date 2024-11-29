/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::ElementsAreArray;

namespace {
// Captured response of `error OffchainLookup(address sender, string[] urls,
// bytes callData, bytes4 callbackFunction, bytes extraData)` for
// offchainexample.eth. https://eips.ethereum.org/EIPS/eip-3668

std::string GetOffchainLookupResponse() {
  constexpr char kOffchainLookupResponse[] =
      // clang-format off
      "556f1830"
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
  return kOffchainLookupResponse;
}

std::vector<uint8_t> ToBytes(const std::string& hex) {
  return *brave_wallet::PrefixedHexStringToBytes("0x" + hex);
}
}  // namespace

namespace brave_wallet::eth_abi {

TEST(EthAbiUtilsTest, OffchainLookup) {
  auto bytes = ToBytes(GetOffchainLookupResponse());

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
  std::vector<uint8_t> bytes_base = ToBytes(GetOffchainLookupResponse());

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

TEST(EthAbiUtilsTest, ExtractFunctionSelectorAndArgsFromCall) {
  {
    std::vector<uint8_t> bytes = ToBytes(GetOffchainLookupResponse());
    auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);
    EXPECT_EQ(GetOffchainLookupResponse().substr(0, 8),
              ToHex(selector).substr(2));
    EXPECT_EQ(GetOffchainLookupResponse().substr(8), ToHex(args).substr(2));
  }

  {
    // Only selector.
    std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04};
    auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);
    EXPECT_TRUE(base::ranges::equal(bytes, selector));
    EXPECT_TRUE(args.empty());
  }

  {
    // Not enough for selector.
    std::vector<uint8_t> bytes = {0x01, 0x02, 0x03};
    auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);
    EXPECT_TRUE(selector.empty());
    EXPECT_TRUE(args.empty());
  }

  {
    // Bad args alignment.
    std::vector<uint8_t> bytes = {0x01, 0x02, 0x03, 0x04, 0x05};
    auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);
    EXPECT_TRUE(selector.empty());
    EXPECT_TRUE(args.empty());
  }

  {
    // Empty case.
    std::vector<uint8_t> bytes = {};
    auto [selector, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);
    EXPECT_TRUE(selector.empty());
    EXPECT_TRUE(args.empty());
  }
}

TEST(EthAbiUtilsTest, ExtractAddress) {
  {
    auto bytes = ToBytes(
        "000000000000000000000000c1735677a60884abbcf72295e88d47764beda282");
    EXPECT_EQ(ExtractAddress(bytes).ToHex(),
              "0xc1735677a60884abbcf72295e88d47764beda282");
  }

  {
    // Missing byte.
    auto bytes = ToBytes(
        "0000000000000000000000c1735677a60884abbcf72295e88d47764beda282");
    EXPECT_TRUE(ExtractAddress(bytes).IsEmpty());
  }

  {
    // Extra byte.
    auto bytes = ToBytes(
        "000000000000000000000000c1735677a60884abbcf72295e88d47764beda28200");
    EXPECT_TRUE(ExtractAddress(bytes).IsEmpty());
  }

  {
    // Zero address.
    auto bytes = ToBytes(
        "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(ExtractAddress(bytes).ToHex(),
              "0x0000000000000000000000000000000000000000");
  }

  {  // Empty.
    auto bytes = std::vector<uint8_t>{};
    EXPECT_TRUE(ExtractAddress(bytes).IsEmpty());
  }
}

TEST(EthAbiUtilsTest, ExtractAddressFromTuple) {
  auto bytes = ToBytes(
      "000000000000000000000000c1735677a60884abbcf72295e88d47764beda282"
      "00000000000000000000000000000000000000000000000000000000000000a0");
  EXPECT_EQ(ExtractAddressFromTuple(bytes, 0).ToHex(),
            "0xc1735677a60884abbcf72295e88d47764beda282");
  EXPECT_EQ(ExtractAddressFromTuple(bytes, 1).ToHex(),
            "0x00000000000000000000000000000000000000a0");
  EXPECT_TRUE(ExtractAddressFromTuple(bytes, 2).IsEmpty());

  // Bad alignment.
  bytes.push_back(0);
  EXPECT_TRUE(ExtractAddressFromTuple(bytes, 0).IsEmpty());
  EXPECT_TRUE(ExtractAddressFromTuple(bytes, 1).IsEmpty());
  EXPECT_TRUE(ExtractAddressFromTuple(bytes, 2).IsEmpty());

  // Empty.
  EXPECT_TRUE(ExtractAddressFromTuple({}, 0).IsEmpty());
  EXPECT_TRUE(ExtractAddressFromTuple({}, 1).IsEmpty());
  EXPECT_TRUE(ExtractAddressFromTuple({}, 2).IsEmpty());
}

TEST(EthAbiUtilsTest, ExtractBytes) {
  std::vector<uint8_t> bytes = ToBytes(
      "0000000000000000000000000000000000000000000000000000000000000047"
      "68747470733a2f2f6f6666636861696e2d7265736f6c7665722d6578616d706c"
      "652e75632e722e61707073706f742e636f6d2f7b73656e6465727d2f7b646174"
      "617d2e6a736f6e00000000000000000000000000000000000000000000000000");
  auto extracted_bytes = ExtractBytes(bytes);
  EXPECT_EQ(size_t(0x47), extracted_bytes->size());
  EXPECT_EQ(
      "68747470733a2f2f6f6666636861696e2d7265736f6c7665722d6578616d706c"
      "652e75632e722e61707073706f742e636f6d2f7b73656e6465727d2f7b646174"
      "617d2e6a736f6e",
      ToHex(*extracted_bytes).substr(2));

  // Non-zero padding.
  bytes.back() = 1;
  EXPECT_FALSE(ExtractBytes(bytes));

  // Empty case.
  EXPECT_FALSE(ExtractBytes({}));

  // Bad alignment.
  EXPECT_FALSE(ExtractBytes(ToBytes(
      "00000000000000000000000000000000000000000000000000000000000000")));

  // Empty array.
  {
    auto empty = ToBytes(
        "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_TRUE(ExtractBytes(empty)->empty());
  }

  // One-byte array.
  {
    auto one_byte = ToBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
        "0100000000000000000000000000000000000000000000000000000000000000");

    EXPECT_EQ(std::vector<uint8_t>{1}, *ExtractBytes(one_byte));
  }
}

TEST(EthAbiUtilsTest, ExtractString) {
  std::vector<uint8_t> bytes = ToBytes(
      "0000000000000000000000000000000000000000000000000000000000000047"
      "68747470733a2f2f6f6666636861696e2d7265736f6c7665722d6578616d706c"
      "652e75632e722e61707073706f742e636f6d2f7b73656e6465727d2f7b646174"
      "617d2e6a736f6e00000000000000000000000000000000000000000000000000");
  auto extracted_string = ExtractString(bytes);
  EXPECT_EQ(
      "https://offchain-resolver-example.uc.r.appspot.com/"
      "{sender}/{data}.json",
      *extracted_string);

  // Non-zero padding.
  bytes.back() = 1;
  EXPECT_FALSE(ExtractString(bytes));

  // Empty case.
  EXPECT_FALSE(ExtractString({}));

  // Bad alignment.
  EXPECT_FALSE(ExtractString(ToBytes(
      "00000000000000000000000000000000000000000000000000000000000000")));

  // Empty string.
  {
    auto empty = ToBytes(
        "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_TRUE(ExtractString(empty)->empty());
  }

  // One-char string.
  {
    auto one_byte = ToBytes(
        "0000000000000000000000000000000000000000000000000000000000000001"
        "4100000000000000000000000000000000000000000000000000000000000000");

    EXPECT_EQ("A", *ExtractString(one_byte));
  }
}

TEST(EthAbiUtilsTest, ExtractStringArray) {
  std::vector<std::string> output;
  EXPECT_THAT(
      *ExtractStringArray(ToBytes(
          // count of elements in input array
          "0000000000000000000000000000000000000000000000000000000000000003"
          // offsets to array elements
          "0000000000000000000000000000000000000000000000000000000000000060"
          "00000000000000000000000000000000000000000000000000000000000000a0"
          "00000000000000000000000000000000000000000000000000000000000000e0"
          // count for "one"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "one"
          "6f6e650000000000000000000000000000000000000000000000000000000000"
          // count for "two"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "two"
          "74776f0000000000000000000000000000000000000000000000000000000000"
          // count for "three"
          "0000000000000000000000000000000000000000000000000000000000000005"
          // encoding for "three"
          "7468726565000000000000000000000000000000000000000000000000000000")),
      ElementsAreArray({"one", "two", "three"}));

  EXPECT_THAT(
      *ExtractStringArray(ToBytes(
          "0000000000000000000000000000000000000000000000000000000000000005"
          // offsets to array elements
          "00000000000000000000000000000000000000000000000000000000000000a0"
          "00000000000000000000000000000000000000000000000000000000000000e0"
          "0000000000000000000000000000000000000000000000000000000000000140"
          "0000000000000000000000000000000000000000000000000000000000000180"
          "00000000000000000000000000000000000000000000000000000000000001e0"
          // count for "one"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "one"
          "6f6e650000000000000000000000000000000000000000000000000000000000"
          // count for "one two three four five six seven eight nine"
          "000000000000000000000000000000000000000000000000000000000000002c"
          // encoding for "one two three four five six seven eight nine"
          "6f6e652074776f20746872656520666f75722066697665207369782073657665"
          "6e206569676874206e696e650000000000000000000000000000000000000000"
          // count for "two"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "two"
          "74776f0000000000000000000000000000000000000000000000000000000000"
          // count for "one two three four five six seven eight nine ten"
          "0000000000000000000000000000000000000000000000000000000000000030"
          // encoding for "one two three four five six seven eight nine ten"
          "6f6e652074776f20746872656520666f75722066697665207369782073657665"
          "6e206569676874206e696e652074656e00000000000000000000000000000000"
          // count for "three"
          "0000000000000000000000000000000000000000000000000000000000000005"
          // encoding for "three"
          "7468726565000000000000000000000000000000000000000000000000000000")),
      ElementsAreArray(
          {"one", "one two three four five six seven eight nine", "two",
           "one two three four five six seven eight nine ten", "three"}));

  EXPECT_THAT(
      *ExtractStringArray(ToBytes(
          "0000000000000000000000000000000000000000000000000000000000000006"
          // offsets to array elements
          "00000000000000000000000000000000000000000000000000000000000000c0"
          "00000000000000000000000000000000000000000000000000000000000000e0"
          "0000000000000000000000000000000000000000000000000000000000000120"
          "0000000000000000000000000000000000000000000000000000000000000140"
          "0000000000000000000000000000000000000000000000000000000000000180"
          "00000000000000000000000000000000000000000000000000000000000001a0"
          // count for ""
          "0000000000000000000000000000000000000000000000000000000000000000"
          // count for "one"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "one"
          "6f6e650000000000000000000000000000000000000000000000000000000000"
          // count for ""
          "0000000000000000000000000000000000000000000000000000000000000000"
          // count for "two"
          "0000000000000000000000000000000000000000000000000000000000000003"
          // encoding for "two"
          "74776f0000000000000000000000000000000000000000000000000000000000"
          // count for ""
          "0000000000000000000000000000000000000000000000000000000000000000"
          // count for "three"
          "0000000000000000000000000000000000000000000000000000000000000005"
          // encoding for "three"
          "7468726565000000000000000000000000000000000000000000000000000000")),
      ElementsAreArray({"", "one", "", "two", "", "three"}));

  // Test invalid input.
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of array elements
      "0000000000000000000000000000000000000000000000000000000000000001"
      // invalid data offset to string element.
      "0000000000000000000000000000000000000000000000000000000000001")));
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of array elements
      "0000000000000000000000000000000000000000000000000000000000000002"
      // out-of-bound offset to array element
      "00000000000000000000000000000000000000000000000000000000000001e0")));

  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // Mismatched count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000003"
      // offsets to array elements
      "0000000000000000000000000000000000000000000000000000000000000060"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000")));

  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000003"
      // offsets to array elements, last offset point to non-existed data
      "0000000000000000000000000000000000000000000000000000000000000060"
      "00000000000000000000000000000000000000000000000000000000000000a0"
      "00000000000000000000000000000000000000000000000000000000000000e0"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000"
      // count for "two"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "two"
      "74776f0000000000000000000000000000000000000000000000000000000000")));

  // Missing data offset and data.
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001")));

  // Missing data.
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one", data missing
      "0000000000000000000000000000000000000000000000000000000000000020")));

  // Missing count.
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one"
      "0000000000000000000000000000000000000000000000000000000000000020"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000")));

  // Missing encoding of string.
  EXPECT_FALSE(ExtractStringArray(ToBytes(
      // count of elements in input array
      "0000000000000000000000000000000000000000000000000000000000000001"
      // offset for "one"
      "0000000000000000000000000000000000000000000000000000000000000020"
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003")));
}

TEST(EthAbiUtilsTest, ExtractStringArrayFromTuple) {
  auto bytes = ToBytes(GetOffchainLookupResponse());

  auto [_, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);

  EXPECT_THAT(
      *ExtractStringArrayFromTuple(args, 1),
      ElementsAreArray({"https://offchain-resolver-example.uc.r.appspot.com/"
                        "{sender}/{data}.json"}));

  // Bad tuple pos.
  EXPECT_FALSE(ExtractStringArrayFromTuple(args, 0));
  EXPECT_FALSE(ExtractStringArrayFromTuple(args, 10));
  EXPECT_FALSE(ExtractStringArrayFromTuple(args, 1000));

  // Empty data.
  EXPECT_FALSE(ExtractStringArrayFromTuple({}, 0));

  // Empty array.
  auto empty_string_array = ToBytes(
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(ExtractStringArrayFromTuple(empty_string_array, 0),
            std::vector<std::string>());
}

TEST(EthAbiUtilsTest, ExtractBytesFromTuple) {
  auto bytes = ToBytes(GetOffchainLookupResponse());

  auto [_, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);

  EXPECT_EQ(ToHex(*ExtractBytesFromTuple(args, 2)).substr(2),
            "9061b92300000000000000000000000000000000000000000000000000000000"
            "0000004000000000000000000000000000000000000000000000000000000000"
            "0000008000000000000000000000000000000000000000000000000000000000"
            "000000150f6f6666636861696e6578616d706c65036574680000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "000000243b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafb"
            "eadf58e6beaaa16c000000000000000000000000000000000000000000000000"
            "00000000");

  EXPECT_EQ(ToHex(*ExtractBytesFromTuple(args, 4)).substr(2),
            "9061b92300000000000000000000000000000000000000000000000000000000"
            "0000004000000000000000000000000000000000000000000000000000000000"
            "0000008000000000000000000000000000000000000000000000000000000000"
            "000000150f6f6666636861696e6578616d706c65036574680000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000"
            "000000243b3b57de42041b0018edd29d7c17154b0c671acc0502ea0b3693cafb"
            "eadf58e6beaaa16c000000000000000000000000000000000000000000000000"
            "00000000");

  // Bad tuple pos.
  EXPECT_FALSE(ExtractBytesFromTuple(args, 0));
  EXPECT_FALSE(ExtractBytesFromTuple(args, 10));
  EXPECT_FALSE(ExtractBytesFromTuple(args, 1000));

  // Empty data.
  EXPECT_FALSE(ExtractBytesFromTuple({}, 0));

  EXPECT_EQ(
      ToHex(*ExtractBytesFromTuple(
          ToBytes(
              "0000000000000000000000000000000000000000000000000000000000000001"
              "0000000000000000000000000000000000000000000000000000000000000040"
              "0000000000000000000000000000000000000000000000000000000000000020"
              "000000000000000000000000000000000000000000000006e83695ab1f893c0"
              "0"),
          1)),
      "0x000000000000000000000000000000000000000000000006e83695ab1f893c00");
}

TEST(EthAbiUtilsTest, ExtractBoolAndBytes) {
  // (true, some data)
  std::optional<std::pair<bool, std::vector<uint8_t>>> result =
      ExtractBoolAndBytes(ToBytes(
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "000000000000000000000000000000000000000000000006e83695ab1f893c00"));
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().first, true);
  EXPECT_EQ(ToHex(result.value().second).substr(2),
            "000000000000000000000000000000000000000000000006e83695ab1f893c00");

  // (true, some zeros)
  result = ExtractBoolAndBytes(ToBytes(
      "0000000000000000000000000000000000000000000000000000000000000001"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000020"
      "0000000000000000000000000000000000000000000000000000000000000000"));

  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().first, true);
  EXPECT_EQ(result.value().second.size(), 32UL);
  EXPECT_EQ(ToHex(result.value().second).substr(2),
            "0000000000000000000000000000000000000000000000000000000000000000");

  // (false, empty data)
  result = ExtractBoolAndBytes(ToBytes(
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000000"));
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().first, false);
  EXPECT_EQ(result.value().second.size(), 0UL);
  EXPECT_EQ(ToHex(result.value().second).substr(2), "0");
}

TEST(EthAbiUtilsTest, ExtractBoolBytesTupleArray) {
  std::optional<std::vector<std::pair<bool, std::vector<uint8_t>>>> result =
      ExtractBoolBytesArrayFromTuple(
          ToBytes(
              // offset of start of the (bool, bytes)[] element in the tuple
              "0000000000000000000000000000000000000000000000000000000000000020"
              // size of the array
              "0000000000000000000000000000000000000000000000000000000000000001"
              // offset of the first element
              "0000000000000000000000000000000000000000000000000000000000000020"
              // value of bool in the first element
              "0000000000000000000000000000000000000000000000000000000000000001"
              // offset of the size of the bytes in the first element
              "0000000000000000000000000000000000000000000000000000000000000040"
              // size of the bytes in the first element
              "0000000000000000000000000000000000000000000000000000000000000020"
              // first element bytes
              "000000000000000000000000000000000000000000000006e83695ab1f893c0"
              "0"),
          0);
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().size(), 1UL);
  EXPECT_EQ(result.value()[0].first, true);
  EXPECT_EQ(ToHex(result.value()[0].second).substr(2),
            "000000000000000000000000000000000000000000000006e83695ab1f893c00");

  result = ExtractBoolBytesArrayFromTuple(
      ToBytes(
          // offset of start of the (bool, bytes)[] element in the tuple
          "0000000000000000000000000000000000000000000000000000000000000020"
          // size of the array
          "0000000000000000000000000000000000000000000000000000000000000003"

          // offsets of each of the elements
          "0000000000000000000000000000000000000000000000000000000000000060"
          "00000000000000000000000000000000000000000000000000000000000000e0"
          "0000000000000000000000000000000000000000000000000000000000000160"

          // the elements
          // 1
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "0000000000000000000000000000000000000000000000000000000000000000"
          // 2
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "0000000000000000000000000000000000000000000000000000000000000000"
          // 3
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "0000000000000000000000000000000000000000000000000000000000000000"),
      0);
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().size(), 3UL);
  EXPECT_EQ(result.value()[0].first, true);
  EXPECT_EQ(ToHex(result.value()[0].second).substr(2),
            "0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(result.value()[1].first, true);
  EXPECT_EQ(ToHex(result.value()[1].second).substr(2),
            "0000000000000000000000000000000000000000000000000000000000000000");
  EXPECT_EQ(result.value()[2].first, true);
  EXPECT_EQ(ToHex(result.value()[2].second).substr(2),
            "0000000000000000000000000000000000000000000000000000000000000000");

  result = ExtractBoolBytesArrayFromTuple(
      ToBytes(
          // offset of start of the (bool, bytes)[] element in the tuple
          "0000000000000000000000000000000000000000000000000000000000000020"
          // size of the array
          "0000000000000000000000000000000000000000000000000000000000000003"

          // offsets of each of the elements
          "0000000000000000000000000000000000000000000000000000000000000060"
          "00000000000000000000000000000000000000000000000000000000000000e0"
          "0000000000000000000000000000000000000000000000000000000000000140"

          // the elements
          // 1
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "000000000000000000000000000000000000000000000006e83695ab1f893c00"
          // 2
          "0000000000000000000000000000000000000000000000000000000000000000"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000000"
          // 3
          "0000000000000000000000000000000000000000000000000000000000000001"
          "0000000000000000000000000000000000000000000000000000000000000040"
          "0000000000000000000000000000000000000000000000000000000000000020"
          "0000000000000000000000000000000000000000000000000000000000000000"),
      0);
  ASSERT_TRUE(result);
  EXPECT_EQ(result.value().size(), 3UL);
  EXPECT_EQ(result.value()[0].first, true);
  EXPECT_EQ(ToHex(result.value()[0].second).substr(2),
            "000000000000000000000000000000000000000000000006e83695ab1f893c00");
  EXPECT_EQ(result.value()[1].first, false);
  EXPECT_EQ(result.value()[1].second.size(), 0UL);
  EXPECT_EQ(ToHex(result.value()[1].second).substr(2), "0");
  EXPECT_EQ(result.value()[2].first, true);
  EXPECT_EQ(ToHex(result.value()[2].second).substr(2),
            "0000000000000000000000000000000000000000000000000000000000000000");
}

TEST(EthAbiUtilsTest, ExtractFixedBytesFromTuple) {
  auto bytes = ToBytes(GetOffchainLookupResponse());

  auto [_, args] = ExtractFunctionSelectorAndArgsFromCall(bytes);

  EXPECT_EQ(ToHex(*ExtractFixedBytesFromTuple(args, 4, 3)), "0xf4d4d2f8");

  // Bad tuple pos.
  EXPECT_FALSE(ExtractFixedBytesFromTuple(args, 4, 0));
  EXPECT_FALSE(ExtractFixedBytesFromTuple(args, 4, 1000));

  // Empty data.
  EXPECT_FALSE(ExtractFixedBytesFromTuple({}, 4, 0));

  bytes[101] = 0;
  EXPECT_EQ(ToHex(*ExtractFixedBytesFromTuple(args, 4, 3)), "0xf400d2f8");

  // Bad padding.
  bytes[111] = 1;
  EXPECT_FALSE(ExtractFixedBytesFromTuple(args, 4, 3));
}

TEST(EthAbiTupleEncoderTest, EncodeCall) {
  std::vector<uint8_t> data(33, 0xbb);
  auto selector_bytes = ToBytes("f400d2f8");
  Span4 selector(selector_bytes.begin(), 4u);
  // f(bytes,bytes)
  EXPECT_EQ(
      "f400d2f8"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000080"
      "0000000000000000000000000000000000000000000000000000000000000001"
      "aa00000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000021"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
      "bb00000000000000000000000000000000000000000000000000000000000000",
      ToHex(TupleEncoder()
                .AddBytes(ToBytes("aa"))
                .AddBytes(data)
                .EncodeWithSelector(selector))
          .substr(2));
  EXPECT_EQ(
      "f400d2f8"
      "0000000000000000000000000000000000000000000000000000000000000040"
      "0000000000000000000000000000000000000000000000000000000000000060"
      "0000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000000000000000000000000000000000",
      ToHex(
          TupleEncoder().AddBytes({}).AddBytes({}).EncodeWithSelector(selector))
          .substr(2));

  // f(bytes32)
  EXPECT_EQ(
      "f400d2f8"
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
      ToHex(TupleEncoder()
                .AddFixedBytes(Span32(data.begin(), 32u))
                .EncodeWithSelector(selector))
          .substr(2));
}

}  // namespace brave_wallet::eth_abi
