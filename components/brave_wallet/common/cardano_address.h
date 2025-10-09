/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_

#include <stdint.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"

namespace brave_wallet {

inline constexpr uint32_t kCardanoKeyHashLength = 28;

// Cardano address wrapper based on
// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
class CardanoAddress {
 public:
  enum class AddressType : uint8_t {
    kPaymentKeyHashStakeKeyHash = 0b0000,
    kScriptHashStakeKeyHash = 0b0001,
    kPaymentKeyHashScriptHash = 0b0010,
    kScriptHashScriptHash = 0b0011,

    kPaymentKeyHashPointer = 0b0100,
    kScriptHashPointer = 0b0101,

    kPaymentKeyHashNoDelegation = 0b0110,
    kScriptHashNoDelegation = 0b0111,

    kNoPaymentStakeHash = 0b1110,
    kNoPaymentScriptHash = 0b1111,
  };

  enum class NetworkTag {
    kTestnets = 0b0000,
    kMainnet = 0b0001,
  };

  CardanoAddress();
  ~CardanoAddress();
  CardanoAddress(const CardanoAddress& other);
  CardanoAddress& operator=(const CardanoAddress& other);
  CardanoAddress(CardanoAddress&& other);
  CardanoAddress& operator=(CardanoAddress&& other);
  auto operator<=>(const CardanoAddress& other) const = default;

  static std::optional<CardanoAddress> FromString(std::string_view sv);
  static std::optional<CardanoAddress> FromPayload(
      AddressType address_type,
      NetworkTag network_tag,
      base::span<const uint8_t> payload);
  std::string ToString() const;
  bool IsTestnet() const;

  std::vector<uint8_t> ToCborBytes() const;
  static std::optional<CardanoAddress> FromCborBytes(
      base::span<const uint8_t> bytes);

 private:
  bool IsStakeOnlyAddress() const;
  NetworkTag GetNetworkTag() const;
  AddressType GetAddressType() const;
  std::string_view GetHrp() const;

  std::vector<uint8_t> bytes_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_CARDANO_ADDRESS_H_
