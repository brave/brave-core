/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"
#include "third_party/zlib/google/compression_utils.h"

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
    EXPECT_EQ(ExtractAddress(base::make_span(bytes)).ToHex(),
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
    EXPECT_EQ(ExtractAddress(base::make_span(bytes)).ToHex(),
              "0x0000000000000000000000000000000000000000");
  }

  {  // Empty.
    auto bytes = std::vector<uint8_t>{};
    EXPECT_TRUE(ExtractAddress(base::make_span(bytes)).IsEmpty());
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
  Span4 selector(selector_bytes.begin(), 4);
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
                .AddFixedBytes(Span32(data.begin(), 32))
                .EncodeWithSelector(selector))
          .substr(2));
}

// Typical test case looks like:
//{
// "result":
// "0x000000000000000000000000d48ce0afbf820f01bf431d0978ec400af7dd39bc", "name":
// "random-753", "types": "[\"address\"]", "values":
// "[{\"type\":\"string\",\"value\":\"0xd48cE0Afbf820F01bF431d0978EC400AF7dD39Bc\"}]",
//}

class EthersProjectCasesUnitTest : public testing::Test {
 public:
 protected:
  std::vector<std::string> ExtractTypes(const base::Value& test_case) {
    auto* types_string = test_case.GetDict().FindString("types");
    EXPECT_TRUE(types_string);
    absl::optional<base::Value> types_value =
        base::JSONReader::Read(*types_string);
    EXPECT_TRUE(types_value);
    EXPECT_TRUE(types_value->is_list());

    std::vector<std::string> result;
    for (auto& item : types_value->GetList()) {
      result.push_back(item.GetString());
    }
    return result;
  }

  absl::optional<int> IsFixedBytesType(const std::string& type) {
    static const base::NoDestructor<RE2> kPattern(R"(bytes(\d+))");
    int sz = 0;
    if (!RE2::FullMatch(type, *kPattern, &sz))
      return absl::nullopt;

    if (sz >= 1 && sz <= 32)
      return sz;
    return absl::nullopt;
  }

  absl::optional<int> IsIntType(const std::string& type) {
    static const base::NoDestructor<RE2> kPattern(R"(int(\d+))");

    if (type == "int") {
      return 256;
    }

    int sz = 0;
    if (!RE2::FullMatch(type, *kPattern, &sz))
      return absl::nullopt;

    if (sz >= 1 && sz <= 256 && (sz % 8 == 0))
      return sz;
    return absl::nullopt;
  }

  absl::optional<int> IsUintType(const std::string& type) {
    static const base::NoDestructor<RE2> kPattern(R"(uint(\d+))");

    if (type == "uint") {
      return 256;
    }

    int sz = 0;
    if (!RE2::FullMatch(type, *kPattern, &sz))
      return absl::nullopt;

    if (sz >= 1 && sz <= 256 && (sz % 8 == 0))
      return sz;
    return absl::nullopt;
  }

  absl::optional<std::string> IsDynamicSizeArrayType(const std::string& type) {
    static const base::NoDestructor<RE2> kPattern(R"((.*)\[\])");

    std::string array_type;
    if (!RE2::FullMatch(type, *kPattern, &array_type))
      return absl::nullopt;

    if (TypeIsSupported(array_type))
      return array_type;
    return absl::nullopt;
  }

  absl::optional<std::string> IsFixedSizeArrayType(const std::string& type) {
    static const base::NoDestructor<RE2> kPattern(R"((.*)\[(\d+)\])");

    std::string array_type;
    int sz = 0;
    if (!RE2::FullMatch(type, *kPattern, &array_type, &sz))
      return absl::nullopt;

    if (sz >= 1 && TypeIsSupported(array_type))
      return array_type;
    return absl::nullopt;
  }

  absl::optional<std::vector<std::string>> IsTupleType(
      const std::string& type) {
    static const base::NoDestructor<RE2> kArgsPattern(R"(tuple\((.*)\))");

    std::string tuple_args_string;
    if (!RE2::FullMatch(type, *kArgsPattern, &tuple_args_string))
      return absl::nullopt;

    std::vector<std::string> tuple_args;

    base::StringPiece input(tuple_args_string);
    while (!input.empty()) {
      size_t comma_search_pos = 0;
      // Should skip all commas in type `tuple(arg1,arg2,...)`.
      if (base::StartsWith(input, "tuple")) {
        base::StringPiece tuple_parens_search = input.substr(5);
        int count = 0;
        while (!tuple_parens_search.empty()) {
          if (tuple_parens_search.at(0) == '(')
            count++;
          if (tuple_parens_search.at(0) == ')')
            count--;
          tuple_parens_search.remove_prefix(1);
          if (count == 0)
            break;
        }
        EXPECT_EQ(0, count);
        comma_search_pos = input.size() - tuple_parens_search.size();
      }

      auto comma_pos = input.find(',', comma_search_pos);
      tuple_args.push_back(std::string(input.substr(0, comma_pos)));
      if (comma_pos == base::StringPiece::npos)
        break;
      input.remove_prefix(comma_pos + 1);
    }

    return tuple_args;
  }

  bool TypeIsSupported(const std::string& type) {
    if (type == "address" || type == "string" || type == "bytes")
      return true;
    if (IsFixedBytesType(type))
      return true;
    if (IsIntType(type))
      return true;
    if (IsUintType(type))
      return true;
    if (IsDynamicSizeArrayType(type))
      return true;
    if (IsFixedSizeArrayType(type))
      return true;
    if (IsTupleType(type))
      return true;

    return false;
  }

  bool CheckTestCaseIsSupported(const base::Value& test_case) {
    EXPECT_TRUE(test_case.is_dict());
    auto& dict = test_case.GetDict();
    EXPECT_TRUE(dict.FindString("result"));
    EXPECT_TRUE(dict.FindString("types"));
    EXPECT_TRUE(dict.FindString("values"));
    EXPECT_TRUE(dict.FindString("name"));

    bool types_are_valid = true;
    for (auto& type : ExtractTypes(test_case)) {
      if (TypeIsSupported(type))
        continue;

      unsupported_types[type]++;
      types_are_valid = false;
    }
    return types_are_valid;
  }

  base::Value ParseValues(const base::Value& test_case) {
    auto* values_string = test_case.GetDict().FindString("values");
    absl::optional<base::Value> result = base::JSONReader::Read(*values_string);
    EXPECT_TRUE(result);
    EXPECT_TRUE(result->is_list());
    return std::move(*result);
  }

  EthAddress ExtractAddressFromValueObject(const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "string");
    EXPECT_TRUE(value.GetDict().FindString("value"));
    return EthAddress::FromHex(*value.GetDict().FindString("value"));
  }

  std::string ExtractStringFromValueObject(const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "string");
    EXPECT_TRUE(value.GetDict().FindString("value"));
    return *value.GetDict().FindString("value");
  }

  std::vector<uint8_t> ExtractBytesFromValueObject(const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "buffer");
    EXPECT_TRUE(value.GetDict().FindString("value"));
    return *PrefixedHexStringToBytes(*value.GetDict().FindString("value"));
  }

  int256_t ExtractIntFromValueObject(const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "number");
    EXPECT_TRUE(value.GetDict().FindString("value"));
    int256_t result = 0;
    EXPECT_TRUE(
        Base10ValueToInt256(*value.GetDict().FindString("value"), &result));
    return result;
  }

  uint256_t ExtractUintFromValueObject(const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "number");
    EXPECT_TRUE(value.GetDict().FindString("value"));
    uint256_t result = 0;
    EXPECT_TRUE(
        Base10ValueToUint256(*value.GetDict().FindString("value"), &result));
    return result;
  }

  std::vector<base::Value> ExtractTupleArgValuesFromValueObject(
      const base::Value& value) {
    EXPECT_TRUE(value.is_dict());
    EXPECT_EQ(*value.GetDict().FindString("type"), "tuple");
    EXPECT_TRUE(value.GetDict().FindList("value"));
    std::vector<base::Value> result;
    for (auto& arg : value.GetDict().Find("value")->GetList()) {
      result.push_back(arg.Clone());
    }
    return result;
  }

  void EncodeValue(TupleEncoder& encoder,
                   const std::string& type,
                   base::Value& value) {
    if (type == "address") {
      encoder.AddAddress(ExtractAddressFromValueObject(value));
    } else if (type == "string") {
      encoder.AddString(ExtractStringFromValueObject(value));
    } else if (type == "bytes") {
      encoder.AddBytes(ExtractBytesFromValueObject(value));
    } else if (IsFixedBytesType(type)) {
      encoder.AddFixedBytes(ExtractBytesFromValueObject(value));
    } else if (IsIntType(type)) {
      encoder.AddInt256(ExtractIntFromValueObject(value));
    } else if (IsUintType(type)) {
      encoder.AddUint256(ExtractUintFromValueObject(value));
    } else if (auto dynamic_size_array_arg_type =
                   IsDynamicSizeArrayType(type)) {
      TupleEncoder array_encoder;
      for (auto& array_item : value.GetList()) {
        ASSERT_NO_FATAL_FAILURE(EncodeValue(
            array_encoder, *dynamic_size_array_arg_type, array_item));
      }
      encoder.AddArray(array_encoder);
    } else if (auto fixed_size_array_arg_type = IsFixedSizeArrayType(type)) {
      TupleEncoder array_encoder;
      for (auto& array_item : value.GetList()) {
        ASSERT_NO_FATAL_FAILURE(
            EncodeValue(array_encoder, *fixed_size_array_arg_type, array_item));
      }
      encoder.AddTuple(array_encoder);
    } else if (auto tuple_arg_types = IsTupleType(type)) {
      auto tuple_arg_values = ExtractTupleArgValuesFromValueObject(value);
      ASSERT_EQ(tuple_arg_types->size(), tuple_arg_values.size());
      TupleEncoder tuple_encoder;

      for (size_t i = 0u; i < tuple_arg_types->size(); ++i) {
        auto& arg_type = tuple_arg_types->at(i);
        auto& arg_value = tuple_arg_values.at(i);
        ASSERT_NO_FATAL_FAILURE(
            EncodeValue(tuple_encoder, arg_type, arg_value));
      }

      encoder.AddTuple(tuple_encoder);
    } else {
      FAIL() << "Unknown type: " << type;
    }
  }

  void RunEncodeTest(const base::Value& test_case) {
    auto types = ExtractTypes(test_case);
    auto values = ParseValues(test_case);

    EXPECT_EQ(types.size(), values.GetList().size());

    TupleEncoder encoder;

    for (size_t i = 0u; i < types.size(); ++i) {
      auto& type = types[i];
      auto& value = values.GetList()[i];

      ASSERT_NO_FATAL_FAILURE(EncodeValue(encoder, type, value));
    }

    EXPECT_TRUE(test_case.GetDict().FindString("result"));
    EXPECT_EQ(ToHex(encoder.Encode()),
              *test_case.GetDict().FindString("result"));
  }

  base::Value ReadTestCasesFromFile() {
    base::FilePath dir_exe;
    EXPECT_TRUE(base::PathService::Get(base::DIR_EXE, &dir_exe));
    base::FilePath test_archive_path = dir_exe.AppendASCII(
        "eth_abi_test_files/contract-interface-abi2.json.gz");

    std::string compressed_json_text;
    EXPECT_TRUE(
        base::ReadFileToString(test_archive_path, &compressed_json_text));

    std::string decompressed_json_text;
    EXPECT_TRUE(compression::GzipUncompress(compressed_json_text,
                                            &decompressed_json_text));
    absl::optional<base::Value> test_cases =
        base::JSONReader::Read(decompressed_json_text);
    EXPECT_TRUE(test_cases);
    EXPECT_TRUE(test_cases->is_list());
    return std::move(*test_cases);
  }

  std::map<std::string, int> unsupported_types;
};

TEST_F(EthersProjectCasesUnitTest, EncodeTest) {
  auto test_cases = ReadTestCasesFromFile();
  size_t supported_test_cases = 0;
  for (auto& test_case : test_cases.GetList()) {
    SCOPED_TRACE(testing::Message() << *test_case.GetDict().FindString("name"));
    if (!CheckTestCaseIsSupported(test_case)) {
      continue;
    }
    supported_test_cases++;
    RunEncodeTest(test_case);
  }
  EXPECT_EQ(1880u, supported_test_cases);
  EXPECT_EQ(1880u, test_cases.GetList().size());

  std::vector<std::pair<std::string, int>> unsupported_types_vector;
  for (auto& [type, count] : unsupported_types)
    unsupported_types_vector.emplace_back(type, count);
  std::sort(unsupported_types_vector.begin(), unsupported_types_vector.end(),
            [](auto& i1, auto& i2) { return i1.second > i2.second; });

  for (auto& [type, count] : base::make_span(unsupported_types_vector)) {
    LOG(ERROR) << type << " " << count;
  }
}

}  // namespace brave_wallet::eth_abi
