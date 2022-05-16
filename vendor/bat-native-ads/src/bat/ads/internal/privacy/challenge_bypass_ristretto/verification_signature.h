/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_  // NOLINT
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_  // NOLINT

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

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
  ~VerificationSignature();

  bool operator==(const VerificationSignature& rhs) const;
  bool operator!=(const VerificationSignature& rhs) const;

  bool has_value() const { return verification_signature_.has_value(); }

  challenge_bypass_ristretto::VerificationSignature get() const {
    DCHECK(has_value());
    return verification_signature_.value();
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

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_SIGNATURE_H_
