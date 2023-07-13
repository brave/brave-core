/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_

#include <string>

namespace brave_ads {

struct VerifiableConversionEnvelopeInfo final {
  VerifiableConversionEnvelopeInfo();

  VerifiableConversionEnvelopeInfo(const VerifiableConversionEnvelopeInfo&);
  VerifiableConversionEnvelopeInfo& operator=(
      const VerifiableConversionEnvelopeInfo&);

  VerifiableConversionEnvelopeInfo(VerifiableConversionEnvelopeInfo&&) noexcept;
  VerifiableConversionEnvelopeInfo& operator=(
      VerifiableConversionEnvelopeInfo&&) noexcept;

  ~VerifiableConversionEnvelopeInfo();

  [[nodiscard]] bool IsValid() const;

  std::string algorithm;
  std::string ciphertext;
  std::string ephemeral_public_key;
  std::string nonce;
};

bool operator==(const VerifiableConversionEnvelopeInfo&,
                const VerifiableConversionEnvelopeInfo&);
bool operator!=(const VerifiableConversionEnvelopeInfo&,
                const VerifiableConversionEnvelopeInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_TYPES_VERIFIABLE_CONVERSION_ENVELOPE_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_
