/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/containers/span_rust.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/internal/polkadot_extrinsic.rs.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace brave_wallet {

PolkadotExtrinsicMetadata::PolkadotExtrinsicMetadata() = default;
PolkadotExtrinsicMetadata::~PolkadotExtrinsicMetadata() = default;

PolkadotExtrinsicMetadata::PolkadotExtrinsicMetadata(
    const PolkadotExtrinsicMetadata&) = default;

PolkadotExtrinsicMetadata& PolkadotExtrinsicMetadata::operator=(
    const PolkadotExtrinsicMetadata&) = default;

PolkadotExtrinsicMetadata::PolkadotExtrinsicMetadata(
    PolkadotExtrinsicMetadata&&) = default;

PolkadotExtrinsicMetadata& PolkadotExtrinsicMetadata::operator=(
    PolkadotExtrinsicMetadata&&) = default;

base::DictValue PolkadotExtrinsicMetadata::ToValue() const {
  base::DictValue value;

  value.Set("block_hash", base::HexEncodeLower(block_hash_));
  value.Set("extrinsic", base::HexEncodeLower(extrinsic_));
  value.Set("block_num",
            base::HexEncodeLower(base::byte_span_from_ref(block_num_)));
  value.Set("mortality_period",
            base::HexEncodeLower(base::byte_span_from_ref(mortality_period_)));

  return value;
}

// static
std::optional<PolkadotExtrinsicMetadata> PolkadotExtrinsicMetadata::FromValue(
    const base::DictValue& value) {
  PolkadotExtrinsicMetadata metadata;

  const auto* json_block_hash = value.FindString("block_hash");
  if (!json_block_hash) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(*json_block_hash, metadata.block_hash_)) {
    return std::nullopt;
  }

  const auto* json_extrinsic = value.FindString("extrinsic");
  if (!json_extrinsic) {
    return std::nullopt;
  }
  if (!json_extrinsic->empty() &&
      !base::HexStringToBytes(*json_extrinsic, &metadata.extrinsic_)) {
    return std::nullopt;
  }

  const auto* json_block_num = value.FindString("block_num");
  if (!json_block_num) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(*json_block_num,
                             base::byte_span_from_ref(metadata.block_num_))) {
    return std::nullopt;
  }

  const auto* json_mortality_period = value.FindString("mortality_period");
  if (!json_mortality_period) {
    return std::nullopt;
  }
  if (!base::HexStringToSpan(
          *json_mortality_period,
          base::byte_span_from_ref(metadata.mortality_period_))) {
    return std::nullopt;
  }

  return metadata;
}
PolkadotUnsignedTransfer::PolkadotUnsignedTransfer(
    base::span<uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    uint128_t send_amount)
    : send_amount_(send_amount) {
  base::span(recipient_).copy_from_nonoverlapping(recipient);
}

PolkadotUnsignedExtrinsic::~PolkadotUnsignedExtrinsic() = default;

std::string PolkadotUnsignedTransfer::Encode(
    const PolkadotChainMetadata& chain_metadata) const {
  std::array<uint8_t, 16> send_amount_bytes = {};
  base::span(send_amount_bytes)
      .copy_from(base::byte_span_from_ref(send_amount_));

  auto buf = encode_unsigned_transfer_allow_death(
      *chain_metadata, send_amount_bytes, recipient_);

  return base::HexEncodeLower(buf);
}

// static
std::optional<PolkadotUnsignedTransfer> PolkadotUnsignedTransfer::Decode(
    const PolkadotChainMetadata& chain_metadata,
    std::string_view input) {
  std::vector<uint8_t> bytes;
  if (!base::HexStringToBytes(input, &bytes)) {
    return std::nullopt;
  }

  auto result = decode_unsigned_transfer_allow_death(
      *chain_metadata, base::SpanToRustSlice(bytes));

  if (!result->is_ok()) {
    return std::nullopt;
  }

  auto decoded = result->unwrap();

  return {PolkadotUnsignedTransfer(
      decoded->recipient,
      base::bit_cast<uint128_t>(decoded->send_amount_bytes))};
}

}  // namespace brave_wallet
