/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/verifiable_conversion_envelope_info.h"

namespace ads::security {

VerifiableConversionEnvelopeInfo::VerifiableConversionEnvelopeInfo() = default;

VerifiableConversionEnvelopeInfo::VerifiableConversionEnvelopeInfo(
    const VerifiableConversionEnvelopeInfo& other) = default;

VerifiableConversionEnvelopeInfo& VerifiableConversionEnvelopeInfo::operator=(
    const VerifiableConversionEnvelopeInfo& other) = default;

VerifiableConversionEnvelopeInfo::VerifiableConversionEnvelopeInfo(
    VerifiableConversionEnvelopeInfo&& other) noexcept = default;

VerifiableConversionEnvelopeInfo& VerifiableConversionEnvelopeInfo::operator=(
    VerifiableConversionEnvelopeInfo&& other) noexcept = default;

VerifiableConversionEnvelopeInfo::~VerifiableConversionEnvelopeInfo() = default;

bool VerifiableConversionEnvelopeInfo::operator==(
    const VerifiableConversionEnvelopeInfo& other) const {
  return algorithm == other.algorithm && ciphertext == other.ciphertext &&
         ephemeral_public_key == other.ephemeral_public_key &&
         nonce == other.nonce;
}

bool VerifiableConversionEnvelopeInfo::operator!=(
    const VerifiableConversionEnvelopeInfo& other) const {
  return !(*this == other);
}

bool VerifiableConversionEnvelopeInfo::IsValid() const {
  return !(algorithm.empty() || ciphertext.empty() ||
           ephemeral_public_key.empty() || nonce.empty());
}

}  // namespace ads::security
