/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_address.h"

#include <optional>
#include <utility>

#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

SolanaAddress::SolanaAddress() = default;
SolanaAddress::~SolanaAddress() = default;
SolanaAddress::SolanaAddress(const SolanaAddress& other) = default;
SolanaAddress& SolanaAddress::operator=(const SolanaAddress& other) = default;
SolanaAddress::SolanaAddress(SolanaAddress&& other) = default;
SolanaAddress& SolanaAddress::operator=(SolanaAddress&& other) = default;

bool SolanaAddress::operator==(const SolanaAddress& other) const {
  return bytes_ == other.bytes_;
}

bool SolanaAddress::operator!=(const SolanaAddress& other) const {
  return !(*this == other);
}

// static
std::optional<SolanaAddress> SolanaAddress::FromBytes(
    base::span<const uint8_t> bytes) {
  if (bytes.size() != kSolanaPubkeySize) {
    return std::nullopt;
  }

  return SolanaAddress(bytes);
}

// static
std::optional<SolanaAddress> SolanaAddress::FromBytes(
    std::vector<uint8_t> bytes) {
  if (bytes.size() != kSolanaPubkeySize) {
    return std::nullopt;
  }

  return SolanaAddress(std::move(bytes));
}

// static
std::optional<SolanaAddress> SolanaAddress::FromBase58(
    const std::string& base58_string) {
  std::vector<uint8_t> bytes;
  if (!Base58Decode(base58_string, &bytes, kSolanaPubkeySize)) {
    return std::nullopt;
  }

  return SolanaAddress(std::move(bytes));
}

// static
SolanaAddress SolanaAddress::ZeroAddress() {
  return SolanaAddress(std::vector<uint8_t>(kSolanaPubkeySize, 0));
}

SolanaAddress::SolanaAddress(base::span<const uint8_t> bytes)
    : bytes_(bytes.begin(), bytes.end()) {
  DCHECK(IsValid());
}

SolanaAddress::SolanaAddress(std::vector<uint8_t> bytes)
    : bytes_(std::move(bytes)) {
  DCHECK(IsValid());
}

std::string SolanaAddress::ToBase58() const {
  return Base58Encode(bytes_);
}

bool SolanaAddress::IsValid() const {
  return bytes_.size() == static_cast<size_t>(kSolanaPubkeySize);
}

}  // namespace brave_wallet
