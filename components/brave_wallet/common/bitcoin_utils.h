
/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/hash_utils.h"

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

std::optional<DecodedBitcoinAddress> DecodeBitcoinAddress(
    const std::string& address);

std::string PubkeyToSegwitAddress(const std::vector<uint8_t>& pubkey,
                                  bool testnet);

uint64_t ApplyFeeRate(double fee_rate, uint32_t vbytes);

// Bitcoin tx outpoint. Pair of transaction id and its output index.
struct BitcoinOutpoint {
  BitcoinOutpoint();
  ~BitcoinOutpoint();
  BitcoinOutpoint(const BitcoinOutpoint& other);
  BitcoinOutpoint& operator=(const BitcoinOutpoint& other);
  BitcoinOutpoint(BitcoinOutpoint&& other);
  BitcoinOutpoint& operator=(BitcoinOutpoint&& other);
  bool operator==(const BitcoinOutpoint& other) const;
  bool operator!=(const BitcoinOutpoint& other) const;
  bool operator<(const BitcoinOutpoint& other) const;

  std::string ToString() const;
  base::Value::Dict ToValue() const;
  static std::optional<BitcoinOutpoint> FromValue(
      const base::Value::Dict& value);

  static std::optional<BitcoinOutpoint> FromRpc(const std::string& txid,
                                                const std::string& vout);

  SHA256HashArray txid;
  uint32_t index = 0;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BITCOIN_UTILS_H_
