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

std::pair<Span, Span> ExtractFunctionSelectorAndArgsFromCall(Span data);

EthAddress ExtractAddress(Span address_encoded);
EthAddress ExtractAddressFromTuple(Span data, size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytes(Span bytes_encoded);
absl::optional<std::string> ExtractString(Span string_encoded);
absl::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    Span data,
    size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytesFromTuple(Span data,
                                                           size_t tuple_pos);
absl::optional<std::vector<uint8_t>>
ExtractFixedBytesFromTuple(Span data, size_t fixed_size, size_t tuple_pos);

// f(bytes,bytes)
std::vector<uint8_t> EncodeCall(Span function_selector,
                                Span bytes_0,
                                Span bytes_1);

// f(bytes32)
std::vector<uint8_t> EncodeCall(Span function_selector, const Span32& arg_0);

}  // namespace brave_wallet::eth_abi

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
