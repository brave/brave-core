/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"

#include <optional>
#include <string>

#include "base/base64url.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/payload/confirmation_payload_json_writer.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/verification_signature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

constexpr char kVerificationSignatureKey[] = "signature";

bool Verify(const ConfirmationInfo& confirmation) {
  CHECK(confirmation.reward);

  std::optional<cbr::VerificationKey> verification_key =
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

  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(credential);
  if (!dict) {
    return false;
  }

  if (const auto* const value = dict->FindString(kVerificationSignatureKey)) {
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
      confirmation.type == mojom::ConfirmationType::kUndefined ||
      confirmation.ad_type == mojom::AdType::kUndefined ||
      !confirmation.created_at) {
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
  database::table::ConfirmationQueue database_table;
  database_table.DeleteAll(/*intentional*/ base::DoNothing());

  RemoveAllConfirmationTokens();

  RemoveAllPaymentTokens();
}

}  // namespace brave_ads
