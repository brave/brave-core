/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/eth_address.h"

namespace brave_wallet::eth_abi {

constexpr size_t kRowLength = 32;
constexpr size_t kSelectorLength = 4;

using Span = base::span<const uint8_t>;
using Span4 = base::span<const uint8_t, kSelectorLength>;
using Span32 = base::span<const uint8_t, kRowLength>;
using Bytes4 = std::array<uint8_t, kSelectorLength>;
using Bytes32 = std::array<uint8_t, kRowLength>;

namespace internal {

std::optional<Span32> ExtractFixedBytesRowFromTuple(Span data,
                                                    size_t fixed_size,
                                                    size_t tuple_pos);

}

std::pair<Span, Span> ExtractFunctionSelectorAndArgsFromCall(Span data);

EthAddress ExtractAddress(Span address_encoded);
EthAddress ExtractAddressFromTuple(Span data, size_t tuple_pos);
std::optional<std::vector<uint8_t>> ExtractBytes(Span bytes_encoded);
std::optional<std::string> ExtractString(Span string_encoded);

std::optional<std::string> ExtractStringFromTuple(Span data, size_t tuple_pos);
std::optional<std::vector<std::string>> ExtractStringArray(Span data);
std::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    Span data,
    size_t tuple_pos);
std::optional<std::vector<uint8_t>> ExtractBytesFromTuple(Span data,
                                                          size_t tuple_pos);
std::optional<std::pair<bool, std::vector<uint8_t>>> ExtractBoolAndBytes(
    Span data);
std::optional<std::vector<std::pair<bool, std::vector<uint8_t>>>>
ExtractBoolBytesArray(Span string_array);
std::optional<std::vector<std::pair<bool, std::vector<uint8_t>>>>
ExtractBoolBytesArrayFromTuple(Span data, size_t tuple_pos);

template <size_t N>
std::optional<std::array<uint8_t, N>> ExtractFixedBytesFromTuple(
    Span data,
    size_t tuple_pos)
  requires(N > 0 && N <= 32)
{
  auto head = internal::ExtractFixedBytesRowFromTuple(data, N, tuple_pos);
  if (!head) {
    return std::nullopt;
  }

  std::array<uint8_t, N> result;
  base::span(result).copy_from(head->first(N));

  return result;
}

class TupleEncoder {
 public:
  TupleEncoder();
  ~TupleEncoder();
  TupleEncoder& AddAddress(const EthAddress& address);
  TupleEncoder& AddUint256(const uint256_t& val);
  TupleEncoder& AddFixedBytes(Span bytes);
  TupleEncoder& AddBytes(Span bytes);
  TupleEncoder& AddString(const std::string& string);
  TupleEncoder& AddStringArray(const std::vector<std::string>& string_array);

  std::vector<uint8_t> Encode() const;
  std::vector<uint8_t> EncodeWithSelector(Span4 selector) const;
  // NOLINTNEXTLINE(runtime/references)
  void EncodeTo(std::vector<uint8_t>& destination) const;

 private:
  struct Element {
    Element();
    ~Element();
    Element(Element&&);
    Element& operator=(Element&&);
    std::array<uint8_t, kRowLength> head;
    std::vector<uint8_t> tail;
  };

  Element& AppendElement();

  std::vector<uint8_t> function_call_;
  std::vector<Element> elements_;
};

enum class TypeKind {
  kAddress,
  // uint<M> where 0 < M <= 256 and M % 8 == 0
  // uint is an alias for uint256.
  kUintM,
  kBool,
  kBytes,
  kString,
  kArray,
  kTuple,
};

struct Type {
  explicit Type(TypeKind kind);
  Type(TypeKind kind, size_t m);

  // Delete copy constructor and copy assignment operator
  Type(const Type&) = delete;
  Type& operator=(const Type&) = delete;

  // Define move constructor
  Type(Type&& other) noexcept;

  ~Type();

  TypeKind kind;

  // Indicates bit length for fixed-size types if applicable.
  std::optional<size_t> m;

  // Indicates the type of the array elements, if kind is kArray.
  std::unique_ptr<Type> array_type;

  // Indicates the types of the tuple elements, if kind is kTuple.
  std::vector<Type> tuple_types;
};

class TypeBuilder {
 public:
  explicit TypeBuilder(TypeKind kind);
  TypeBuilder(TypeKind kind, size_t m);

  TypeBuilder& SetArrayType(Type array_type);
  TypeBuilder& AddTupleType(Type tuple_type);
  Type build();

 private:
  Type type_;
};

Type Address();
Type Uint(size_t m);
Type Uint();
Type Bool();
Type Bytes();
Type Bytes(size_t m);
Type String();
TypeBuilder Array();
TypeBuilder Array(size_t m);
TypeBuilder Tuple();

}  // namespace brave_wallet::eth_abi

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
