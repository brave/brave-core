/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token.h"

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/account/confirmations/confirmation_info.h"
#include "bat/ads/internal/account/confirmations/confirmations.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/security/confirmations/confirmations_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_url_request_builder.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_dto_user_data_builder.h"
#include "net/http/http_status_code.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::UnblindedToken;

RedeemUnblindedToken::RedeemUnblindedToken() = default;

RedeemUnblindedToken::~RedeemUnblindedToken() = default;

void RedeemUnblindedToken::set_delegate(
    RedeemUnblindedTokenDelegate* delegate) {
  delegate_ = delegate;
}

void RedeemUnblindedToken::Redeem(const ConfirmationInfo& confirmation) {
  BLOG(1, "Redeem unblinded token");

  if (!confirmation.created) {
    CreateConfirmation(confirmation);
    return;
  }

  FetchPaymentToken(confirmation);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemUnblindedToken::CreateConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "CreateConfirmation");
  BLOG(2, "POST /v1/confirmation/{confirmation_id}/{credential}");

  CreateConfirmationUrlRequestBuilder url_request_builder(confirmation);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback = std::bind(&RedeemUnblindedToken::OnCreateConfirmation, this,
                            std::placeholders::_1, confirmation);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnCreateConfirmation(
    const UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(1, "OnCreateConfirmation");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    // OnFetchPaymentToken handles HTTP response status codes for duplicate/bad
    // confirmations as we cannot guarantee if the confirmation was created or
    // not, i.e. after an internal server error 500
    BLOG(1, "Duplicate/bad confirmation");
  }

  if (url_response.status_code == 418) {  // I'm a teapot
    if (delegate_) {
      delegate_->OnDidSendConfirmation(confirmation);
    }

    return;
  }

  ConfirmationInfo new_confirmation = confirmation;
  new_confirmation.created = true;

  FetchPaymentToken(new_confirmation);
}

void RedeemUnblindedToken::FetchPaymentToken(
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(1, "FetchPaymentToken");
  BLOG(2, "GET /v1/confirmation/{confirmation_id}/paymentToken");

  FetchPaymentTokenUrlRequestBuilder url_request_builder(confirmation);
  UrlRequestPtr url_request = url_request_builder.Build();
  BLOG(5, UrlRequestToString(url_request));
  BLOG(7, UrlRequestHeadersToString(url_request));

  auto callback = std::bind(&RedeemUnblindedToken::OnFetchPaymentToken, this,
                            std::placeholders::_1, confirmation);
  AdsClientHelper::Get()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnFetchPaymentToken(
    const UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "OnFetchPaymentToken");

  BLOG(6, UrlResponseToString(url_response));
  BLOG(7, UrlResponseHeadersToString(url_response));

  if (url_response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(1, "Confirmation not found");

    if (!security::Verify(confirmation)) {
      BLOG(1, "Failed to verify confirmation");
      OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
      return;
    }

    ConfirmationInfo new_confirmation = confirmation;
    new_confirmation.created = false;

    OnFailedToRedeemUnblindedToken(new_confirmation, /* should_retry */ true);
    return;
  }

  if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(1, "Credential is invalid");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ false);
    return;
  }

  if (url_response.status_code == net::HTTP_ACCEPTED) {
    BLOG(1, "Payment token is not ready");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch payment token");
    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary =
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
  PublicKey public_key = PublicKey::decode_base64(*public_key_base64);
  if (privacy::ExceptionOccurred()) {
    BLOG(0, "Invalid public key");
    NOTREACHED();
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
  BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::decode_base64(*batch_dleq_proof_base64);
  if (privacy::ExceptionOccurred()) {
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

  std::vector<SignedToken> signed_tokens;
  for (const auto& value : signed_tokens_list->GetList()) {
    DCHECK(value.is_string());
    const std::string signed_token_base64 = value.GetString();
    SignedToken signed_token = SignedToken::decode_base64(signed_token_base64);
    if (privacy::ExceptionOccurred()) {
      BLOG(0, "Invalid signed token");
      NOTREACHED();
      continue;
    }

    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  const std::vector<Token> tokens = {confirmation.payment_token};

  const std::vector<BlindedToken> blinded_tokens = {
      confirmation.blinded_payment_token};

  const std::vector<UnblindedToken> batch_dleq_proof_unblinded_tokens =
      batch_dleq_proof.verify_and_unblind(tokens, blinded_tokens, signed_tokens,
                                          public_key);
  if (privacy::ExceptionOccurred()) {
    BLOG(1, "Failed to verify and unblind tokens");
    BLOG(1, "  Batch proof: " << *batch_dleq_proof_base64);
    BLOG(1, "  Public key: " << *public_key_base64);

    OnFailedToRedeemUnblindedToken(confirmation, /* should_retry */ true);
    return;
  }

  privacy::UnblindedTokenInfo unblinded_payment_token;
  unblinded_payment_token.value = batch_dleq_proof_unblinded_tokens.front();
  unblinded_payment_token.public_key = public_key;

  OnDidRedeemUnblindedToken(confirmation, unblinded_payment_token);
}

void RedeemUnblindedToken::OnDidRedeemUnblindedToken(
    const ConfirmationInfo& confirmation,
    const privacy::UnblindedTokenInfo& unblinded_payment_token) {
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
