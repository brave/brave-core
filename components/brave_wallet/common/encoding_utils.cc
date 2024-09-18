/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/encoding_utils.h"

#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/third_party/bitcoin-core/src/src/base58.h"

namespace brave_wallet {

std::string Base58EncodeWithCheck(const std::vector<uint8_t>& bytes) {
  auto with_checksum = bytes;
  auto checksum = DoubleSHA256Hash(bytes);
  with_checksum.insert(with_checksum.end(), checksum.begin(),
                       checksum.begin() + 4);
  return Base58Encode(with_checksum);
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

std::optional<std::vector<uint8_t>> Base58Decode(const std::string& str,
                                                 int len,
                                                 bool strict) {
  std::vector<uint8_t> result;
  if (Base58Decode(str, &result, len, strict)) {
    return result;
  }
  return {};
}

std::string Base58Encode(const std::vector<uint8_t>& bytes) {
  return EncodeBase58(bytes);
}

std::string Base58Encode(base::span<const uint8_t> bytes) {
  return EncodeBase58(bytes);
}

}  // namespace brave_wallet
