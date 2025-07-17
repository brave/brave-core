// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/polkadot/rust/sr25519.h"

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/internal/polkadot/rust/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet::polkadot {

SchnorrkelKeyPair::~SchnorrkelKeyPair() = default;

class SchnorrkelKeyPairImpl : public SchnorrkelKeyPair {
 public:
  explicit SchnorrkelKeyPairImpl(rust::Box<CxxSchnorrkelKeyPair> impl);

  ~SchnorrkelKeyPairImpl() override;
  SR25519PublicKey GetPublicKey() override;
  SR25519Signature SignMessage(base::span<const uint8_t> msg) override;
  bool VerifyMessage(SR25519Signature const& sig,
                     base::span<const uint8_t> msg) override;
  std::unique_ptr<SchnorrkelKeyPair> DeriveHard(
      base::span<const uint8_t> derive_junction) override;

 private:
  rust::Box<CxxSchnorrkelKeyPair> impl_;
};

SchnorrkelKeyPairImpl::~SchnorrkelKeyPairImpl() = default;

SchnorrkelKeyPairImpl::SchnorrkelKeyPairImpl(
    rust::Box<CxxSchnorrkelKeyPair> impl)
    : impl_(std::move(impl)) {}

std::unique_ptr<SchnorrkelKeyPair> SchnorrkelKeyPair::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  rust::Slice<const uint8_t> bytes{seed.data(), seed.size()};
  auto mk = generate_sr25519_keypair_from_seed(bytes);
  if (mk->is_ok()) {
    return std::make_unique<SchnorrkelKeyPairImpl>(mk->unwrap());
  }
  return nullptr;
}

SR25519PublicKey SchnorrkelKeyPairImpl::GetPublicKey() {
  return impl_->get_public_key();
}

SR25519Signature SchnorrkelKeyPairImpl::SignMessage(
    base::span<const uint8_t> msg) {
  rust::Slice<const uint8_t> bytes{msg.data(), msg.size()};
  return impl_->sign_message(bytes);
}

bool SchnorrkelKeyPairImpl::VerifyMessage(SR25519Signature const& sig,
                                          base::span<const uint8_t> msg) {
  rust::Slice<const uint8_t> sig_bytes{sig.data(), sig.size()};
  rust::Slice<const uint8_t> bytes{msg.data(), msg.size()};
  return impl_->verify_message(sig_bytes, bytes);
}

std::unique_ptr<SchnorrkelKeyPair> SchnorrkelKeyPairImpl::DeriveHard(
    base::span<const uint8_t> derive_junction) {
  rust::Slice<const uint8_t> bytes{derive_junction.data(),
                                   derive_junction.size()};
  return std::make_unique<SchnorrkelKeyPairImpl>(impl_->derive_hard(bytes));
}

}  // namespace brave_wallet::polkadot
