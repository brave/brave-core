/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_utils.h"

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

void CompactU16Encode(uint16_t u16, std::vector<uint8_t>* compact_u16) {
  DCHECK(compact_u16);

  uint16_t rem_val = u16;
  while (true) {
    uint8_t elem = rem_val & 0x7f;
    rem_val >>= 7;
    if (rem_val == 0) {
      compact_u16->push_back(elem);
      break;
    } else {
      elem |= 0x80;
      compact_u16->push_back(elem);
    }
  }
}

absl::optional<std::tuple<uint16_t, size_t>> CompactU16Decode(
    const std::vector<uint8_t>& bytes,
    size_t start_index) {
  uint32_t val = 0;
  for (size_t i = 0; i + start_index < bytes.size(); ++i) {
    if (i == 3) {  // TooLong error.
      return absl::nullopt;
    }
    uint32_t elem = bytes[i + start_index];
    uint32_t elem_val = elem & 0x7f;
    uint32_t elem_done = (elem & 0x80) == 0;

    if (i == 2 && !elem_done) {  // ByteThreeContinues error.
      return absl::nullopt;
    }

    // Alias error.
    if (elem == 0 && i != 0) {
      return absl::nullopt;
    }

    val |= (elem_val) << (i * 7);

    // Overflow error.
    if (val > UINT16_MAX) {
      return absl::nullopt;
    }

    if (elem_done) {
      return std::tuple<uint16_t, size_t>(val, i + 1);
    }
  }

  return absl::nullopt;
}

bool IsBase58EncodedSolanaPubkey(const std::string& key) {
  std::vector<uint8_t> bytes;
  return Base58Decode(key, &bytes, kSolanaPubkeySize);
}

bool Uint8ArrayDecode(const std::string& str,
                      std::vector<uint8_t>* ret,
                      size_t len) {
  if (!base::StartsWith(str, "[") || !base::EndsWith(str, "]")) {
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

absl::optional<std::vector<uint8_t>> CompactArrayDecode(
    const std::vector<uint8_t>& bytes,
    size_t* bytes_index) {
  DCHECK(bytes_index);

  // Decode length.
  auto ret = CompactU16Decode(bytes, *bytes_index);
  if (!ret) {
    return absl::nullopt;
  }

  const uint16_t array_length = std::get<0>(*ret);
  *bytes_index +=
      std::get<1>(*ret) /* array_length for encoded length */ + array_length;
  if (*bytes_index > bytes.size()) {
    return absl::nullopt;
  }

  return std::vector<uint8_t>(bytes.begin() + *bytes_index - array_length,
                              bytes.begin() + *bytes_index);
}

absl::optional<uint8_t> GetUint8FromStringDict(const base::Value::Dict& dict,
                                               const std::string& key) {
  const std::string* string_value = dict.FindString(key);
  if (!string_value) {
    return absl::nullopt;
  }
  unsigned val = 0;
  if (!base::StringToUint(*string_value, &val)) {
    return absl::nullopt;
  }
  if (val > UINT8_MAX) {
    return absl::nullopt;
  }

  return val;
}

bool IsValidCommitmentString(const std::string& commitment) {
  return commitment == "processed" || commitment == "confirmed" ||
         commitment == "finalized";
}

bool IsValidEncodingString(const std::string& encoding) {
  return encoding == "base58" || encoding == "base64" ||
         encoding == "jsonParsed";
}

}  // namespace brave_wallet
