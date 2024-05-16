/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"

#include <limits>
#include <map>
#include <optional>
#include <stack>
#include <tuple>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

// This file implements decoding rules of calldata according to the EVM
// Contract Application Binary Interface (ABI).
//
// Method arguments are packed in chunks of 32 bytes, with types smaller than
// 32 bytes zero padded if necessary. Fixed-size types like uint256, address,
// bytes32, etc., are represented within the available 32 bytes, however,
// dynamic types like bytes and arrays follow head-tail encoding. In this
// scheme, the data is packaged at the tail-end of the transaction's calldata.
// The arguments are references into the calldata where the content is.
//
// The decoder will parse the calldata and return a base::Value object, which
// can be serialized to JSON or used in other contexts. If the decoder
// encounters an unknown type or an error, it will return an empty optional.
//
// Unsupported types:
// - int<M>
// - fixed<M>x<N>
// - ufixed<M>x<N>
// - function
//
// Function selector (first 4 bytes) should NOT be part of the calldata being
// parsed.
//
// References:
//   - https://docs.soliditylang.org/en/latest/abi-spec.html
//   -
//   https://github.com/web3/web3.js/tree/4.x/packages/web3-eth-abi/src/coders/base

namespace brave_wallet {

namespace {

using ByteArray = std::vector<uint8_t>;

// DecoderResult is a tuple that contains the decoded value, the remaining
// calldata, and the number of bytes consumed.
template <typename T>
using DecoderResult = std::tuple<T, ByteArray, size_t>;

constexpr size_t kWordSize = 32;
constexpr size_t kAddressSize = 20;

// GetSubByteArray returns a subarray of the input starting from the specified
// offset. If the offset is out of bounds, an empty ByteArray is returned.
//
// It mimics the behavior of JavaScript's Uint8Array.prototype.subarray.
ByteArray GetSubByteArray(const ByteArray& input, size_t offset) {
  if (offset >= input.size()) {
    return ByteArray();
  }

  return ByteArray(input.begin() + offset, input.end());
}

// GetAddressFromData extracts an Ethereum address from the calldata segment.
// The address type is static and 32-bytes wide, but we only consider the last
// 20 bytes, discarding the leading 12 bytes of 0-padded chars.
//
// The parsed address value is prefixed by "0x".
//
// In the future, addresses in Ethereum may become 32 bytes long:
// https://ethereum-magicians.org/t/increasing-address-size-from-20-to-32-bytes
std::optional<DecoderResult<base::Value>> GetAddressFromData(
    const ByteArray& input) {
  if (input.size() < kWordSize) {
    return std::nullopt;
  }

  return std::make_tuple(
      base::Value("0x" + HexEncodeLower(input.data() + kWordSize - kAddressSize,
                                        kAddressSize)),
      GetSubByteArray(input, kWordSize), kWordSize);
}

// GetUintFromData extracts a 32-byte wide integral value of type M from the
// calldata segment.
//
// Using this function to extract an integer outside the range of M is
// considered an error.
template <typename M>
std::optional<DecoderResult<M>> GetUintFromData(const ByteArray& input) {
  static_assert(std::is_integral<M>::value, "M must be an integer type");

  if (input.size() < kWordSize) {
    return std::nullopt;
  }

  auto arg = HexEncodeLower(input.data(), kWordSize);

  uint256_t value;
  if (!HexValueToUint256("0x" + arg, &value)) {
    return std::nullopt;
  }

  // To prevent runtime errors, we make sure the value is within safe
  // limits of M.
  if (value > std::numeric_limits<M>::max()) {
    return std::nullopt;
  }

  return std::make_tuple(static_cast<M>(value),
                         GetSubByteArray(input, kWordSize), kWordSize);
}

// GetUintHexFromData encodes the return value of GetUintFromData as a compact
// hex string (without leading 0s), and prefixed by "0x".
template <typename M>
std::optional<DecoderResult<base::Value>> GetUintHexFromData(
    const ByteArray& input) {
  auto result = GetUintFromData<M>(input);
  if (!result) {
    return std::nullopt;
  }

  auto [value, remaining, consumed] = *result;

  return std::make_tuple(base::Value(Uint256ValueToHex(value)), remaining,
                         consumed);
}

// GetBoolFromData extracts a 32-byte wide boolean value from the
// calldata segment.
std::optional<DecoderResult<base::Value>> GetBoolFromData(
    const ByteArray& input) {
  auto result = GetUintFromData<uint8_t>(input);
  if (!result) {
    return std::nullopt;
  }

  auto [value, remaining, consumed] = *result;

  if (value == static_cast<uint8_t>(0)) {
    return std::make_tuple(base::Value(false), remaining, consumed);
  } else if (value == static_cast<uint8_t>(1)) {
    return std::make_tuple(base::Value(true), remaining, consumed);
  }

  return std::nullopt;
}

// GetBytesHexFromData extracts a bytes value from the calldata segment using
// head-tail encoding mechanism. bytes are packed in chunks of 32 bytes, with
// the first 32 bytes encoding the length, followed by the actual content.
//
// The param argument indicates the type of the bytes value to be extracted.
// `bytes` indicates a dynamic type, while `bytes<M>` indicates a fixed-size
// type, where 0 < M <= 32.
//
// The result is serialized as a hex string prefixed by "0x".
std::optional<DecoderResult<base::Value>> GetBytesHexFromData(
    const std::string& param,
    const ByteArray& input) {
  std::vector<std::string> param_parts = base::SplitString(
      param, "bytes", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  size_t size = 0;
  if (param_parts.size() == 1 && !base::StringToSizeT(param_parts[0], &size)) {
    return std::nullopt;
  }

  ByteArray remaining(input.begin(), input.end());
  size_t parts_count = 1;
  size_t consumed = 0;

  if (size == 0) {
    // Extract the length of the bytes since it is a dynamic type
    auto length_result = GetUintFromData<size_t>(remaining);
    if (!length_result) {
      return std::nullopt;
    }

    size = std::get<0>(*length_result);

    remaining = std::get<1>(*length_result);
    consumed += std::get<2>(*length_result);
    parts_count = std::ceil(static_cast<double>(size) / kWordSize);
  }

  if (size > remaining.size()) {
    return std::nullopt;
  }

  size_t parts_size = parts_count * kWordSize;
  return std::make_tuple(
      base::Value("0x" + HexEncodeLower(remaining.data(), size)),
      GetSubByteArray(remaining, parts_size), consumed + parts_size);
}

// GetStringFromData extracts a string value from the calldata segment using
// head-tail encoding mechanism. Strings in calldata are represented as bytes,
// with the first 32 bytes encoding the length, followed by the actual content.
std::optional<DecoderResult<base::Value>> GetStringFromData(
    const ByteArray& input) {
  auto bytes_result = GetBytesHexFromData("bytes", input);
  if (!bytes_result) {
    return std::nullopt;
  }

  auto& bytes_value = std::get<0>(*bytes_result);
  if (!bytes_value.is_string()) {
    return std::nullopt;
  }

  std::string result;
  if (base::HexStringToString(bytes_value.GetString().substr(2), &result)) {
    return std::make_tuple(base::Value(result), std::get<1>(*bytes_result),
                           std::get<2>(*bytes_result));
  }

  return std::nullopt;
}

bool IsTuple(const std::string& param) {
  return param.length() > 2 && param.front() == '(' && param.back() == ')';
}

// Forward declarations for recursive functions.
std::optional<DecoderResult<base::Value>> GetTupleFromData(
    const std::string& param,
    const ByteArray& input);
std::optional<DecoderResult<base::Value>> GetArrayFromData(
    const std::string& param,
    const ByteArray& input);

std::optional<DecoderResult<base::Value>> DecodeParam(const std::string& param,
                                                      const ByteArray& input) {
  if (param == "address") {
    return GetAddressFromData(input);
  }

  // Handle all unsigned integers of M bits,
  // where 0 < M <= 256 and M % 8 == 0.
  if (param == "uint8") {
    return GetUintHexFromData<uint8_t>(input);
  }

  if (param == "uint16") {
    return GetUintHexFromData<uint16_t>(input);
  }

  if (param == "uint32") {
    return GetUintHexFromData<uint32_t>(input);
  }

  if (param == "uint64") {
    return GetUintHexFromData<uint64_t>(input);
  }

  if (param == "uint128") {
    return GetUintHexFromData<uint128_t>(input);
  }

  if (param == "uint256" || param == "uint") {
    return GetUintHexFromData<uint256_t>(input);
  }

  if (param == "bool") {
    return GetBoolFromData(input);
  }

  if (base::StartsWith(param, "bytes")) {
    return GetBytesHexFromData(param, input);
  }

  if (param == "string") {
    return GetStringFromData(input);
  }

  if (base::EndsWith(param, "]")) {
    return GetArrayFromData(param, input);
  }

  if (IsTuple(param)) {
    return GetTupleFromData(param, input);
  }

  // Unrecognized types are considered errors.
  return std::nullopt;
}

// Function to trim spaces from both ends of a string
std::string Trim(const std::string& str) {
  size_t start = str.find_first_not_of(' ');
  size_t end = str.find_last_not_of(' ');
  if (start == std::string::npos || end == std::string::npos) {
    return "";
  }
  return str.substr(start, end - start + 1);
}

// ExtractTupleComponents parses a tuple string and returns a vector of its
// components. The tuple string should be enclosed in parentheses and
// separated by commas.
std::vector<std::string> ExtractTupleComponents(const std::string& param) {
  std::vector<std::string> components;

  // Check if the input string is a valid tuple
  if (IsTuple(param)) {
    // Remove the enclosing parentheses
    std::string trimmed_param = param.substr(1, param.length() - 2);

    std::stack<char> parentheses_stack;
    std::string component;
    for (char ch : trimmed_param) {
      if (ch == ',' && parentheses_stack.empty()) {
        // If we encounter a comma at the top level, it signifies the end of a
        // component
        components.push_back(Trim(component));
        component.clear();
      } else {
        if (ch == '(') {
          parentheses_stack.push(ch);
        } else if (ch == ')') {
          if (!parentheses_stack.empty()) {
            parentheses_stack.pop();
          }
        }
        component += ch;
      }
    }
    // Add the last component
    if (!component.empty()) {
      components.push_back(Trim(component));
    }
  }

  return components;
}

// ExtractArrayType parses an array type string and returns a pair of its size
// and element type. The array type string should be suffixed with "[]" or [M],
// where M is the size of the fixed array.
//
// A size of 0 indicates a dynamic array.
std::optional<std::pair<size_t, std::string>> ExtractArrayType(
    const std::string& param) {
  size_t array_paranthesis_start = param.find_last_of('[');
  if (array_paranthesis_start == std::string::npos) {
    return std::nullopt;
  }

  auto array_param_type = param.substr(0, array_paranthesis_start);
  if (array_param_type.empty()) {
    return std::nullopt;
  }

  auto size_str = param.substr(array_paranthesis_start);

  if (size_str != "[]") {
    // Extract the size of the fixed array
    size_t size;
    if (base::StringToSizeT(size_str.substr(1, size_str.size() - 2), &size)) {
      return std::make_pair(size, array_param_type);
    }

    return std::nullopt;
  }

  return std::make_pair(0, array_param_type);
}

// IsDynamicType checks if a parameter is a dynamic type or contains dynamic
// types within it. Dynamic types include bytes, string, and dynamic-sized
// arrays.
bool IsDynamicType(const std::string& param) {
  if (param == "bytes" || param == "string" || base::EndsWith(param, "[]")) {
    return true;
  }

  if (base::EndsWith(param, "]")) {
    auto array_type_result = ExtractArrayType(param);
    if (!array_type_result) {
      return false;
    }

    return IsDynamicType(array_type_result->second);
  }

  auto tuple_components = ExtractTupleComponents(param);
  for (const auto& component : tuple_components) {
    if (IsDynamicType(component)) {
      return true;
    }
  }

  return false;
}

// GetTupleFromData extracts a tuple value from the calldata segment. The tuple
// is represented as a list of parameters, each of which is extracted according
// to its type.
//
// Dynamic types within the tuple are extracted using head-tail encoding.
//
// The result is a base::Value object containing a list of the extracted values.
std::optional<DecoderResult<base::Value>> GetTupleFromData(
    const std::vector<std::string>& params,
    const ByteArray& input) {
  base::Value::List result;
  size_t consumed = 0;
  size_t dynamic_consumed = 0;
  for (const auto& child_param : params) {
    if (IsDynamicType(child_param)) {
      auto offset_result =
          GetUintFromData<size_t>(GetSubByteArray(input, consumed));
      if (!offset_result) {
        return std::nullopt;
      }

      auto decoded_result = DecodeParam(
          child_param, GetSubByteArray(input, std::get<0>(*offset_result)));
      if (!decoded_result) {
        return std::nullopt;
      }

      consumed += std::get<2>(*offset_result);
      dynamic_consumed += std::get<2>(*decoded_result);
      result.Append(std::move(std::get<0>(*decoded_result)));
    } else {
      // static type
      auto decoded_result =
          DecodeParam(child_param, GetSubByteArray(input, consumed));
      if (!decoded_result) {
        return std::nullopt;
      }

      consumed += std::get<2>(*decoded_result);
      result.Append(std::move(std::get<0>(*decoded_result)));
    }
  }

  return std::make_tuple(base::Value(std::move(result)),
                         GetSubByteArray(input, consumed + dynamic_consumed),
                         consumed + dynamic_consumed);
}

// GetTupleFromData extracts a tuple value from the calldata segment. The tuple
// type is represented as a string enclosed in parentheses, with each component
// separated by commas.
std::optional<DecoderResult<base::Value>> GetTupleFromData(
    const std::string& param,
    const ByteArray& input) {
  auto child_params = ExtractTupleComponents(param);
  return GetTupleFromData(child_params, input);
}

// GetArrayFromData parses a calldata segment to iterate over an array of
// elements. The array type is represented as a string suffixed with "[]" or
// "[M]", where M is the size of the fixed array.
//
// The underlying elements of the array can be both dynamic, or fixed-size
// types. If the array contains at least one dynamic type, the entire array
// is encoded using head-tail encoding, otherwise, the array is encoded as a
// contiguous sequence of elements.
std::optional<DecoderResult<base::Value>> GetArrayFromData(
    const std::string& param,
    const ByteArray& input) {
  auto array_type_result = ExtractArrayType(param);
  if (!array_type_result) {
    return std::nullopt;
  }

  auto array_type = array_type_result->second;
  size_t size = array_type_result->first;
  bool is_dynamic = size == 0;
  size_t consumed = 0;
  ByteArray remaining(input.begin(), input.end());

  if (is_dynamic) {
    // Extract the array length since it is a dynamic array
    auto length_result = GetUintFromData<size_t>(input);
    if (!length_result) {
      return std::nullopt;
    }

    size = std::get<0>(*length_result);
    remaining = std::get<1>(*length_result);
    consumed = std::get<2>(*length_result);
  }

  auto has_dynamic_child = IsDynamicType(array_type);
  base::Value::List result;

  if (has_dynamic_child) {
    for (size_t i = 0; i < size; i++) {
      auto offset_result =
          GetUintFromData<size_t>(GetSubByteArray(remaining, i * kWordSize));
      if (!offset_result) {
        return std::nullopt;
      }

      consumed += std::get<2>(*offset_result);

      auto decoded_child_result = DecodeParam(
          array_type, GetSubByteArray(remaining, std::get<0>(*offset_result)));
      if (!decoded_child_result) {
        return std::nullopt;
      }

      result.Append(std::move(std::get<0>(*decoded_child_result)));
      consumed += std::get<2>(*decoded_child_result);
    }

    return std::make_tuple(base::Value(std::move(result)),
                           GetSubByteArray(remaining, consumed), consumed);
  }

  for (size_t i = 0; i < size; i++) {
    auto decoded_child_result =
        DecodeParam(array_type, GetSubByteArray(input, consumed));
    if (!decoded_child_result) {
      return std::nullopt;
    }

    result.Append(std::move(std::get<0>(*decoded_child_result)));
    consumed += std::get<2>(*decoded_child_result);
  }

  return std::make_tuple(base::Value(std::move(result)),
                         GetSubByteArray(input, consumed), consumed);
}

}  // namespace

// UniswapEncodedPathDecode parses a Uniswap-encoded path and return a vector
// of addresses representing each hop involved in the swap.
//
// Single-hop swap: Token1 → Token2
// Multi-hop swap: Token1 → Token2 → WETH → Token3
//
// Each encoded hop contains a 3-byte pool fee, which is associated with the
// address that follows. It is excluded from the result of this function.
//
// ┌──────────┬──────────┬──────────┬─────┐
// │ address  │ pool fee │ address  │     │
// │          │          │          │ ... │
// │ 20 bytes │ 3 bytes  │ 20 bytes │     │
// └──────────┴──────────┴──────────┴─────┘
std::optional<std::vector<std::string>> UniswapEncodedPathDecode(
    const std::string& encoded_path) {
  ByteArray data;
  if (!PrefixedHexStringToBytes(encoded_path, &data)) {
    return std::nullopt;
  }
  size_t offset = 0;
  std::vector<std::string> path;

  // The path should be long enough to encode a single-hop swap.
  // 43 = 20(address) + 3(fee) + 20(address)
  if (data.size() < 43) {
    return std::nullopt;
  }

  // Parse first hop address.
  path.push_back("0x" + HexEncodeLower(data.data(), 20));
  offset += 20;

  while (true) {
    if (offset == data.size()) {
      break;
    }

    // Parse the pool fee, and ignore.
    if (data.size() - offset < 3) {
      return std::nullopt;
    }

    offset += 3;

    // Parse next hop.
    if (data.size() - offset < 20) {
      return std::nullopt;
    }
    path.push_back("0x" + HexEncodeLower(data.data() + offset, 20));
    offset += 20;
  }

  // Require a minimum of 2 addresses for a single-hop swap.
  if (path.size() < 2) {
    return std::nullopt;
  }

  return path;
}

std::optional<base::Value::List> ABIDecode(
    const std::vector<std::string>& params,
    const ByteArray& data) {
  auto result = GetTupleFromData(params, data);
  if (!result) {
    return std::nullopt;
  }

  if (std::get<0>(*result).is_list()) {
    return std::move(std::get<0>(*result).GetList());
  }

  return std::nullopt;
}

}  // namespace brave_wallet
