/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

using EthAbiSpan = base::span<const uint8_t>;

uint256_t BytesToUint256(EthAbiSpan data);

std::pair<EthAbiSpan, EthAbiSpan> ExtractFunctionSelectorAndArgsFromCall(
    EthAbiSpan data);

EthAddress ExtractAddress(EthAbiSpan address_encoded);
EthAddress ExtractAddressFromTuple(EthAbiSpan data, size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytes(EthAbiSpan bytes_encoded);
absl::optional<std::string> ExtractString(EthAbiSpan string_encoded);
absl::optional<std::vector<std::string>> ExtractStringArrayFromTuple(
    EthAbiSpan data,
    size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractBytesFromTuple(EthAbiSpan data,
                                                           size_t tuple_pos);
absl::optional<std::vector<uint8_t>> ExtractFixedBytesFromTuple(
    EthAbiSpan data,
    size_t fixed_size,
    size_t tuple_pos);

// f(bytes,bytes)
std::vector<uint8_t> EncodeCall(EthAbiSpan function_selector,
                                EthAbiSpan bytes_0,
                                EthAbiSpan bytes_1);

// f(bytes32)
std::vector<uint8_t> EncodeCall(EthAbiSpan function_selector,
                                const uint256_t& arg_0);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ETH_ABI_UTILS_H_
