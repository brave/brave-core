// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/polkadot/rust/sr25519.h"

#include <utility>

#include "brave/components/brave_wallet/browser/internal/polkadot/rust/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::polkadot {

SchnorrkelKeyPair::~SchnorrkelKeyPair() = default;
SchnorrkelKeyPair::SchnorrkelKeyPair(SchnorrkelKeyPair&&) = default;

SchnorrkelKeyPair::SchnorrkelKeyPair(rust::Box<CxxSchnorrkelKeyPair> impl)
    : impl_(std::move(impl)) {}

std::optional<SchnorrkelKeyPair> SchnorrkelKeyPair::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  rust::Slice<const uint8_t> bytes{seed.data(), seed.size()};
  auto mk = generate_sr25519_keypair_from_seed(bytes);
  if (!mk->is_ok()) {
    return std::nullopt;
  }
  return SchnorrkelKeyPair(mk->unwrap());
}

SR25519PublicKey SchnorrkelKeyPair::GetPublicKey() {
  return impl_->get_public_key();
}

SR25519Signature SchnorrkelKeyPair::SignMessage(base::span<const uint8_t> msg) {
  rust::Slice<const uint8_t> bytes{msg.data(), msg.size()};
  return impl_->sign_message(bytes);
}

bool SchnorrkelKeyPair::VerifyMessage(SR25519Signature const& sig,
                                      base::span<const uint8_t> msg) {
  rust::Slice<const uint8_t> sig_bytes{sig.data(), sig.size()};
  rust::Slice<const uint8_t> bytes{msg.data(), msg.size()};
  return impl_->verify_message(sig_bytes, bytes);
}

SchnorrkelKeyPair SchnorrkelKeyPair::DeriveHard(
    base::span<const uint8_t> derive_junction) {
  rust::Slice<const uint8_t> bytes{derive_junction.data(),
                                   derive_junction.size()};
  return SchnorrkelKeyPair(impl_->derive_hard(bytes));
}

}  // namespace brave_wallet::polkadot
