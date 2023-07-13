/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/envelope/verifiable_conversion_envelope_info.h"

#include <tuple>

namespace brave_ads {

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

bool operator==(const VerifiableConversionEnvelopeInfo& lhs,
                const VerifiableConversionEnvelopeInfo& rhs) {
  const auto tie = [](const VerifiableConversionEnvelopeInfo&
                          verifiable_conversion_envelope) {
    return std::tie(verifiable_conversion_envelope.algorithm,
                    verifiable_conversion_envelope.ciphertext,
                    verifiable_conversion_envelope.ephemeral_public_key,
                    verifiable_conversion_envelope.nonce);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const VerifiableConversionEnvelopeInfo& lhs,
                const VerifiableConversionEnvelopeInfo& rhs) {
  return !(lhs == rhs);
}

bool VerifiableConversionEnvelopeInfo::IsValid() const {
  return !algorithm.empty() && !ciphertext.empty() &&
         !ephemeral_public_key.empty() && !nonce.empty();
}

}  // namespace brave_ads
