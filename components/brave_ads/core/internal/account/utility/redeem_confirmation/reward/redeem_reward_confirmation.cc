/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/redeem_reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/fetch_payment_token_url_request_builder.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_request_string_util.h"
#include "brave/components/brave_ads/core/internal/common/url/url_response_string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_status_code.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

RedeemRewardConfirmation::RedeemRewardConfirmation(
    RedeemRewardConfirmation&&) noexcept = default;

RedeemRewardConfirmation& RedeemRewardConfirmation::operator=(
    RedeemRewardConfirmation&&) noexcept = default;

RedeemRewardConfirmation::~RedeemRewardConfirmation() = default;

// static
void RedeemRewardConfirmation::CreateAndRedeem(
    base::WeakPtr<RedeemConfirmationDelegate> delegate,
    const ConfirmationInfo& confirmation) {
  RedeemRewardConfirmation redeem_confirmation(std::move(delegate));
  RedeemRewardConfirmation::Redeem(std::move(redeem_confirmation),
                                   confirmation);
}

///////////////////////////////////////////////////////////////////////////////

RedeemRewardConfirmation::RedeemRewardConfirmation(
    base::WeakPtr<RedeemConfirmationDelegate> delegate) {
  CHECK(delegate);
  delegate_ = std::move(delegate);
}

// static
void RedeemRewardConfirmation::Redeem(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  CHECK(IsValid(confirmation));
  CHECK(confirmation.reward);

  BLOG(1, "Redeem reward confirmation");

  if (!HasIssuers()) {
    BLOG(1, "Failed to redeem confirmation token due to missing issuers");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  CreateConfirmation(std::move(redeem_confirmation), confirmation);
}

// static
void RedeemRewardConfirmation::CreateConfirmation(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Create reward confirmation");

  CreateRewardConfirmationUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  UrlRequest(
      std::move(url_request),
      base::BindOnce(&RedeemRewardConfirmation::CreateConfirmationCallback,
                     std::move(redeem_confirmation), confirmation));
}

// static
void RedeemRewardConfirmation::CreateConfirmationCallback(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  FetchPaymentToken(std::move(redeem_confirmation), confirmation);
}

// static
void RedeemRewardConfirmation::FetchPaymentToken(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Fetch payment token");

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  UrlRequest(
      std::move(url_request),
      base::BindOnce(&RedeemRewardConfirmation::FetchPaymentTokenCallback,
                     std::move(redeem_confirmation), confirmation));
}

// static
void RedeemRewardConfirmation::FetchPaymentTokenCallback(
    RedeemRewardConfirmation redeem_confirmation,
    const ConfirmationInfo& confirmation,
    const mojom::UrlResponseInfo& url_response) {
  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(1, "Confirmation not found");

    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(1, "Credential is invalid");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  if (url_response.status_code == net::HTTP_ACCEPTED) {
    BLOG(1, "Payment token is not ready");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch payment token");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  const absl::optional<base::Value> root =
      base::JSONReader::Read(url_response.body);
  if (!root || !root->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }
  const base::Value::Dict& dict = root->GetDict();

  // Get id
  const std::string* const id = dict.FindString("id");
  if (!id) {
    BLOG(0, "Response is missing id");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  // Validate id
  if (*id != confirmation.transaction_id) {
    BLOG(0, "Response id " << *id
                           << " does not match confirmation transaction id "
                           << confirmation.transaction_id);
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  // Get payment token
  const auto* const payment_token_dict = dict.FindDict("paymentToken");
  if (!payment_token_dict) {
    BLOG(1, "Response is missing paymentToken");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  // Get public key
  const std::string* const public_key_base64 =
      payment_token_dict->FindString("publicKey");
  if (!public_key_base64) {
    BLOG(0, "Response is missing paymentToken/publicKey");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  const cbr::PublicKey public_key = cbr::PublicKey(*public_key_base64);
  if (!public_key.has_value()) {
    BLOG(0, "Invalid paymentToken/publicKey");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  if (!PublicKeyExistsForIssuerType(IssuerType::kPayments,
                                    *public_key_base64)) {
    BLOG(0, "Response paymentToken/publicKey "
                << *public_key_base64 << " does not exist in payment issuers");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/true);
  }

  // Get batch dleq proof
  const std::string* const batch_dleq_proof_base64 =
      payment_token_dict->FindString("batchProof");
  if (!batch_dleq_proof_base64) {
    BLOG(0, "Response is missing paymentToken/batchProof");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }
  cbr::BatchDLEQProof batch_dleq_proof =
      cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    BLOG(0, "Invalid paymentToken/batchProof");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  // Get signed tokens
  const auto* const signed_tokens_list =
      payment_token_dict->FindList("signedTokens");
  if (!signed_tokens_list) {
    BLOG(0, "Response is missing paymentToken/signedTokens");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  std::vector<cbr::SignedToken> signed_tokens;
  for (const auto& item : *signed_tokens_list) {
    const std::string& signed_token_base64 = item.GetString();
    const cbr::SignedToken signed_token = cbr::SignedToken(signed_token_base64);
    if (!signed_token.has_value()) {
      BLOG(0, "Invalid paymentToken/signedToken");
      return redeem_confirmation.FailedToRedeemConfirmation(
          confirmation, /*should_retry=*/false);
    }

    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  if (!confirmation.reward->token.has_value()) {
    BLOG(0, "Invalid confirmation reward token");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }
  const std::vector<cbr::Token> tokens = {confirmation.reward->token};

  if (!confirmation.reward->blinded_token.has_value()) {
    BLOG(0, "Invalid confirmation reward blinded token");
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }
  const std::vector<cbr::BlindedToken> blinded_tokens = {
      confirmation.reward->blinded_token};

  const absl::optional<std::vector<cbr::UnblindedToken>>
      batch_dleq_proof_unblinded_tokens = batch_dleq_proof.VerifyAndUnblind(
          tokens, blinded_tokens, signed_tokens, public_key);
  if (!batch_dleq_proof_unblinded_tokens ||
      batch_dleq_proof_unblinded_tokens->empty()) {
    BLOG(1, "Failed to verify and unblind tokens");
    BLOG(1, "  Batch DLEQ proof: " << *batch_dleq_proof_base64);
    BLOG(1, "  Public key: " << *public_key_base64);
    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation, /*should_retry=*/false);
  }

  PaymentTokenInfo payment_token;
  payment_token.transaction_id = confirmation.transaction_id;
  payment_token.unblinded_token = batch_dleq_proof_unblinded_tokens->front();
  payment_token.public_key = public_key;
  payment_token.confirmation_type = confirmation.type;
  payment_token.ad_type = confirmation.ad_type;

  const auto result = MaybeAddPaymentToken(payment_token);
  if (!result.has_value()) {
    BLOG(1, result.error());

    return redeem_confirmation.FailedToRedeemConfirmation(
        confirmation,
        /*should_retry=*/false);
  }

  redeem_confirmation.SuccessfullyRedeemedConfirmation(confirmation);
}

void RedeemRewardConfirmation::SuccessfullyRedeemedConfirmation(
    const ConfirmationInfo& confirmation) {
  LogPaymentTokenStatus();

  NotifyDidRedeemConfirmation(confirmation);
}

void RedeemRewardConfirmation::FailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  NotifyFailedToRedeemConfirmation(confirmation, should_retry);
}

void RedeemRewardConfirmation::NotifyDidRedeemConfirmation(
    const ConfirmationInfo& confirmation) const {
  if (delegate_) {
    delegate_->OnDidRedeemConfirmation(confirmation);
  }
}

void RedeemRewardConfirmation::NotifyFailedToRedeemConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) const {
  if (delegate_) {
    delegate_->OnFailedToRedeemConfirmation(confirmation, should_retry);
  }
}

}  // namespace brave_ads
