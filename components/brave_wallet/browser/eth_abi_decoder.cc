/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_abi_decoder.h"

#include <limits>
#include <map>
#include <tuple>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

// This file implements decoding rules of calldata according to the Contract
// Application Binary Interface (ABI).
//
// Method arguments are packed in chunks of 32 bytes, with types smaller than
// 32 bytes zero padded if necessary. Fixed-size types like uint256, address
// are represented within the available 32 bytes, however, dynamic types like
// bytes and arrays follow head-tail encoding. In this scheme, the data is
// packaged at the tail-end of the transaction's calldata. The arguments are
// references into the calldata where the content is.
//
// Parsing of complex cases such as composite types, nested arrays, etc., are
// not implemented. The decoder only extracts the calldata tail reference in
// such cases.
//
// Function selector (first 4 bytes) should NOT be part of the calldata being
// parsed.
//
// References:
//   https://docs.soliditylang.org/en/latest/abi-spec.html

namespace brave_wallet {

namespace {
// GetArgFromData extracts a 32-byte wide hex string from the calldata at the
// specified offset. The parsed value is NOT prefixed by "0x".
absl::optional<std::string> GetArgFromData(const std::vector<uint8_t>& input,
                                           size_t offset) {
  if (offset > input.size() || input.size() - offset < 32)
    return absl::nullopt;

  return HexEncodeLower(input.data() + offset, 32);
}

// GetAddressFromData extracts an Ethereum address from the calldata at the
// specified index. The address type is static and 32-bytes wide, but we
// only consider the last 20 bytes, discarding the leading 12 bytes of
// 0-padded chars.
//
// In the future, addresses in Ethereum may become 32 bytes long:
// https://ethereum-magicians.org/t/increasing-address-size-from-20-to-32-bytes
//
// The parsed address value is prefixed by "0x".
absl::optional<std::string> GetAddressFromData(
    const std::vector<uint8_t>& input,
    size_t offset) {
  if (offset > input.size() || input.size() - offset < 32)
    return absl::nullopt;

  return "0x" + HexEncodeLower(input.data() + offset + 12, 20);
}

// GetUint256FromData extracts a 32-byte wide uint256 value from the
// calldata at the specified offset.
absl::optional<uint256_t> GetUint256FromData(const std::vector<uint8_t>& input,
                                             size_t offset) {
  auto value = GetArgFromData(input, offset);
  if (!value)
    return absl::nullopt;

  uint256_t arg;
  if (!HexValueToUint256("0x" + *value, &arg))
    return absl::nullopt;
  return arg;
}

// GetUint256FromData extracts a 32-byte wide uint256 hex value from the
// calldata at the specified offset.
//
// The parsed uint256 value is serialized as a compact hex string (without)
// leading 0s, and prefixed by "0x".
absl::optional<std::string> GetUint256HexFromData(
    const std::vector<uint8_t>& input,
    size_t offset) {
  auto arg_uint256 = GetUint256FromData(input, offset);
  if (!arg_uint256)
    return absl::nullopt;

  return Uint256ValueToHex(*arg_uint256);
}

// GetSizeFromData extracts a 32-byte wide size_t value from the calldata
// at the specified offset.
//
// Using this function to extract an integer outside the range of size_t is
// considered an error. Ideal candidates are calldata tail references, length
// of dynamic types, etc.
absl::optional<size_t> GetSizeFromData(const std::vector<uint8_t>& input,
                                       size_t offset) {
  auto value = GetUint256FromData(input, offset);
  if (!value)
    return absl::nullopt;

  // Since we use value as an array index, we need to cast the type to
  // something that can be used as an index, viz. size_t. To prevent runtime
  // errors, we make sure the value is within safe limits of size_t.
  if (*value > std::numeric_limits<size_t>::max())
    return absl::nullopt;

  return static_cast<size_t>(*value);
}

// GetBoolFromData extracts a 32-byte wide boolean value from the
// calldata at the specified offset.
//
// The parsed bool value is serialized as "true" or "false" strings.
absl::optional<std::string> GetBoolFromData(const std::vector<uint8_t>& input,
                                            size_t offset) {
  auto value = GetUint256FromData(input, offset);
  if (!value)
    return absl::nullopt;

  if (value == static_cast<uint256_t>(0))
    return "false";
  else if (value == static_cast<uint256_t>(1))
    return "true";

  return absl::nullopt;
}

// GetBytesHexFromData extracts a bytes value from the calldata at the
// specified offset using head-tail encoding mechanism. bytes are packed
// tightly in chunks of 32 bytes, with the first 32 bytes encoding the length,
// followed by the actual content.
//
// The parsed bytearray is serialized as a hex string prefixed by "0x".
absl::optional<std::string> GetBytesHexFromData(
    const std::vector<uint8_t>& input,
    size_t offset) {
  auto pointer = GetSizeFromData(input, offset);
  if (!pointer)
    return absl::nullopt;

  auto bytes_len = GetSizeFromData(input, *pointer);
  if (!bytes_len)
    return absl::nullopt;

  if (input.size() < static_cast<uint256_t>(*pointer) + 32 + *bytes_len)
    return absl::nullopt;
  return "0x" + HexEncodeLower(input.data() + *pointer + 32, *bytes_len);
}

// GetAddressArrayFromData parses a calldata sequence to extract a dynamic
// array of addresses at the specified offset using head-tail encoding
// mechanism. The encoding is similar to bytes, with the first 32 bytes
// representing the number of elements in the array, followed by each element.
//
// The parsed data is joined together into a hex string prefixed by "0x".
absl::optional<std::string> GetAddressArrayFromData(
    const std::vector<uint8_t>& input,
    size_t offset) {
  auto pointer = GetSizeFromData(input, offset);
  if (!pointer)
    return absl::nullopt;

  auto array_len = GetSizeFromData(input, *pointer);
  if (!array_len)
    return absl::nullopt;

  size_t array_offset = *pointer + 32;
  std::string arg = "0x";
  for (size_t i = 0; i < *array_len; i++) {
    auto value = GetAddressFromData(input, array_offset);
    if (!value)
      return absl::nullopt;
    base::StrAppend(&arg, {value->substr(2)});
    array_offset += 32;
  }

  return arg;
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
absl::optional<std::vector<std::string>> UniswapEncodedPathDecode(
    const std::string& encoded_path) {
  std::vector<uint8_t> data;
  if (!PrefixedHexStringToBytes(encoded_path, &data))
    return absl::nullopt;
  size_t offset = 0;
  std::vector<std::string> path;

  // The path should be long enough to encode a single-hop swap.
  // 43 = 20(address) + 3(fee) + 20(address)
  if (data.size() < 43)
    return absl::nullopt;

  // Parse first hop address.
  path.push_back("0x" + HexEncodeLower(data.data(), 20));
  offset += 20;

  while (true) {
    if (offset == data.size())
      break;

    // Parse the pool fee, and ignore.
    if (data.size() - offset < 3)
      return absl::nullopt;

    offset += 3;

    // Parse next hop.
    if (data.size() - offset < 20)
      return absl::nullopt;
    path.push_back("0x" + HexEncodeLower(data.data() + offset, 20));
    offset += 20;
  }

  // Require a minimum of 2 addresses for a single-hop swap.
  if (path.size() < 2)
    return absl::nullopt;

  return path;
}

absl::optional<std::tuple<std::vector<std::string>,   // params
                          std::vector<std::string>>>  // args
ABIDecode(const std::vector<std::string>& types,
          const std::vector<uint8_t>& data) {
  size_t offset = 0;
  size_t calldata_tail = 0;
  std::vector<std::string> params;
  std::vector<std::string> args;

  for (const auto& type : types) {
    absl::optional<std::string> value;
    if (type == "address") {
      value = GetAddressFromData(data, offset);
    } else if (type == "uint256") {
      value = GetUint256HexFromData(data, offset);
    } else if (type == "bool") {
      value = GetBoolFromData(data, offset);
    } else if (type == "bytes") {
      value = GetBytesHexFromData(data, offset);
    } else if (type == "address[]") {
      value = GetAddressArrayFromData(data, offset);
    } else {
      // For unknown/unsupported types, we only extract 32-bytes. In case of
      // dynamic types, this value is a calldata reference.
      value = GetArgFromData(data, offset);
    }

    if (!value)
      return absl::nullopt;

    // On encountering a dynamic type, we extract the reference to the start
    // of the tail section of the calldata.
    if ((type == "bytes" || type == "string" || base::EndsWith(type, "[]")) &&
        calldata_tail == 0) {
      auto pointer = GetSizeFromData(data, offset);
      if (!pointer)
        return absl::nullopt;

      calldata_tail = *pointer;
    }

    offset += 32;

    args.push_back(*value);
    params.push_back(type);
  }

  // Extra calldata bytes are ignored.

  return std::make_tuple(params, args);
}

}  // namespace brave_wallet
