/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

inline constexpr size_t kZCashDigestSize = 32;
inline constexpr size_t kOrchardRawBytesSize = 43;
inline constexpr uint32_t kZip32Purpose = 32u;
inline constexpr uint32_t kTestnetCoinType = 1u;
inline constexpr uint32_t kDefaultZCashBlockHeightDelta = 20;
inline constexpr uint32_t kDefaultTransparentOutputsCount = 2;
inline constexpr uint32_t kGraceActionsCount = 2;
inline constexpr uint64_t kMarginalFee = 5000;

// https://zips.z.cash/zip-0316#encoding-of-unified-addresses
enum ZCashAddrType {
  kP2PKH = 0x00,
  kP2PSH = 0x01,
  kSapling = 0x02,
  kOrchard = 0x03,
  kMaxValue = kOrchard
};

enum class OrchardAddressKind {
  // External kind, can be used in account addresses
  External,
  // Internal "change" address
  Internal
};

using ParsedAddress = std::pair<ZCashAddrType, std::vector<uint8_t>>;

struct DecodedZCashAddress {
  DecodedZCashAddress();
  ~DecodedZCashAddress();
  DecodedZCashAddress(const DecodedZCashAddress& other);
  DecodedZCashAddress& operator=(const DecodedZCashAddress& other);
  DecodedZCashAddress(DecodedZCashAddress&& other);
  DecodedZCashAddress& operator=(DecodedZCashAddress&& other);

  std::vector<uint8_t> pubkey_hash;
  bool testnet = false;
};

struct OrchardOutput {
  uint64_t value = 0;
  std::array<std::uint8_t, ::brave_wallet::kOrchardRawBytesSize> addr;
  auto operator<=>(const OrchardOutput&) const = default;
};

bool OutputZCashAddressSupported(const std::string& address, bool is_testnet);
// https://zips.z.cash/zip-0317
uint64_t CalculateZCashTxFee(const uint32_t tx_input_count,
                             const uint32_t orchard_actions_count);
bool IsUnifiedAddress(const std::string& address);
bool IsUnifiedTestnetAddress(const std::string& address);

std::string PubkeyToTransparentAddress(base::span<const uint8_t> pubkey,
                                       bool testnet);

std::optional<std::string> PubkeyHashToTransparentAddress(
    base::span<const uint8_t> pubkey_hash,
    bool is_testnet);

std::optional<DecodedZCashAddress> DecodeZCashAddress(
    const std::string& address);

std::vector<uint8_t> ZCashAddressToScriptPubkey(const std::string& address,
                                                bool testnet);

std::optional<std::string> GetMergedUnifiedAddress(
    const std::vector<ParsedAddress>& parts,
    bool is_testnet);

std::optional<std::string> GetOrchardUnifiedAddress(
    base::span<const uint8_t> orchard_part,
    bool is_testnet);

std::optional<std::array<uint8_t, kOrchardRawBytesSize>> GetOrchardRawBytes(
    const std::string& unified_address,
    bool is_testnet);

std::optional<std::vector<uint8_t>> GetTransparentRawBytes(
    const std::string& unified_address,
    bool is_testnet);

std::optional<std::vector<ParsedAddress>> ExtractParsedAddresses(
    const std::string& unified_address,
    bool is_testnet);

std::optional<std::string> ExtractTransparentPart(
    const std::string& unified_address,
    bool is_testnet);

std::optional<std::string> ExtractOrchardPart(
    const std::string& unified_address,
    bool is_testnet);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_
