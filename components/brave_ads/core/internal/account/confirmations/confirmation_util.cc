/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"

#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/check.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_payload_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_credential_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace brave_ads {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";

absl::optional<OptedInInfo> CreateOptedIn(
    privacy::TokenGeneratorInterface* token_generator,
    const ConfirmationInfo& confirmation,
    const OptedInUserDataInfo& opted_in_user_data) {
  CHECK(token_generator);
  CHECK(ShouldRewardUser());

  OptedInInfo opted_in;

  // Token
  const std::vector<privacy::cbr::Token> tokens =
      token_generator->Generate(/*count*/ 1);
  CHECK(!tokens.empty());
  opted_in.token = tokens.front();

  // Blinded token
  const std::vector<privacy::cbr::BlindedToken> blinded_tokens =
      privacy::cbr::BlindTokens(tokens);
  CHECK(!blinded_tokens.empty());
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
  opted_in.user_data = opted_in_user_data;

  // Credential
  ConfirmationInfo confirmation_copy(confirmation);
  confirmation_copy.opted_in = opted_in;
  opted_in.credential_base64url = CreateOptedInCredential(confirmation_copy);

  return opted_in;
}

}  // namespace

absl::optional<std::string> CreateOptedInCredential(
    const ConfirmationInfo& confirmation) {
  if (!confirmation.opted_in) {
    return absl::nullopt;
  }

  const absl::optional<std::string> credential =
      json::writer::WriteOptedInCredential(
          confirmation.opted_in->unblinded_token,
          json::writer::WriteConfirmationPayload(confirmation));
  if (!credential) {
    BLOG(0, "Failed to create opted-in credential");
    return absl::nullopt;
  }

  std::string credential_base64url;
  base::Base64UrlEncode(*credential,
                        base::Base64UrlEncodePolicy::INCLUDE_PADDING,
                        &credential_base64url);
  return credential_base64url;
}

absl::optional<ConfirmationInfo> CreateConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    const TransactionInfo& transaction,
    const OptedInUserDataInfo& opted_in_user_data) {
  CHECK(token_generator);
  CHECK(transaction.IsValid());

  ConfirmationInfo confirmation;
  confirmation.transaction_id = transaction.id;
  confirmation.creative_instance_id = transaction.creative_instance_id;
  confirmation.type = transaction.confirmation_type;
  confirmation.ad_type = transaction.ad_type;
  confirmation.created_at = transaction.created_at;

  if (!ShouldRewardUser()) {
    return confirmation;
  }

  const absl::optional<OptedInInfo> opted_in =
      CreateOptedIn(token_generator, confirmation, opted_in_user_data);
  if (!opted_in) {
    BLOG(0, "Failed to create opted-in");
    return absl::nullopt;
  }
  confirmation.opted_in = opted_in;

  CHECK(IsValid(confirmation));

  return confirmation;
}

bool IsValid(const ConfirmationInfo& confirmation) {
  if (confirmation.transaction_id.empty() ||
      confirmation.creative_instance_id.empty() ||
      confirmation.type == ConfirmationType::kUndefined ||
      confirmation.ad_type == AdType::kUndefined ||
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

  if (const auto* const value = dict.FindString(kVerificationSignatureKey)) {
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
  ConfirmationStateManager::GetInstance().reset_failed_confirmations();
  ConfirmationStateManager::GetInstance().Save();

  privacy::RemoveAllUnblindedPaymentTokens();

  privacy::RemoveAllUnblindedTokens();
}

}  // namespace brave_ads
