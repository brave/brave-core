/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_

#include <ostream>
#include <string>
#include <vector>

#include "absl/types/optional.h"
#include "base/check.h"
#include "wrapper.hpp"

namespace ads::privacy::cbr {

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

  BatchDLEQProof(const BatchDLEQProof& other) = delete;
  BatchDLEQProof& operator=(const BatchDLEQProof& other) = delete;

  BatchDLEQProof(BatchDLEQProof&& other) noexcept = delete;
  BatchDLEQProof& operator=(BatchDLEQProof&& other) noexcept = delete;

  ~BatchDLEQProof();

  bool operator==(const BatchDLEQProof& other) const;
  bool operator!=(const BatchDLEQProof& other) const;

  bool has_value() const {
    return batch_dleq_proof_ && batch_dleq_proof_.has_value();
  }

  challenge_bypass_ristretto::BatchDLEQProof& get() {
    DCHECK(batch_dleq_proof_);
    return *batch_dleq_proof_;
  }

  const challenge_bypass_ristretto::BatchDLEQProof& get() const {
    DCHECK(batch_dleq_proof_);
    return *batch_dleq_proof_;
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

}  // namespace ads::privacy::cbr

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
