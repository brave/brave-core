/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/redeem_token.h"

#include <memory>
#include <vector>

#include "bat/confirmations/confirmation_type.h"
#include "bat/confirmations/internal/ads_serve_helper.h"
#include "bat/confirmations/internal/confirmation_info.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/create_confirmation_request.h"
#include "bat/confirmations/internal/fetch_payment_token_request.h"
#include "bat/confirmations/internal/logging.h"
#include "bat/confirmations/internal/platform_helper.h"
#include "bat/confirmations/internal/privacy_utils.h"
#include "bat/confirmations/internal/static_values.h"
#include "bat/confirmations/internal/time_util.h"
#include "bat/confirmations/internal/token_info.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "wrapper.hpp"  // NOLINT

#include "base/base64.h"
#include "base/guid.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave_base/random.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using challenge_bypass_ristretto::SignedToken;
using challenge_bypass_ristretto::BatchDLEQProof;
using challenge_bypass_ristretto::VerificationSignature;
using challenge_bypass_ristretto::PublicKey;

namespace confirmations {

RedeemToken::RedeemToken(
    ConfirmationsImpl* confirmations,
    UnblindedTokens* unblinded_tokens,
    UnblindedTokens* unblinded_payment_tokens)
    : confirmations_(confirmations),
      unblinded_tokens_(unblinded_tokens),
      unblinded_payment_tokens_(unblinded_payment_tokens) {
}

RedeemToken::~RedeemToken() = default;

void RedeemToken::Redeem(
    const AdInfo& ad,
    const ConfirmationType confirmation_type) {
  BLOG(1, "Redeem token");

  if (unblinded_tokens_->IsEmpty()) {
    BLOG(1, "No unblinded tokens to redeem");
    return;
  }

  const TokenInfo token = unblinded_tokens_->GetToken();
  unblinded_tokens_->RemoveToken(token);

  const ConfirmationInfo confirmation =
      CreateConfirmationInfo(ad, confirmation_type, token);
  CreateConfirmation(confirmation);

  confirmations_->RefillTokensIfNecessary();
}

void RedeemToken::Redeem(
    const std::string& creative_instance_id,
    const std::string& creative_set_id,
    const ConfirmationType confirmation_type) {
  // TODO(https://github.com/brave/brave-browser/issues/8479): Deprecate this
  // function which is used by |ConfirmAction| and replace code with |ConfirmAd|

  DCHECK(!creative_instance_id.empty());
  DCHECK(!creative_set_id.empty());

  AdInfo ad;
  ad.creative_instance_id = creative_instance_id;
  ad.creative_set_id = creative_set_id;
  Redeem(ad, confirmation_type);
}

void RedeemToken::Redeem(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "Redeem token");

  if (!confirmation.created) {
    CreateConfirmation(confirmation);

    return;
  }

  FetchPaymentToken(confirmation);
}

///////////////////////////////////////////////////////////////////////////////

void RedeemToken::CreateConfirmation(
    const ConfirmationInfo& confirmation) {
  BLOG(1, "CreateConfirmation");
  BLOG(2, "POST /v1/confirmation/{confirmation_id}/{credential}");

  CreateConfirmationRequest request(confirmations_);

  auto url = request.BuildUrl(confirmation.id, confirmation.credential);

  auto method = request.GetMethod();

  const auto client_info = confirmations_->get_client()->GetClientInfo();
  const std::string build_channel = client_info->channel;

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code =
      brave_l10n::LocaleHelper::GetCountryCode(locale);

  auto confirmation_request_dto = request.CreateConfirmationRequestDTO(
      confirmation, build_channel, platform, country_code);

  auto body = request.BuildBody(confirmation_request_dto);

  auto headers = request.BuildHeaders();

  auto content_type = request.GetContentType();

  auto callback = std::bind(&RedeemToken::OnCreateConfirmation,
      this, _1, confirmation);

  BLOG(5, UrlRequestToString(url, headers, body, content_type, method));
  confirmations_->get_client()->LoadURL(url, headers, body, content_type,
      method, callback);
}

void RedeemToken::OnCreateConfirmation(
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

void RedeemToken::FetchPaymentToken(
    const ConfirmationInfo& confirmation) {
  DCHECK(!confirmation.id.empty());

  BLOG(1, "FetchPaymentToken");
  BLOG(2, "GET /v1/confirmation/{confirmation_id}/paymentToken");

  FetchPaymentTokenRequest request;

  auto url = request.BuildUrl(confirmation.id);
  auto method = request.GetMethod();

  auto callback = std::bind(&RedeemToken::OnFetchPaymentToken,
      this, _1, confirmation);

  BLOG(5, UrlRequestToString(url, {}, "", "", method));
  confirmations_->get_client()->LoadURL(url, {}, "", "", method, callback);
}

void RedeemToken::OnFetchPaymentToken(
    const UrlResponse& url_response,
    const ConfirmationInfo& confirmation) {
  BLOG(1, "OnFetchPaymentToken");

  BLOG(6, UrlResponseToString(url_response));

  if (url_response.status_code == net::HTTP_NOT_FOUND) {
    BLOG(1, "Confirmation not found");

    if (!Verify(confirmation)) {
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
  auto* id_value = dictionary->FindKey("id");
  if (!id_value) {
    BLOG(0, "Response is missing id");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  auto id = id_value->GetString();

  // Validate id
  if (id != confirmation.id) {
    BLOG(0, "Response id " << id << " does not match confirmation id "
        << confirmation.id);
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  // Get payment token
  auto* payment_token_value = dictionary->FindKey("paymentToken");
  if (!payment_token_value) {
    BLOG(1, "Response is missing paymentToken");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get payment token dictionary
  base::DictionaryValue* payment_token_dictionary;
  if (!payment_token_value->GetAsDictionary(&payment_token_dictionary)) {
    BLOG(1, "Response is missing paymentToken dictionary");
    OnRedeem(FAILED, confirmation, false);

    // Token is in a bad state so redeem a new token
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get public key
  auto* public_key_value = payment_token_dictionary->FindKey("publicKey");
  if (!public_key_value) {
    BLOG(0, "Response is missing publicKey in paymentToken dictionary");
    OnRedeem(FAILED, confirmation, true);
    return;
  }
  auto public_key_base64 = public_key_value->GetString();
  auto public_key = PublicKey::decode_base64(public_key_base64);

  // Validate public key
  if (!confirmations_->IsValidPublicKeyForCatalogIssuers(public_key_base64)) {
    BLOG(0, "Response public key " << public_key_base64 << " was not found "
        "in the catalog issuers");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Get batch proof
  auto* batch_proof_value = payment_token_dictionary->FindKey("batchProof");
  if (!batch_proof_value) {
    BLOG(0, "Response is missing batchProof");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  auto batch_proof_base64 = batch_proof_value->GetString();
  auto batch_proof = BatchDLEQProof::decode_base64(batch_proof_base64);

  // Get signed tokens
  auto* signed_tokens_value = payment_token_dictionary->FindKey("signedTokens");
  if (!signed_tokens_value) {
    BLOG(0, "Response is missing signedTokens");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  base::ListValue signed_token_base64_values(signed_tokens_value->GetList());
  if (signed_token_base64_values.GetSize() != 1) {
    BLOG(0, "Response has too many signedTokens");
    OnRedeem(FAILED, confirmation, true);
    return;
  }

  std::vector<SignedToken> signed_tokens;
  for (const auto& signed_token_base64_value : signed_token_base64_values) {
    auto signed_token_base64 = signed_token_base64_value.GetString();
    auto signed_token = SignedToken::decode_base64(signed_token_base64);
    signed_tokens.push_back(signed_token);
  }

  // Verify and unblind payment token
  auto payment_tokens = {confirmation.payment_token};
  auto blinded_payment_tokens = {confirmation.blinded_payment_token};

  auto unblinded_payment_tokens = batch_proof.verify_and_unblind(
     payment_tokens, blinded_payment_tokens, signed_tokens, public_key);

  if (unblinded_payment_tokens.size() != 1) {
    BLOG(1, "Failed to verify and unblind payment tokens");

    BLOG(1, "  Batch proof: " << batch_proof_base64);

    BLOG(1, "  Payment tokens (" << payment_tokens.size() << "):");
    auto payment_token_base64 = confirmation.payment_token.encode_base64();
    BLOG(1, "    " << payment_token_base64);

    BLOG(1, "  Blinded payment tokens (" << blinded_payment_tokens.size()
        << "):");
    auto blinded_payment_token_base64 =
        confirmation.blinded_payment_token.encode_base64();
    BLOG(1, "    " << blinded_payment_token_base64);

    BLOG(1, "  Signed tokens (" << signed_tokens.size() << "):");
    for (const auto& signed_token : signed_tokens) {
      auto signed_token_base64 = signed_token.encode_base64();
      BLOG(1, "    " << signed_token_base64);
    }

    BLOG(1, "  Public key: " << public_key_base64);

    OnRedeem(FAILED, confirmation, true);
    return;
  }

  // Add unblinded payment token
  TokenInfo unblinded_payment_token_info;
  unblinded_payment_token_info.unblinded_token
      = unblinded_payment_tokens.front();
  unblinded_payment_token_info.public_key = public_key_base64;

  if (unblinded_payment_tokens_->TokenExists(unblinded_payment_token_info)) {
    BLOG(1, "Unblinded payment token is a duplicate");
    OnRedeem(FAILED, confirmation, false);
    return;
  }

  TokenList tokens = {unblinded_payment_token_info};
  unblinded_payment_tokens_->AddTokens(tokens);

  // Add transaction to history
  double estimated_redemption_value =
      confirmations_->GetEstimatedRedemptionValue(
          unblinded_payment_token_info.public_key);

  BLOG(1, "Added 1 unblinded payment token with an estimated redemption value "
      "of " << estimated_redemption_value << " BAT, you now have "
          << unblinded_payment_tokens_->Count() << " unblinded payment tokens");

  confirmations_->AppendTransactionToHistory(
      estimated_redemption_value, confirmation.type);

  OnRedeem(SUCCESS, confirmation, false);
}

void RedeemToken::OnRedeem(
    const Result result,
    const ConfirmationInfo& confirmation,
    const bool should_retry) {
  if (result != SUCCESS) {
    BLOG(1, "Failed to redeem token with confirmation id " << confirmation.id
        << ", creative instance id " <<  confirmation.creative_instance_id
            << " and " << std::string(confirmation.type));

    if (should_retry) {
      if (!confirmation.created) {
        CreateAndAppendNewConfirmationToRetryQueue(confirmation);
      } else {
        AppendConfirmationToRetryQueue(confirmation);
      }
    }
  } else {
    BLOG(1, "Successfully redeemed token with confirmation id "
        << confirmation.id << ", creative instance id "
            << confirmation.creative_instance_id << " and "
                << std::string(confirmation.type));
  }
}

void RedeemToken::CreateAndAppendNewConfirmationToRetryQueue(
    const ConfirmationInfo& confirmation) {
  if (unblinded_tokens_->IsEmpty()) {
    AppendConfirmationToRetryQueue(confirmation);
    return;
  }

  AdInfo ad;
  ad.creative_instance_id = confirmation.creative_instance_id;

  const TokenInfo token = unblinded_tokens_->GetToken();
  unblinded_tokens_->RemoveToken(token);

  const ConfirmationInfo new_confirmation =
      CreateConfirmationInfo(ad, confirmation.type, token);

  AppendConfirmationToRetryQueue(new_confirmation);

  confirmations_->RefillTokensIfNecessary();
}

void RedeemToken::AppendConfirmationToRetryQueue(
    const ConfirmationInfo& confirmation) {
  confirmations_->AppendConfirmationToQueue(confirmation);
}

ConfirmationInfo RedeemToken::CreateConfirmationInfo(
    const AdInfo& ad,
    const ConfirmationType confirmation_type,
    const TokenInfo& token) {
  DCHECK(!ad.creative_instance_id.empty());

  ConfirmationInfo confirmation;

  confirmation.id = base::GenerateGUID();
  confirmation.creative_instance_id = ad.creative_instance_id;
  confirmation.type = confirmation_type;
  confirmation.token_info = token;

  auto payment_tokens = privacy::GenerateTokens(1);
  confirmation.payment_token = payment_tokens.front();

  auto blinded_payment_tokens = privacy::BlindTokens(payment_tokens);
  auto blinded_payment_token = blinded_payment_tokens.front();
  confirmation.blinded_payment_token = blinded_payment_token;

  const auto client_info = confirmations_->get_client()->GetClientInfo();
  const std::string build_channel = client_info->channel;

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code =
      brave_l10n::LocaleHelper::GetCountryCode(locale);

  CreateConfirmationRequest request(confirmations_);
  auto payload = request.CreateConfirmationRequestDTO(confirmation,
      build_channel, platform, country_code);

  confirmation.credential = request.CreateCredential(token, payload);
  confirmation.timestamp_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  return confirmation;
}

bool RedeemToken::Verify(
    const ConfirmationInfo& confirmation) const {
  std::string credential;
  base::Base64Decode(confirmation.credential, &credential);

  base::Optional<base::Value> value = base::JSONReader::Read(credential);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  auto* signature_value = dictionary->FindKey("signature");
  if (!signature_value) {
    return false;
  }

  auto signature = signature_value->GetString();
  auto verification_signature = VerificationSignature::decode_base64(signature);

  const auto client_info = confirmations_->get_client()->GetClientInfo();
  const std::string build_channel = client_info->channel;

  const std::string platform = PlatformHelper::GetInstance()->GetPlatformName();

  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code =
      brave_l10n::LocaleHelper::GetCountryCode(locale);

  CreateConfirmationRequest request(confirmations_);
  auto payload = request.CreateConfirmationRequestDTO(confirmation,
      build_channel, platform, country_code);

  auto unblinded_token = confirmation.token_info.unblinded_token;
  auto verification_key = unblinded_token.derive_verification_key();

  return verification_key.verify(verification_signature, payload);
}

}  // namespace confirmations
