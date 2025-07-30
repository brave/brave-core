// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include <utility>

#include "base/containers/span_rust.h"
#include "brave/components/brave_wallet/browser/internal/polkadot/rust/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"

namespace brave_wallet {

namespace {
bool IsBoxNonNull(rust::Box<polkadot::CxxSchnorrkelKeyPair> const& keypair) {
  return keypair.operator->();
}
}  // namespace

HDKeySr25519::HDKeySr25519(rust::Box<polkadot::CxxSchnorrkelKeyPair> keypair)
    : keypair_(std::move(keypair)) {}

HDKeySr25519::HDKeySr25519(HDKeySr25519&&) noexcept = default;
HDKeySr25519& HDKeySr25519::operator=(HDKeySr25519&& rhs) noexcept {
  if (this != &rhs) {
    keypair_ = std::move(rhs.keypair_);
  }
  return *this;
}

HDKeySr25519::~HDKeySr25519() = default;

std::optional<HDKeySr25519> HDKeySr25519::GenerateFromSeed(
    base::span<const uint8_t> seed) {
  auto bytes = base::SpanToRustSlice(seed);
  auto mk = polkadot::generate_sr25519_keypair_from_seed(bytes);
  if (!mk->is_ok()) {
    return std::nullopt;
  }
  return HDKeySr25519(mk->unwrap());
}

std::array<uint8_t, kSr25519PublicKeySize> HDKeySr25519::GetPublicKey() {
  CHECK(IsBoxNonNull(keypair_));
  return keypair_->get_public_key();
}

std::array<uint8_t, kSr25519SignatureSize> HDKeySr25519::SignMessage(
    base::span<const uint8_t> msg) {
  CHECK(IsBoxNonNull(keypair_));
  auto bytes = base::SpanToRustSlice(msg);
  return keypair_->sign_message(bytes);
}

bool HDKeySr25519::VerifyMessage(
    std::array<uint8_t, kSr25519SignatureSize> const& sig,
    base::span<const uint8_t> msg) {
  CHECK(IsBoxNonNull(keypair_));
  auto sig_bytes = base::SpanToRustSlice(sig);
  auto bytes = base::SpanToRustSlice(msg);
  return keypair_->verify_message(sig_bytes, bytes);
}

HDKeySr25519 HDKeySr25519::DeriveHard(
    base::span<const uint8_t> derive_junction) {
  CHECK(IsBoxNonNull(keypair_));
  return HDKeySr25519(
      keypair_->derive_hard(base::SpanToRustSlice(derive_junction)));
}

}  // namespace brave_wallet
