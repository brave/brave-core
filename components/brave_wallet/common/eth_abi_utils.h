/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet::eth_abi {
constexpr size_t kRowLength = 32;

using Span = base::span<const uint8_t>;
using Span32 = base::span<const uint8_t, kRowLength>;
using Bytes32 = std::array<uint8_t, 32>;
using Bytes4 = std::array<uint8_t, 4>;

std::pair<Span, Span> ExtractFunctionSelectorAndArgsFromCall(Span data);

EthAddress ExtractAddress(Span address_encoded);
EthAddress ExtractAddressFromTuple(Span data, size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytes(Span bytes_encoded);
absl::optional<std::string> ExtractString(Span string_encoded);

absl::optional<std::string> ExtractStringFromTuple(Span data, size_t tuple_pos);
absl::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    Span data,
    size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytesFromTuple(Span data,
                                                           size_t tuple_pos);
absl::optional<std::vector<uint8_t>>
ExtractFixedBytesFromTuple(Span data, size_t fixed_size, size_t tuple_pos);

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
  std::vector<uint8_t> EncodeWithSelector(Span selector) const;
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

}  // namespace brave_wallet::eth_abi

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
