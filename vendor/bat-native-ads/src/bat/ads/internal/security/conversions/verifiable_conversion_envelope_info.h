/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_

#include <string>

namespace ads {
namespace security {

struct VerifiableConversionEnvelopeInfo final {
  VerifiableConversionEnvelopeInfo();
  VerifiableConversionEnvelopeInfo(
      const VerifiableConversionEnvelopeInfo& info);
  ~VerifiableConversionEnvelopeInfo();

  bool operator==(const VerifiableConversionEnvelopeInfo& rhs) const;
  bool operator!=(const VerifiableConversionEnvelopeInfo& rhs) const;

  bool IsValid() const;

  std::string algorithm;
  std::string ciphertext;
  std::string ephemeral_public_key;
  std::string nonce;
};

}  // namespace security
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_SECURITY_CONVERSIONS_VERIFIABLE_CONVERSION_ENVELOPE_INFO_H_
