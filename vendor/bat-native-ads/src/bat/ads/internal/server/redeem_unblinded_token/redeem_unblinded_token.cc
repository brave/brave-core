/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/server/redeem_unblinded_token/redeem_unblinded_token.h"

#include <functional>
#include <utility>
#include <vector>

#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave_base/random.h"
#include "net/http/http_status_code.h"
#include "wrapper.hpp"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/privacy/privacy_util.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/security/security_util.h"
#include "bat/ads/internal/server/ads_server_util.h"
#include "bat/ads/internal/server/redeem_unblinded_token/create_confirmation_url_request_builder.h"
#include "bat/ads/internal/server/redeem_unblinded_token/create_confirmation_util.h"
#include "bat/ads/internal/server/redeem_unblinded_token/fetch_payment_token_url_request_builder.h"
#include "bat/ads/internal/server/refill_unblinded_tokens/refill_unblinded_tokens.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

using std::placeholders::_1;

using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::UnblindedToken;

RedeemUnblindedToken::RedeemUnblindedToken(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

RedeemUnblindedToken::~RedeemUnblindedToken() = default;

void RedeemUnblindedToken::set_delegate(
    RedeemUnblindedTokenDelegate* delegate) {
  DCHECK(delegate);

  delegate_ = delegate;
}

void RedeemUnblindedToken::Redeem(
    const AdInfo& ad,
    const ConfirmationType confirmation_type) {
  BLOG(1, "Redeem token");

  if (ads_->get_confirmations()->get_unblinded_tokens()->IsEmpty()) {
    BLOG(1, "No unblinded tokens to redeem");
    return;
  }

  const privacy::UnblindedTokenInfo unblinded_token
      = ads_->get_confirmations()->get_unblinded_tokens()->GetToken();
  ads_->get_confirmations()->get_unblinded_tokens()->
      RemoveToken(unblinded_token);

  const ConfirmationInfo confirmation =
      CreateConfirmationInfo(ad, confirmation_type, unblinded_token);
  CreateConfirmation(confirmation);

  ads_->get_refill_unblinded_tokens()->MaybeRefill();
}

void RedeemUnblindedToken::Redeem(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Redeem token");

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

  auto callback = std::bind(&RedeemUnblindedToken::OnCreateConfirmation,
      this, _1, confirmation);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnCreateConfirmation(
    const UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(1, "OnCreateConfirmation");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    // OnFetchPaymentToken handles HTTP response status codes for duplicate/bad
    // confirmations as we cannot guarantee if the confirmation was created or
    // not, i.e. after an internal server error 500
    BLOG(1, "Duplicate/bad confirmation");
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

  auto callback = std::bind(&RedeemUnblindedToken::OnFetchPaymentToken,
      this, _1, confirmation);
  ads_->get_ads_client()->UrlRequest(std::move(url_request), callback);
}

void RedeemUnblindedToken::OnFetchPaymentToken(
    const UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "OnFetchPaymentToken");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(1, "Confirmation not found");

    if (!security::Verify(confirmation)) {
      BLOG(1, "Failed to verify confirmation");
      OnRedeem(FAILED, confirmation, false);
      return;
    }

    ConfirmationInfo new_confirmation = confirmation;
    new_confirmation.created = false;

    OnRedeem(FAILED, new_confirmation, true);
    return;
  }

  if (url_response.status_code == net::HTTP_BAD_REQUEST) {
    BLOG(1, "Credential is invalid");
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  if (url_response.status_code != net::HTTP_OK) {
    BLOG(1, "Failed to fetch payment token");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Parse JSON response
  base::Optional<base::Value> dictionary =
      base::JSONReader::Read(url_response.body);
  if (!dictionary || !dictionary->is_dict()) {
    BLOG(3, "Failed to parse response: " << url_response.body);
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get id
  const std::string* id = dictionary->FindStringKey("id");
  if (!id) {
    BLOG(0, "Response is missing id");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Validate id
  if (*id != confirmation.id) {
    BLOG(0, "Response id " << *id << " does not match confirmation id "
        << confirmation.id);
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  // Get payment token
  base::Value* payment_token_dictionary =
      dictionary->FindDictKey("paymentToken");
  if (!payment_token_dictionary) {
    BLOG(1, "Response is missing paymentToken");

    // Token is in a bad state so redeem a new token
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get public key
  const std::string* public_key_base64 =
      payment_token_dictionary->FindStringKey("publicKey");
  if (!public_key_base64) {
    BLOG(0, "Response is missing publicKey in paymentToken dictionary");
    OnRedeem(FAILED, confirmation, true);
    return;
  }
  PublicKey public_key = PublicKey::decode_base64(*public_key_base64);

  // Get catalog issuers
  const CatalogIssuersInfo catalog_issuers =
      ads_->get_confirmations()->GetCatalogIssuers();

  // Validate public key
  if (!catalog_issuers.PublicKeyExists(*public_key_base64)) {
    BLOG(0, "Response public key " << *public_key_base64 << " was not found "
        "in the catalog issuers");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get batch dleq proof
  const std::string* batch_dleq_proof_base64 =
      payment_token_dictionary->FindStringKey("batchProof");
  if (!batch_dleq_proof_base64) {
    BLOG(0, "Response is missing batchProof");
    OnRedeem(FAILED, confirmation, true);
    return;
  }
  BatchDLEQProof batch_dleq_proof =
      BatchDLEQProof::decode_base64(*batch_dleq_proof_base64);

  // Get signed tokens
  const base::Value* signed_tokens_list =
      payment_token_dictionary->FindListKey("signedTokens");
  if (!signed_tokens_list) {
    BLOG(0, "Response is missing signedTokens");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  if (signed_tokens_list->GetList().size() != 1) {
    BLOG(0, "Response has too many signedTokens");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (const auto& value : signed_tokens_list->GetList()) {
    DCHECK(value.is_string());
    const std::string signed_token_base64 = value.GetString();
    SignedToken signed_token = SignedToken::decode_base64(signed_token_base64);

    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind tokens
  const std::vector<Token> tokens = {
    confirmation.payment_token
  };

  const std::vector<BlindedToken> blinded_tokens = {
    confirmation.blinded_payment_token
  };

  const std::vector<UnblindedToken> batch_dleq_proof_unblinded_tokens =
      batch_dleq_proof.verify_and_unblind(tokens, blinded_tokens,
          signed_tokens, public_key);

  if (batch_dleq_proof_unblinded_tokens.size() != 1) {
    BLOG(1, "Failed to verify and unblind tokens");

    BLOG(1, "  Batch proof: " << batch_dleq_proof_base64);

    BLOG(1, "  Tokens (" << tokens.size() << "):");
    const std::string payment_token_base64 =
        confirmation.payment_token.encode_base64();
    BLOG(1, "    " << payment_token_base64);

    BLOG(1, "  Blinded tokens (" << blinded_tokens.size() << "):");
    const std::string blinded_payment_token_base64 =
        confirmation.blinded_payment_token.encode_base64();
    BLOG(1, "    " << blinded_payment_token_base64);

    BLOG(1, "  Signed tokens (" << signed_tokens.size() << "):");
    for (const auto& signed_token : signed_tokens) {
      const std::string signed_token_base64 = signed_token.encode_base64();
      BLOG(1, "    " << signed_token_base64);
    }

    BLOG(1, "  Public key: " << *public_key_base64);

    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Add unblinded token
  privacy::UnblindedTokenInfo unblinded_token;
  unblinded_token.value = batch_dleq_proof_unblinded_tokens.front();
  unblinded_token.public_key = public_key;

  if (ads_->get_confirmations()->get_unblinded_payment_tokens()->
      TokenExists(unblinded_token)) {
    BLOG(1, "Unblinded payment token is a duplicate");
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  const std::vector<privacy::UnblindedTokenInfo> unblinded_tokens = {
    unblinded_token
  };

  ads_->get_confirmations()->get_unblinded_payment_tokens()->
      AddTokens(unblinded_tokens);

  // Get estimated redemption value
  const double estimated_redemption_value =
      catalog_issuers.GetEstimatedRedemptionValue(
          unblinded_token.public_key.encode_base64());

  // Add transaction to history
  BLOG(1, "Added 1 unblinded payment token with an estimated redemption value "
      "of " << estimated_redemption_value << " BAT, you now have "
          << ads_->get_confirmations()->get_unblinded_payment_tokens()->Count()
              << " unblinded payment tokens");

  ads_->get_confirmations()->AppendTransaction(estimated_redemption_value,
      confirmation.type);

  OnRedeem(SUCCESS, confirmation, false);
}

void RedeemUnblindedToken::OnRedeem(
    const Result result,
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (result != SUCCESS) {
    if (delegate_) {
      delegate_->OnFailedToRedeemUnblindedToken(confirmation);
    }

    if (should_retry) {
      if (!confirmation.created) {
        CreateAndAppendNewConfirmationToRetryQueue(confirmation);
      } else {
        AppendConfirmationToRetryQueue(confirmation);
      }
    }

    return;
  }

  if (delegate_) {
    delegate_->OnDidRedeemUnblindedToken(confirmation);
  }
}

void RedeemUnblindedToken::CreateAndAppendNewConfirmationToRetryQueue(
    const ConfirmationInfo& confirmation) {
  if (ads_->get_confirmations()->get_unblinded_tokens()->IsEmpty()) {
    AppendConfirmationToRetryQueue(confirmation);
    return;
  }

  AdInfo ad;
  ad.creative_instance_id = confirmation.creative_instance_id;

  const privacy::UnblindedTokenInfo unblinded_token =
      ads_->get_confirmations()->get_unblinded_tokens()->GetToken();
  ads_->get_confirmations()->get_unblinded_tokens()->
      RemoveToken(unblinded_token);

  const ConfirmationInfo new_confirmation =
      CreateConfirmationInfo(ad, confirmation.type, unblinded_token);

  AppendConfirmationToRetryQueue(new_confirmation);

  ads_->get_refill_unblinded_tokens()->MaybeRefill();
}

void RedeemUnblindedToken::AppendConfirmationToRetryQueue(
    const ConfirmationInfo& confirmation) {
  ads_->get_confirmations()->AppendConfirmationToRetryQueue(confirmation);
}

ConfirmationInfo RedeemUnblindedToken::CreateConfirmationInfo(
    const AdInfo& ad,
    const ConfirmationType confirmation_type,
    const privacy::UnblindedTokenInfo& unblinded_token) {
  DCHECK(!ad.creative_instance_id.empty());

  ConfirmationInfo confirmation;

  confirmation.id = base::GenerateGUID();
  confirmation.creative_instance_id = ad.creative_instance_id;
  confirmation.type = confirmation_type;
  confirmation.unblinded_token = unblinded_token;

  const std::vector<Token> tokens = privacy::GenerateTokens(1);
  confirmation.payment_token = tokens.front();

  const std::vector<BlindedToken> blinded_tokens = privacy::BlindTokens(tokens);
  const BlindedToken blinded_token = blinded_tokens.front();
  confirmation.blinded_payment_token = blinded_token;

  const std::string payload = CreateConfirmationRequestDTO(confirmation);
  confirmation.credential = CreateCredential(unblinded_token, payload);

  confirmation.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  return confirmation;
}

}  // namespace ads
