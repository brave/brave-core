/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/zcash_utils.h"

#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

std::string PubkeyToTransparentAddress(const std::vector<uint8_t>& pubkey,
                                       bool testnet) {
  std::vector<uint8_t> result = testnet ? std::vector<uint8_t>({0x1d, 0x25})
                                        : std::vector<uint8_t>({0x1c, 0xb8});

  std::vector<uint8_t> data_part = Hash160(pubkey);
  result.insert(result.end(), data_part.begin(), data_part.end());
  return Base58EncodeWithCheck(result);
}

}  // namespace brave_wallet
