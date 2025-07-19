// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "brave/components/brave_wallet/browser/internal/polkadot/rust/sr25519.h"

namespace brave_wallet {

HDKeySr25519::HDKeySr25519(std::unique_ptr<polkadot::SchnorrkelKeyPair> skp)
    : schnorrkel_key_pair_(std::move(skp)) {}

HDKeySr25519::~HDKeySr25519() = default;

// static
std::unique_ptr<HDKeySr25519> HDKeySr25519::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  auto skp = polkadot::SchnorrkelKeyPair::GenerateFromSeed(seed);
  if (skp) {
    return base::WrapUnique(new HDKeySr25519(std::move(skp)));
  }
  return nullptr;
}

polkadot::SR25519PublicKey HDKeySr25519::GetPublicKey() {
  return schnorrkel_key_pair_->GetPublicKey();
}

polkadot::SR25519Signature HDKeySr25519::SignMessage(
    base::span<const uint8_t> msg) {
  return schnorrkel_key_pair_->SignMessage(msg);
}

bool HDKeySr25519::VerifyMessage(polkadot::SR25519Signature const& sig,
                                 base::span<const uint8_t> msg) {
  return schnorrkel_key_pair_->VerifyMessage(sig, msg);
}

std::unique_ptr<HDKeySr25519> HDKeySr25519::DeriveHard(
    base::span<const uint8_t> derive_junction) {
  return base::WrapUnique(
      new HDKeySr25519(schnorrkel_key_pair_->DeriveHard(derive_junction)));
}

}  // namespace brave_wallet
