/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_

#include <stdint.h>

#include <array>
#include <optional>
#include <string>

#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"

namespace brave_wallet {

class SolanaAddress {
 public:
  SolanaAddress() = delete;
  explicit SolanaAddress(base::span<const uint8_t, kSolanaPubkeySize> bytes);
  ~SolanaAddress();
  SolanaAddress(const SolanaAddress& other);
  SolanaAddress& operator=(const SolanaAddress& other);
  SolanaAddress(SolanaAddress&& other);
  SolanaAddress& operator=(SolanaAddress&& other);

  bool operator==(const SolanaAddress& other) const = default;

  static std::optional<SolanaAddress> ReadFrom(
      base::SpanReader<const uint8_t>& reader);
  static std::optional<SolanaAddress> FromBytes(
      base::span<const uint8_t> bytes);
  static std::optional<SolanaAddress> FromBase58(
      const std::string& base58_string);
  static SolanaAddress ZeroAddress();

  base::span<const uint8_t, kSolanaPubkeySize> bytes() const { return bytes_; }

  std::string ToBase58() const;

 private:
  std::array<uint8_t, kSolanaPubkeySize> bytes_ = {};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_
