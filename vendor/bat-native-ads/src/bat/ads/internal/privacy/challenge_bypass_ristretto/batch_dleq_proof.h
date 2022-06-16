/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_

#include <ostream>
#include <string>
#include <vector>

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
class Token;
class UnblindedToken;

// A |BatchDLEQProof| is a proof of the equivalence of the discrete logarithm
// between a common pair of points and one or more other pairs of points.

class BatchDLEQProof {
 public:
  BatchDLEQProof();
  explicit BatchDLEQProof(const std::string& batch_dleq_proof_base64);
  BatchDLEQProof(const std::vector<BlindedToken>& blinded_tokens,
                 const std::vector<SignedToken>& signed_tokens,
                 const SigningKey& signing_key);
  BatchDLEQProof(const BatchDLEQProof& other);
  BatchDLEQProof& operator=(const BatchDLEQProof& other);
  ~BatchDLEQProof();

  bool operator==(const BatchDLEQProof& rhs) const;
  bool operator!=(const BatchDLEQProof& rhs) const;

  bool has_value() const { return batch_dleq_proof_.has_value(); }

  challenge_bypass_ristretto::BatchDLEQProof get() const {
    DCHECK(has_value());
    return batch_dleq_proof_.value();
  }

  static BatchDLEQProof DecodeBase64(
      const std::string& batch_dleq_proof_base64);
  absl::optional<std::string> EncodeBase64() const;

  bool Verify(const std::vector<BlindedToken>& blinded_tokens,
              const std::vector<SignedToken>& signed_tokens,
              const PublicKey& public_key);

  absl::optional<std::vector<UnblindedToken>> VerifyAndUnblind(
      const std::vector<Token>& tokens,
      const std::vector<BlindedToken>& blinded_tokens,
      const std::vector<SignedToken>& signed_tokens,
      const PublicKey& public_key);

 private:
  absl::optional<challenge_bypass_ristretto::BatchDLEQProof> batch_dleq_proof_;
};

std::ostream& operator<<(std::ostream& os,
                         const BatchDLEQProof& batch_dleq_proof);

}  // namespace cbr
}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
