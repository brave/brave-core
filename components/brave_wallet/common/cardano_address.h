/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
inline constexpr uint32_t kPaymentKeyHashLength = 28;
inline constexpr uint32_t kStakeKeyHashLength = 28;

class CardanoAddress {
 public:
  CardanoAddress();
  ~CardanoAddress();
  CardanoAddress(const CardanoAddress& other);
  CardanoAddress& operator=(const CardanoAddress& other);
  CardanoAddress(CardanoAddress&& other);
  CardanoAddress& operator=(CardanoAddress&& other);
  auto operator<=>(const CardanoAddress& other) const = default;

  static std::optional<CardanoAddress> FromString(std::string_view sv);
  static CardanoAddress FromParts(
      bool testnet,
      base::span<const uint8_t, kPaymentKeyHashLength> payment_part,
      base::span<const uint8_t, kStakeKeyHashLength> delegation_part);
  std::string ToString() const;
  bool IsTestnet() const;

  std::vector<uint8_t> ToCborBytes() const;

 private:
  std::vector<uint8_t> bytes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_
