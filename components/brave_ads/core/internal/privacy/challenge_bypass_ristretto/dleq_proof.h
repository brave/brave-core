/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_

#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/third_party/challenge_bypass_ristretto_ffi/src/wrapper.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::privacy::cbr {

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

  DLEQProof(const DLEQProof&) = delete;
  DLEQProof& operator=(const DLEQProof&) = delete;

  DLEQProof(DLEQProof&&) noexcept = delete;
  DLEQProof& operator=(DLEQProof&&) noexcept = delete;

  ~DLEQProof();

  bool operator==(const DLEQProof&) const;
  bool operator!=(const DLEQProof&) const;

  bool has_value() const { return dleq_proof_.has_value(); }

  challenge_bypass_ristretto::DLEQProof& get() {
    CHECK(dleq_proof_);
    return *dleq_proof_;
  }

  const challenge_bypass_ristretto::DLEQProof& get() const {
    CHECK(dleq_proof_);
    return *dleq_proof_;
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

}  // namespace brave_ads::privacy::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_DLEQ_PROOF_H_
