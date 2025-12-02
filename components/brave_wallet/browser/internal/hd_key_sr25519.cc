// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/internal/hd_key_sr25519.h"

#include <utility>

#include "base/containers/span_rust.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_wallet/browser/internal/sr25519.rs.h"

namespace brave_wallet {

namespace {
bool IsBoxNonNull(rust::Box<CxxSchnorrkelKeyPair> const& keypair) {
  return keypair.operator->();
}
}  // namespace

HDKeySr25519::HDKeySr25519(rust::Box<CxxSchnorrkelKeyPair> keypair)
    : keypair_(std::move(keypair)) {}

HDKeySr25519::HDKeySr25519(HDKeySr25519&&) noexcept = default;
HDKeySr25519& HDKeySr25519::operator=(HDKeySr25519&& rhs) noexcept {
  if (this != &rhs) {
    keypair_ = std::move(rhs.keypair_);
  }
  return *this;
}

HDKeySr25519::~HDKeySr25519() = default;

HDKeySr25519 HDKeySr25519::GenerateFromSeed(
    base::span<const uint8_t, kSr25519SeedSize> seed) {
  auto bytes = base::SpanToRustSlice(seed);
  auto mk = generate_sr25519_keypair_from_seed(bytes);
  return HDKeySr25519(std::move(mk));
}

std::array<uint8_t, kSr25519PublicKeySize> HDKeySr25519::GetPublicKey() const {
  CHECK(IsBoxNonNull(keypair_));
  return keypair_->get_public_key();
}

std::array<uint8_t, kSr25519SignatureSize> HDKeySr25519::SignMessage(
    base::span<const uint8_t> msg) const {
  CHECK(IsBoxNonNull(keypair_));
  auto bytes = base::SpanToRustSlice(msg);
  return keypair_->sign_message(bytes);
}

bool HDKeySr25519::VerifyMessage(
    base::span<uint8_t const, kSr25519SignatureSize> signature,
    base::span<const uint8_t> message) const {
  CHECK(IsBoxNonNull(keypair_));
  auto sig_bytes = base::SpanToRustSlice(signature);
  auto bytes = base::SpanToRustSlice(message);
  return keypair_->verify_message(sig_bytes, bytes);
}

HDKeySr25519 HDKeySr25519::DeriveHard(
    base::span<const uint8_t> derive_junction) const {
  CHECK(IsBoxNonNull(keypair_));
  return HDKeySr25519(
      keypair_->derive_hard(base::SpanToRustSlice(derive_junction)));
}

void HDKeySr25519::UseMockRngForTesting() {
  keypair_->use_mock_rng_for_testing();  // IN-TEST
}

}  // namespace brave_wallet
