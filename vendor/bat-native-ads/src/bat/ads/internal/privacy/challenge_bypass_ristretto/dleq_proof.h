/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {
namespace cbr {

class BlindedToken;
class PublicKey;
class SignedToken;
class SigningKey;

// A |DLEQProof| is a proof of the equivalence of the discrete logarithm between
// two pairs of points.

class DLEQProof {
 public:
  DLEQProof();
  explicit DLEQProof(const std::string& dleq_proof_base64);
  DLEQProof(const BlindedToken& blinded_token,
            const SignedToken& signed_token,
            const SigningKey& signing_key);
  DLEQProof(const DLEQProof& other);
  DLEQProof& operator=(const DLEQProof& other);
  ~DLEQProof();

  bool operator==(const DLEQProof& rhs) const;
  bool operator!=(const DLEQProof& rhs) const;

  bool has_value() const { return dleq_proof_.has_value(); }

  challenge_bypass_ristretto::DLEQProof get() const {
    DCHECK(has_value());
    return dleq_proof_.value();
  }

  static DLEQProof DecodeBase64(const std::string& dleq_proof_base64);
  absl::optional<std::string> EncodeBase64() const;

  bool Verify(const BlindedToken& blinded_token,
              const SignedToken& signed_token,
              const PublicKey& public_key);

 private:
  absl::optional<challenge_bypass_ristretto::DLEQProof> dleq_proof_;
};

std::ostream& operator<<(std::ostream& os, const DLEQProof& dleq_proof);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_
