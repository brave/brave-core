/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cardano_address.h"

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/containers/span_rust.h"
#include "base/containers/span_writer.h"
#include "base/containers/to_vector.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/common/bech32.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/lib.rs.h"

namespace brave_wallet {

namespace {

// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
constexpr char kMainnetHrp[] = "addr";
constexpr char kTestnetHrp[] = "addr_test";
constexpr char kStakeMainnetHrp[] = "stake";
constexpr char kStakeTestnetHrp[] = "stake_test";
constexpr size_t kByronAddressSizeLimit = 128u;

uint8_t MakeHeaderByte(CardanoAddress::AddressType address_type,
                       CardanoAddress::NetworkTag network_tag) {
  return std::to_underlying(address_type) << 4 |
         std::to_underlying(network_tag);
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

CardanoAddress::CardanoAddress(std::vector<uint8_t> bytes)
    : bytes_(std::move(bytes)) {}

CardanoAddress::~CardanoAddress() = default;
CardanoAddress::CardanoAddress(const CardanoAddress& other) = default;
CardanoAddress& CardanoAddress::operator=(const CardanoAddress& other) =
    default;
CardanoAddress::CardanoAddress(CardanoAddress&& other) = default;
CardanoAddress& CardanoAddress::operator=(CardanoAddress&& other) = default;

std::optional<CardanoAddress> CardanoAddress::FromString(std::string_view sv) {
  auto decoded = bech32::Decode(sv);
  if (!decoded) {
    // Try to decode as a Byron address.
    auto byron_address =
        Base58Decode(std::string(sv), kByronAddressSizeLimit, false);
    if (!byron_address) {
      return std::nullopt;
    }
    auto address = FromCborBytes(*byron_address);
    if (address && !address->IsByronAddress()) {
      return std::nullopt;
    }
    return address;
  }

  if (decoded->encoding != bech32::Encoding::kBech32) {
    return std::nullopt;
  }

  auto address = FromCborBytes(decoded->data);
  if (!address) {
    return std::nullopt;
  }

  if (address->IsByronAddress()) {
    return std::nullopt;
  }

  if (address->GetHrp() != decoded->hrp) {
    return std::nullopt;
  }

  return address;
}

// static
std::optional<CardanoAddress> CardanoAddress::FromShellyPayload(
    AddressType address_type,
    NetworkTag network_tag,
    base::span<const uint8_t> payload) {
  CHECK(address_type != AddressType::kByronAddress);

  if (IsPaymentAndDelegationAddressType(address_type)) {
    if (payload.size() != 2 * kCardanoKeyHashLength) {
      return std::nullopt;
    }
  } else if ((IsPaymentOnlyAddressType(address_type) ||
              IsDelegationOnlyAddressType(address_type))) {
    if (payload.size() != kCardanoKeyHashLength) {
      return std::nullopt;
    }
  } else if (IsPaymentAndPointerAddressType(address_type)) {
    // Pointer part has varying length.
    if (payload.size() <= kCardanoKeyHashLength) {
      return std::nullopt;
    }
  } else {
    NOTREACHED() << std::to_underlying(address_type);
  }

  std::vector<uint8_t> address_bytes(1 + payload.size());
  auto span_writer = base::SpanWriter(base::span(address_bytes));
  span_writer.Write(MakeHeaderByte(address_type, network_tag));
  span_writer.Write(payload);
  DCHECK_EQ(span_writer.remaining(), 0u);

  return CardanoAddress(std::move(address_bytes));
}

// https://cips.cardano.org/cip/CIP-0019#user-facing-encoding
std::string CardanoAddress::ToString() const {
  if (IsByronAddress()) {
    return Base58Encode(bytes_);
  }

  return bech32::Encode(bytes_, GetHrp(), bech32::Encoding::kBech32);
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

  auto address_type = GetAddressTypeFromHeaderByte(bytes[0]);
  if (address_type == AddressType::kByronAddress) {
    if (!validate_byron_address(base::SpanToRustSlice(bytes))) {
      return std::nullopt;
    }
    return CardanoAddress(base::ToVector(bytes));
  }

  auto network_tag = GetNetworkTagFromHeaderByte(bytes[0]);
  if (network_tag != NetworkTag::kMainnet &&
      network_tag != NetworkTag::kTestnets) {
    return std::nullopt;
  }

  return FromShellyPayload(address_type, network_tag,
                           base::span(bytes).subspan(1u));
}

bool CardanoAddress::IsStakeOnlyAddress() const {
  auto type = GetAddressType();
  return type == AddressType::kNoPaymentStakeHash ||
         type == AddressType::kNoPaymentScriptHash;
}

bool CardanoAddress::IsByronAddress() const {
  return GetAddressType() == AddressType::kByronAddress;
}

CardanoAddress::NetworkTag CardanoAddress::GetNetworkTag() const {
  CHECK(!IsByronAddress());

  return GetNetworkTagFromHeaderByte(bytes_[0]);
}

CardanoAddress::AddressType CardanoAddress::GetAddressType() const {
  return GetAddressTypeFromHeaderByte(bytes_[0]);
}

std::string_view CardanoAddress::GetHrp() const {
  CHECK(!IsByronAddress());

  bool is_testnet = GetNetworkTag() == NetworkTag::kTestnets;

  if (IsStakeOnlyAddress()) {
    return is_testnet ? kStakeTestnetHrp : kStakeMainnetHrp;
  }
  return is_testnet ? kTestnetHrp : kMainnetHrp;
}

}  // namespace brave_wallet
