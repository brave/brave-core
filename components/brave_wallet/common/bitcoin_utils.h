
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

inline constexpr uint8_t kBitcoinSigHashAll = 1;
inline constexpr uint32_t kBitcoinReceiveIndex = 0;
inline constexpr uint32_t kBitcoinChangeIndex = 1;

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

std::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address);

std::string PubkeyToSegwitAddress(base::span<const uint8_t> pubkey,
                                  bool testnet);

uint64_t ApplyFeeRate(double fee_rate, uint32_t vbytes);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
