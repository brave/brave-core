/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"

#include <string>

#include "base/base64url.h"
#include "base/json/json_reader.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"

namespace brave_ads {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";

bool Verify(const ConfirmationInfo& confirmation) {
  CHECK(confirmation.reward);

  absl::optional<cbr::VerificationKey> verification_key =
      confirmation.reward->unblinded_token.DeriveVerificationKey();
  if (!verification_key) {
    return false;
  }

  std::string credential;
  if (!base::Base64UrlDecode(confirmation.reward->credential_base64url,
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
    const cbr::VerificationSignature verification_signature =
        cbr::VerificationSignature(*value);
    if (!verification_signature.has_value()) {
      return false;
    }

    return verification_key->Verify(
        verification_signature,
        json::writer::WriteConfirmationPayload(confirmation));
  }

  return false;
}

}  // namespace

bool IsValid(const ConfirmationInfo& confirmation) {
  if (confirmation.transaction_id.empty() ||
      confirmation.creative_instance_id.empty() ||
      confirmation.type == ConfirmationType::kUndefined ||
      confirmation.ad_type == AdType::kUndefined ||
      confirmation.created_at.is_null()) {
    return false;
  }

  if (!confirmation.reward) {
    return true;
  }

  if (!confirmation.reward->IsValid()) {
    return false;
  }

  return Verify(confirmation);
}

void ResetTokens() {
  ConfirmationStateManager::GetInstance().reset_confirmations();
  ConfirmationStateManager::GetInstance().SaveState();

  RemoveAllConfirmationTokens();

  RemoveAllPaymentTokens();
}

}  // namespace brave_ads
