/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

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

  VerificationSignature(const VerificationSignature&);
  VerificationSignature& operator=(const VerificationSignature&);

  VerificationSignature(VerificationSignature&&) noexcept;
  VerificationSignature& operator=(VerificationSignature&&) noexcept;

  ~VerificationSignature();

  bool operator==(const VerificationSignature&) const;
  bool operator!=(const VerificationSignature&) const;

  bool has_value() const {
    return verification_signature_ && verification_signature_.has_value();
  }

  challenge_bypass_ristretto::VerificationSignature& get() {
    CHECK(verification_signature_);
    return *verification_signature_;
  }

  const challenge_bypass_ristretto::VerificationSignature& get() const {
    CHECK(verification_signature_);
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

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_
