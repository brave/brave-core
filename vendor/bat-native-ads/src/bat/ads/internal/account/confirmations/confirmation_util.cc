/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/confirmations/confirmation_util.h"

#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/check_op.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmation_payload_json_writer.h"
#include "bat/ads/internal/account/confirmations/opted_in_credential_json_writer.h"
#include "bat/ads/internal/account/confirmations/opted_in_info.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace ads {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";

absl::optional<OptedInInfo> CreateOptedIn(
    privacy::TokenGeneratorInterface* token_generator,
    const ConfirmationInfo& confirmation,
    base::Value::Dict user_data) {
  DCHECK(token_generator);
  DCHECK(ShouldRewardUser());

  OptedInInfo opted_in;

  // Token
  const std::vector<privacy::cbr::Token> tokens = token_generator->Generate(1);
  DCHECK(!tokens.empty());
  opted_in.token = tokens.front();

  // Blinded token
  const std::vector<privacy::cbr::BlindedToken> blinded_tokens =
      privacy::cbr::BlindTokens(tokens);
  DCHECK(!blinded_tokens.empty());
  opted_in.blinded_token = blinded_tokens.front();

  // Unblinded token
  const absl::optional<privacy::UnblindedTokenInfo> unblinded_token =
      privacy::MaybeGetUnblindedToken();
  if (!unblinded_token) {
    BLOG(0, "Failed to get unblinded token");
    return absl::nullopt;
  }

  if (!privacy::RemoveUnblindedToken(*unblinded_token)) {
    BLOG(0, "Failed to remove unblinded token");
    return absl::nullopt;
  }

  opted_in.unblinded_token = *unblinded_token;

  // User data
  opted_in.user_data = std::move(user_data);

  // Credential
  ConfirmationInfo new_confirmation = confirmation;
  new_confirmation.opted_in = opted_in;

  const absl::optional<std::string> credential =
      json::writer::WriteOptedInCredential(
          *unblinded_token,
          json::writer::WriteConfirmationPayload(new_confirmation));
  if (!credential) {
    BLOG(0, "Failed to create opted-in credential");
    return absl::nullopt;
  }

  std::string credential_base64url;
  base::Base64UrlEncode(*credential,
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &credential_base64url);
  opted_in.credential_base64url = credential_base64url;

  return opted_in;
}

}  // namespace

absl::optional<ConfirmationInfo> CreateConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    const base::Time created_at,
    const std::string& transaction_id,
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type,
    const AdType& ad_type,
    base::Value::Dict user_data) {
  DCHECK(token_generator);
  DCHECK(!created_at.is_null());
  DCHECK(!transaction_id.empty());
  DCHECK(!creative_instance_id.empty());
  DCHECK_NE(ConfirmationType::kUndefined, confirmation_type.value());
  DCHECK_NE(AdType::kUndefined, ad_type.value());

  ConfirmationInfo confirmation;
  confirmation.transaction_id = transaction_id;
  confirmation.creative_instance_id = creative_instance_id;
  confirmation.type = confirmation_type;
  confirmation.ad_type = ad_type;
  confirmation.created_at = created_at;

  if (!ShouldRewardUser()) {
    return confirmation;
  }

  const absl::optional<OptedInInfo> opted_in =
      CreateOptedIn(token_generator, confirmation, std::move(user_data));
  if (!opted_in) {
    BLOG(0, "Failed to create opted-in");
    return absl::nullopt;
  }
  confirmation.opted_in = opted_in;

  return confirmation;
}

bool IsValid(const ConfirmationInfo& confirmation) {
  if (confirmation.transaction_id.empty() ||
      confirmation.creative_instance_id.empty() ||
      confirmation.type.value() == ConfirmationType::kUndefined ||
      confirmation.ad_type.value() == AdType::kUndefined ||
      confirmation.created_at.is_null()) {
    return false;
  }

  if (!confirmation.opted_in) {
    return true;
  }

  if (!confirmation.opted_in->token.has_value() ||
      !confirmation.opted_in->blinded_token.has_value() ||
      !IsValid(confirmation.opted_in->unblinded_token) ||
      !confirmation.opted_in->credential_base64url) {
    return false;
  }

  const privacy::cbr::UnblindedToken& unblinded_token =
      confirmation.opted_in->unblinded_token.value;
  absl::optional<privacy::cbr::VerificationKey> verification_key =
      unblinded_token.DeriveVerificationKey();
  if (!verification_key) {
    return false;
  }

  CHECK(confirmation.opted_in->credential_base64url)
      << "IsValid should only be called if credential_base64url has been set";

  std::string credential;
  if (!base::Base64UrlDecode(*confirmation.opted_in->credential_base64url,
                             base::Base64UrlDecodePolicy::REQUIRE_PADDING,
                             &credential)) {
    return false;
  }

  const absl::optional<base::Value> root = base::JSONReader::Read(credential);
  if (!root || !root->is_dict()) {
    return false;
  }
  const base::Value::Dict& dict = root->GetDict();

  if (const std::string* const value =
          dict.FindString(kVerificationSignatureKey)) {
    const privacy::cbr::VerificationSignature verification_signature =
        privacy::cbr::VerificationSignature(*value);
    if (!verification_signature.has_value()) {
      return false;
    }

    return verification_key->Verify(
        verification_signature,
        json::writer::WriteConfirmationPayload(confirmation));
  }

  return false;
}

void ResetConfirmations() {
  ConfirmationStateManager::GetInstance()->reset_failed_confirmations();
  ConfirmationStateManager::GetInstance()->Save();

  privacy::RemoveAllUnblindedPaymentTokens();

  privacy::RemoveAllUnblindedTokens();
}

}  // namespace ads
