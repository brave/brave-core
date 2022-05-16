/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/dleq_proof.h"

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signing_key.h"

namespace ads {
namespace privacy {
namespace cbr {

namespace {

absl::optional<challenge_bypass_ristretto::DLEQProof> Create(
    const BlindedToken& blinded_token,
    const SignedToken& signed_token,
    const SigningKey& signing_key) {
  if (!blinded_token.has_value() || !signed_token.has_value() ||
      !signing_key.has_value()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::DLEQProof raw_dleq_proof =
      challenge_bypass_ristretto::DLEQProof(
          blinded_token.get(), signed_token.get(), signing_key.get());
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_dleq_proof;
}

absl::optional<challenge_bypass_ristretto::DLEQProof> Create(
    const std::string& dleq_proof_base64) {
  if (dleq_proof_base64.empty()) {
    return absl::nullopt;
  }

  const challenge_bypass_ristretto::DLEQProof raw_dleq_proof =
      challenge_bypass_ristretto::DLEQProof::decode_base64(dleq_proof_base64);
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return raw_dleq_proof;
}

}  // namespace

DLEQProof::DLEQProof() = default;

DLEQProof::DLEQProof(const std::string& dleq_proof_base64)
    : dleq_proof_(Create(dleq_proof_base64)) {}

DLEQProof::DLEQProof(const BlindedToken& blinded_token,
                     const SignedToken& signed_token,
                     const SigningKey& signing_key)
    : dleq_proof_(Create(blinded_token, signed_token, signing_key)) {}

DLEQProof::DLEQProof(const DLEQProof& other) = default;

DLEQProof::~DLEQProof() = default;

bool DLEQProof::operator==(const DLEQProof& rhs) const {
  return EncodeBase64().value_or("") == rhs.EncodeBase64().value_or("");
}

bool DLEQProof::operator!=(const DLEQProof& rhs) const {
  return !(*this == rhs);
}

DLEQProof DLEQProof::DecodeBase64(const std::string& dleq_proof_base64) {
  return DLEQProof(dleq_proof_base64);
}

absl::optional<std::string> DLEQProof::EncodeBase64() const {
  if (!has_value()) {
    return absl::nullopt;
  }

  const std::string encoded_base64 = dleq_proof_->encode_base64();
  if (ExceptionOccurred()) {
    return absl::nullopt;
  }

  return encoded_base64;
}

bool DLEQProof::Verify(const BlindedToken& blinded_token,
                       const SignedToken& signed_token,
                       const PublicKey& public_key) {
  if (!has_value() || !blinded_token.has_value() || !signed_token.has_value() ||
      !public_key.has_value()) {
    return false;
  }

  const bool is_valid = dleq_proof_->verify(
      blinded_token.get(), signed_token.get(), public_key.get());

  return !ExceptionOccurred() && is_valid;
}

std::ostream& operator<<(std::ostream& os, const DLEQProof& dleq_proof) {
  os << dleq_proof.EncodeBase64().value_or("");
  return os;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
