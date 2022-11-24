/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_

#include <string>
#include <vector>

#include "base/containers/span.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class SolanaAddress {
 public:
  SolanaAddress();
  ~SolanaAddress();
  SolanaAddress(const SolanaAddress& other);
  SolanaAddress& operator=(const SolanaAddress& other);
  SolanaAddress(SolanaAddress&& other);
  SolanaAddress& operator=(SolanaAddress&& other);

  bool operator==(const SolanaAddress& other) const;
  bool operator!=(const SolanaAddress& other) const;

  static absl::optional<SolanaAddress> FromBytes(
      base::span<const uint8_t> bytes);
  static absl::optional<SolanaAddress> FromBytes(std::vector<uint8_t> bytes);
  static absl::optional<SolanaAddress> FromBase58(
      const std::string& base58_string);

  static SolanaAddress ZeroAddress();

  const std::vector<uint8_t>& bytes() const { return bytes_; }

  std::string ToBase58() const;

  bool IsValid() const;

 private:
  explicit SolanaAddress(base::span<const uint8_t> bytes);
  explicit SolanaAddress(std::vector<uint8_t> bytes);

  std::vector<uint8_t> bytes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_SOLANA_ADDRESS_H_
