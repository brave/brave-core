/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/verification_signature.h"

#include <utility>

namespace challenge_bypass_ristretto {

namespace {}  // namespace

VerificationSignature::VerificationSignature(CxxVerificationSignatureBox raw)
    : raw_(base::MakeRefCounted<CxxVerificationSignatureRefData>(
          std::move(raw))) {}

VerificationSignature::VerificationSignature(
    const VerificationSignature& other) = default;

VerificationSignature& VerificationSignature::operator=(
    const VerificationSignature& other) = default;

VerificationSignature::VerificationSignature(
    VerificationSignature&& other) noexcept = default;

VerificationSignature& VerificationSignature::operator=(
    VerificationSignature&& other) noexcept = default;

VerificationSignature::~VerificationSignature() = default;

// static
base::expected<VerificationSignature, std::string>
VerificationSignature::DecodeBase64(const std::string& encoded) {
  rust::Box<cbr_cxx::VerificationSignatureResult> raw_signature_result(
      cbr_cxx::decode_base64_verification_signature(encoded));

  if (!raw_signature_result->is_ok()) {
    return base::unexpected("Failed to decode verification signature");
  }

  return VerificationSignature(raw_signature_result->unwrap());
}

std::string VerificationSignature::EncodeBase64() const {
  return static_cast<std::string>(raw().encode_base64());
}

}  // namespace challenge_bypass_ristretto
