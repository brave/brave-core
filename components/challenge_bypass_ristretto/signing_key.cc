/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/signing_key.h"

#include <utility>

#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/public_key.h"
#include "brave/components/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/challenge_bypass_ristretto/unblinded_token.h"

namespace challenge_bypass_ristretto {

SigningKey::SigningKey(CxxSigningKeyBox raw)
    : raw_(base::MakeRefCounted<CxxSigningKeyRefData>(std::move(raw))) {}

SigningKey::SigningKey(const SigningKey& other) = default;

SigningKey& SigningKey::operator=(const SigningKey& other) = default;

SigningKey::SigningKey(SigningKey&& other) noexcept = default;

SigningKey& SigningKey::operator=(SigningKey&& other) noexcept = default;

SigningKey::~SigningKey() = default;

// static
SigningKey SigningKey::Random() {
  CxxSigningKeyBox raw_key(cbr_cxx::generate_signing_key());
  return SigningKey(std::move(raw_key));
}

base::expected<SignedToken, std::string> SigningKey::Sign(
    const BlindedToken& blinded_token) const {
  rust::Box<cbr_cxx::SignedTokenResult> raw_signed_key_result(
      raw().sign(blinded_token.raw()));

  if (!raw_signed_key_result->is_ok()) {
    base::unexpected("Failed to sign blinded token");
  }

  return SignedToken(raw_signed_key_result->unwrap());
}

UnblindedToken SigningKey::RederiveUnblindedToken(const TokenPreimage& t) {
  return UnblindedToken(raw().rederive_unblinded_token(t.raw()));
}

PublicKey SigningKey::GetPublicKey() {
  return PublicKey(raw().public_key());
}

base::expected<SigningKey, std::string> SigningKey::DecodeBase64(
    const std::string& encoded) {
  rust::Box<cbr_cxx::SigningKeyResult> raw_signing_key_result(
      cbr_cxx::decode_base64_signing_key(encoded));

  if (!raw_signing_key_result->is_ok()) {
    return base::unexpected("Failed to decode signing key");
  }

  return SigningKey(raw_signing_key_result->unwrap());
}

std::string SigningKey::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

bool SigningKey::operator==(const SigningKey& rhs) const {
  return EncodeBase64() == rhs.EncodeBase64();
}

}  // namespace challenge_bypass_ristretto
