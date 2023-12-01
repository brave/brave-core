/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/challenge_bypass_ristretto/verification_key.h"

#include <utility>

#include "brave/components/challenge_bypass_ristretto/verification_signature.h"
#include "brave/third_party/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

VerificationKey::VerificationKey(CxxVerificationKeyBox raw)
    : raw_(base::MakeRefCounted<CxxVerificationKeyRefData>(std::move(raw))) {}

VerificationKey::VerificationKey(const VerificationKey& other) = default;

VerificationKey& VerificationKey::operator=(const VerificationKey& other) =
    default;

VerificationKey::VerificationKey(VerificationKey&& other) noexcept = default;

VerificationKey& VerificationKey::operator=(VerificationKey&& other) noexcept =
    default;

VerificationKey::~VerificationKey() = default;

base::expected<VerificationSignature, std::string> VerificationKey::Sign(
    const std::string& message) {
  rust::Box<cbr_cxx::VerificationSignature> verification_signature_box(
      raw().sign(message));

  return VerificationSignature(std::move(verification_signature_box));
}

base::expected<bool, std::string> VerificationKey::Verify(
    const VerificationSignature& sig,
    const std::string& message) {
  return raw().verify(sig.raw(), message);
}

}  // namespace challenge_bypass_ristretto
