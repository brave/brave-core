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
  while (1) {
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

bool Base58Decode(const std::string& str, std::vector<uint8_t>* ret, int len) {
  DCHECK(ret);
  ret->clear();
  return DecodeBase58(str, *ret, len) && static_cast<int>(ret->size()) == len;
}

std::string Base58Encode(const std::vector<uint8_t>& bytes) {
  return EncodeBase58(bytes);
}

bool IsBase58EncodedSolanaPubkey(const std::string& key) {
  std::vector<uint8_t> bytes;
  return Base58Decode(key, &bytes, kSolanaPubkeySize);
}

}  // namespace brave_wallet
