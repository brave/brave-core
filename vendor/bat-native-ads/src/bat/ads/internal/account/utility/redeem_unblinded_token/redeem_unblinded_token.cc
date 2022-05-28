/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/values.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations_util.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_url_request_builder.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/http_status_code.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/server/url/url_request_string_util.h"
#include "bat/ads/internal/server/url/url_response_string_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

RedeemUnblindedToken::RedeemUnblindedToken() = default;

RedeemUnblindedToken::~RedeemUnblindedToken() {
  delegate_ = nullptr;
}

void RedeemUnblindedToken::Redeem(const ConfirmationInfo& confirmation) {
  DCHECK(confirmation.IsValid());

  BLOG(1, "Redeem unblinded token");

  if (ShouldRewardUser() && !HasIssuers()) {
    BLOG(1, "Failed to redeem unblinded token due to missing issuers");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  if (!confirmation.was_created) {
    CreateConfirmation(confirmation);
    return;
  }

  FetchPaymentToken(confirmation);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedToken::CreateConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "CreateConfirmation");
  BLOG(2, "POST /v2/confirmation/{confirmation_id}/{credential}");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  const auto callback = std::bind(&RedeemUnblindedToken::OnCreateConfirmation,
                                  this, std::placeholders::_1, confirmation);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnCreateConfirmation(
    const mojom::UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "OnCreateConfirmation");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (confirmation.credential.empty()) {
    if (url_response.status_code == net::HTTP_IM_A_TEAPOT) {
      OnDidSendConfirmation(confirmation);
      return;
    } else if (url_response.status_code == net::HTTP_CONFLICT) {
      OnFailedToSendConfirmation(confirmation, /* should_retry */ false);
      return;
    } else if (url_response.status_code == net::HTTP_BAD_REQUEST) {
      OnFailedToSendConfirmation(confirmation, /* should_retry */ false);
      return;
    } else if (url_response.status_code == net::HTTP_UPGRADE_REQUIRED) {
      BLOG(1, "Failed to create confirmation as a browser upgrade is required");
      OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
      return;
    }

    OnFailedToSendConfirmation(confirmation, /* should_retry */ true);
    return;
  }

  ConfirmationInfo new_confirmation = confirmation;
  new_confirmation.was_created = true;

  FetchPaymentToken(new_confirmation);
}

void RedeemUnblindedToken::FetchPaymentToken(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "FetchPaymentToken");
  BLOG(2, "GET /v2/confirmation/{confirmation_id}/paymentToken");

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);
  mojom::UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(6, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  const auto callback = std::bind(&RedeemUnblindedToken::OnFetchPaymentToken,
                                  this, std::placeholders::_1, confirmation);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnFetchPaymentToken(
    const mojom::UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "OnFetchPaymentToken");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(1, "Confirmation not found");

    ConfirmationInfo new_confirmation = confirmation;
    new_confirmation.was_created = false;

    OnFailedToRedeemUnblindedToken(new_confirmation, /* should_retry */ true);
    return;
  } else if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(1, "Credential is invalid");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  } else if (url_response.status_code == net::HTTP_ACCEPTED) {
    BLOG(1, "Payment token is not ready");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  } else if (url_response.status_code == net::HTTP_UPGRADE_REQUIRED) {
    BLOG(1, "Failed to fetch payment token as a browser upgrade is required");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  } else if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch payment token");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  if (!VerifyConfirmation(confirmation)) {
    BLOG(1, "Failed to verify confirmation");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  // Parse JSON response
  absl::optional<base::Value> dictionary =
      base::JSONReader::Read(url_response.body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Get id
  const std::string* id = dictionary->FindStringKey("id");
  if (!id) {
    BLOG(0, "Response is missing id");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Validate id
  if (*id != confirmation.id) {
    BLOG(0, "Response id " << *id << " does not match confirmation id "
                           << confirmation.id);
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  // Get payment token
  base::Value* payment_token_dictionary =
      dictionary->FindDictKey("paymentToken");
  if (!payment_token_dictionary) {
    BLOG(1, "Response is missing paymentToken");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Get public key
  const std::string* public_key_base64 =
      payment_token_dictionary->FindStringKey("publicKey");
  if (!public_key_base64) {
    BLOG(0, "Response is missing publicKey in paymentToken dictionary");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  const privacy::cbr::PublicKey public_key =
      privacy::cbr::PublicKey(*public_key_base64);
  if (!public_key.has_value()) {
    BLOG(0, "Invalid public key");
    NOTREACHED();
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  if (!PublicKeyExistsForIssuerType(IssuerType::kPayments,
                                    *public_key_base64)) {
    BLOG(0, "Response public key " << *public_key_base64 << " does not exist "
                                   << "in payments issuer public keys");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Get batch dleq proof
  const std::string* batch_dleq_proof_base64 =
      payment_token_dictionary->FindStringKey("batchProof");
  if (!batch_dleq_proof_base64) {
    BLOG(0, "Response is missing batchProof");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }
  privacy::cbr::BatchDLEQProof batch_dleq_proof =
      privacy::cbr::BatchDLEQProof(*batch_dleq_proof_base64);
  if (!batch_dleq_proof.has_value()) {
    BLOG(0, "Invalid batch DLEQ proof");
    NOTREACHED();
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Get signed tokens
  const base::Value* signed_tokens_list =
      payment_token_dictionary->FindListKey("signedTokens");
  if (!signed_tokens_list) {
    BLOG(0, "Response is missing signedTokens");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  if (signed_tokens_list->GetList().size() != 1) {
    BLOG(0, "Response has too many signedTokens");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  std::vector<privacy::cbr::SignedToken> signed_tokens;
  for (const auto& value : signed_tokens_list->GetList()) {
    DCHECK(value.is_string());
    const std::string signed_token_base64 = value.GetString();
    const privacy::cbr::SignedToken signed_token =
        privacy::cbr::SignedToken(signed_token_base64);
    if (!signed_token.has_value()) {
      BLOG(0, "Invalid signed token");
      NOTREACHED();
      continue;
    }

    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  const std::vector<privacy::cbr::Token> tokens = {confirmation.payment_token};

  const std::vector<privacy::cbr::BlindedToken> blinded_tokens = {
      confirmation.blinded_payment_token};

  const absl::optional<std::vector<privacy::cbr::UnblindedToken>>
      batch_dleq_proof_unblinded_tokens_optional =
          batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens,
                                            signed_tokens, public_key);
  if (!batch_dleq_proof_unblinded_tokens_optional) {
    BLOG(1, "Failed to verify and unblind tokens");
    BLOG(1, "  Batch proof: " << *batch_dleq_proof_base64);
    BLOG(1, "  Public key: " << *public_key_base64);

    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }
  const std::vector<privacy::cbr::UnblindedToken>&
      batch_dleq_proof_unblinded_tokens =
          batch_dleq_proof_unblinded_tokens_optional.value();

  privacy::UnblindedPaymentTokenInfo unblinded_payment_token;
  unblinded_payment_token.transaction_id = confirmation.transaction_id;
  unblinded_payment_token.value = batch_dleq_proof_unblinded_tokens.front();
  unblinded_payment_token.public_key = public_key;
  unblinded_payment_token.confirmation_type = confirmation.type;
  unblinded_payment_token.ad_type = confirmation.ad_type;

  OnDidRedeemUnblindedToken(confirmation, unblinded_payment_token);
}

void RedeemUnblindedToken::OnDidSendConfirmation(
    const ConfirmationInfo& confirmation) {
  if (!delegate_) {
    return;
  }

  delegate_->OnDidSendConfirmation(confirmation);
}

void RedeemUnblindedToken::OnFailedToSendConfirmation(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (!delegate_) {
    return;
  }

  delegate_->OnFailedToSendConfirmation(confirmation, should_retry);
}

void RedeemUnblindedToken::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token) {
  if (!delegate_) {
    return;
  }

  delegate_->OnDidRedeemUnblindedToken(confirmation, unblinded_payment_token);
}

void RedeemUnblindedToken::OnFailedToRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (!delegate_) {
    return;
  }

  delegate_->OnFailedToRedeemUnblindedToken(confirmation, should_retry);
}

}  // namespace ads
