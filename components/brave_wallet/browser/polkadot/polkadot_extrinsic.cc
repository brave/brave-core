/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_extrinsic.h"

#include "base/containers/span_rust.h"
#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {

bool IsBoxNonNull(const rust::Box<CxxPolkadotChainMetadata>& chain_metadata) {
  return chain_metadata.operator->();
}

::rust::Str StringViewToRustStr(std::string_view sv) {
  return ::rust::Str(sv.data(), sv.size());
}

}  // namespace

PolkadotChainMetadata::PolkadotChainMetadata(PolkadotChainMetadata&&) = default;

PolkadotChainMetadata::~PolkadotChainMetadata() = default;

std::optional<PolkadotChainMetadata> PolkadotChainMetadata::FromChainName(
    std::string_view chain_name) {
  auto result = make_chain_metadata(StringViewToRustStr(chain_name));
  if (!result->is_ok()) {
    return std::nullopt;
  }

  return PolkadotChainMetadata(result->unwrap());
}

const CxxPolkadotChainMetadata& PolkadotChainMetadata::operator*() const {
  CHECK(IsBoxNonNull(chain_metadata_));
  return *chain_metadata_;
}

PolkadotChainMetadata::PolkadotChainMetadata(
    ::rust::Box<CxxPolkadotChainMetadata> chain_metadata)
    : chain_metadata_(std::move(chain_metadata)) {}

PolkadotUnsignedTransfer::PolkadotUnsignedTransfer(
    base::span<uint8_t, kPolkadotSubstrateAccountIdSize> recipient,
    uint128_t send_amount)
    : send_amount_(send_amount) {
  base::span(recipient_).copy_from_nonoverlapping(recipient);
}

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

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  std::array<uint8_t, 16> send_amount_bytes = {};

  if (!decode_unsigned_transfer_allow_death(*chain_metadata,
                                            base::SpanToRustSlice(bytes),
                                            pubkey, send_amount_bytes)) {
    return std::nullopt;
  }

  auto send_amount = base::bit_cast<uint128_t>(send_amount_bytes);

  return {PolkadotUnsignedTransfer(pubkey, send_amount)};
}

}  // namespace brave_wallet
