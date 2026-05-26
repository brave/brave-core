/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_utils.h"

#include <optional>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"

namespace brave_wallet {

void ExtendWithEmptySignatures(std::vector<uint8_t>& bytes,
                               uint8_t signatures_count) {
  bytes.insert(bytes.end(), signatures_count * kSolanaSignatureSize, 0u);
}

absl::InlinedVector<uint8_t, 3> CompactU16Encode(uint16_t u16) {
  absl::InlinedVector<uint8_t, 3> compact_u16;
  while (true) {
    uint8_t elem = u16 & 0x7f;
    u16 >>= 7;
    if (u16 == 0) {
      compact_u16.push_back(elem);
      return compact_u16;
    }

    compact_u16.push_back(elem | 0x80);
  }
}

std::optional<uint16_t> CompactU16Decode(
    base::SpanReader<const uint8_t>& reader) {
  uint32_t val = 0;
  for (size_t i = 0; i < 3; ++i) {
    if (i == 3) {  // TooLong error.
      return std::nullopt;
    }
    uint8_t elem = 0;
    if (!reader.ReadU8LittleEndian(elem)) {
      return std::nullopt;
    }
    uint32_t elem_val = elem & 0x7f;
    bool elem_done = (elem & 0x80) == 0;

    if (i == 2 && !elem_done) {  // ByteThreeContinues error.
      return std::nullopt;
    }

    // Alias error.
    if (elem == 0 && i != 0) {
      return std::nullopt;
    }

    val |= elem_val << (i * 7);

    // Overflow error.
    if (val > UINT16_MAX) {
      return std::nullopt;
    }

    if (elem_done) {
      return val;
    }
  }

  return std::nullopt;
}

bool Uint8ArrayDecode(std::string_view str,
                      std::vector<uint8_t>* ret,
                      size_t len) {
  if (!str.starts_with('[') || !str.ends_with(']')) {
    return false;
  }
  DCHECK(ret);
  ret->clear();
  for (const auto& item : base::SplitStringPiece(
           str, "[,]", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    uint32_t item_int;
    if (!base::StringToUint(item, &item_int) || item_int > UINT8_MAX) {
      ret->clear();
      return false;
    }
    ret->push_back(static_cast<uint8_t>(item_int));
  }
  if (ret->empty() || ret->size() != len) {
    ret->clear();
    return false;
  }
  return true;
}

std::optional<std::vector<uint8_t>> CompactArrayDecode(
    base::SpanReader<const uint8_t>& reader) {
  // Decode length.
  auto length = CompactU16Decode(reader);
  if (!length) {
    return std::nullopt;
  }

  auto data = reader.Read(*length);
  if (!data) {
    return std::nullopt;
  }
  return base::ToVector(*data);
}

std::optional<uint8_t> GetUint8FromStringDict(const base::DictValue& dict,
                                              std::string_view key) {
  const std::string* string_value = dict.FindString(key);
  if (!string_value) {
    return std::nullopt;
  }
  unsigned val = 0;
  if (!base::StringToUint(*string_value, &val)) {
    return std::nullopt;
  }
  if (val > UINT8_MAX) {
    return std::nullopt;
  }

  return val;
}

bool IsValidCommitmentString(std::string_view commitment) {
  return commitment == "processed" || commitment == "confirmed" ||
         commitment == "finalized";
}

bool IsValidEncodingString(std::string_view encoding) {
  return encoding == "base58" || encoding == "base64" ||
         encoding == "jsonParsed";
}

bool IsSPLToken(const mojom::BlockchainTokenPtr& token) {
  return token->coin == mojom::CoinType::SOL &&
         !token->contract_address.empty();
}

}  // namespace brave_wallet
