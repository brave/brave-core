/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cardano_address.h"

#include <optional>
#include <string_view>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_wallet/common/bech32.h"

namespace brave_wallet {

namespace {

// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
constexpr char kMainnetHrp[] = "addr";
constexpr char kTestnetHrp[] = "addr_test";
constexpr char kStakeMainnetHrp[] = "stake";
constexpr char kStakeTestnetHrp[] = "stake_test";

uint8_t MakeHeaderByte(CardanoAddress::AddressType address_type,
                       CardanoAddress::NetworkTag network_tag) {
  return base::to_underlying(address_type) << 4 |
         base::to_underlying(network_tag);
}

CardanoAddress::NetworkTag GetNetworkTagFromHeaderByte(uint8_t header) {
  return static_cast<CardanoAddress::NetworkTag>(header & 0b00001111);
}

CardanoAddress::AddressType GetAddressTypeFromHeaderByte(uint8_t header) {
  return static_cast<CardanoAddress::AddressType>(header >> 4);
}

bool IsPaymentAndDelegationAddressType(
    CardanoAddress::AddressType address_type) {
  return address_type ==
             CardanoAddress::AddressType::kPaymentKeyHashStakeKeyHash ||
         address_type == CardanoAddress::AddressType::kScriptHashStakeKeyHash ||
         address_type ==
             CardanoAddress::AddressType::kPaymentKeyHashScriptHash ||
         address_type == CardanoAddress::AddressType::kScriptHashScriptHash;
}

bool IsPaymentAndPointerAddressType(CardanoAddress::AddressType address_type) {
  return address_type == CardanoAddress::AddressType::kPaymentKeyHashPointer ||
         address_type == CardanoAddress::AddressType::kScriptHashPointer;
}

bool IsPaymentOnlyAddressType(CardanoAddress::AddressType address_type) {
  return address_type ==
             CardanoAddress::AddressType::kPaymentKeyHashNoDelegation ||
         address_type == CardanoAddress::AddressType::kScriptHashNoDelegation;
}

bool IsDelegationOnlyAddressType(CardanoAddress::AddressType address_type) {
  return address_type == CardanoAddress::AddressType::kNoPaymentStakeHash ||
         address_type == CardanoAddress::AddressType::kNoPaymentScriptHash;
}

}  // namespace

CardanoAddress::CardanoAddress() = default;
CardanoAddress::~CardanoAddress() = default;
CardanoAddress::CardanoAddress(const CardanoAddress& other) = default;
CardanoAddress& CardanoAddress::operator=(const CardanoAddress& other) =
    default;
CardanoAddress::CardanoAddress(CardanoAddress&& other) = default;
CardanoAddress& CardanoAddress::operator=(CardanoAddress&& other) = default;

std::optional<CardanoAddress> CardanoAddress::FromString(std::string_view sv) {
  auto decoded = bech32::Decode(sv);
  if (!decoded || decoded->encoding != bech32::Encoding::kBech32) {
    return std::nullopt;
  }

  auto address = FromCborBytes(decoded->data);
  if (!address) {
    return std::nullopt;
  }

  if (address->GetHrp() != decoded->hrp) {
    return std::nullopt;
  }

  return address;
}

// static
std::optional<CardanoAddress> CardanoAddress::FromPayload(
    AddressType address_type,
    NetworkTag network_tag,
    base::span<const uint8_t> payload) {
  if (IsPaymentAndDelegationAddressType(address_type) &&
      payload.size() != 2 * kCardanoKeyHashLength) {
    return std::nullopt;
  }
  if ((IsPaymentOnlyAddressType(address_type) ||
       IsDelegationOnlyAddressType(address_type)) &&
      payload.size() != kCardanoKeyHashLength) {
    return std::nullopt;
  }
  // Pointer part has varying length.
  if (IsPaymentAndPointerAddressType(address_type) &&
      payload.size() <= kCardanoKeyHashLength) {
    return std::nullopt;
  }

  CardanoAddress result;
  result.bytes_.resize(1 + payload.size());

  auto span_writer = base::SpanWriter(base::span(result.bytes_));
  span_writer.Write(MakeHeaderByte(address_type, network_tag));
  span_writer.Write(payload);
  DCHECK_EQ(span_writer.remaining(), 0u);

  return result;
}

// https://cips.cardano.org/cip/CIP-0019#user-facing-encoding
std::string CardanoAddress::ToString() const {
  return bech32::Encode(bytes_, GetHrp(), bech32::Encoding::kBech32);
}

bool CardanoAddress::IsTestnet() const {
  return GetNetworkTag() == NetworkTag::kTestnets;
}

std::vector<uint8_t> CardanoAddress::ToCborBytes() const {
  return bytes_;
}

// static
std::optional<CardanoAddress> CardanoAddress::FromCborBytes(
    base::span<const uint8_t> bytes) {
  if (bytes.empty()) {
    return std::nullopt;
  }

  auto network_tag = GetNetworkTagFromHeaderByte(bytes[0]);
  if (network_tag != NetworkTag::kMainnet &&
      network_tag != NetworkTag::kTestnets) {
    return std::nullopt;
  }

  return FromPayload(GetAddressTypeFromHeaderByte(bytes[0]), network_tag,
                     base::span(bytes).subspan(1u));
}

bool CardanoAddress::IsStakeOnlyAddress() const {
  auto type = GetAddressType();
  return type == AddressType::kNoPaymentStakeHash ||
         type == AddressType::kNoPaymentScriptHash;
}

CardanoAddress::NetworkTag CardanoAddress::GetNetworkTag() const {
  CHECK(!bytes_.empty());
  return GetNetworkTagFromHeaderByte(bytes_[0]);
}

CardanoAddress::AddressType CardanoAddress::GetAddressType() const {
  CHECK(!bytes_.empty());
  return GetAddressTypeFromHeaderByte(bytes_[0]);
}

std::string_view CardanoAddress::GetHrp() const {
  if (IsStakeOnlyAddress()) {
    return IsTestnet() ? kStakeTestnetHrp : kStakeMainnetHrp;
  }
  return IsTestnet() ? kTestnetHrp : kMainnetHrp;
}

}  // namespace brave_wallet
