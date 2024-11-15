/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_abi_utils.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <optional>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"

namespace brave_wallet::eth_abi {
namespace {

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

uint256_t BytesToUint256(Span32 data) {
  uint256_t result = 0;
  for (auto& b : data) {
    result <<= uint256_t(8);
    result |= b;
  }

  return result;
}

bool BytesToBool(Span32 data) {
  auto value = BytesToUint256(data);
  return value != 0;
}

std::optional<size_t> BytesToSize(Span32 data) {
  auto result = BytesToUint256(data);
  if (result > std::numeric_limits<size_t>::max()) {
    return std::nullopt;
  }

  return static_cast<size_t>(result);
}

std::optional<Span32> ToSpan32(Span data) {
  if (data.size() != kRowLength) {
    return std::nullopt;
  }

  return UNSAFE_TODO(Span32(data.data(), kRowLength));
}

Span ExtractRows(Span data, size_t row, size_t row_count) {
  if (data.size() % kRowLength) {
    return {};
  }

  size_t total_rows = data.size() / kRowLength;
  if (total_rows < row_count) {
    return {};
  }
  if (row > total_rows - row_count) {
    return {};
  }
  return data.subspan(row * kRowLength, row_count * kRowLength);
}

std::optional<Span32> ExtractRow(Span data, size_t row) {
  return ToSpan32(ExtractRows(data, row, 1));
}

bool CheckPadding(Span data, size_t padded_data_size) {
  if (data.size() < padded_data_size) {
    return false;
  }
  return base::ranges::all_of(data.subspan(padded_data_size),
                              [](uint8_t b) { return b == 0; });
}

std::optional<Span32> ExtractHeadFromTuple(Span data, size_t tuple_pos) {
  return ExtractRow(data, tuple_pos);
}

EthAddress ExtractAddress(Span32 address_encoded) {
  return EthAddress::FromBytes(address_encoded.subspan(12));
}

}  // namespace

std::pair<Span, Span> ExtractFunctionSelectorAndArgsFromCall(Span data) {
  if (data.size() < 4) {
    return {};
  }
  if ((data.size() - 4) % kRowLength) {
    return {};
  }
  return {data.subspan(0, 4), data.subspan(4)};
}

std::pair<std::optional<size_t>, Span> ExtractArrayInfo(Span data) {
  auto array_size_row = ExtractRow(data, 0);
  if (!array_size_row) {
    return {};
  }
  if (data.size() < kRowLength) {
    return {};
  }
  return {BytesToSize(*array_size_row), data.subspan(kRowLength)};
}

EthAddress ExtractAddress(Span address_encoded) {
  auto span32 = ToSpan32(address_encoded);
  if (!span32) {
    return EthAddress();
  }
  return ExtractAddress(*span32);
}

EthAddress ExtractAddressFromTuple(Span data, size_t tuple_pos) {
  // Address is placed in tuple head.
  auto address_head = ExtractHeadFromTuple(data, tuple_pos);
  if (!address_head) {
    return EthAddress();
  }
  return ExtractAddress(*address_head);
}

std::optional<std::vector<uint8_t>> ExtractBytes(Span bytes_encoded) {
  // uint256 size followed by padded bytes.
  auto bytes_len_row = ExtractRow(bytes_encoded, 0);
  if (!bytes_len_row) {
    return std::nullopt;
  }
  std::optional<size_t> bytes_len = BytesToSize(*bytes_len_row);
  if (!bytes_len) {
    return std::nullopt;
  }
  if (*bytes_len == 0) {
    return std::vector<uint8_t>();
  }

  Span padded_bytes_data =
      ExtractRows(bytes_encoded, 1, PaddedRowCount(*bytes_len));
  if (*bytes_len > padded_bytes_data.size()) {
    return std::nullopt;
  }
  if (!CheckPadding(padded_bytes_data, *bytes_len)) {
    return std::nullopt;
  }
  Span bytes_result = padded_bytes_data.subspan(0, *bytes_len);
  return std::vector<uint8_t>{bytes_result.begin(), bytes_result.end()};
}

std::optional<std::string> ExtractString(Span string_encoded) {
  // uint256 size followed by padded string bytes.
  auto string_len_row = ExtractRow(string_encoded, 0);
  if (!string_len_row) {
    return std::nullopt;
  }
  std::optional<size_t> string_len = BytesToSize(*string_len_row);
  if (!string_len) {
    return std::nullopt;
  }
  if (*string_len == 0) {
    return std::string();
  }

  Span padded_string_data =
      ExtractRows(string_encoded, 1, PaddedRowCount(*string_len));
  if (*string_len > padded_string_data.size()) {
    return std::nullopt;
  }
  if (!CheckPadding(padded_string_data, *string_len)) {
    return std::nullopt;
  }

  Span string_result = padded_string_data.subspan(0, *string_len);
  return std::string{string_result.begin(), string_result.end()};
}

std::optional<std::vector<std::string>> ExtractStringArray(Span string_array) {
  // Array is stored as size row and tuple of that size.
  auto [tuple_size, tuple_header] = ExtractArrayInfo(string_array);
  if (!tuple_size) {
    return std::nullopt;
  }
  // Row count in array is reasonable upper limit.
  if (*tuple_size > PaddedRowCount(string_array.size())) {
    return std::nullopt;
  }
  if (*tuple_size == 0) {
    return std::vector<std::string>();
  }

  std::vector<std::string> result;
  result.reserve(*tuple_size);
  for (auto i = 0u; i < *tuple_size; ++i) {
    // Each tuple head row contains offset to encoded string.
    auto tuple_element_head = ExtractHeadFromTuple(tuple_header, i);
    if (!tuple_element_head) {
      return std::nullopt;
    }

    auto tuple_element_offset = BytesToSize(*tuple_element_head);
    if (!tuple_element_offset || *tuple_element_offset > tuple_header.size()) {
      return std::nullopt;
    }

    auto string = ExtractString(tuple_header.subspan(*tuple_element_offset));
    if (!string) {
      return std::nullopt;
    }
    result.emplace_back(std::move(*string));
  }
  return result;
}

std::optional<std::pair<bool, std::vector<uint8_t>>> ExtractBoolAndBytes(
    Span data) {
  auto bool_row = ExtractRow(data, 0);
  if (!bool_row) {
    return std::nullopt;
  }

  auto bytes = ExtractBytesFromTuple(data, 1);
  if (!bytes) {
    return std::nullopt;
  }

  return std::make_pair(BytesToBool(*bool_row), std::move(*bytes));
}

std::optional<std::vector<std::pair<bool, std::vector<uint8_t>>>>
ExtractBoolBytesArrayFromTuple(Span data, size_t tuple_pos) {
  // Head row contains offset to (bool, bytes)[] start.
  auto array_head = ExtractHeadFromTuple(data, tuple_pos);
  if (!array_head) {
    return std::nullopt;
  }

  std::optional<size_t> array_offset = BytesToSize(*array_head);
  if (!array_offset || *array_offset > data.size()) {
    return std::nullopt;
  }

  Span array_data = data.subspan(*array_offset);
  return ExtractBoolBytesArray(array_data);
}

std::optional<std::vector<std::pair<bool, std::vector<uint8_t>>>>
ExtractBoolBytesArray(Span tuple_array) {
  // Array is stored as size row and tuple of that size.
  auto [tuple_size, tuple_header] = ExtractArrayInfo(tuple_array);
  if (!tuple_size) {
    return std::nullopt;
  }
  // Row count in array is reasonable upper limit.
  if (*tuple_size > PaddedRowCount(tuple_array.size())) {
    return std::nullopt;
  }
  if (*tuple_size == 0) {
    return std::vector<std::pair<bool, std::vector<uint8_t>>>();
  }

  std::vector<std::pair<bool, std::vector<uint8_t>>> result;
  result.reserve(*tuple_size);
  for (auto i = 0u; i < *tuple_size; ++i) {
    // Each tuple head row contains offset to encoded tuple.
    auto tuple_element_head = ExtractHeadFromTuple(tuple_header, i);
    if (!tuple_element_head) {
      return std::nullopt;
    }

    auto tuple_element_offset = BytesToSize(*tuple_element_head);
    if (!tuple_element_offset || *tuple_element_offset > tuple_header.size()) {
      return std::nullopt;
    }

    auto bool_bytes =
        ExtractBoolAndBytes(tuple_header.subspan(*tuple_element_offset));
    if (!bool_bytes) {
      return std::nullopt;
    }
    result.emplace_back(std::move(*bool_bytes));
  }

  return result;
}

std::optional<std::string> ExtractStringFromTuple(Span data, size_t tuple_pos) {
  // Head contains offset to string start.
  auto head = ExtractHeadFromTuple(data, tuple_pos);
  if (!head) {
    return std::nullopt;
  }

  std::optional<size_t> offset = BytesToSize(*head);
  if (!offset || *offset > data.size()) {
    return std::nullopt;
  }

  Span string = data.subspan(*offset);
  return ExtractString(string);
}

std::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    Span data,
    size_t tuple_pos) {
  // Head contains offset to string[] start.
  auto head = ExtractHeadFromTuple(data, tuple_pos);
  if (!head) {
    return std::nullopt;
  }

  std::optional<size_t> offset = BytesToSize(*head);
  if (!offset || *offset > data.size()) {
    return std::nullopt;
  }

  Span string_array = data.subspan(*offset);
  return ExtractStringArray(string_array);
}

std::optional<std::vector<uint8_t>> ExtractBytesFromTuple(Span data,
                                                          size_t tuple_pos) {
  // Head contains offset to bytes start.
  auto head = ExtractHeadFromTuple(data, tuple_pos);
  if (!head) {
    return std::nullopt;
  }

  std::optional<size_t> offset = BytesToSize(*head);
  if (!offset || *offset > data.size()) {
    return std::nullopt;
  }

  Span bytes = data.subspan(*offset);
  return ExtractBytes(bytes);
}

std::optional<std::vector<uint8_t>>
ExtractFixedBytesFromTuple(Span data, size_t fixed_size, size_t tuple_pos) {
  CHECK(fixed_size > 0 && fixed_size <= 32);
  // Head contains bytes itself.
  auto head = ExtractHeadFromTuple(data, tuple_pos);
  if (!head) {
    return std::nullopt;
  }

  if (!CheckPadding(head->subspan(0), fixed_size)) {
    return std::nullopt;
  }

  return std::vector<uint8_t>{head->begin(), head->begin() + fixed_size};
}

// NOLINTNEXTLINE(runtime/references)
size_t AppendEmptyRow(std::vector<uint8_t>& destination) {
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
size_t AppendRow(std::vector<uint8_t>& destination, Span32 value) {
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
  bytes_added += AppendEmptyRow(destination);  // bytes_0 offset placeholder
  bytes_added += AppendEmptyRow(destination);  // bytes_1 offset placeholder

  // fill bytes_0 offset placeholder
  Uint256ToBytes(uint256_t(bytes_added),
                 base::make_span(destination)
                     .subspan(tuple_base)
                     .subspan(0 * kRowLength, kRowLength));

  bytes_added += AppendBytes(destination, bytes_0);

  // fill bytes_1 offset placeholder
  Uint256ToBytes(uint256_t(bytes_added),
                 base::make_span(destination)
                     .subspan(tuple_base)
                     .subspan(1 * kRowLength, kRowLength));

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

std::vector<uint8_t> EncodeCall(Span function_selector, const Span32& arg_0) {
  std::vector<uint8_t> destination(function_selector.begin(),
                                   function_selector.end());
  AppendRow(destination, arg_0);
  return destination;
}

TupleEncoder::TupleEncoder() = default;
TupleEncoder::~TupleEncoder() = default;

TupleEncoder::Element::Element() {
  head.fill(0);
}
TupleEncoder::Element::~Element() = default;
TupleEncoder::Element::Element(Element&&) = default;
TupleEncoder::Element& TupleEncoder::Element::operator=(Element&&) = default;

TupleEncoder& TupleEncoder::AddAddress(const EthAddress& address) {
  DCHECK(address.IsValid());
  auto& element = AppendElement();
  auto address_size = address.bytes().size();
  DCHECK_GE(element.head.size(), address_size);
  // Address is uint160 which should be right aligned in 32 bytes row.
  base::ranges::copy(address.bytes(),
                     base::make_span(element.head).last(address_size).begin());
  return *this;
}

TupleEncoder& TupleEncoder::AddUint256(const uint256_t& val) {
  auto& element = AppendElement();
  Uint256ToBytes(val, element.head);
  return *this;
}

TupleEncoder& TupleEncoder::AddFixedBytes(Span bytes) {
  DCHECK_GT(bytes.size(), 0u);
  DCHECK_LE(bytes.size(), kRowLength);
  auto& element = AppendElement();
  // Copy bytes at the beginning of head. Remaining bytes are padded with 0.
  base::ranges::copy(bytes.first(std::min(bytes.size(), kRowLength)),
                     element.head.begin());
  return *this;
}

TupleEncoder& TupleEncoder::AddBytes(Span bytes) {
  auto& element = AppendElement();
  AppendBytes(element.tail, bytes);
  return *this;
}

TupleEncoder& TupleEncoder::AddString(const std::string& string) {
  auto& element = AppendElement();
  AppendBytes(element.tail, base::as_bytes(base::make_span(string)));
  return *this;
}

TupleEncoder& TupleEncoder::AddStringArray(
    const std::vector<std::string>& string_array) {
  auto& element = AppendElement();
  // Encoded as tuple size.
  AppendRow(element.tail, uint256_t(string_array.size()));

  // And then tuple itself.
  TupleEncoder string_tuple;
  for (auto& str : string_array) {
    string_tuple.AddString(str);
  }
  string_tuple.EncodeTo(element.tail);
  return *this;
}

TupleEncoder::Element& TupleEncoder::AppendElement() {
  return elements_.emplace_back();
}

std::vector<uint8_t> TupleEncoder::Encode() const {
  std::vector<uint8_t> result;
  EncodeTo(result);
  return result;
}

std::vector<uint8_t> TupleEncoder::EncodeWithSelector(Span4 selector) const {
  DCHECK_EQ(4u, selector.size());
  std::vector<uint8_t> result(selector.begin(), selector.end());
  EncodeTo(result);
  return result;
}

void TupleEncoder::EncodeTo(std::vector<uint8_t>& destination) const {
  size_t tuple_base = destination.size();
  size_t bytes_added = 0;
  // Fills head rows with in-place values or with empty offset placeholders.
  for (auto& element : elements_) {
    DCHECK_EQ(kRowLength, element.head.size());
    bytes_added += AppendRow(destination, element.head);
  }

  for (auto i = 0u; i < elements_.size(); ++i) {
    if (elements_[i].tail.empty()) {
      continue;
    }

    // Fills offset placeholder with current bytes offset.
    Uint256ToBytes(uint256_t(bytes_added),
                   base::make_span(destination)
                       .subspan(tuple_base)
                       .subspan(i * kRowLength, kRowLength));

    bytes_added += AppendBytesWithPadding(destination, elements_[i].tail);
  }
}

Type::Type(TypeKind kind)
    : kind(kind), m(std::nullopt), array_type(nullptr), tuple_types() {}
Type::Type(TypeKind kind, size_t m)
    : kind(kind), m(m), array_type(nullptr), tuple_types() {}
Type::~Type() = default;
Type::Type(Type&& other) noexcept
    : kind(other.kind),
      m(other.m),
      array_type(std::move(other.array_type)),
      tuple_types(std::move(other.tuple_types)) {}

TypeBuilder::TypeBuilder(TypeKind kind) : type_{kind} {}
TypeBuilder::TypeBuilder(TypeKind kind, size_t m) : type_{kind, m} {}

TypeBuilder& TypeBuilder::SetArrayType(Type array_type) {
  type_.array_type = std::make_unique<Type>(std::move(array_type));
  return *this;
}

TypeBuilder& TypeBuilder::AddTupleType(Type tuple_type) {
  type_.tuple_types.push_back(std::move(tuple_type));
  return *this;
}

Type TypeBuilder::build() {
  return std::move(type_);
}

TypeBuilder Array(size_t m) {
  return TypeBuilder(TypeKind::kArray, m);
}

Type Address() {
  return Type{TypeKind::kAddress};
}

Type Uint(size_t m) {
  CHECK(m > 0 && m <= 256 && m % 8 == 0) << "Invalid M for uint<M> type: " << m;
  return Type{TypeKind::kUintM, m};
}

Type Uint() {
  return Type{TypeKind::kUintM, 256};
}

Type Bool() {
  return Type{TypeKind::kBool};
}

Type Bytes() {
  return Type{TypeKind::kBytes};
}

Type Bytes(size_t m) {
  CHECK(m > 0 && m <= 32) << "Invalid M for bytes<M> type: " << m;
  return Type{TypeKind::kBytes, m};
}

Type String() {
  return Type{TypeKind::kString};
}

TypeBuilder Array() {
  return TypeBuilder(TypeKind::kArray);
}

TypeBuilder Tuple() {
  return TypeBuilder(TypeKind::kTuple);
}

}  // namespace brave_wallet::eth_abi
