/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_utils.h"

#include "base/check.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

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
    if (i == 3)  // TooLong error.
      return absl::nullopt;
    uint32_t elem = bytes[i + start_index];
    uint32_t elem_val = elem & 0x7f;
    uint32_t elem_done = (elem & 0x80) == 0;

    if (i == 2 && !elem_done)  // ByteThreeContinues error.
      return absl::nullopt;

    // Alias error.
    if (elem == 0 && i != 0)
      return absl::nullopt;

    val |= (elem_val) << (i * 7);

    // Overflow error.
    if (val > UINT16_MAX)
      return absl::nullopt;

    if (elem_done)
      return std::tuple<uint16_t, size_t>(val, i + 1);
  }

  return absl::nullopt;
}

bool Base58Decode(const std::string& str,
                  std::vector<uint8_t>* ret,
                  int len,
                  bool strict) {
  DCHECK(ret);
  ret->clear();
  return DecodeBase58(str, *ret, len) &&
         (!strict || static_cast<int>(ret->size()) == len);
}

std::string Base58Encode(const std::vector<uint8_t>& bytes) {
  return EncodeBase58(bytes);
}

bool IsBase58EncodedSolanaPubkey(const std::string& key) {
  std::vector<uint8_t> bytes;
  return Base58Decode(key, &bytes, kSolanaPubkeySize);
}

}  // namespace brave_wallet
