/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_credential_json_writer.h"

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace brave_ads::json::writer {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";
constexpr char kTokenPreimageKey[] = "t";

}  // namespace

absl::optional<std::string> WriteOptedInCredential(
    const privacy::UnblindedTokenInfo& unblinded_token,
    const std::string& payload) {
  CHECK(IsValid(unblinded_token));
  CHECK(!payload.empty());

  absl::optional<privacy::cbr::VerificationKey> verification_key =
      unblinded_token.value.DeriveVerificationKey();
  if (!verification_key) {
    return absl::nullopt;
  }

  const absl::optional<privacy::cbr::VerificationSignature>
      verification_signature = verification_key->Sign(payload);
  if (!verification_signature) {
    return absl::nullopt;
  }

  const absl::optional<std::string> verification_signature_base64 =
      verification_signature->EncodeBase64();
  if (!verification_signature_base64) {
    return absl::nullopt;
  }

  const absl::optional<privacy::cbr::TokenPreimage> token_preimage =
      unblinded_token.value.GetTokenPreimage();
  if (!token_preimage) {
    return absl::nullopt;
  }

  const absl::optional<std::string> token_preimage_base64 =
      token_preimage->EncodeBase64();
  if (!token_preimage_base64) {
    return absl::nullopt;
  }

  base::Value::Dict dict;
  dict.Set(kVerificationSignatureKey, *verification_signature_base64);
  dict.Set(kTokenPreimageKey, *token_preimage_base64);

  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

}  // namespace brave_ads::json::writer
