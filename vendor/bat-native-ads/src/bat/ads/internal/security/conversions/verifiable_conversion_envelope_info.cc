/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"

namespace ads {
namespace security {

VerifiableConversionEnvelopeInfo::VerifiableConversionEnvelopeInfo() = default;

VerifiableConversionEnvelopeInfo::VerifiableConversionEnvelopeInfo(
    const VerifiableConversionEnvelopeInfo& info) = default;

VerifiableConversionEnvelopeInfo::~VerifiableConversionEnvelopeInfo() = default;

bool VerifiableConversionEnvelopeInfo::operator==(
    const VerifiableConversionEnvelopeInfo& rhs) const {
  return algorithm == rhs.algorithm && ciphertext == rhs.ciphertext &&
         ephemeral_public_key == rhs.ephemeral_public_key && nonce == rhs.nonce;
}

bool VerifiableConversionEnvelopeInfo::operator!=(
    const VerifiableConversionEnvelopeInfo& rhs) const {
  return !(*this == rhs);
}

bool VerifiableConversionEnvelopeInfo::IsValid() const {
  if (algorithm.empty() || ciphertext.empty() || ephemeral_public_key.empty() ||
      nonce.empty()) {
    return false;
  }

  return true;
}

}  // namespace security
}  // namespace ads
