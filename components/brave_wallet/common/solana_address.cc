/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/solana_address.h"

#include <stddef.h>

#include <optional>

#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

SolanaAddress::~SolanaAddress() = default;
SolanaAddress::SolanaAddress(const SolanaAddress& other) = default;
SolanaAddress& SolanaAddress::operator=(const SolanaAddress& other) = default;
SolanaAddress::SolanaAddress(SolanaAddress&& other) = default;
SolanaAddress& SolanaAddress::operator=(SolanaAddress&& other) = default;

// static
std::optional<SolanaAddress> SolanaAddress::ReadFrom(
    base::SpanReader<const uint8_t>& reader) {
  auto bytes = reader.Read<kSolanaPubkeySize>();
  if (!bytes) {
    return std::nullopt;
  }
  return SolanaAddress(*bytes);
}

// static
std::optional<SolanaAddress> SolanaAddress::FromBytes(
    base::span<const uint8_t> bytes) {
  if (auto fixed_bytes = bytes.to_fixed_extent<kSolanaPubkeySize>()) {
    return SolanaAddress(*fixed_bytes);
  }
  return std::nullopt;
}

// static
std::optional<SolanaAddress> SolanaAddress::FromBase58(
    const std::string& base58_string) {
  auto bytes = Base58Decode(base58_string, kSolanaPubkeySize);
  if (!bytes) {
    return std::nullopt;
  }
  return SolanaAddress::FromBytes(*bytes);
}

// static
SolanaAddress SolanaAddress::ZeroAddress() {
  return SolanaAddress(std::array<uint8_t, kSolanaPubkeySize>());
}

SolanaAddress::SolanaAddress(
    base::span<const uint8_t, kSolanaPubkeySize> bytes) {
  base::span(bytes_).copy_from(bytes);
}

std::string SolanaAddress::ToBase58() const {
  return Base58Encode(bytes_);
}

}  // namespace brave_wallet
