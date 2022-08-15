/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

#include <algorithm>
#include <iterator>
#include <limits>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/parameter_pack.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet::eth_abi {
namespace {
constexpr size_t kRowLength = 32;

void Uint256ToBytes(uint256_t value, base::span<uint8_t> destination) {
  DCHECK_EQ(destination.size(), kRowLength);
  for (size_t i = 0; i < kRowLength; ++i) {
    destination[kRowLength - i - 1] = static_cast<uint8_t>(value & 0xFF);
    value >>= 8;
  }
}

size_t PaddedSize(size_t bytes_size) {
  size_t row_pad = bytes_size % kRowLength;
  return bytes_size + (row_pad ? kRowLength - row_pad : 0);
}

size_t PaddedRowCount(size_t bytes_size) {
  return PaddedSize(bytes_size) / kRowLength;
}

absl::optional<size_t> BytesToSize(Span data) {
  auto result = BytesToUint256(data);
  if (result > std::numeric_limits<size_t>::max())
    return absl::nullopt;

  return static_cast<size_t>(result);
}

Span ExtractRows(Span data, size_t row, size_t row_count) {
  if (data.size() % kRowLength)
    return {};

  size_t total_rows = data.size() / kRowLength;
  if (total_rows < row_count)
    return {};
  if (row > total_rows - row_count)
    return {};
  return data.subspan(row * kRowLength, row_count * kRowLength);
}

Span ExtractRow(Span data, size_t row) {
  return ExtractRows(data, row, 1);
}

bool CheckPadding(Span data, size_t data_size) {
  DCHECK_LE(data_size, data.size());
  return base::ranges::all_of(data.subspan(data_size),
                              [](uint8_t b) { return b == 0; });
}

Span ExtractHeadFromTuple(Span data, size_t tuple_pos) {
  return ExtractRow(data, tuple_pos);
}

}  // namespace

// Data must have 32 bytes size.
uint256_t BytesToUint256(Span data) {
  DCHECK_EQ(kRowLength, data.size());

  uint256_t result;
  for (auto& b : data) {
    result <<= 8;
    result |= b;
  }

  return result;
}

std::pair<Span, Span> ExtractFunctionSelectorAndArgsFromCall(Span data) {
  if (data.size() < 4)
    return {};
  if ((data.size() - 4) % kRowLength)
    return {};
  return {data.subspan(0, 4), data.subspan(4)};
}

std::pair<absl::optional<size_t>, Span> ExtractArrayInfo(Span data) {
  auto array_size_row = ExtractRow(data, 0);
  if (array_size_row.empty())
    return {};
  DCHECK_EQ(array_size_row.size(), kRowLength);
  return {BytesToSize(array_size_row), data.subspan(kRowLength)};
}

EthAddress ExtractAddress(Span address_encoded) {
  if (address_encoded.size() != kRowLength)
    return EthAddress();
  return EthAddress::FromBytes(address_encoded.subspan(12));
}

EthAddress ExtractAddressFromTuple(Span data, size_t tuple_pos) {
  // Address is placed in tuple head.
  auto address_head = ExtractHeadFromTuple(data, tuple_pos);
  if (address_head.empty())
    return EthAddress();
  DCHECK_EQ(address_head.size(), kRowLength);
  return ExtractAddress(address_head);
}

absl::optional<std::vector<uint8_t>> ExtractBytes(Span bytes_encoded) {
  // uint256 size followed by padded bytes.
  Span bytes_len_row = ExtractRow(bytes_encoded, 0);
  if (bytes_len_row.empty())
    return absl::nullopt;
  absl::optional<size_t> bytes_len = BytesToSize(bytes_len_row);
  if (!bytes_len)
    return absl::nullopt;
  if (*bytes_len == 0)
    return std::vector<uint8_t>();

  Span padded_bytes_data =
      ExtractRows(bytes_encoded, 1, PaddedRowCount(*bytes_len));
  if (*bytes_len > padded_bytes_data.size())
    return absl::nullopt;
  if (!CheckPadding(padded_bytes_data, *bytes_len))
    return absl::nullopt;
  Span bytes_result = padded_bytes_data.subspan(0, *bytes_len);
  return std::vector<uint8_t>{bytes_result.begin(), bytes_result.end()};
}

absl::optional<std::string> ExtractString(Span string_encoded) {
  // uint256 size followed by padded string bytes.
  Span string_len_row = ExtractRow(string_encoded, 0);
  if (string_len_row.empty())
    return absl::nullopt;
  absl::optional<size_t> string_len = BytesToSize(string_len_row);
  if (!string_len)
    return absl::nullopt;
  if (*string_len == 0)
    return std::string();

  Span padded_string_data =
      ExtractRows(string_encoded, 1, PaddedRowCount(*string_len));
  if (*string_len > padded_string_data.size())
    return absl::nullopt;
  if (!CheckPadding(padded_string_data, *string_len))
    return absl::nullopt;

  Span string_result = padded_string_data.subspan(0, *string_len);
  return std::string{string_result.begin(), string_result.end()};
}

absl::optional<std::vector<std::string>> ExtractStringArray(Span string_array) {
  // Array is stored as size row and tuple of that size.
  auto [tuple_size, tuple_header] = ExtractArrayInfo(string_array);
  if (!tuple_size)
    return absl::nullopt;
  // Row count in array is reasonable upper limit.
  if (*tuple_size > PaddedRowCount(string_array.size()))
    return absl::nullopt;
  if (*tuple_size == 0) {
    return std::vector<std::string>();
  }

  std::vector<std::string> result;
  result.reserve(*tuple_size);
  for (auto i = 0u; i < *tuple_size; ++i) {
    // Each tuple head row contains offset to encoded string.
    Span tuple_element_head = ExtractHeadFromTuple(tuple_header, i);
    if (tuple_element_head.empty())
      return absl::nullopt;

    auto tuple_element_offset = BytesToSize(tuple_element_head);
    if (!tuple_element_offset || *tuple_element_offset > tuple_header.size())
      return absl::nullopt;

    auto string = ExtractString(tuple_header.subspan(*tuple_element_offset));
    if (!string)
      return absl::nullopt;
    result.emplace_back(std::move(*string));
  }
  return result;
}

absl::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    Span data,
    size_t tuple_pos) {
  // Head contains offset to string[] start.
  Span head = ExtractHeadFromTuple(data, tuple_pos);
  if (head.empty())
    return absl::nullopt;

  absl::optional<size_t> offset = BytesToSize(head);
  if (!offset || *offset > data.size())
    return absl::nullopt;

  Span string_array = data.subspan(*offset);
  return ExtractStringArray(string_array);
}

absl::optional<std::vector<uint8_t>> ExtractBytesFromTuple(Span data,
                                                           size_t tuple_pos) {
  // Head contains offset to bytes start.
  Span head = ExtractHeadFromTuple(data, tuple_pos);
  if (head.empty())
    return absl::nullopt;

  absl::optional<size_t> offset = BytesToSize(head);
  if (!offset || *offset > data.size())
    return absl::nullopt;

  Span bytes = data.subspan(*offset);
  return ExtractBytes(bytes);
}

absl::optional<std::vector<uint8_t>>
ExtractFixedBytesFromTuple(Span data, size_t fixed_size, size_t tuple_pos) {
  DCHECK_GT(fixed_size, 0u);
  DCHECK_LE(fixed_size, 32u);
  // Head contains bytes itself.
  Span head = ExtractHeadFromTuple(data, tuple_pos);
  if (head.empty())
    return absl::nullopt;

  DCHECK_EQ(head.size(), kRowLength);

  return std::vector<uint8_t>{head.begin(), head.begin() + fixed_size};
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendRow(std::vector<uint8_t>& destination) {
  destination.resize(destination.size() + kRowLength, 0);
  return kRowLength;
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendRow(std::vector<uint8_t>& destination, uint256_t value) {
  // Append 32 bytes.
  destination.resize(destination.size() + kRowLength, 0);
  // Pick last 32 bytes and copy value to it.
  Uint256ToBytes(value, base::make_span(destination).last(kRowLength));
  return kRowLength;
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendRow(std::vector<uint8_t>& destination, Bytes32 value) {
  DCHECK_EQ(value.size(), kRowLength);
  // Append 32 bytes.
  destination.resize(destination.size() + kRowLength, 0);
  // Pick last 32 bytes and copy value to it.
  base::ranges::copy(value,
                     base::make_span(destination).last(kRowLength).begin());
  return kRowLength;
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendBytesWithPadding(std::vector<uint8_t>& destination, Span bytes) {
  auto padded_size = PaddedSize(bytes.size());
  destination.resize(destination.size() + padded_size);
  base::ranges::copy(bytes,
                     base::make_span(destination).last(padded_size).begin());
  return padded_size;
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendBytes(std::vector<uint8_t>& destination, Span bytes) {
  size_t total_added_bytes = 0;
  total_added_bytes += AppendRow(destination, bytes.size());
  total_added_bytes += AppendBytesWithPadding(destination, bytes);
  return total_added_bytes;
}

// NOLINTNEXTLINE(runtime/references)
void EncodeTuple(std::vector<uint8_t>& destination,
                 Span bytes_0,
                 Span bytes_1) {
  size_t tuple_base = destination.size();
  size_t bytes_added = 0;
  bytes_added += AppendRow(destination);
  bytes_added += AppendRow(destination);

  Uint256ToBytes(uint256_t(bytes_added),
                 base::make_span(destination)
                     .subspan(tuple_base + 0 * kRowLength, kRowLength));
  bytes_added += AppendBytes(destination, bytes_0);

  Uint256ToBytes(uint256_t(bytes_added),
                 base::make_span(destination)
                     .subspan(tuple_base + 1 * kRowLength, kRowLength));
  bytes_added += AppendBytes(destination, bytes_1);
}

std::vector<uint8_t> EncodeCall(Span function_selector,
                                Span bytes_0,
                                Span bytes_1) {
  std::vector<uint8_t> destination(function_selector.begin(),
                                   function_selector.end());
  EncodeTuple(destination, bytes_0, bytes_1);
  return destination;
}

std::vector<uint8_t> EncodeCall(Span function_selector, const Bytes32& arg_0) {
  std::vector<uint8_t> destination(function_selector.begin(),
                                   function_selector.end());
  AppendRow(destination, arg_0);
  return destination;
}

}  // namespace brave_wallet::eth_abi
