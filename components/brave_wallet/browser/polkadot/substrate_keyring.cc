/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/substrate_keyring.h"

#include "base/containers/span.h"
#include "base/containers/span_writer.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"

namespace brave_wallet {

PolkadotSubstrateKeyring::PolkadotSubstrateKeyring(base::span<const uint8_t> seed) {
  root_ = HDKeyEd25519::GenerateFromSeed(seed);
}
PolkadotSubstrateKeyring::~PolkadotSubstrateKeyring() = default;

std::optional<std::string> PolkadotSubstrateKeyring::GetAccountAddress(uint16_t network_prefix, size_t index) {
  CHECK_EQ(index, 0u);
  Ss58Address address;
  address.prefix = network_prefix;
  auto pk_writer = base::SpanWriter(base::span(address.public_key));
  pk_writer.Write(root_->GetPublicKeyAsSpan());
  return Ss58Encode(address);
}

std::optional<std::string> PolkadotSubstrateKeyring::AddNewHDAccount(size_t index) {
  auto addr = GetAccountAddress(0, index);
  LOG(ERROR) << "XXXZZZ " << addr.value();
  return addr;
}

}  // brave_wallet
