/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/cardano_address.h"

#include <optional>
#include <utility>

#include "base/containers/span_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/bech32.h"

namespace brave_wallet {

namespace {

// https://cips.cardano.org/cip/CIP-0019#shelley-addresses
constexpr char kMainnetHrp[] = "addr";
constexpr char kTestnetHrp[] = "addr_test";

uint8_t GetShellyPaymentStakeHeader(bool testnet) {
  // https://cips.cardano.org/cip/CIP-0019#shelley-addresses
  const uint8_t shelly_type = 0;  // PaymentKeyHash | StakeKeyHash
  const uint8_t network_tag = testnet ? 0 : 1;

  const uint8_t header = (shelly_type << 4) | network_tag;
  return header;
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

  if (decoded->hrp != kMainnetHrp && decoded->hrp != kTestnetHrp) {
    return std::nullopt;
  }

  if (decoded->data.size() != 1 + kPaymentKeyHashLength + kStakeKeyHashLength) {
    return std::nullopt;
  }

  if (decoded->data[0] !=
      GetShellyPaymentStakeHeader(decoded->hrp == kTestnetHrp)) {
    return std::nullopt;
  }

  CardanoAddress result;
  result.bytes_ = std::move(decoded->data);

  return result;
}

// static
CardanoAddress CardanoAddress::FromParts(
    bool testnet,
    base::span<const uint8_t, kPaymentKeyHashLength> payment_part,
    base::span<const uint8_t, kStakeKeyHashLength> delegation_part) {
  CardanoAddress result;
  result.bytes_.resize(1 + kPaymentKeyHashLength + kStakeKeyHashLength);

  auto span_writer = base::SpanWriter(base::span(result.bytes_));
  span_writer.Write(GetShellyPaymentStakeHeader(testnet));
  span_writer.Write(payment_part);
  span_writer.Write(delegation_part);
  DCHECK_EQ(span_writer.remaining(), 0u);

  return result;
}

// https://cips.cardano.org/cip/CIP-0019#user-facing-encoding
std::string CardanoAddress::ToString() const {
  return bech32::Encode(bytes_, IsTestnet() ? kTestnetHrp : kMainnetHrp,
                        bech32::Encoding::kBech32);
}

bool CardanoAddress::IsTestnet() const {
  CHECK(!bytes_.empty());
  return (bytes_[0] & 0b00001111) == 0;
}

std::vector<uint8_t> CardanoAddress::ToCborBytes() const {
  return bytes_;
}

}  // namespace brave_wallet
