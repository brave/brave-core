/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
#define BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_

#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "brave/third_party/rust/challenge_bypass_ristretto_cxx/src/lib.rs.h"

namespace challenge_bypass_ristretto {

class BlindedToken;
class SignedToken;
class SigningKey;
class PublicKey;
class Token;
class UnblindedToken;

class COMPONENT_EXPORT(CHALLENGE_BYPASS_RISTRETTO) BatchDLEQProof {
  using CxxBatchDLEQProofResultBox = rust::Box<cbr_cxx::BatchDLEQProofResult>;
  using CxxBatchDLEQProofRefData =
      base::RefCountedData<CxxBatchDLEQProofResultBox>;

 public:
  explicit BatchDLEQProof(CxxBatchDLEQProofResultBox raw);
  BatchDLEQProof(const BatchDLEQProof&);
  BatchDLEQProof& operator=(const BatchDLEQProof&);
  BatchDLEQProof(BatchDLEQProof&&) noexcept;
  BatchDLEQProof& operator=(BatchDLEQProof&&) noexcept;
  ~BatchDLEQProof();

  const cbr_cxx::BatchDLEQProof& raw() const { return raw_->data->unwrap(); }

  static base::expected<BatchDLEQProof, std::string> Create(
      const std::vector<BlindedToken>& blinded_tokens,
      const std::vector<SignedToken>& signed_tokens,
      const SigningKey& signing_key);

  base::expected<bool, std::string> Verify(
      const std::vector<BlindedToken>& blinded_tokens,
      const std::vector<SignedToken>& signed_tokens,
      const PublicKey& public_key);

  base::expected<std::vector<UnblindedToken>, std::string> VerifyAndUnblind(
      const std::vector<Token>& tokens,
      const std::vector<BlindedToken>& blinded_tokens,
      const std::vector<SignedToken>& signed_tokens,
      const PublicKey& public_key);

  static base::expected<BatchDLEQProof, std::string> DecodeBase64(
      const std::string& encoded);
  std::string EncodeBase64() const;

  bool operator==(const BatchDLEQProof& rhs) const;

 private:
  scoped_refptr<CxxBatchDLEQProofRefData> raw_;
};

}  // namespace challenge_bypass_ristretto

#endif  // BRAVE_COMPONENTS_CHALLENGE_BYPASS_RISTRETTO_BATCH_DLEQ_PROOF_H_
