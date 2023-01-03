/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_

#include <ostream>
#include <string>

#include "absl/types/optional.h"
#include "base/check.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

// A |VerificationSignature| which can be verified given the |VerificationKey|
// and message.

class VerificationSignature {
 public:
  VerificationSignature();
  explicit VerificationSignature(
      const std::string& verification_signature_base64);
  explicit VerificationSignature(
      const challenge_bypass_ristretto::VerificationSignature&
          verification_signature);

  VerificationSignature(const VerificationSignature& other);
  VerificationSignature& operator=(const VerificationSignature& other);

  VerificationSignature(VerificationSignature&& other) noexcept;
  VerificationSignature& operator=(VerificationSignature&& other) noexcept;

  ~VerificationSignature();

  bool operator==(const VerificationSignature& other) const;
  bool operator!=(const VerificationSignature& other) const;

  bool has_value() const {
    return verification_signature_ && verification_signature_.has_value();
  }

  challenge_bypass_ristretto::VerificationSignature& get() {
    DCHECK(verification_signature_);
    return *verification_signature_;
  }

  const challenge_bypass_ristretto::VerificationSignature& get() const {
    DCHECK(verification_signature_);
    return *verification_signature_;
  }

  static VerificationSignature DecodeBase64(
      const std::string& verification_signature_base64);
  absl::optional<std::string> EncodeBase64() const;

 private:
  absl::optional<challenge_bypass_ristretto::VerificationSignature>
      verification_signature_;
};

std::ostream& operator<<(std::ostream& os,
                         const VerificationSignature& verification_signature);

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_
