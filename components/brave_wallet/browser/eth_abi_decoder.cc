/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"

#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
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

constexpr size_t kWordSize = 32;
constexpr size_t kAddressSize = 20;

using ByteArray = std::vector<uint8_t>;
using ByteView = base::span<const uint8_t>;

template <typename T>
struct DecoderResult {
  DecoderResult(T result, ByteView remaining, size_t consumed)
      : result(std::move(result)),
        remaining(std::move(remaining)),
        consumed(consumed) {}

  T result;            // decoded value
  ByteView remaining;  // remaining calldata
  size_t consumed;     // number of bytes consumed
};

ByteView GetSubByteView(ByteView input, size_t offset) {
  if (offset >= input.size()) {
    return ByteView();
  }

  return input.subspan(offset);
}

// GetAddressFromData extracts an Ethereum address from the calldata segment.
// The address type is static and 32-bytes wide, but we only consider the last
// 20 bytes, discarding the leading 12 bytes of 0-padded chars.
//
// The parsed address value is prefixed by "0x".
//
// In the future, addresses in Ethereum may become 32 bytes long:
// https://ethereum-magicians.org/t/increasing-address-size-from-20-to-32-bytes
std::optional<DecoderResult<base::Value>> GetAddressFromData(ByteView input) {
  if (input.size() < kWordSize) {
    return std::nullopt;
  }

  return DecoderResult<base::Value>(
      base::Value("0x" + HexEncodeLower(input.data() + kWordSize - kAddressSize,
                                        kAddressSize)),
      GetSubByteView(input, kWordSize), kWordSize);
}

// GetUintFromData extracts a 32-byte wide integral value of type M from the
// calldata segment.
//
// Using this function to extract an integer outside the range of M is
// considered an error.
template <typename M>
std::optional<DecoderResult<M>> GetUintFromData(ByteView input) {
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

  return DecoderResult<M>(static_cast<M>(value),
                          GetSubByteView(input, kWordSize), kWordSize);
}

// GetUintHexFromData encodes the return value of GetUintFromData as a compact
// hex string (without leading 0s), and prefixed by "0x".
template <typename M>
std::optional<DecoderResult<base::Value>> GetUintHexFromData(ByteView input) {
  auto result = GetUintFromData<M>(input);
  if (!result) {
    return std::nullopt;
  }

  auto [value, remaining, consumed] = *result;

  return DecoderResult<base::Value>(base::Value(Uint256ValueToHex(value)),
                                    remaining, consumed);
}

// GetBoolFromData extracts a 32-byte wide boolean value from the
// calldata segment.
std::optional<DecoderResult<base::Value>> GetBoolFromData(ByteView input) {
  auto result = GetUintFromData<uint8_t>(input);
  if (!result) {
    return std::nullopt;
  }

  auto [value, remaining, consumed] = *result;

  if (value == static_cast<uint8_t>(0)) {
    return DecoderResult<base::Value>(base::Value(false), remaining, consumed);
  } else if (value == static_cast<uint8_t>(1)) {
    return DecoderResult<base::Value>(base::Value(true), remaining, consumed);
  }

  return std::nullopt;
}

// GetBytesHexFromData extracts a bytes value from the calldata segment using
// head-tail encoding mechanism. bytes are packed in chunks of 32 bytes, with
// the first 32 bytes encoding the length, followed by the actual content.
//
// The first argument indicates the type of the bytes value to be extracted.
// If the Type::m property is set, it indicates a fixed-size bytes<M> type,
// where 0 < M <= 32, otherwise it indicates a dynamic bytes type.
//
// The result is serialized as a hex string prefixed by "0x".
std::optional<DecoderResult<base::Value>> GetBytesHexFromData(
    const eth_abi::Type& type,
    ByteView input) {
  size_t size = type.m.value_or(0);
  if (size > kWordSize) {
    return std::nullopt;
  }

  ByteView remaining = input;
  size_t parts_count = 1;
  size_t consumed = 0;

  // dynamic bytes
  if (!type.m.has_value()) {
    // Extract the length of the bytes since it is a dynamic type
    auto length_result = GetUintFromData<size_t>(remaining);
    if (!length_result) {
      return std::nullopt;
    }

    size = length_result->result;
    remaining = length_result->remaining;
    consumed += length_result->consumed;
    parts_count = (size + kWordSize - 1) / kWordSize;
  }

  if (size > remaining.size()) {
    return std::nullopt;
  }

  size_t parts_size = parts_count * kWordSize;
  return DecoderResult<base::Value>(
      base::Value("0x" + HexEncodeLower(remaining.data(), size)),
      GetSubByteView(remaining, parts_size), consumed + parts_size);
}

// GetStringFromData extracts a string value from the calldata segment using
// head-tail encoding mechanism. Strings in calldata are represented as bytes,
// with the first 32 bytes encoding the length, followed by the actual content.
std::optional<DecoderResult<base::Value>> GetStringFromData(ByteView input) {
  // Extract the string value from the calldata as dynamic bytes
  auto bytes_result = GetBytesHexFromData(eth_abi::Bytes(), input);
  if (!bytes_result) {
    return std::nullopt;
  }

  auto& bytes_value = bytes_result->result;
  if (!bytes_value.is_string()) {
    return std::nullopt;
  }

  std::string result;
  if (base::HexStringToString(bytes_value.GetString().substr(2), &result)) {
    return DecoderResult<base::Value>(
        base::Value(result), bytes_result->remaining, bytes_result->consumed);
  }

  return std::nullopt;
}

// Forward declarations for recursive functions.
std::optional<DecoderResult<base::Value>> GetTupleFromData(
    const eth_abi::Type& type,
    ByteView input);
std::optional<DecoderResult<base::Value>> GetArrayFromData(
    const eth_abi::Type& type,
    ByteView input);

std::optional<DecoderResult<base::Value>> DecodeParam(const eth_abi::Type& type,
                                                      ByteView input) {
  if (type.kind == eth_abi::TypeKind::kAddress) {
    return GetAddressFromData(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 8) {
    return GetUintHexFromData<uint8_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 16) {
    return GetUintHexFromData<uint16_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 32) {
    return GetUintHexFromData<uint32_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 64) {
    return GetUintHexFromData<uint64_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 128) {
    return GetUintHexFromData<uint128_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kUintM && type.m == 256) {
    return GetUintHexFromData<uint256_t>(input);
  }

  if (type.kind == eth_abi::TypeKind::kBool) {
    return GetBoolFromData(input);
  }

  if (type.kind == eth_abi::TypeKind::kBytes) {
    return GetBytesHexFromData(type, input);
  }

  if (type.kind == eth_abi::TypeKind::kString) {
    return GetStringFromData(input);
  }

  if (type.kind == eth_abi::TypeKind::kArray) {
    return GetArrayFromData(type, input);
  }

  if (type.kind == eth_abi::TypeKind::kTuple) {
    return GetTupleFromData(type, input);
  }

  // Unrecognized types are considered errors.
  NOTREACHED();
}

std::optional<DecoderResult<base::Value>> DecodeParam(
    const std::unique_ptr<eth_abi::Type>& type_ptr,
    ByteView input) {
  return DecodeParam(*type_ptr, input);
}

// IsDynamicType checks if a parameter is a dynamic type or contains dynamic
// types within it. Dynamic types include bytes, string, and dynamic-sized
// arrays.
bool IsDynamicType(const eth_abi::Type& type) {
  if ((type.kind == eth_abi::TypeKind::kBytes && !type.m.has_value()) ||
      type.kind == eth_abi::TypeKind::kString ||
      (type.kind == eth_abi::TypeKind::kArray && !type.m.has_value())) {
    return true;
  }

  if (type.kind == eth_abi::TypeKind::kArray && type.m.has_value()) {
    return IsDynamicType(*type.array_type);
  }

  if (type.kind == eth_abi::TypeKind::kTuple) {
    for (const auto& component_type : type.tuple_types) {
      if (IsDynamicType(component_type)) {
        return true;
      }
    }
  }

  return false;
}

// GetTupleFromData extracts a tuple value from the calldata segment. A tuple
// is represented as a sequence of parameters, each of which is extracted
// according to its type.
//
// Dynamic types within the tuple are extracted using head-tail encoding.
//
// The result is a base::Value object containing a list of the extracted
// values.
std::optional<DecoderResult<base::Value>> GetTupleFromData(
    const eth_abi::Type& type,
    ByteView input) {
  base::Value::List result;
  size_t consumed = 0;
  size_t dynamic_consumed = 0;
  for (const auto& child_type : type.tuple_types) {
    if (IsDynamicType(child_type)) {
      auto offset_result =
          GetUintFromData<size_t>(GetSubByteView(input, consumed));
      if (!offset_result) {
        return std::nullopt;
      }

      auto decoded_result =
          DecodeParam(child_type, GetSubByteView(input, offset_result->result));
      if (!decoded_result) {
        return std::nullopt;
      }

      consumed += offset_result->consumed;
      dynamic_consumed += decoded_result->consumed;
      result.Append(std::move(decoded_result->result));
    } else {
      // static type
      auto decoded_result =
          DecodeParam(child_type, GetSubByteView(input, consumed));
      if (!decoded_result) {
        return std::nullopt;
      }

      consumed += decoded_result->consumed;
      result.Append(std::move(decoded_result->result));
    }
  }

  return DecoderResult<base::Value>(
      base::Value(std::move(result)),
      GetSubByteView(input, consumed + dynamic_consumed),
      consumed + dynamic_consumed);
}

// GetArrayFromData parses a calldata segment to iterate over an array of
// elements. The array type could be <type>[] indicating a dynamic array, or
// <type>[M] where M is the size of the fixed array.
//
// The underlying elements of the array can be a mix of both dynamic and
// fixed-size types. If the array contains at least one dynamic type, the
// entire array is encoded using head-tail encoding, otherwise, the array
// is encoded as a contiguous sequence of elements.
std::optional<DecoderResult<base::Value>> GetArrayFromData(
    const eth_abi::Type& type,
    ByteView input) {
  // Extract the array length. If the array is dynamic, the length is
  // temporarily set to 0, which is later overwritten by the actual length.
  size_t size = type.m.value_or(0);
  size_t consumed = 0;
  ByteView remaining = input;

  // dynamic array
  if (!type.m.has_value()) {
    // Extract the array length since it is a dynamic array
    auto length_result = GetUintFromData<size_t>(input);
    if (!length_result) {
      return std::nullopt;
    }

    size = length_result->result;
    remaining = length_result->remaining;
    consumed = length_result->consumed;
  }

  auto has_dynamic_child = IsDynamicType(*type.array_type);
  base::Value::List result;

  if (has_dynamic_child) {
    for (size_t i = 0; i < size; i++) {
      auto offset_result =
          GetUintFromData<size_t>(GetSubByteView(remaining, i * kWordSize));
      if (!offset_result) {
        return std::nullopt;
      }

      consumed += offset_result->consumed;

      auto decoded_child_result = DecodeParam(
          type.array_type, GetSubByteView(remaining, offset_result->result));
      if (!decoded_child_result) {
        return std::nullopt;
      }

      result.Append(std::move(decoded_child_result->result));
      consumed += decoded_child_result->consumed;
    }

    return DecoderResult<base::Value>(base::Value(std::move(result)),
                                      GetSubByteView(remaining, consumed),
                                      consumed);
  }

  for (size_t i = 0; i < size; i++) {
    auto decoded_child_result =
        DecodeParam(type.array_type, GetSubByteView(input, consumed));
    if (!decoded_child_result) {
      return std::nullopt;
    }

    result.Append(std::move(decoded_child_result->result));
    consumed += decoded_child_result->consumed;
  }

  return DecoderResult<base::Value>(base::Value(std::move(result)),
                                    GetSubByteView(input, consumed), consumed);
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

std::optional<base::Value::List> ABIDecode(const eth_abi::Type& type,
                                           const ByteArray& data) {
  ByteView input = base::make_span(data.data(), data.size());

  auto decoded = DecodeParam(type, input);
  if (!decoded) {
    return std::nullopt;
  }

  if (decoded->result.is_list()) {
    return std::move(decoded->result.GetList());
  }

  return std::nullopt;
}

}  // namespace brave_wallet
