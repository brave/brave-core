/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "base/containers/span.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"

namespace brave_wallet {

// Encode uint16_t value into 1-3 bytes compact-u16.
// See
// https://docs.solana.com/developing/programming-model/transactions#compact-u16-format
void CompactU16Encode(uint16_t u16, std::vector<uint8_t>* compact_u16);

std::optional<std::tuple<uint16_t, size_t>> CompactU16Decode(
    const std::vector<uint8_t>& compact_u16,
    size_t start_index);

bool IsBase58EncodedSolanaPubkey(const std::string& key);

bool Uint8ArrayDecode(std::string_view str,
                      std::vector<uint8_t>* ret,
                      size_t len);

std::optional<uint8_t> GetUint8FromStringDict(const base::Value::Dict& dict,
                                              std::string_view key);

// A compact-array is serialized as the array length, followed by each array
// item. bytes_index will be increased by the bytes read (consumed) in this
// function.
std::optional<std::vector<uint8_t>> CompactArrayDecode(
    const std::vector<uint8_t>& bytes,
    size_t* bytes_index);

bool IsValidCommitmentString(std::string_view commitment);

bool IsValidEncodingString(std::string_view encoding);

bool IsSPLToken(const mojom::BlockchainTokenPtr& token);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_
