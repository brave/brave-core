/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/credential_builder.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"

namespace brave_ads::cbr {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";
constexpr char kTokenPreimageKey[] = "t";

}  // namespace

absl::optional<base::Value::Dict> BuildCredential(
    const UnblindedToken& unblinded_token,
    const std::string& payload) {
  CHECK(unblinded_token.has_value());
  CHECK(!payload.empty());

  absl::optional<VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();
  if (!verification_key) {
    return absl::nullopt;
  }

  const absl::optional<VerificationSignature> verification_signature =
      verification_key->Sign(payload);
  if (!verification_signature) {
    return absl::nullopt;
  }

  const absl::optional<std::string> verification_signature_base64 =
      verification_signature->EncodeBase64();
  if (!verification_signature_base64) {
    return absl::nullopt;
  }

  const absl::optional<TokenPreimage> token_preimage =
      unblinded_token.GetTokenPreimage();
  if (!token_preimage) {
    return absl::nullopt;
  }

  const absl::optional<std::string> token_preimage_base64 =
      token_preimage->EncodeBase64();
  if (!token_preimage_base64) {
    return absl::nullopt;
  }

  return base::Value::Dict()
      .Set(kVerificationSignatureKey, *verification_signature_base64)
      .Set(kTokenPreimageKey, *token_preimage_base64);
}

}  // namespace brave_ads::cbr
