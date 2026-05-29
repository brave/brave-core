/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <string_view>
#include <vector>

#include "base/containers/span_reader.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "third_party/abseil-cpp/absl/container/inlined_vector.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_wallet {

// Adds `signatures_count` empty signatures to the bytes vector.
void ExtendWithEmptySignatures(std::vector<uint8_t>& bytes,
                               base::StrictNumeric<uint8_t> signatures_count);

// Encode uint16_t value into 1-3 bytes compact-u16.
// See
// https://docs.solana.com/developing/programming-model/transactions#compact-u16-format
absl::InlinedVector<uint8_t, 3> CompactU16Encode(
    base::StrictNumeric<uint16_t> u16);
std::optional<uint16_t> CompactU16Decode(
    base::SpanReader<const uint8_t>& reader);

bool Uint8ArrayDecode(std::string_view str,
                      std::vector<uint8_t>* ret,
                      size_t len);

std::optional<uint8_t> GetUint8FromStringDict(const base::DictValue& dict,
                                              std::string_view key);

// A compact-array is serialized as the array length, followed by each array
// item.
std::optional<std::vector<uint8_t>> CompactArrayDecode(
    base::SpanReader<const uint8_t>& reader);

bool IsValidCommitmentString(std::string_view commitment);

bool IsValidEncodingString(std::string_view encoding);

bool IsSPLToken(const mojom::BlockchainTokenPtr& token);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_UTILS_H_
