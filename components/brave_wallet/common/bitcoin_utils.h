
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

constexpr uint8_t kBitcoinSigHashAll = 1;
constexpr uint32_t kBitcoinReceiveIndex = 0;
constexpr uint32_t kBitcoinChangeIndex = 1;

enum BitcoinAddressType {
  kPubkeyHash,
  kScriptHash,
  kWitnessV0ScriptHash,
  kWitnessV0PubkeyHash,
  kWitnessV1Taproot
};

struct DecodedBitcoinAddress {
  DecodedBitcoinAddress();
  DecodedBitcoinAddress(BitcoinAddressType address_type,
                        std::vector<uint8_t> pubkey_hash,
                        bool testnet = false);
  ~DecodedBitcoinAddress();
  DecodedBitcoinAddress(const DecodedBitcoinAddress& other);
  DecodedBitcoinAddress& operator=(const DecodedBitcoinAddress& other);
  DecodedBitcoinAddress(DecodedBitcoinAddress&& other);
  DecodedBitcoinAddress& operator=(DecodedBitcoinAddress&& other);

  BitcoinAddressType address_type;
  std::vector<uint8_t> pubkey_hash;
  bool testnet = false;
};

absl::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address);

std::string PubkeyToSegwitAddress(const std::vector<uint8_t>& pubkey,
                                  bool testnet);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
