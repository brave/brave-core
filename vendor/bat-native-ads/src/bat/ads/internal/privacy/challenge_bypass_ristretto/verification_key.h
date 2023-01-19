/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_

#include <string>

#include "absl/types/optional.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

class VerificationSignature;

// A shared |VerificationKey| for proving and verifying the validity of an
// |UnblindedToken|. (K = H_2(t, W)).

class VerificationKey {
 public:
  explicit VerificationKey(
      const challenge_bypass_ristretto::VerificationKey& verification_key);

  challenge_bypass_ristretto::VerificationKey& get() {
    return verification_key_;
  }

  const challenge_bypass_ristretto::VerificationKey& get() const {
    return verification_key_;
  }

  absl::optional<VerificationSignature> Sign(const std::string& message);

  bool Verify(const VerificationSignature& verification_signature,
              const std::string& message);

 private:
  challenge_bypass_ristretto::VerificationKey verification_key_;
};

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_VERIFICATION_KEY_H_
