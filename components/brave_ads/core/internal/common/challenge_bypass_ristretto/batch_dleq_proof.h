/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_

#include <optional>
#include <ostream>
#include <string>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/challenge_bypass_ristretto/batch_dleq_proof.h"

namespace brave_ads::cbr {

class PublicKey;
class SigningKey;

// A `BatchDLEQProof` is a proof of the equivalence of the discrete logarithm
// between a common pair of points and one or more other pairs of points.

class BatchDLEQProof {
 public:
  BatchDLEQProof();
  explicit BatchDLEQProof(const std::string& batch_dleq_proof_base64);
  BatchDLEQProof(const BlindedTokenList& blinded_tokens,
                 const SignedTokenList& signed_tokens,
                 const SigningKey& signing_key);

  BatchDLEQProof(const BatchDLEQProof&) = delete;
  BatchDLEQProof& operator=(const BatchDLEQProof&) = delete;

  ~BatchDLEQProof();

  bool operator==(const BatchDLEQProof&) const;

  bool has_value() const {
    return batch_dleq_proof_ && batch_dleq_proof_.has_value();
  }

  challenge_bypass_ristretto::BatchDLEQProof& get() {
    CHECK(batch_dleq_proof_);
    return *batch_dleq_proof_;
  }

  const challenge_bypass_ristretto::BatchDLEQProof& get() const {
    CHECK(batch_dleq_proof_);
    return *batch_dleq_proof_;
  }

  static BatchDLEQProof DecodeBase64(
      const std::string& batch_dleq_proof_base64);
  std::optional<std::string> EncodeBase64() const;

  [[nodiscard]] bool Verify(const BlindedTokenList& blinded_tokens,
                            const SignedTokenList& signed_tokens,
                            const PublicKey& public_key);

  [[nodiscard]] std::optional<UnblindedTokenList> VerifyAndUnblind(
      const TokenList& tokens,
      const BlindedTokenList& blinded_tokens,
      const SignedTokenList& signed_tokens,
      const PublicKey& public_key);

 private:
  std::optional<challenge_bypass_ristretto::BatchDLEQProof> batch_dleq_proof_;
};

std::ostream& operator<<(std::ostream& os,
                         const BatchDLEQProof& batch_dleq_proof);

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
