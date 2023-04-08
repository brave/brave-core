
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_

#include <map>
#include <string>
#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// TODO(apaymyshev): support more
enum BitcoinAddressType {
  WitnessV0ScriptHash,
  WitnessV0PubkeyHash,
  kWitnessUnknown
};

struct DecodedBitcoinAddress {
  DecodedBitcoinAddress();
  ~DecodedBitcoinAddress();
  DecodedBitcoinAddress(const DecodedBitcoinAddress& other);
  DecodedBitcoinAddress& operator=(const DecodedBitcoinAddress& other);
  DecodedBitcoinAddress(DecodedBitcoinAddress&& other);
  DecodedBitcoinAddress& operator=(DecodedBitcoinAddress&& other);

  BitcoinAddressType address_type;
  std::vector<uint8_t> pubkey_hash;
  uint32_t witness_version = 0;
};

absl::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address,
    bool testnet);

std::string PubkeyToSegwitAddress(const std::vector<uint8_t>& pubkey,
                                  bool testnet);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
